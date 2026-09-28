#include "QbeEmitter.h"
#include "QbeSupport.h"

#include <algorithm>

using QbeSupport::isUnconstrainedArray;

Value QbeEmitter::emitCall(CallExpr* expr)
{
    Symbol* subprogram = expr->subprogram;
    if (subprogram == nullptr) {
        return Value { "0", 'w' };
    }
    if (subprogram->m_inheritedFrom != nullptr) {
        CallExpr inherited;
        inherited.location = expr->location;
        inherited.form = CallForm::Subprogram;
        inherited.subprogram = subprogram->m_inheritedFrom;
        inherited.type = expr->type;
        inherited.resolvedArguments = expr->resolvedArguments;
        Value result = emitCall(&inherited);
        if (expr->type != nullptr && expr->type->m_tagged
            && rootType(expr->type) != rootType(inherited.subprogram->returnType)) {
            line("storel " + rootType(expr->type)->m_tagName + ", " + result.name);
        }
        return result;
    }
    if (subprogram->m_negatedEquality != nullptr) {
        CallExpr equality;
        equality.location = expr->location;
        equality.form = CallForm::Subprogram;
        equality.subprogram = subprogram->m_negatedEquality;
        equality.type = subprogram->returnType;
        equality.resolvedArguments = expr->resolvedArguments;
        Value result = emitCall(&equality);
        std::string complement = newTemp();
        line(complement + " =w ceqw " + result.name + ", 0");
        return Value { complement, 'w' };
    }
    // Bare names arrive without an explicit argument list. Complete it from
    // the resolved declaration before either Ada or imported-call marshalling.
    expr->resolvedArguments.resize(subprogram->parameters.size(), nullptr);
    for (std::size_t i = 0; i < subprogram->parameters.size(); ++i) {
        if (expr->resolvedArguments[i] == nullptr) {
            expr->resolvedArguments[i] = subprogram->parameters[i]->defaultExpr;
        }
        if (expr->resolvedArguments[i] == nullptr) {
            m_diagnostics.error(expr->location, "internal error: missing resolved call argument");
            return Value { "0", 'w' };
        }
    }
    if (subprogram->m_renamedSubprogram != nullptr || subprogram->m_renamedAccess != nullptr) {
        CallExpr renamed;
        renamed.location = expr->location;
        renamed.form = CallForm::Subprogram;
        renamed.type = expr->type;
        renamed.resolvedArguments = expr->resolvedArguments;
        renamed.subprogram = subprogram->m_renamedSubprogram;
        if (subprogram->m_renamedAccess != nullptr) {
            Symbol* binding = subprogram->m_renamedAccess;
            auto callee = std::make_unique<IdentifierExpr>();
            callee->symbol = binding;
            callee->type = binding->type;
            callee->location = expr->location;
            renamed.callee = std::move(callee);
            renamed.m_indirect = true;
            renamed.subprogram = binding->type->m_accessProfile;
        }
        return emitCall(&renamed);
    }
    if (subprogram->builtin == BuiltinKind::Runtime) {
        Value result = emitRuntimeCall(expr, subprogram);
        if (subprogram->canRaise) {
            emitExceptionCheck();
        }
        if (subprogram->returnType != nullptr && subprogram->returnType->m_scalarBoundsSymbol != nullptr) {
            emitRangeCheck(result, subprogram->returnType, expr->location);
        }
        return result;
    }

    std::string callee = subprogram->qbeName;
    std::string indirectLink;
    if (expr->m_indirect) {
        Value descriptor = emitExpr(expr->callee.get());
        checkNotNull(descriptor);
        callee = newTemp();
        std::string linkSlot = newTemp();
        indirectLink = newTemp();
        line(callee + " =l loadl " + descriptor.name);
        line(linkSlot + " =l add " + descriptor.name + ", 8");
        line(indirectLink + " =l loadl " + linkSlot);
    }

    struct ScalarCopyBack
    {
        Value actual;
        Value temporary;
        Type* formalType;
        Expr* argument;
    };
    std::vector<ScalarCopyBack> copyBacks;
    std::vector<std::string> arguments;
    bool compositeResult = isComposite(subprogram->returnType);
    bool dynamicResult = isUnconstrainedArray(subprogram->returnType);
    std::string resultStorage;
    if (compositeResult) {
        resultStorage = allocScratch(dynamicResult ? 24 + 8 * (subprogram->returnType->arrayRank - 1) : std::max(1LL, typeSize(subprogram->returnType)));
        arguments.push_back("l " + resultStorage);
    }
    if (subprogram->level > 0) {
        Value link = staticLinkFor(subprogram->level - 1);
        arguments.push_back("l " + link.name);
    }

    for (std::size_t i = 0; i < subprogram->parameters.size(); ++i) {
        Symbol* parameter = subprogram->parameters[i];
        Expr* argument = expr->resolvedArguments[i];
        if (parameter->byReference) {
            if (isScalar(parameter->type)) {
                // Keep the pointer ABI, but give each formal a distinct object.
                // Evaluate the actual's address once, including indexed names.
                Value actual = emitAddress(argument);
                Value temporary { allocScratch(std::max(1LL, typeSize(parameter->type))), 'l' };
                if (parameter->mode == ParameterMode::InOut || parameter->type->kind == TypeKind::Access) {
                    Value value = loadFrom(actual, argument->type);
                    if (parameter->mode == ParameterMode::InOut) {
                        emitRangeCheck(value, parameter->type, argument->location);
                    }
                    storeInto(temporary, value, parameter->type);
                }
                // Numeric/enumeration out formals start uninitialized. Access
                // out formals retain the actual value without a constraint check.
                arguments.push_back("l " + temporary.name);
                copyBacks.push_back({ actual, temporary, parameter->type, argument });
                continue;
            }
            // Composite values are already addresses and carry their bounds.
            Value value = isComposite(argument->type) ? emitExpr(argument) : emitAddress(argument);
            if (parameter->type->m_boundsSymbol != nullptr) {
                Value target = withBounds(Value {}, parameter->type, nullptr);
                checkArrayShape(target, parameter->type, value, argument->type);
                value.first = target.first;
                value.last = target.last;
                value.innerBounds = target.innerBounds;
            }
            arguments.push_back("l " + value.name);
            if (isUnconstrainedArray(parameter->type)) {
                if (!value.hasBounds()) {
                    value = withBounds(value, argument->type, nullptr);
                }
                arguments.push_back("w " + (value.first.empty() ? std::string("1") : value.first));
                arguments.push_back("w " + (value.last.empty() ? std::string("0") : value.last));
                for (const auto& bounds : value.innerBounds) {
                    arguments.push_back("w " + bounds.first);
                    arguments.push_back("w " + bounds.second);
                }
            } else if (parameter->type->kind == TypeKind::Array && parameter->type->arrayRank > 1) {
                checkArrayShape(withBounds(Value {}, parameter->type, nullptr), parameter->type, value, argument->type);
            }
        } else {
            Value value = emitExpr(argument);
            emitRangeCheck(value, parameter->type, argument->location);
            arguments.push_back(std::string(1, qbeClass(parameter->type)) + " " + value.name);
        }
    }

    std::string argumentList;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) {
            argumentList += ", ";
        }
        argumentList += arguments[i];
    }

    Value result { "0", 'w' };
    auto invoke = [&](const std::string& actuals) {
        if (subprogram->returnType != nullptr && !compositeResult) {
            char type = qbeClass(subprogram->returnType);
            std::string temp = newTemp();
            line(temp + " =" + std::string(1, type) + " call " + callee + "(" + actuals + ")");
            return Value { temp, type };
        }
        line("call " + callee + "(" + actuals + ")");
        return Value { "0", 'w' };
    };
    if (expr->m_indirect) {
        std::string nested = newLabel("nestedcallback");
        std::string library = newLabel("librarycallback");
        std::string done = newLabel("callbackdone");
        std::string hasLink = newTemp();
        line(hasLink + " =w cnel " + indirectLink + ", 0");
        branch(Value { hasLink, 'w' }, nested, library);
        label(nested);
        std::vector<std::string> linked = arguments;
        linked.insert(linked.begin() + (compositeResult ? 1 : 0), "l " + indirectLink);
        std::string linkedArguments;
        for (const std::string& argument : linked) {
            linkedArguments += (linkedArguments.empty() ? "" : ", ") + argument;
        }
        Value nestedResult = invoke(linkedArguments);
        jump(done);
        label(library);
        Value libraryResult = invoke(argumentList);
        jump(done);
        label(done);
        if (subprogram->returnType != nullptr && !compositeResult) {
            result = Value { newTemp(), nestedResult.type };
            line(result.name + " =" + std::string(1, result.type) + " phi " + nested + " " + nestedResult.name
                 + ", " + library + " " + libraryResult.name);
        }
    } else {
        result = invoke(argumentList);
    }

    emitExceptionCheck();
    if (dynamicResult) {
        // Adopt the transfer buffer into the caller's expression lifetime.
        std::string pointer = newTemp();
        line(pointer + " =l loadl " + resultStorage);
        std::string firstAddress = newTemp();
        std::string lastAddress = newTemp();
        line(firstAddress + " =l add " + resultStorage + ", 8");
        line(lastAddress + " =l add " + resultStorage + ", 12");
        std::string first = newTemp();
        std::string last = newTemp();
        line(first + " =w loadw " + firstAddress);
        line(last + " =w loadw " + lastAddress);
        line("call $__ada_array_adopt(l " + storageArena(true, true) + ", l " + pointer + ")");
        emitExceptionCheck();
        result = Value { pointer, 'l', first, last };
        for (int dimension = 1; dimension < subprogram->returnType->arrayRank; ++dimension) {
            std::string firstSlot = newTemp();
            std::string lastSlot = newTemp();
            std::string rowFirst = newTemp();
            std::string rowLast = newTemp();
            line(firstSlot + " =l add " + resultStorage + ", " + std::to_string(16 + dimension * 8));
            line(lastSlot + " =l add " + firstSlot + ", 4");
            line(rowFirst + " =w loadsw " + firstSlot);
            line(rowLast + " =w loadsw " + lastSlot);
            result.innerBounds.push_back({ rowFirst, rowLast });
        }
    } else if (compositeResult) {
        result = withBounds(Value { resultStorage, 'l' }, subprogram->returnType, nullptr);
    }
    // The exception check above bypasses every copy-back on propagation.
    // Adopt dynamic results first so a failed copy-back cannot leak them.
    // Formal declaration order is our choice of Ada's arbitrary copy-back order.
    for (const ScalarCopyBack& copy : copyBacks) {
        Value value = loadFrom(copy.temporary, copy.formalType);
        emitRangeCheck(value, copy.argument->type, copy.argument->location);
        storeInto(copy.actual, value, copy.argument->type);
    }
    return result;
}

// Marshals a call into the run time library straight from the parameter list:
// arrays become a pointer and a length, anything writable or composite becomes
// an address, and everything else travels by value.
Value QbeEmitter::emitRuntimeCall(CallExpr* expr, Symbol* subprogram)
{
    std::vector<std::string> arguments;

    for (std::size_t i = 0; i < subprogram->parameters.size(); ++i) {
        Symbol* parameter = subprogram->parameters[i];
        Expr* argument = expr->resolvedArguments[i];
        Type* formal = baseType(parameter->type);
        char formalClass = qbeClass(parameter->type);

        if (formal != nullptr && formal->kind == TypeKind::Array) {
            Value pointer = emitExpr(argument);
            Value length = lengthOf(pointer, argument->type);
            arguments.push_back("l " + pointer.name);
            arguments.push_back("w " + length.name);
            continue;
        }

        if (parameter->byReference) {
            Value address = isComposite(argument->type) ? emitExpr(argument) : emitAddress(argument);
            arguments.push_back("l " + address.name);
            continue;
        }

        Value value = emitExpr(argument);
        emitRangeCheck(value, parameter->type, argument->location);
        // Imported C functions use the declared scalar parameter class:
        // Float travels as s and Long_Float as d, without default promotion.
        arguments.push_back(std::string(1, formalClass) + " " + value.name);
    }

    std::string argumentList;
    for (std::size_t i = 0; i < arguments.size(); ++i) {
        if (i > 0) {
            argumentList += ", ";
        }
        argumentList += arguments[i];
    }

    if (subprogram->returnType == nullptr) {
        line("call " + subprogram->runtimeSymbol + "(" + argumentList + ")");
        return Value { "0", 'w' };
    }

    if (isUnconstrainedArray(subprogram->returnType)) {
        // A run time function that yields a string hands back a C string, so
        // its bounds are recovered here.
        std::string pointer = newTemp();
        line(pointer + " =l call " + subprogram->runtimeSymbol + "(" + argumentList + ")");
        std::string size = newTemp();
        line(size + " =l call $strlen(l " + pointer + ")");
        std::string length = newTemp();
        line(length + " =w copy " + size);
        return Value { pointer, 'l', "1", length };
    }

    char type = qbeClass(subprogram->returnType);
    std::string temp = newTemp();
    line(temp + " =" + std::string(1, type) + " call " + subprogram->runtimeSymbol + "(" + argumentList + ")");
    return Value { temp, type };
}
