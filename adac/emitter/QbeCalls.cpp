#include "QbeEmitter.h"
#include "QbeSupport.h"

#include <algorithm>
#include <utility>

using QbeSupport::isUnconstrainedArray;

// Canonical native snapshots contain logical components, zero padding and a
// static tag. They never acquire ownership or register finalization actions.
void QbeEmitter::emitStorageIoComponents(const Value& target, const Value& source, Type* type, bool reading)
{
    if (type->packedBoolean) {
        Value view = source.hasBounds() ? source : withBounds(source, type, nullptr);
        std::string bits = lengthOf(view, type).name;
        emitCallInstruction("call $__ada_unchecked_copy(l " + target.name + ", l " + bits
            + ", l " + source.name + ", l " + bits + ")", CallEffects::CheckException);
        return;
    }
    if (type->kind == TypeKind::Array) {
        Value view = source.hasBounds() ? source : withBounds(source, type, nullptr);
        std::string stride = arrayElementSize(view, type);
        std::string length = lengthOf(view, type).name;
        std::string cursor = allocScratch(8);
        line("storel 0, " + cursor);
        std::string head = newLabel("storage_elements"), body = newLabel("storage_element");
        std::string done = newLabel("storage_elements_done");
        jump(head);
        label(head);
        std::string index = newTemp(), more = newTemp();
        line(index + " =l loadl " + cursor);
        line(more + " =w cultl " + index + ", " + length);
        branch(Value {more, 'w'}, body, done);
        label(body);
        std::string offset = newTemp();
        Value from = arrayRow(view, type), to = from;
        from.name = newTemp();
        to.name = newTemp();
        line(offset + " =l mul " + index + ", " + stride);
        line(from.name + " =l add " + source.name + ", " + offset);
        line(to.name + " =l add " + target.name + ", " + offset);
        emitStorageIoComponents(to, from, type->element, reading);
        std::string next = newTemp();
        line(next + " =l add " + index + ", 1");
        line("storel " + next + ", " + cursor);
        jump(head);
        label(done);
        return;
    }
    if (type->kind == TypeKind::Record) {
        if (type->tagged) {
            if (reading) {
                std::string tag = newTemp(), same = newTemp();
                line(tag + " =l loadl " + source.name);
                line(same + " =w ceql " + tag + ", 0");
                std::string good = newLabel("storage_tag"), bad = newLabel("storage_bad_tag");
                branch(Value {same, 'w'}, good, bad);
                label(bad);
                raiseConstraintError();
                label(good);
            }
            line("storel " + (reading ? typeTag(type).name : "0") + ", " + target.name);
        }
        Type* record = baseType(type);
        for (const FieldInfo& field : record->fields) {
            std::string done;
            if (field.variant >= 0) {
                std::string active = newLabel("storage_variant");
                done = newLabel("storage_variant_done");
                branch(variantCondition(source, record, field.variant), active, done);
                label(active);
            }
            emitStorageIoComponents(fieldAddress(target, record, field), fieldAddress(source, record, field), field.type, reading);
            if (!done.empty()) { jump(done); label(done); }
        }
        return;
    }
    Value value = loadFrom(source, type);
    emitRangeCheck(value, type, m_context->sourceLocation);
    storeInto(target, value, type);
}

Value QbeEmitter::emitStorageIo(CallExpr* expr, bool reading)
{
    Type* type = expr->subprogram->parameters[1]->type;
    Value buffer = emitAddress(expr->resolvedArguments[0]);
    Value item;
    if (reading || isComposite(type)) {
        item = emitAddress(expr->resolvedArguments[1]);
    } else {
        Value value = emitExpr(expr->resolvedArguments[1]);
        item = Value {allocTypeScratch(type), 'l'};
        storeInto(item, value, type);
    }
    Value snapshot {allocTypeScratch(type), 'l'};
    std::string size = runtimeSize(type);
    std::string same = newTemp();
    line(same + " =w ceql " + lengthOf(buffer, expr->subprogram->parameters[0]->type).name + ", " + size);
    std::string fits = newLabel("storage_buffer"), bad = newLabel("storage_bad_buffer");
    branch(Value {same, 'w'}, fits, bad);
    label(bad);
    raiseConstraintError();
    label(fits);
    emitCallInstruction("call $memset(l " + snapshot.name + ", w 0, l " + size + ")", CallEffects::DeferredCheck);
    Value source = item;
    if (reading) {
        // Buffer_Type has byte alignment, even when it is a slice. Do not load
        // scalars or tags directly from its potentially unaligned storage.
        source = Value {allocTypeScratch(type), 'l'};
        emitCallInstruction("call $memmove(l " + source.name + ", l " + buffer.name
            + ", l " + size + ")", CallEffects::DeferredCheck);
    }
    auto savedChecks = m_suppressedChecks;
    m_suppressedChecks = 0;
    emitStorageIoComponents(snapshot, source, type, reading);
    m_suppressedChecks = savedChecks;
    if (reading) {
        if (needsFinalization(type)) {
            // A byte snapshot borrows its resources. Acquire an adjusted
            // temporary before assignment can finalize an aliasing target.
            bool savedTemporary = m_context->initializingTemporary;
            m_context->initializingTemporary = true;
            prepareControlledObject(snapshot, type);
            m_context->initializingTemporary = savedTemporary;
            adjustControlledObject(snapshot, type, type->tagged ? typeTag(type).name : "");
        }
        Symbol temporary;
        temporary.type = type;
        temporary.owner = m_context->symbol;
        m_context->locals[&temporary] = snapshot.name;
        if (type->kind == TypeKind::Array) { m_context->bounds[&temporary] = withBounds(snapshot, type, nullptr); }
        IdentifierExpr value;
        value.type = type;
        value.symbol = &temporary;
        assignInto(item, type, &value, false, recordConstrained(expr->resolvedArguments[1], &item).name);
        m_context->locals.erase(&temporary);
        m_context->bounds.erase(&temporary);
    } else {
        emitCallInstruction("call $memmove(l " + buffer.name + ", l " + snapshot.name
            + ", l " + size + ")", CallEffects::DeferredCheck);
    }
    return Value {"0", 'w'};
}

Value QbeEmitter::emitUncheckedConversion(CallExpr* expr)
{
    Type* sourceType = expr->subprogram->parameters.front()->type;
    Type* targetType = expr->subprogram->returnType;
    Value source = emitExpr(expr->resolvedArguments.front());
    auto bits = [&](Type* type, const Value& value) {
        if (type->packedBoolean) { return lengthOf(value, type).name; }
        return std::to_string(type->valueSizeBits >= 0 ? type->valueSizeBits : typeSize(type) * 8);
    };
    std::string sourceBits = bits(sourceType, source);
    if (!isComposite(sourceType)) {
        Value slot { allocTypeScratch(sourceType), 'l' };
        storeInto(slot, source, sourceType);
        source = slot;
    }
    Value target;
    if (targetType->packedBoolean) {
        target = withBounds(Value {}, targetType, nullptr);
        target.name = newTemp();
        target.type = 'l';
        emitCallInstruction(target.name + " =l call $__ada_array_local(l " + storageArena(true, true)
            + ", l " + target.first + ", l " + target.last + ", l " + std::to_string(ADA_PACKED_BOOLEAN_STRIDE) + ")", CallEffects::CheckException);
    } else {
        target = Value { allocTypeScratch(targetType), 'l' };
        emitCallInstruction("call $memset(l " + target.name + ", w 0, l " + std::to_string(typeSize(targetType)) + ")", CallEffects::DeferredCheck);
    }
    emitCallInstruction("call $__ada_unchecked_copy(l " + target.name + ", l " + bits(targetType, target)
        + ", l " + source.name + ", l " + sourceBits + ")", CallEffects::CheckException);
    if (targetType->packedBoolean) { return arrayView(target, targetType); }
    if (targetType->valueSizeBits > 0 && targetType->valueSizeBits < typeSize(targetType) * 8) {
        target.bitWidth = static_cast<int>(targetType->valueSizeBits);
    }
    Value result = loadFrom(target, targetType);
    emitRangeCheck(result, targetType, expr->location);
    return result;
}

Value QbeEmitter::emitCall(CallExpr* expr, Type* inheritedControllingType)
{
    Value result = emitCallValue(expr, inheritedControllingType);
    if (expr->subprogram != nullptr && expr->subprogram->noReturn) {
        emitExceptionCheck();
        emitCallInstruction("call $__ada_raise(l $__ada_exc_program_error)", CallEffects::CheckException);
    }
    return result;
}

Value QbeEmitter::emitCallValue(CallExpr* expr, Type* inheritedControllingType)
{
    // Constructor arguments own their temporaries independently of the
    // destination being constructed by this call.
    struct ConstructionContext
    {
        std::string& owner;
        std::string& arena;
        std::string savedOwner;
        std::string savedArena;
        std::string& accessLevel;
        std::string savedAccessLevel;
        std::string& master;
        std::string& collection;
        std::string& group;
        std::string savedMaster;
        std::string savedCollection;
        std::string savedGroup;
        ~ConstructionContext()
        {
            owner = savedOwner;
            arena = savedArena;
            accessLevel = savedAccessLevel;
            master = savedMaster;
            collection = savedCollection;
            group = savedGroup;
        }
    } construction { m_context->constructionOwner, m_context->constructionArena,
        std::exchange(m_context->constructionOwner, ""), std::exchange(m_context->constructionArena, ""),
        m_context->constructionAccessLevel, std::exchange(m_context->constructionAccessLevel, ""),
        m_context->taskMaster, m_context->taskCollection, m_context->taskGroup,
        std::exchange(m_context->taskMaster, "0"), std::exchange(m_context->taskCollection, "0"),
        std::exchange(m_context->taskGroup, "0") };
    Symbol* subprogram = expr->subprogram;
    if (subprogram == nullptr) {
        return Value { "0", 'w' };
    }
    if (subprogram->builtin == BuiltinKind::StorageIoRead || subprogram->builtin == BuiltinKind::StorageIoWrite) {
        return emitStorageIo(expr, subprogram->builtin == BuiltinKind::StorageIoRead);
    }
    Symbol* intrinsic = subprogram;
    while (!intrinsic->uncheckedConversion && intrinsic->dispatchingConstructor == nullptr && intrinsic->builtin != BuiltinKind::AddressToPointer
        && intrinsic->renamedSubprogram != nullptr) { intrinsic = intrinsic->renamedSubprogram; }
    if (intrinsic->dispatchingConstructor != nullptr) {
        // Operand calls must not consume the ultimate constructor destination.
        std::string target = std::exchange(m_context->resultTarget, "");
        Value shape = std::exchange(m_context->resultTargetShape, Value {});
        std::string descriptor = std::exchange(m_context->resultDescriptor, "");
        std::string provided = std::exchange(m_context->resultProvided, "");
        std::string owner = std::exchange(m_context->resultTargetOwner, "");
        std::string arena = std::exchange(m_context->resultTargetArena, "");
        Value tag = emitExpr(expr->resolvedArguments[0]);
        Value params = emitExpr(expr->resolvedArguments[1]);
        m_context->resultTarget = std::move(target);
        m_context->resultTargetShape = std::move(shape);
        m_context->resultDescriptor = std::move(descriptor);
        m_context->resultProvided = std::move(provided);
        m_context->resultTargetOwner = std::move(owner);
        m_context->resultTargetArena = std::move(arena);
        checkNotNull(params);
        std::string masterSlot = intrinsic->qbeName + ".master";
        if (intrinsic->owner != nullptr) {
            masterSlot = newTemp();
            line(masterSlot + " =l add " + staticLinkFor(intrinsic->owner->level).name
                + ", " + std::to_string(intrinsic->constructorMasterOffset));
        }
        std::string master = newTemp();
        line(master + " =l loadl " + masterSlot);
        Symbol* constructor = intrinsic->dispatchingConstructor;
        emitCallInstruction("call $__ada_tag_check_constructor(l " + tag.name + ", l "
            + typeTag(constructor->controllingType).name + ", l " + master + ")", CallEffects::CheckException);
        Symbol parameter;
        parameter.type = constructor->parameters[0]->type;
        parameter.owner = m_context->symbol;
        std::string slot = allocTypeScratch(parameter.type);
        storeInto(Value {slot, 'l'}, params, parameter.type);
        m_context->locals[&parameter] = slot;
        IdentifierExpr argument;
        argument.symbol = &parameter;
        argument.type = parameter.type;
        CallExpr dispatch;
        dispatch.location = expr->location;
        dispatch.form = CallForm::Subprogram;
        dispatch.subprogram = constructor;
        dispatch.type = expr->type;
        dispatch.dispatching = true;
        dispatch.tagSelected = true;
        dispatch.resolvedArguments = { &argument };
        std::string savedTag = m_context->controllingTag;
        m_context->controllingTag = tag.name;
        m_context->constructionOwner = construction.savedOwner;
        m_context->constructionArena = construction.savedArena;
        Value result = emitCall(&dispatch);
        m_context->controllingTag = savedTag;
        m_context->locals.erase(&parameter);
        return result;
    }
    if (intrinsic->uncheckedConversion) { return emitUncheckedConversion(expr); }
    if (intrinsic->builtin == BuiltinKind::AddressToPointer) {
        Value address = emitExpr(expr->resolvedArguments.front());
        std::string result = newTemp();
        emitCallInstruction(result + " =l call $__ada_address_to_pointer(l " + address.name
            + ", l " + std::to_string(typeAlignment(intrinsic->returnType->target)) + ")", CallEffects::CheckException);
        return Value { result, 'l' };
    }
    if (subprogram->classWideWrapper || subprogram->classWideResultRenaming) {
        Symbol* target = subprogram;
        while (!target->classWideWrapper && target->renamedSubprogram != nullptr) {
            target = target->renamedSubprogram;
        }
        bool controllingParameters = false;
        for (Symbol* parameter : target->parameters) {
            controllingParameters = controllingParameters
                || controllingParameterType(parameter->type) == target->controllingType;
        }
        auto arguments = expr->resolvedArguments;
        arguments.resize(subprogram->parameters.size(), nullptr);
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            if (arguments[i] == nullptr) { arguments[i] = subprogram->parameters[i]->defaultExpr; }
        }
        if (subprogram->classWideResultRenaming || (!controllingParameters
            && (!expr->dispatching || m_context->controllingTag.empty()))) {
            for (Expr* argument : arguments) {
                if (argument != nullptr) { emitExpr(argument); }
            }
            emitCallInstruction("call $__ada_raise(l $__ada_exc_program_error)", CallEffects::CheckException);
            return Value { "0", 'l' };
        }
        CallExpr dispatch;
        dispatch.location = expr->location;
        dispatch.form = CallForm::Subprogram;
        dispatch.subprogram = target->renamedSubprogram;
        dispatch.type = expr->type;
        dispatch.dispatching = true;
        dispatch.resolvedArguments = std::move(arguments);
        return emitCall(&dispatch);
    }
    if (expr->indirect && expr->callee != nullptr && expr->callee->type->accessConventionC) {
        Value code = emitExpr(expr->callee.get());
        checkNotNull(code);
        Symbol imported = *subprogram;
        imported.runtimeSymbol = code.name;
        imported.importAda = false;
        Value result = emitRuntimeCall(expr, &imported);
        emitExceptionCheck();
        emitRangeCheck(result, imported.returnType, expr->location);
        return result;
    }
    if (!expr->dispatching && subprogram->inheritedFrom != nullptr && subprogram->renamedSubprogram == nullptr) {
        CallExpr inherited;
        inherited.location = expr->location;
        inherited.form = CallForm::Subprogram;
        inherited.subprogram = subprogram->inheritedFrom;
        inherited.type = expr->type != nullptr && expr->type->kind == TypeKind::Record && !expr->type->tagged
            ? inherited.subprogram->returnType : expr->type;
        inherited.resolvedArguments = expr->resolvedArguments;
        Value result = emitCall(&inherited, inheritedControllingType != nullptr
            ? inheritedControllingType : subprogram->controllingType);
        if (needsRecordRepack(expr->type, inherited.subprogram->returnType)) {
            Value converted {allocTypeScratch(expr->type), 'l'};
            repackRecord(converted, expr->type, result, inherited.subprogram->returnType);
            emitRecordConstraintCheck(converted, expr->type);
            return converted;
        }
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
    if (subprogram->negatedEquality != nullptr) {
        CallExpr equality;
        equality.location = expr->location;
        equality.form = CallForm::Subprogram;
        equality.subprogram = subprogram->negatedEquality;
        equality.type = subprogram->returnType;
        equality.resolvedArguments = expr->resolvedArguments;
        equality.dispatching = expr->dispatching;
        equality.tagIndeterminate = expr->tagIndeterminate;
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
        std::unique_ptr<IdentifierExpr> object;
        if (subprogram->boundConcurrentObject != nullptr) {
            Symbol* binding = subprogram->boundConcurrentObject;
            object = std::make_unique<IdentifierExpr>();
            object->symbol = binding;
            object->type = binding->type;
            object->location = expr->location;
            if (subprogram->boundTaskIdentity) { renamed.taskReceiver = binding; }
            else if (subprogram->boundProtectedReceiver) { renamed.protectedReceiver = binding; }
            else if (renamed.subprogram->taskType != nullptr) { renamed.taskObject = object.get(); }
            else { renamed.protectedObject = object.get(); }
            if (subprogram->boundConcurrentIndex != nullptr) {
                auto family = std::make_unique<CallExpr>();
                auto index = std::make_unique<IdentifierExpr>();
                index->symbol = subprogram->boundConcurrentIndex;
                index->type = index->symbol->type;
                index->location = expr->location;
                Association argument;
                argument.value = std::move(index);
                family->arguments.push_back(std::move(argument));
                renamed.callee = std::move(family);
            }
        }
        auto* plan = m_context->selectCall;
        Symbol* savedTarget = plan != nullptr ? plan->target : nullptr;
        if (plan != nullptr && plan->target == subprogram) { plan->target = renamed.subprogram; }
        Value result = emitCall(&renamed, inheritedControllingType);
        if (plan != nullptr) { plan->target = savedTarget; }
        return result;
    }
    DiscriminantScope discriminants(m_context);
    bool adaImport = isAdaImport(subprogram);
    if (subprogram->builtin == BuiltinKind::Runtime && !adaImport) {
        // File queries return borrowed handle snapshots rather than constructing
        // an owning limited result through the Ada result-buffer ABI.
        std::string fileTarget;
        std::string fileProvided;
        std::string fileOwner;
        std::string fileArena;
        if (isFileHandleView(subprogram->returnType)) {
            fileTarget = std::move(m_context->resultTarget);
            m_context->resultTarget.clear();
            if (fileTarget.empty()) { fileTarget = std::move(m_context->resultDescriptor); }
            fileProvided = std::move(m_context->resultProvided);
            fileOwner = std::move(m_context->resultTargetOwner);
            fileArena = std::move(m_context->resultTargetArena);
            m_context->resultDescriptor.clear();
            m_context->resultProvided.clear();
            m_context->resultTargetOwner.clear();
            m_context->resultTargetArena.clear();
        }
        Value result = emitRuntimeCall(expr, subprogram);
        if (!fileTarget.empty()) {
            emitExceptionCheck();
            Value target {fileTarget, 'l'};
            // Forwarded scratch results have no registration yet. Honor the
            // caller's provided-storage flag, just as an Ada constructor does.
            // Keep cleanup active: this variable may later open its own file.
            if (!fileProvided.empty() && fileProvided != "1") {
                std::string reserve = newLabel("fileresultreserve");
                std::string ready = newLabel("fileresultready");
                branch(Value {fileProvided, 'w'}, ready, reserve);
                label(reserve);
                if (fileOwner.empty()) { fileOwner = m_context->temporaryFinalizationChain; }
                if (fileArena.empty()) { fileArena = storageArena(true, true); }
                emitCallInstruction("call $__ada_finalization_reserve(l " + fileOwner + ", l " + fileArena
                    + ", l " + fileTarget + ", l $__ada_file_finalize)", CallEffects::CheckException);
                jump(ready);
                label(ready);
            }
            copyControlledObject(target, result, subprogram->returnType, true);
            result = target;
        }
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
    if (subprogram->protectedType != nullptr && !protectedObject.name.empty()) {
        m_context->discriminantObjects[subprogram->protectedType] = {protectedObject, subprogram->protectedType};
    }
    Value taskIdentity;
    Value entryIndex { "0", 'l' };
    if (subprogram->entry && subprogram->taskType != nullptr) {
        if (expr->taskObject != nullptr) {
            Value object = emitExpr(expr->taskObject);
            if (baseType(expr->taskObject->type)->kind == TypeKind::Access) { checkNotNull(object); }
            m_context->discriminantObjects[subprogram->taskType] = {object, subprogram->taskType};
            taskIdentity = Value {newTemp(), 'l'};
            emitCallInstruction(taskIdentity.name + " =l call $__ada_task_identity(l " + object.name + ")", CallEffects::CheckException);
        } else if (expr->taskReceiver != nullptr) {
            taskIdentity = loadFrom(addressOf(expr->taskReceiver), expr->taskReceiver->type);
        }
        if (subprogram->entryIndexType != nullptr && expr->callee->kind == ExprKind::Call) {
            Expr* index = static_cast<CallExpr*>(expr->callee.get())->arguments.front().value.get();
            entryIndex = emitExpr(index);
            emitRangeCheck(entryIndex, subprogram->entryIndexType, index->location);
            entryIndex = wideBound(entryIndex);
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

    Symbol* dispatchOperation = subprogram;
    if (subprogram->genericFormalSubprogram && subprogram->abstract) {
        while (dispatchOperation->renamedSubprogram != nullptr) { dispatchOperation = dispatchOperation->renamedSubprogram; }
    }
    bool dispatchEquality = expr->dispatching && (dispatchOperation->name == "=" || dispatchOperation->name == "/=")
        && subprogram->parameters.size() == 2
        && rootType(subprogram->parameters[0]->type) == subprogram->controllingType
        && rootType(subprogram->parameters[1]->type) == subprogram->controllingType;
    std::string callee = adaImport ? subprogram->runtimeSymbol : subprogram->qbeName;
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
    bool controlledResult = accessResult || needsFinalization(subprogram->returnType) || hasLimitedParts(subprogram->returnType)
        || (subprogram->returnType != nullptr && subprogram->returnType->tagged);
    bool taskResult = hasTasks(subprogram->returnType);
    std::string taskTemporary;
    if (taskResult && resultTarget.empty() && resultDescriptor.empty()
        && resultOwner.empty() && construction.savedOwner.empty()) {
        taskTemporary = beginTaskTemporary();
    }
    std::string taskMaster = taskTemporary.empty() ? construction.savedMaster : taskTemporary;
    std::string taskCollection = taskTemporary.empty() ? construction.savedCollection : taskTemporary;
    std::string taskGroup = taskTemporary.empty() ? construction.savedGroup : allocScratch(1);
    if (taskResult && taskMaster == "0") {
        taskMaster = newTemp();
        emitCallInstruction(taskMaster + " =l call $__ada_task_owner(l %.taskContext, l " + taskCollection + ")", CallEffects::RuntimeHook);
    }
    bool dynamicResult = isUnconstrainedArray(subprogram->returnType);
    bool taggedResult = subprogram->returnType != nullptr && subprogram->returnType->tagged;
    std::string resultStorage;
    bool limitedResult = hasLimitedParts(subprogram->returnType);
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
    if (taskResult) {
        arguments.push_back("l " + taskMaster);
        arguments.push_back("l " + taskCollection);
        arguments.push_back("l " + taskGroup);
    }
    if (hasProtected(subprogram->returnType)) {
        arguments.push_back("w " + (construction.savedAccessLevel.empty() ? currentAccessLevel() : construction.savedAccessLevel));
    }
    if (accessResult) {
        arguments.push_back("w " + (m_context->coextensionLevel.empty() ? currentAccessLevel() : m_context->coextensionLevel));
    }
    if (subprogram->level > 0 && !adaImport && !expr->dispatching && subprogram->taskType == nullptr) {
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
    // Statically bound inherited calls still supply the descendant's tag to
    // tag-indeterminate operands/defaults, even while invoking a parent body.
    if (!expr->dispatching && subprogram->controllingType != nullptr && subprogram->controllingType->tagged) {
        m_context->controllingTag = typeTag(inheritedControllingType != nullptr
            ? inheritedControllingType : subprogram->controllingType).name;
    }
    std::string controllingTag;
    std::vector<Value> controllingValues(subprogram->parameters.size());
    auto checkControllingTag = [&](const Value& value) {
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
    };
    if (expr->dispatching && !expr->tagSelected) {
        for (std::size_t i = 0; i < subprogram->parameters.size(); ++i) {
            Expr* argument = expr->resolvedArguments[i];
            if (controllingParameterType(subprogram->parameters[i]->type) == subprogram->controllingType
                && !argument->tagIndeterminate) {
                Value value = emitExpr(argument);
                controllingValues[i] = value;
                if (subprogram->parameters[i]->type->kind == TypeKind::Access) { checkNotNull(value); }
                checkControllingTag(value);
                m_context->controllingTag = controllingTag;
            }
        }
    }
    auto* selectPlan = m_context->selectCall != nullptr && m_context->selectCall->target == subprogram
        ? m_context->selectCall : nullptr;
    for (std::size_t i = 0; i < subprogram->parameters.size(); ++i) {
        Symbol* parameter = subprogram->parameters[i];
        Expr* argument = expr->resolvedArguments[i];
        if (selectPlan != nullptr && selectPlan->requeue && parameter->byReference && isScalar(parameter->type)) {
            Value address = emitAddress(argument);
            arguments.push_back("l " + address.name);
            if (parameter->type->accessParameter) {
                Value value = loadFrom(address, argument->type);
                arguments.push_back("w " + value.accessLevel);
            }
            continue;
        }
        if (parameter->byReference) {
            if (isScalar(parameter->type)) {
                // A private formal can retain the pointer ABI after its full
                // view becomes scalar. An in actual may also be an expression.
                if (parameter->mode == ParameterMode::In) {
                    Value value = emitExpr(argument);
                    emitRangeCheck(value, parameter->type, argument->location);
                    Value temporary { allocScratch(std::max(1LL, typeSize(parameter->type)), typeAlignment(parameter->type)), 'l' };
                    // Entry actuals belong to the whole call, including all
                    // requeues; an allocator must not die with its first body.
                    if (subprogram->entry && parameter->type->accessParameter && argument->kind == ExprKind::Allocator) {
                        emitCallInstruction("call $__ada_access_parameter_adopt(l " + m_context->temporaryFinalizationChain
                            + ", l " + storageArena(true, true) + ", l " + value.name + ")", CallEffects::CheckException);
                    }
                    storeInto(temporary, value, parameter->type);
                    arguments.push_back("l " + temporary.name);
                    if (parameter->type->accessParameter) { arguments.push_back("w " + value.accessLevel); }
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
            Value value = selectPlan != nullptr && selectPlan->requeue ? emitExpr(argument)
                : !controllingValues[i].name.empty() ? controllingValues[i]
                : argument->type->kind == TypeKind::Array ? emitArrayArgument(argument, parameter->mode, arrayCopies)
                : isComposite(argument->type) ? emitExpr(argument) : emitAddress(argument);
            if (parameter->type->kind == TypeKind::Array
                && (parameter->type->constrained || parameter->type->boundsSymbol != nullptr)) {
                Value target = withBounds(Value {}, parameter->type, nullptr);
                checkArrayShape(target, parameter->type, value, argument->type);
                value.first = target.first;
                value.last = target.last;
                value.innerBounds = target.innerBounds;
            }
            if (expr->dispatching && !expr->tagSelected && rootType(parameter->type) == subprogram->controllingType
                && argument->tagIndeterminate) {
                checkControllingTag(value);
            }
            bool constrainedParameter = needsConstrainedParameter(parameter);
            std::string constrained = constrainedParameter ? recordConstrained(argument, &value).name : "1";
            if (selectPlan == nullptr || !selectPlan->requeue) {
                value = adaptCompositeStorage(value, argument->type, parameter->type, parameter->mode, storageCopies, constrained);
            }
            arguments.push_back("l " + value.name);
            if (constrainedParameter) { arguments.push_back("w " + constrained); }
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
            if (expr->dispatching && !expr->tagSelected && controllingParameterType(parameter->type) == subprogram->controllingType
                && argument->tagIndeterminate) {
                checkControllingTag(value);
            }
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
        } else if (subprogram->controllingType->interfaceKind != InterfaceKind::None) {
            emitCallInstruction(slot + " =l call $__ada_tag_dispatch(l " + controllingTag + ", l "
                + typeTag(subprogram->controllingType).name + ", w " + std::to_string(subprogram->dispatchSlot)
                + ")", CallEffects::CheckException);
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

    AdaSelectClock selectClock = ADA_SELECT_MONOTONIC;
    Value selectDeadline = selectPlan != nullptr ? emitSelectDeadline(selectPlan->delay, selectClock) : Value {"0", 'l'};
    std::string selectCancelled = selectPlan != nullptr ? newLabel("callcancelled") : "";
    std::string selectReturned = selectPlan != nullptr ? newLabel("callreturned") : "";
    auto copyEntryBack = [this, copyBacks, storageCopies, arrayCopies]() mutable {
        for (const ScalarCopyBack& copy : copyBacks) {
            Value value = loadFrom(copy.temporary, copy.formalType);
            emitRangeCheck(value, copy.argument->type, copy.argument->location, &copy.actual);
            storeInto(copy.actual, value, copy.argument->type);
        }
        emitStorageCopyBacks(storageCopies);
        emitArrayCopyBacks(arrayCopies);
    };
    if ((selectPlan != nullptr && (selectPlan->asynchronous || selectPlan->requeue))
        || (subprogram->entry && subprogram->protectedType != nullptr)) {
        std::string packet = allocScratch(std::max<std::size_t>(8, arguments.size() * 8));
        for (std::size_t i = 0; i < arguments.size(); ++i) {
            char type = arguments[i][0];
            std::string slot = newTemp();
            line(slot + " =l add " + packet + ", " + std::to_string(i * 8));
            line(std::string("store") + type + " " + arguments[i].substr(2) + ", " + slot);
        }
        bool requeue = selectPlan != nullptr && selectPlan->requeue;
        bool asynchronous = selectPlan != nullptr && selectPlan->asynchronous;
        std::string result = newTemp();
        if (subprogram->taskType != nullptr) {
            if (requeue) {
                emitCallInstruction("call $__ada_requeue_task(l %.taskContext, l " + taskIdentity.name
                    + ", w " + std::to_string(subprogram->taskEntryId) + ", l " + entryIndex.name
                    + ", l " + packet + ", l " + std::to_string(arguments.size() * 8)
                    + ", w " + (selectPlan->withAbort ? "1" : "0") + ")", CallEffects::CheckException);
            } else {
                emitCallInstruction(result + " =l call $__ada_async_start_task(l %.taskContext, l " + taskIdentity.name
                    + ", w " + std::to_string(subprogram->taskEntryId) + ", l " + entryIndex.name + ", l " + packet + ")", CallEffects::CheckException);
            }
        } else {
            std::string thunk = "$." + m_unitTag + ".entry." + std::to_string(m_dataCounter++);
            std::ostringstream body;
            body << "function " << thunk << "(l %packet) {\n@start\n";
            std::string actuals;
            for (std::size_t i = 0; i < arguments.size(); ++i) {
                std::string suffix = std::to_string(i);
                char type = arguments[i][0];
                body << "    %slot" << suffix << " =l add %packet, " << i * 8 << "\n"
                     << "    %arg" << suffix << " =" << type << " load" << type << " %slot" << suffix << "\n";
                actuals += (actuals.empty() ? "" : ", ") + std::string(1, type) + " %arg" + suffix;
            }
            body << "    call " << callee << "(" << actuals << ")\n    ret\n}\n";
            m_functions.push_back(body.str());
            std::string link = subprogram->level > 0 ? staticLinkFor(subprogram->level - 1).name : "0";
            std::string operands = "l %.taskContext, l " + protectedObject.name
                + ", l " + subprogram->qbeName + ".barrier, l " + link + ", l " + thunk + ", l " + packet;
            if (requeue) {
                emitCallInstruction("call $__ada_requeue_protected(" + operands
                    + ", l " + std::to_string(arguments.size() * 8) + ", w " + (selectPlan->withAbort ? "1" : "0")
                    + ", w " + (selectPlan->internal ? "1" : "0") + ")", CallEffects::CheckException);
            } else if (asynchronous) {
                emitCallInstruction(result + " =l call $__ada_async_start_protected(" + operands + ")", CallEffects::CheckException);
            } else {
                emitCallInstruction(result + " =w call $__ada_protected_entry_call(" + operands
                    + ", l " + selectDeadline.name + ", w " + std::to_string(selectClock)
                    + ", w " + std::to_string(selectPlan != nullptr ? selectPlan->mode : ADA_ENTRY_WAIT)
                    + ")", CallEffects::CheckException);
            }
        }
        if (requeue) { return Value {"0", 'w'}; }
        if (asynchronous) {
            selectPlan->handle = result;
            selectPlan->copyBack = copyEntryBack;
            return Value {"0", 'w'};
        }
        if (selectPlan != nullptr) { selectPlan->selected = result; }
        std::string committed = newLabel("entrycommitted"), done = newLabel("entryreturned");
        branch(Value {result, 'w'}, committed, done);
        label(committed);
        copyEntryBack();
        jump(done);
        label(done);
        return Value {"0", 'w'};
    }
    std::string protectedAction;
    if ((expr->protectedObject != nullptr && subprogram->protectedType != nullptr) || subprogram->protectedAccessProfile) {
        protectedAction = allocScratch(16);
        if (subprogram->entry) {
            std::string link = subprogram->level > 0 ? staticLinkFor(subprogram->level - 1).name : "0";
            if (selectPlan != nullptr) {
                selectPlan->selected = newTemp();
                emitCallInstruction(selectPlan->selected + " =w call $__ada_protected_entry_enter_until(l %.taskContext, l " + protectedObject.name
                    + ", l " + protectedAction + ", l " + subprogram->qbeName + ".barrier, l " + link
                    + ", l " + selectDeadline.name + ", w " + std::to_string(selectClock)
                    + ", w " + std::to_string(selectPlan->mode) + ")", CallEffects::DeferredCheck);
                emitExceptionCheck();
                std::string committed = newLabel("protectedcommitted");
                branch(Value {selectPlan->selected, 'w'}, committed, selectCancelled);
                label(committed);
            } else {
                emitCallInstruction("call $__ada_protected_entry_enter(l %.taskContext, l " + protectedObject.name
                    + ", l " + protectedAction + ", l " + subprogram->qbeName + ".barrier, l " + link + ")", CallEffects::DeferredCheck);
            }
        } else {
            emitCallInstruction("call $__ada_protected_enter(l %.taskContext, l " + protectedObject.name + ", l " + protectedAction + ")", CallEffects::DeferredCheck);
        }
        emitExceptionCheck();
    }
    std::string taskMark, taskFailed, taskReady;
    if (taskResult) {
        taskMark = newTemp();
        taskFailed = newLabel("taskresultfailed");
        taskReady = newLabel("taskresultready");
        emitCallInstruction(taskMark + " =l call $__ada_task_result_mark(l " + taskMaster + ")", CallEffects::RuntimeHook);
        pushHandler(taskFailed);
    }
    Value result { "0", 'w' };
    auto invoke = [&](const std::string& actuals) {
        if (subprogram->entry && subprogram->taskType != nullptr) {
            std::string packet = allocScratch(std::max<std::size_t>(8, arguments.size() * 8));
            for (std::size_t i = 0; i < arguments.size(); ++i) {
                char type = arguments[i][0];
                std::string slot = newTemp();
                line(slot + " =l add " + packet + ", " + std::to_string(i * 8));
                line(std::string(type == 'l' ? "storel " : type == 'w' ? "storew " : type == 's' ? "stores " : "stored ")
                    + arguments[i].substr(2) + ", " + slot);
            }
            if (selectPlan != nullptr) {
                selectPlan->selected = newTemp();
                emitCallInstruction(selectPlan->selected + " =w call $__ada_task_entry_call_until(l %.taskContext, l " + taskIdentity.name
                    + ", w " + std::to_string(subprogram->taskEntryId) + ", l " + entryIndex.name
                    + ", l " + packet + ", l " + selectDeadline.name + ", w " + std::to_string(selectClock)
                    + ", w " + std::to_string(selectPlan->mode) + ")", CallEffects::DeferredCheck);
            } else {
                emitCallInstruction("call $__ada_task_entry_call(l %.taskContext, l " + taskIdentity.name
                    + ", w " + std::to_string(subprogram->taskEntryId) + ", l " + entryIndex.name
                    + ", l " + packet + ")", CallEffects::DeferredCheck);
            }
            return Value { "0", 'w' };
        }
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
        linked.insert(linked.begin() + ((controlledResult ? ((taggedResult || limitedResult) ? 4 : 3) : compositeResult ? 1 : 0) + (accessResult ? 1 : 0) + (accessReturn ? 1 : 0) + (taskResult ? 3 : 0) + (hasProtected(subprogram->returnType) ? 1 : 0)), "l " + indirectLink);
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
    if (selectPlan != nullptr && subprogram->taskType != nullptr) {
        std::string committed = newLabel("taskcommitted");
        branch(Value {selectPlan->selected, 'w'}, committed, selectCancelled);
        label(committed);
    }
    if (taskResult) {
        if (!taskTemporary.empty()) { activateTaskTemporary(taskTemporary); }
        m_context->handlerLabels.pop_back();
        jump(taskReady);
        label(taskFailed);
        emitCallInstruction("call $__ada_task_result_discard(l " + taskMaster + ", l " + taskMark + ", l " + taskGroup + ")", CallEffects::RuntimeHook);
        if (!m_context->handlerLabels.empty()) { jump(m_context->handlerLabels.back()); }
        else { m_context->usesPropagate = true; jump(m_context->propagateLabel); }
        label(taskReady);
    }
    if (accessReturn) {
        result.accessLevel = newTemp();
        line(result.accessLevel + " =w loadsw " + accessResultLevel);
    }
    if (dispatchEquality && dispatchOperation->name == "/=") {
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
        emitRangeCheck(value, copy.argument->type, copy.argument->location, &copy.actual);
        storeInto(copy.actual, value, copy.argument->type);
    }
    emitStorageCopyBacks(storageCopies);
    emitArrayCopyBacks(arrayCopies);
    if (selectPlan != nullptr) {
        jump(selectReturned);
        label(selectCancelled);
        jump(selectReturned);
        label(selectReturned);
    }
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
                emitRangeCheck(value, copy.argument->type, copy.argument->location, &copy.actual);
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

        const FieldInfo* component = objectField(argument);
        if (parameter->byReference && isScalar(formal)
            && (formal->excludesNull || sharedObjectKind(argument) != 0 || sharedTypeKind(formal) != 0 || !formal->enumCodes.empty()
                || typeSize(formal) != typeSize(argument->type) || packedArrayComponent(argument) || (component != nullptr && component->bitWidth != 0))) {
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
            std::string constrained = needsRecordRepack(formal, argument->type)
                ? recordConstrained(argument, &address).name : "1";
            address = adaptCompositeStorage(address, argument->type, formal, parameter->mode, storageCopies, constrained);
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

    if (subprogram->runtimeSymbol == "$__ada_file_stream_view" && m_sema.fileStreamType() != nullptr) {
        arguments.push_back("l " + typeTag(m_sema.fileStreamType()).name);
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

    if (subprogram->runtimeSymbol == "$__ada_wtext_line"
        || subprogram->runtimeSymbol == "$__ada_wtext_scan"
        || subprogram->runtimeSymbol == "$__ada_utf8_wide"
        || subprogram->runtimeSymbol == "$__ada_utf8_wide_wide"
        || subprogram->runtimeSymbol == "$__ada_text_scan"
        || subprogram->runtimeSymbol == "$__ada_scan"
        || subprogram->runtimeSymbol == "$__ada_text_line"
        || subprogram->runtimeSymbol == "$__ada_line"
        || subprogram->runtimeSymbol == "$__ada_file_name"
        || (subprogram->runtimeSymbol.starts_with("$__ada_host_") && isUnconstrainedArray(subprogram->returnType))) {
        std::string descriptor = allocScratch(adaArrayResultSize(1));
        emitCallInstruction("call " + subprogram->runtimeSymbol + "(l " + descriptor
            + (argumentList.empty() ? "" : ", " + argumentList) + ")", CallEffects::CheckException);
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
