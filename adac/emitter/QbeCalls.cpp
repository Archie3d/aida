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
        std::string& owner;
        std::string& arena;
        std::string savedOwner;
        std::string savedArena;
        ~ConstructionContext()
        {
            owner = savedOwner;
            arena = savedArena;
        }
    } construction { m_context->constructionOwner, m_context->constructionArena,
        std::exchange(m_context->constructionOwner, ""), std::exchange(m_context->constructionArena, "") };
    Symbol* subprogram = expr->subprogram;
    if (subprogram == nullptr) {
        return Value { "0", 'w' };
    }
    if (!expr->dispatching && subprogram->inheritedFrom != nullptr) {
        CallExpr inherited;
        inherited.location = expr->location;
        inherited.form = CallForm::Subprogram;
        inherited.subprogram = subprogram->inheritedFrom;
        inherited.type = expr->type;
        inherited.resolvedArguments = expr->resolvedArguments;
        Value result = emitCall(&inherited);
        if (isComposite(expr->type) && !expr->type->tagged
            && (expr->type->kind != TypeKind::Array || expr->type->constrained)
            && typeSize(expr->type) != typeSize(inherited.subprogram->returnType)
            && (typeSize(expr->type) != typeLayoutSize(expr->type)
                || typeSize(inherited.subprogram->returnType) != typeLayoutSize(inherited.subprogram->returnType))) {
            Value converted = withBounds(Value {allocScratch(std::max(1LL, typeSize(expr->type)), typeAlignment(expr->type)), 'l'}, expr->type, nullptr);
            copyInto(converted, result, expr->type);
            result = converted;
        }
        if (expr->type != nullptr && expr->type->tagged
            && rootType(expr->type) != rootType(inherited.subprogram->returnType)) {
            line("storel " + typeTag(expr->type).name + ", " + result.name);
        }
        return result;
    }
    if (!expr->dispatching && subprogram->negatedEquality != nullptr) {
        CallExpr equality;
        equality.location = expr->location;
        equality.form = CallForm::Subprogram;
        equality.subprogram = subprogram->negatedEquality;
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
    if (!expr->dispatching && (subprogram->renamedSubprogram != nullptr || subprogram->renamedAccess != nullptr)) {
        CallExpr renamed;
        renamed.location = expr->location;
        renamed.form = CallForm::Subprogram;
        renamed.type = expr->type;
        renamed.resolvedArguments = expr->resolvedArguments;
        renamed.subprogram = subprogram->renamedSubprogram;
        if (subprogram->renamedAccess != nullptr) {
            Symbol* binding = subprogram->renamedAccess;
            auto callee = std::make_unique<IdentifierExpr>();
            callee->symbol = binding;
            callee->type = binding->type;
            callee->location = expr->location;
            renamed.callee = std::move(callee);
            renamed.indirect = true;
            renamed.subprogram = binding->type->accessProfile;
        }
        return emitCall(&renamed);
    }
    if (subprogram->builtin == BuiltinKind::Runtime) {
        Value result = emitRuntimeCall(expr, subprogram);
        if (subprogram->canRaise) {
            emitExceptionCheck();
        }
        if (subprogram->returnType != nullptr && subprogram->returnType->scalarBoundsSymbol != nullptr) {
            emitRangeCheck(result, subprogram->returnType, expr->location);
        }
        return result;
    }

    Value protectedObject;
    if (subprogram->protectedType != nullptr) {
        if (expr->protectedObject != nullptr) {
            protectedObject = emitExpr(expr->protectedObject);
            if (baseType(expr->protectedObject->type)->kind == TypeKind::Access) {
                checkNotNull(protectedObject);
            }
        } else if (expr->protectedReceiver != nullptr) {
            protectedObject = addressOf(expr->protectedReceiver);
        }
    }
    std::string resultTarget = std::move(m_context->resultTarget);
    Value resultShape = m_context->resultTargetShape;
    std::string resultDescriptor = std::move(m_context->resultDescriptor);
    std::string resultProvided = std::move(m_context->resultProvided);
    m_context->resultDescriptor.clear();
    m_context->resultProvided.clear();
    std::string resultOwner = std::move(m_context->resultTargetOwner);
    std::string resultArena = std::move(m_context->resultTargetArena);
    m_context->resultTarget.clear();
    m_context->resultTargetOwner.clear();
    m_context->resultTargetArena.clear();

    bool dispatchEquality = expr->dispatching && (subprogram->name == "=" || subprogram->name == "/=")
        && subprogram->parameters.size() == 2
        && rootType(subprogram->parameters[0]->type) == subprogram->controllingType
        && rootType(subprogram->parameters[1]->type) == subprogram->controllingType;
    std::string callee = subprogram->qbeName;
    std::string indirectLink;
    if (expr->indirect) {
        Value descriptor = emitExpr(expr->callee.get());
        checkNotNull(descriptor);
        callee = newTemp();
        std::string linkSlot = newTemp();
        indirectLink = newTemp();
        line(callee + " =l loadl " + descriptor.name);
        line(linkSlot + " =l add " + descriptor.name + ", 8");
        line(indirectLink + " =l loadl " + linkSlot);
        if (subprogram->protectedAccessProfile) {
            std::string objectSlot = newTemp();
            protectedObject = Value {newTemp(), 'l'};
            line(objectSlot + " =l add " + descriptor.name + ", 16");
            line(protectedObject.name + " =l loadl " + objectSlot);
        }
    }

    struct ScalarCopyBack
    {
        Value actual;
        Value temporary;
        Type* formalType;
        Expr* argument;
    };
    std::vector<ScalarCopyBack> copyBacks;
    std::vector<StorageCopyBack> storageCopies;
    std::vector<ArrayCopyBack> arrayCopies;
    std::vector<std::string> arguments;
    bool accessReturn = subprogram->returnType != nullptr && subprogram->returnType->anonymousAccess;
    std::string accessResultLevel;
    if (accessReturn) {
        accessResultLevel = allocScratch(4, 4);
        line("storew 0, " + accessResultLevel);
        arguments.push_back("l " + accessResultLevel);
    }
    bool compositeResult = isComposite(subprogram->returnType);
    bool accessResult = hasAccessDiscriminants(subprogram->returnType);
    bool controlledResult = accessResult || needsFinalization(subprogram->returnType)
        || (subprogram->returnType != nullptr && subprogram->returnType->tagged);
    bool dynamicResult = isUnconstrainedArray(subprogram->returnType);
    bool taggedResult = subprogram->returnType != nullptr && subprogram->returnType->tagged;
    std::string resultStorage;
    bool limitedResult = hasLimitedControlledParts(subprogram->returnType);
    if (accessResult) {
        if (resultOwner.empty()) { resultOwner = construction.savedOwner.empty() ? m_context->finalizationChain : construction.savedOwner; }
        if (resultArena.empty()) { resultArena = construction.savedArena.empty() ? storageArena(false, true) : construction.savedArena; }
    }
    if (compositeResult) {
        resultStorage = taggedResult || dynamicResult
            ? allocScratch(taggedResult ? 8 : adaArrayResultSize(subprogram->returnType->arrayRank), 8)
            : allocTypeScratch(subprogram->returnType);
        if (!resultTarget.empty() && !taggedResult && !dynamicResult) {
            resultStorage = resultTarget;
        }
        if (limitedResult && taggedResult) {
            line("storel " + (resultTarget.empty() ? "0" : resultTarget) + ", " + resultStorage);
        }
        if (limitedResult && dynamicResult && !resultTarget.empty()) {
            line("storel " + resultTarget + ", " + resultStorage);
            storeArrayView(resultStorage, resultShape, subprogram->returnType, true);
        }
        if (!resultDescriptor.empty()) {
            resultStorage = resultDescriptor;
        }
        arguments.push_back("l " + resultStorage);
        if (controlledResult) {
            arguments.push_back("l " + (resultOwner.empty() ? m_context->temporaryFinalizationChain : resultOwner));
            arguments.push_back("l " + (resultArena.empty() ? storageArena(true, true) : resultArena));
            if (taggedResult || limitedResult) {
                arguments.push_back("w " + (resultProvided.empty() ? (resultTarget.empty() ? "0" : "1") : resultProvided));
            }
        }
    }
    if (accessResult) {
        arguments.push_back("w " + (m_context->coextensionLevel.empty() ? currentAccessLevel() : m_context->coextensionLevel));
    }
    if (subprogram->level > 0 && !expr->dispatching) {
        Value link = staticLinkFor(subprogram->level - 1);
        arguments.push_back("l " + link.name);
    }

    if (subprogram->protectedType != nullptr || subprogram->protectedAccessProfile) {
        arguments.push_back("l " + protectedObject.name);
    }

    struct TagContext
    {
        std::string& current;
        std::string saved;
        ~TagContext()
        {
            current = saved;
        }
    } tagContext { m_context->controllingTag, m_context->controllingTag };
    std::string controllingTag;
    std::vector<Value> controllingValues(subprogram->parameters.size());
    if (expr->dispatching) {
        for (std::size_t i = 0; i < subprogram->parameters.size(); ++i) {
            Expr* argument = expr->resolvedArguments[i];
            if (controllingParameterType(subprogram->parameters[i]->type) == subprogram->controllingType
                && !argument->tagIndeterminate) {
                Value value = emitExpr(argument);
                controllingValues[i] = value;
                if (subprogram->parameters[i]->type->kind == TypeKind::Access) { checkNotNull(value); }
                if (m_context->controllingTag == tagContext.saved) {
                    std::string tag = newTemp();
                    line(tag + " =l loadl " + value.name);
                    m_context->controllingTag = tag;
                }
            }
        }
    }
    for (std::size_t i = 0; i < subprogram->parameters.size(); ++i) {
        Symbol* parameter = subprogram->parameters[i];
        Expr* argument = expr->resolvedArguments[i];
        if (parameter->byReference) {
            if (isScalar(parameter->type)) {
                // A private formal can retain the pointer ABI after its full
                // view becomes scalar. An in actual may also be an expression.
                if (parameter->mode == ParameterMode::In) {
                    Value value = emitExpr(argument);
                    emitRangeCheck(value, parameter->type, argument->location);
                    Value temporary { allocScratch(std::max(1LL, typeSize(parameter->type)), typeAlignment(parameter->type)), 'l' };
                    storeInto(temporary, value, parameter->type);
                    arguments.push_back("l " + temporary.name);
                    continue;
                }
                // Keep the pointer ABI, but give each formal a distinct object.
                // Evaluate the actual's address once, including indexed names.
                Value actual = emitAddress(argument);
                Value temporary { allocScratch(std::max(1LL, typeSize(parameter->type)), typeAlignment(parameter->type)), 'l' };
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
                : argument->type->kind == TypeKind::Array ? emitArrayArgument(argument, parameter->mode, arrayCopies)
                : isComposite(argument->type) ? emitExpr(argument) : emitAddress(argument);
            if (parameter->type->kind == TypeKind::Record) {
                emitRecordConstraintCheck(value, parameter->type);
            }
            if (parameter->type->kind == TypeKind::Array
                && (parameter->type->constrained || parameter->type->boundsSymbol != nullptr)) {
                Value target = withBounds(Value {}, parameter->type, nullptr);
                checkArrayShape(target, parameter->type, value, argument->type);
                value.first = target.first;
                value.last = target.last;
                value.innerBounds = target.innerBounds;
            }
            if (expr->dispatching && rootType(parameter->type) == subprogram->controllingType) {
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
            value = adaptCompositeStorage(value, argument->type, parameter->type, parameter->mode, storageCopies);
            arguments.push_back("l " + value.name);
            if (needsConstrainedParameter(parameter)) {
                arguments.push_back("w " + recordConstrained(argument, &value).name);
            }
            if (isUnconstrainedArray(parameter->type)) {
                if (!value.hasBounds()) {
                    value = withBounds(value, argument->type, nullptr);
                }
                value = arrayView(value, argument->type);
                for (int dimension = 0; dimension < parameter->type->arrayRank; ++dimension) {
                    arguments.push_back("l " + value.axisFirst(dimension));
                    arguments.push_back("l " + value.axisLast(dimension));
                    arguments.push_back("l " + value.lengths[dimension]);
                    arguments.push_back("l " + value.strides[dimension]);
                }
            }
        } else {
            Value value = expr->dispatching && controllingParameterType(parameter->type) == subprogram->controllingType
                ? controllingValues[i] : Value {};
            if (value.name.empty()) { value = emitExpr(argument); }
            if (parameter->type->accessParameter && subprogram->controllingType != nullptr
                && controllingParameterType(parameter->type) == subprogram->controllingType) { checkNotNull(value); }
            emitRangeCheck(value, parameter->type, argument->location);
            arguments.push_back(std::string(1, qbeClass(parameter->type)) + " " + value.name);
            if (parameter->type->accessParameter) { arguments.push_back("w " + (argument->kind == ExprKind::Allocator ? "-1" : value.accessLevel)); }
        }
    }

    if (expr->dispatching) {
        if (controllingTag.empty()) {
            controllingTag = m_context->controllingTag;
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
            line(slot + " =l add " + table + ", " + std::to_string(16 * subprogram->dispatchSlot));
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

    std::string protectedAction;
    if ((expr->protectedObject != nullptr && subprogram->protectedType != nullptr) || subprogram->protectedAccessProfile) {
        protectedAction = allocScratch(16);
        if (subprogram->entry) {
            std::string link = subprogram->level > 0 ? staticLinkFor(subprogram->level - 1).name : "0";
            emitCallInstruction("call $__ada_protected_entry_enter(l %.taskContext, l " + protectedObject.name
                + ", l " + protectedAction + ", l " + subprogram->qbeName + ".barrier, l " + link + ")", CallEffects::DeferredCheck);
        } else {
            emitCallInstruction("call $__ada_protected_enter(l %.taskContext, l " + protectedObject.name + ", l " + protectedAction + ")", CallEffects::DeferredCheck);
        }
        emitExceptionCheck();
    }
    Value result { "0", 'w' };
    auto invoke = [&](const std::string& actuals) {
        if (subprogram->returnType != nullptr && !compositeResult) {
            char type = qbeClass(subprogram->returnType);
            std::string temp = newTemp();
            emitCallInstruction(temp + " =" + std::string(1, type) + " call " + callee + "(" + actuals + ")", CallEffects::DeferredCheck);
            return Value { temp, type };
        }
        emitCallInstruction("call " + callee + "(" + actuals + ")", CallEffects::DeferredCheck);
        return Value { "0", 'w' };
    };
    if (expr->indirect || expr->dispatching) {
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
        linked.insert(linked.begin() + ((controlledResult ? ((taggedResult || limitedResult) ? 4 : 3) : compositeResult ? 1 : 0) + (accessResult ? 1 : 0) + (accessReturn ? 1 : 0)), "l " + indirectLink);
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

    if (!protectedAction.empty()) {
        if (subprogram->returnType == nullptr) {
            emitCallInstruction("call $__ada_protected_complete(l %.taskContext, l " + protectedAction + ")", CallEffects::DeferredCheck);
        } else {
            emitCallInstruction("call $__ada_protected_leave(l %.taskContext, l " + protectedAction + ")", CallEffects::DeferredCheck);
        }
    }
    emitExceptionCheck();
    if (accessReturn) {
        result.accessLevel = newTemp();
        line(result.accessLevel + " =w loadsw " + accessResultLevel);
    }
    if (dispatchEquality && subprogram->name == "/=") {
        std::string negated = newTemp();
        line(negated + " =w ceqw " + result.name + ", 0");
        result = Value { negated, 'w' };
    }
    if (taggedResult) {
        std::string pointer = newTemp();
        line(pointer + " =l loadl " + resultStorage);
        result = Value { pointer, 'l' };
        if (expr->dispatching && rootType(subprogram->returnType) == subprogram->controllingType) {
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
    emitStorageCopyBacks(storageCopies);
    emitArrayCopyBacks(arrayCopies);
    return result;
}

// Marshals a call into the run time library straight from the parameter list:
// arrays become a pointer and a length, anything writable or composite becomes
// an address, and everything else travels by value.
Value QbeEmitter::emitRuntimeCall(CallExpr* expr, Symbol* subprogram)
{
    std::vector<std::string> arguments;
    std::vector<ArrayCopyBack> arrayCopies;
    std::vector<StorageCopyBack> storageCopies;
    struct SharedCopyBack
    {
        Value actual;
        Value temporary;
        Type* type;
        Expr* argument;
    };
    std::vector<SharedCopyBack> sharedCopies;
    auto copyBack = [&]() {
        if (!sharedCopies.empty()) {
            emitExceptionCheck();
            for (const auto& copy : sharedCopies) {
                Value value = loadFrom(copy.temporary, copy.type);
                emitRangeCheck(value, copy.argument->type, copy.argument->location);
                storeInto(copy.actual, value, copy.argument->type);
            }
        }
        if (!storageCopies.empty()) {
            emitExceptionCheck();
            emitStorageCopyBacks(storageCopies);
        }
        if (!arrayCopies.empty()) {
            emitExceptionCheck();
            emitArrayCopyBacks(arrayCopies);
        }
    };

    for (std::size_t i = 0; i < subprogram->parameters.size(); ++i) {
        Symbol* parameter = subprogram->parameters[i];
        Expr* argument = expr->resolvedArguments[i];
        Type* formal = parameter->type;
        char formalClass = qbeClass(parameter->type);

        if (formal != nullptr && formal->kind == TypeKind::Array) {
            Value pointer = emitArrayArgument(argument, parameter->mode, arrayCopies);
            if (parameter->type->constrained || parameter->type->boundsSymbol != nullptr) {
                pointer = convertArrayBounds(pointer, argument->type, parameter->type, argument->location);
            }
            pointer = adaptCompositeStorage(pointer, argument->type, formal, parameter->mode, storageCopies);
            Value length = narrowArrayLength(lengthOf(pointer, argument->type));
            arguments.push_back("l " + pointer.name);
            arguments.push_back("w " + length.name);
            continue;
        }

        if (parameter->byReference && isScalar(formal)
            && (formal->excludesNull || sharedObjectKind(argument) != 0 || sharedTypeKind(formal) != 0 || !formal->enumCodes.empty()
                || typeSize(formal) != typeSize(argument->type))) {
            Value temporary { allocTypeScratch(formal), 'l' };
            Value actual;
            if (parameter->mode != ParameterMode::In) {
                actual = emitAddress(argument);
                sharedCopies.push_back({actual, temporary, formal, argument});
            }
            if (parameter->mode != ParameterMode::Out || formal->kind == TypeKind::Access) {
                Value value = parameter->mode == ParameterMode::In ? emitExpr(argument) : loadFrom(actual, argument->type);
                if (parameter->mode != ParameterMode::Out) {
                    emitRangeCheck(value, formal, argument->location);
                }
                storeInto(temporary, value, formal);
            }
            arguments.push_back("l " + temporary.name);
            continue;
        }
        if (parameter->byReference) {
            Value address = isComposite(argument->type) ? emitExpr(argument) : emitAddress(argument);
            if (parameter->type->kind == TypeKind::Record) {
                emitRecordConstraintCheck(address, parameter->type);
            }
            address = adaptCompositeStorage(address, argument->type, formal, parameter->mode, storageCopies);
            arguments.push_back("l " + address.name);
            continue;
        }

        Value value = emitExpr(argument);
        emitRangeCheck(value, parameter->type, argument->location);
        if (!subprogram->importAda) { value = enumCode(value, parameter->type); }
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
        emitCallInstruction("call " + subprogram->runtimeSymbol + "(" + argumentList + ")", CallEffects::DeferredCheck);
        copyBack();
        return Value { "0", 'w' };
    }

    if (subprogram->runtimeSymbol == "$__ada_file_name") {
        std::string descriptor = allocScratch(adaArrayResultSize(1));
        emitCallInstruction("call $__ada_file_name(l " + descriptor + ", " + argumentList + ")", CallEffects::CheckException);
        Value result = consumeArrayResult(descriptor, 1, true);
        copyBack();
        return result;
    }

    if (isUnconstrainedArray(subprogram->returnType)) {
        // A run time function that yields a string hands back a C string, so
        // its bounds are recovered here.
        std::string pointer = newTemp();
        emitCallInstruction(pointer + " =l call " + subprogram->runtimeSymbol + "(" + argumentList + ")", CallEffects::DeferredCheck);
        std::string size = newTemp();
        emitCallInstruction(size + " =l call $strlen(l " + pointer + ")", CallEffects::DeferredCheck);
        std::string length = newTemp();
        line(length + " =l copy " + size);
        copyBack();
        return Value { pointer, 'l', "1", length };
    }

    char type = qbeClass(subprogram->returnType);
    std::string temp = newTemp();
    emitCallInstruction(temp + " =" + std::string(1, type) + " call " + subprogram->runtimeSymbol + "(" + argumentList + ")", CallEffects::DeferredCheck);
    copyBack();
    if (!subprogram->importAda && !subprogram->returnType->enumCodes.empty()) {
        emitExceptionCheck();
        Value value = enumPosition(Value { temp, type }, subprogram->returnType);
        emitRangeCheck(value, subprogram->returnType, expr->location);
        return value;
    }
    return Value { temp, type };
}

Value QbeEmitter::consumeArrayResult(const std::string& resultStorage, int rank, bool adopt)
{
    // Adopt the transfer buffer into the caller's expression lifetime.
    std::string pointer = newTemp();
    line(pointer + " =l loadl " + resultStorage);
    Value result = loadArrayView(resultStorage, rank, true);
    result.name = pointer;
    result.type = 'l';
    if (adopt) {
        emitCallInstruction("call $__ada_array_adopt(l " + storageArena(true, true) + ", l " + pointer + ")", CallEffects::CheckException);
    }
    return result;
}
