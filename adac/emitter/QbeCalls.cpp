#include "QbeEmitter.h"
#include "QbeSupport.h"

#include <algorithm>
#include <utility>

using QbeSupport::isUnconstrainedArray;

Value QbeEmitter::emitCall(CallExpr* expr)
{
    // Constructor arguments own their temporaries independently of the
    // destination being constructed by this call.
    struct ConstructionContext
    {
        std::string& m_owner;
        std::string& m_arena;
        std::string m_savedOwner;
        std::string m_savedArena;
        ~ConstructionContext()
        {
            m_owner = m_savedOwner;
            m_arena = m_savedArena;
        }
    } construction { m_context->m_constructionOwner, m_context->m_constructionArena,
        std::exchange(m_context->m_constructionOwner, ""), std::exchange(m_context->m_constructionArena, "") };
    Symbol* subprogram = expr->subprogram;
    if (subprogram == nullptr) {
        return Value { "0", 'w' };
    }
    if (!expr->m_dispatching && subprogram->m_inheritedFrom != nullptr) {
        CallExpr inherited;
        inherited.location = expr->location;
        inherited.form = CallForm::Subprogram;
        inherited.subprogram = subprogram->m_inheritedFrom;
        inherited.type = expr->type;
        inherited.resolvedArguments = expr->resolvedArguments;
        Value result = emitCall(&inherited);
        if (expr->type != nullptr && expr->type->m_tagged
            && rootType(expr->type) != rootType(inherited.subprogram->returnType)) {
            line("storel " + typeTag(expr->type).name + ", " + result.name);
        }
        return result;
    }
    if (!expr->m_dispatching && subprogram->m_negatedEquality != nullptr) {
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
    if (!expr->m_dispatching && (subprogram->m_renamedSubprogram != nullptr || subprogram->m_renamedAccess != nullptr)) {
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

    std::string resultTarget = std::move(m_context->m_resultTarget);
    Value resultShape = m_context->m_resultTargetShape;
    std::string resultDescriptor = std::move(m_context->m_resultDescriptor);
    std::string resultProvided = std::move(m_context->m_resultProvided);
    m_context->m_resultDescriptor.clear();
    m_context->m_resultProvided.clear();
    std::string resultOwner = std::move(m_context->m_resultTargetOwner);
    std::string resultArena = std::move(m_context->m_resultTargetArena);
    m_context->m_resultTarget.clear();
    m_context->m_resultTargetOwner.clear();
    m_context->m_resultTargetArena.clear();

    bool dispatchEquality = expr->m_dispatching && (subprogram->name == "=" || subprogram->name == "/=")
        && subprogram->parameters.size() == 2
        && rootType(subprogram->parameters[0]->type) == subprogram->m_controllingType
        && rootType(subprogram->parameters[1]->type) == subprogram->m_controllingType;
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
    bool controlledResult = needsFinalization(subprogram->returnType)
        || (subprogram->returnType != nullptr && subprogram->returnType->m_tagged);
    bool dynamicResult = isUnconstrainedArray(subprogram->returnType);
    bool taggedResult = subprogram->returnType != nullptr && subprogram->returnType->m_tagged;
    std::string resultStorage;
    bool limitedResult = hasLimitedControlledParts(subprogram->returnType);
    if (compositeResult) {
        resultStorage = allocScratch(taggedResult ? 8 : dynamicResult ? 24 + 8 * (subprogram->returnType->arrayRank - 1) : std::max(1LL, typeSize(subprogram->returnType)));
        if (!resultTarget.empty() && !taggedResult && !dynamicResult) {
            resultStorage = resultTarget;
        }
        if (limitedResult && taggedResult) {
            line("storel " + (resultTarget.empty() ? "0" : resultTarget) + ", " + resultStorage);
        }
        if (limitedResult && dynamicResult && !resultTarget.empty()) {
            line("storel " + resultTarget + ", " + resultStorage);
            for (int dimension = 0; dimension < subprogram->returnType->arrayRank; ++dimension) {
                std::string first = dimension == 0 ? resultShape.first : resultShape.innerBounds[dimension - 1].first;
                std::string last = dimension == 0 ? resultShape.last : resultShape.innerBounds[dimension - 1].second;
                std::string firstSlot = newTemp();
                std::string lastSlot = newTemp();
                int offset = dimension == 0 ? 8 : 16 + dimension * 8;
                line(firstSlot + " =l add " + resultStorage + ", " + std::to_string(offset));
                line(lastSlot + " =l add " + firstSlot + ", 4");
                line("storew " + first + ", " + firstSlot);
                line("storew " + last + ", " + lastSlot);
            }
        }
        if (!resultDescriptor.empty()) {
            resultStorage = resultDescriptor;
        }
        arguments.push_back("l " + resultStorage);
        if (controlledResult) {
            arguments.push_back("l " + (resultOwner.empty() ? m_context->m_temporaryFinalizationChain : resultOwner));
            arguments.push_back("l " + (resultArena.empty() ? storageArena(true, true) : resultArena));
            if (taggedResult || limitedResult) {
                arguments.push_back("w " + (resultProvided.empty() ? (resultTarget.empty() ? "0" : "1") : resultProvided));
            }
        }
    }
    if (subprogram->level > 0 && !expr->m_dispatching) {
        Value link = staticLinkFor(subprogram->level - 1);
        arguments.push_back("l " + link.name);
    }

    struct TagContext
    {
        std::string& m_current;
        std::string m_saved;
        ~TagContext()
        {
            m_current = m_saved;
        }
    } tagContext { m_context->m_controllingTag, m_context->m_controllingTag };
    std::string controllingTag;
    std::vector<Value> controllingValues(subprogram->parameters.size());
    if (expr->m_dispatching) {
        for (std::size_t i = 0; i < subprogram->parameters.size(); ++i) {
            Expr* argument = expr->resolvedArguments[i];
            if (rootType(subprogram->parameters[i]->type) == subprogram->m_controllingType
                && !argument->m_tagIndeterminate) {
                Value value = emitExpr(argument);
                controllingValues[i] = value;
                if (m_context->m_controllingTag == tagContext.m_saved) {
                    std::string tag = newTemp();
                    line(tag + " =l loadl " + value.name);
                    m_context->m_controllingTag = tag;
                }
            }
        }
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
            Value value = !controllingValues[i].name.empty() ? controllingValues[i]
                : isComposite(argument->type) ? emitExpr(argument) : emitAddress(argument);
            if (parameter->type->m_boundsSymbol != nullptr) {
                Value target = withBounds(Value {}, parameter->type, nullptr);
                checkArrayShape(target, parameter->type, value, argument->type);
                value.first = target.first;
                value.last = target.last;
                value.innerBounds = target.innerBounds;
            }
            if (expr->m_dispatching && rootType(parameter->type) == subprogram->m_controllingType) {
                std::string tag = newTemp();
                line(tag + " =l loadl " + value.name);
                if (controllingTag.empty()) {
                    controllingTag = tag;
                } else {
                    std::string same = newTemp();
                    std::string valid = newLabel("sametag");
                    std::string invalid = newLabel("tagmismatch");
                    line(same + " =w ceql " + controllingTag + ", " + tag);
                    branch(Value { same, 'w' }, valid, invalid);
                    label(invalid);
                    raiseConstraintError();
                    label(valid);
                }
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

    if (expr->m_dispatching) {
        if (controllingTag.empty()) {
            controllingTag = m_context->m_controllingTag;
        }
        if (controllingTag.empty()) {
            m_diagnostics.error(expr->location, "a dispatching result requires a controlling tag");
            return Value { "0", 'l' };
        }
        std::string tableSlot = newTemp();
        std::string table = newTemp();
        std::string slot = newTemp();
        std::string linkSlot = newTemp();
        callee = newTemp();
        indirectLink = newTemp();
        if (dispatchEquality) {
            line(slot + " =l add " + controllingTag + ", 32");
        } else {
            line(tableSlot + " =l add " + controllingTag + ", 24");
            line(table + " =l loadl " + tableSlot);
            line(slot + " =l add " + table + ", " + std::to_string(16 * subprogram->m_dispatchSlot));
        }
        line(callee + " =l loadl " + slot);
        line(linkSlot + " =l add " + slot + ", 8");
        line(indirectLink + " =l loadl " + linkSlot);
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
    if (expr->m_indirect || expr->m_dispatching) {
        std::string nested = newLabel("nestedcallback");
        std::string library = newLabel("librarycallback");
        std::string done = newLabel("callbackdone");
        std::string hasLink = newTemp();
        if (dispatchEquality) {
            line(hasLink + " =w copy 1");
        } else {
            line(hasLink + " =w cnel " + indirectLink + ", 0");
        }
        branch(Value { hasLink, 'w' }, nested, library);
        label(nested);
        std::vector<std::string> linked = arguments;
        linked.insert(linked.begin() + (controlledResult ? ((taggedResult || limitedResult) ? 4 : 3) : compositeResult ? 1 : 0), "l " + indirectLink);
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
    if (dispatchEquality && subprogram->name == "/=") {
        std::string negated = newTemp();
        line(negated + " =w ceqw " + result.name + ", 0");
        result = Value { negated, 'w' };
    }
    if (taggedResult) {
        std::string pointer = newTemp();
        line(pointer + " =l loadl " + resultStorage);
        result = Value { pointer, 'l' };
        if (expr->m_dispatching && rootType(subprogram->returnType) == subprogram->m_controllingType) {
            line("storel " + controllingTag + ", " + pointer);
        }
    } else if (dynamicResult) {
        result = consumeArrayResult(resultStorage, subprogram->returnType->arrayRank, !controlledResult);
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

Value QbeEmitter::consumeArrayResult(const std::string& resultStorage, int rank, bool adopt)
{
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
    if (adopt) {
        line("call $__ada_array_adopt(l " + storageArena(true, true) + ", l " + pointer + ")");
        emitExceptionCheck();
    }
    Value result { pointer, 'l', first, last };
    for (int dimension = 1; dimension < rank; ++dimension) {
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
    return result;
}
