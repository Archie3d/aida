#include "QbeEmitter.h"
#include "QbeSupport.h"

using QbeSupport::isUnconstrainedArray;
using QbeSupport::isLiteralOperand;

long long QbeEmitter::reserveFrame(long long size, long long alignment)
{
    __int128 offset = ((__int128)m_context->frameSize + alignment - 1) / alignment * alignment;
    if (size < 0 || offset + size > INT64_MAX) {
        m_diagnostics.error(m_context->sourceLocation, "activation frame exceeds signed 64-bit storage capacity");
        return 0;
    }
    if (offset + size > m_frameStackLimit) {
        m_diagnostics.error(m_context->sourceLocation, "activation frame exceeds backend stack capacity");
        return 0;
    }
    m_context->frameSize = (long long)(offset + size);
    return (long long)offset;
}

std::string QbeEmitter::allocScratch(long long size, long long alignment)
{
    std::string temp = newTemp();
    if (size > m_arrayStackLimit) {
        emitCallInstruction(temp + " =l call $__ada_array_local(l " + storageArena(true, true) + ", l 1, l 1, l " + std::to_string(size) + ")", CallEffects::CheckException);
        return temp;
    }
    const char* instruction = size <= 4 && alignment <= 4 ? "alloc4" : "alloc8";
    m_context->prologue << "    " << temp << " =l " << instruction << " " << size << "\n";
    return temp;
}

std::string QbeEmitter::storageArena(bool temporary, bool allocate)
{
    if (allocate) {
        (temporary ? m_context->temporaryArenaUsed : m_context->arrayArenaUsed) = true;
    }
    std::string& arena = temporary ? m_context->temporaryArena : m_context->arrayArena;
    if (arena.empty()) {
        arena = allocScratch(8);
        m_context->prologue << "    storel 0, " << arena << "\n";
    }
    return arena;
}

QbeEmitter::StorageCheckpoint QbeEmitter::storageCheckpoint()
{
    std::string local = newTemp();
    std::string temporary = newTemp();
    line(local + " =l loadl " + storageArena(false));
    line(temporary + " =l loadl " + storageArena(true));
    std::string finalization;
    if (!m_context->m_finalizationChain.empty()) {
        finalization = newTemp();
        line(finalization + " =l loadl " + m_context->m_finalizationChain);
    }
    std::string temporaryFinalization;
    if (!m_context->m_temporaryFinalizationChain.empty()) {
        temporaryFinalization = newTemp();
        line(temporaryFinalization + " =l loadl " + m_context->m_temporaryFinalizationChain);
    }
    return { local, temporary, finalization, temporaryFinalization };
}

void QbeEmitter::rewindStorage(const StorageCheckpoint& checkpoint, bool checkException)
{
    if (!checkpoint.m_temporaryFinalization.empty()) {
        emitCallInstruction("call $__ada_finalize_to(l " + m_context->m_temporaryFinalizationChain
             + ", l " + checkpoint.m_temporaryFinalization + ")", CallEffects::DeferredCheck);
    }
    if (!checkpoint.m_finalization.empty()) {
        emitCallInstruction("call $__ada_finalize_to(l " + m_context->m_finalizationChain + ", l " + checkpoint.m_finalization + ")", CallEffects::DeferredCheck);
    }
    emitCallInstruction("call $__ada_array_rewind(l " + storageArena(true) + ", l " + checkpoint.m_temporary + ")", CallEffects::RuntimeHook);
    emitCallInstruction("call $__ada_array_rewind(l " + storageArena(false) + ", l " + checkpoint.m_local + ")", CallEffects::RuntimeHook);
    if (!checkpoint.m_finalization.empty() && checkException) {
        emitExceptionCheck();
    }
}

void QbeEmitter::emitBlockExits(std::size_t depth)
{
    // Emit cleanup for this control-flow edge without changing lexical state:
    // another branch may still execute inside these masters. Finalize each
    // inner scope before awaiting the next outer scope.
    for (std::size_t i = m_context->m_blockMasters.size(); i > depth; --i) {
        const auto& master = m_context->m_blockMasters[i - 1];
        emitCallInstruction("call $__ada_master_await(l %.taskContext, l " + master.m_record + ")", CallEffects::RuntimeHook);
        rewindStorage(master.m_storage, false);
        emitCallInstruction("call $__ada_master_leave(l %.taskContext, l " + master.m_record + ")", CallEffects::RuntimeHook);
    }
}

void QbeEmitter::pushHandler(const std::string& label)
{
    m_context->handlerLabels.push_back(label);
    m_context->m_transferMasterDepth[label] = m_context->m_blockMasters.size();
}

Value QbeEmitter::staticLinkFor(int targetLevel)
{
    Symbol* current = m_context->symbol;
    int currentLevel = current != nullptr ? current->level : 0;

    if (targetLevel == currentLevel) {
        if (m_context->hasFrame) {
            return Value { m_context->frameTemp, 'l' };
        }
        return Value { "0", 'l' };
    }

    if (currentLevel == 0) {
        return Value { "0", 'l' };
    }

    std::string pointer = "%.link";
    for (int level = currentLevel - 1; level > targetLevel; --level) {
        std::string next = newTemp();
        line(next + " =l loadl " + pointer);
        pointer = next;
    }
    return Value { pointer, 'l' };
}

Value QbeEmitter::addressOf(Symbol* symbol)
{
    Value address;
    if (symbol->isGlobal) {
        address = Value { symbol->qbeName, 'l' };
    } else if (symbol->frameOffset >= 0) {
        Value frame = symbol->owner == m_context->symbol
                          ? Value { m_context->frameTemp, 'l' }
                          : staticLinkFor(symbol->owner->level);
        address = Value { newTemp(), 'l' };
        line(address.name + " =l add " + frame.name + ", " + std::to_string(symbol->frameOffset));
        if ((symbol->kind == SymbolKind::Parameter && symbol->byReference)
            || (symbol->kind == SymbolKind::Object && !symbol->m_objectReference
                && (isUnconstrainedArray(symbol->type)
                    || (symbol->type->kind == TypeKind::Array && typeSize(symbol->type) > m_arrayStackLimit)))) {
            std::string pointer = newTemp();
            line(pointer + " =l loadl " + address.name);
            address.name = pointer;
        }
    } else {
        auto it = m_context->locals.find(symbol);
        if (it == m_context->locals.end()) {
            m_diagnostics.error(symbol->location, "internal error: '" + symbol->displayName + "' has no storage");
            return Value { "0", 'l' };
        }
        address = Value { it->second, 'l' };
    }
    if (symbol->m_objectReference || symbol->m_classWideObject) {
        std::string pointer = newTemp();
        line(pointer + " =l loadl " + address.name);
        address.name = pointer;
    }
    address.m_shared = sharedSymbolKind(symbol) != 0;
    return withBounds(address, symbol->type, symbol);
}

Value QbeEmitter::loadFrom(const Value& address, Type* type)
{
    char resultType = qbeClass(type);
    std::string temp = newTemp();
    if (address.m_shared || sharedTypeKind(type) != 0) {
        long long bits = typeSize(type) * 8;
        char rawClass = bits == 64 ? 'l' : 'w';
        emitCallInstruction(temp + " =" + std::string(1, rawClass) + " call $__ada_atomic_load_"
            + std::to_string(bits) + "(l " + address.name + ")", CallEffects::DeferredCheck);
        if (resultType == 's' || resultType == 'd') {
            std::string converted = newTemp();
            line(converted + " =" + std::string(1, resultType) + " cast " + temp);
            temp = converted;
        } else if ((bits == 8 || bits == 16) && scalarStorageSigned(type)) {
            std::string extended = newTemp();
            line(extended + " =w " + (bits == 8 ? "extsb " : "extsh ") + temp);
            temp = extended;
        } else if (bits == 8 || bits == 16) {
            std::string masked = newTemp();
            line(masked + " =w and " + temp + ", " + (bits == 8 ? "255" : "65535"));
            temp = masked;
        }
    } else {
        char loadType = resultType == 'w' && typeSize(type) == 8 ? 'l' : resultType;
        line(temp + " =" + std::string(1, loadType) + " " + qbeLoadInstruction(type) + " " + address.name);
    }
    if (resultType == 'l' && typeSize(type) < 8 && (address.m_shared || sharedTypeKind(type) != 0)) {
        std::string wide = newTemp();
        line(wide + " =l " + (scalarStorageSigned(type) ? "extsw " : "extuw ") + temp);
        temp = wide;
    } else if (resultType == 'w' && typeSize(type) == 8) {
        std::string narrow = newTemp();
        line(narrow + " =w copy " + temp);
        temp = narrow;
    }
    return enumPosition(Value { temp, resultType }, type);
}

void QbeEmitter::storeInto(const Value& address, const Value& position, Type* type)
{
    Value value = enumCode(position, type);
    if (value.type == 'w' && typeSize(type) == 8) {
        std::string wide = newTemp();
        line(wide + " =l " + (scalarStorageSigned(type) ? "extsw " : "extuw ") + value.name);
        value = Value {wide, 'l'};
    } else if (value.type == 'l' && typeSize(type) < 8) {
        std::string narrow = newTemp();
        line(narrow + " =w copy " + value.name);
        value = Value {narrow, 'w'};
    }
    if (address.m_shared || sharedTypeKind(type) != 0) {
        long long bits = typeSize(type) * 8;
        char rawClass = bits == 64 ? 'l' : 'w';
        std::string raw = value.name;
        if (value.type == 's' || value.type == 'd') {
            raw = newTemp();
            line(raw + " =" + std::string(1, rawClass) + " cast " + value.name);
        }
        emitCallInstruction("call $__ada_atomic_store_" + std::to_string(bits) + "(l " + address.name
            + ", " + std::string(1, rawClass) + " " + raw + ")", CallEffects::DeferredCheck);
    } else {
        line(std::string(qbeStoreInstruction(type)) + " " + value.name + ", " + address.name);
    }
}

void QbeEmitter::copyInto(const Value& destination, const Value& source, Type* type)
{
    if (type->m_classRoot != nullptr) {
        emitCallInstruction("call $__ada_tag_check_copy(l " + source.name + ")", CallEffects::CheckException);
        std::string targetTag = newTemp();
        std::string sourceTag = newTemp();
        std::string same = newTemp();
        line(targetTag + " =l loadl " + destination.name);
        line(sourceTag + " =l loadl " + source.name);
        line(same + " =w ceql " + targetTag + ", " + sourceTag);
        std::string valid = newLabel("copytagvalid");
        std::string invalid = newLabel("copytaginvalid");
        branch(Value { same, 'w' }, valid, invalid);
        label(invalid);
        raiseConstraintError();
        label(valid);
        std::string sizeSlot = newTemp();
        std::string size = newTemp();
        line(sizeSlot + " =l add " + targetTag + ", 8");
        line(size + " =l loadl " + sizeSlot);
        emitCallInstruction("call $memmove(l " + destination.name + ", l " + source.name + ", l " + size + ")", CallEffects::DeferredCheck);
        return;
    }
    if (type->kind == TypeKind::Array && !type->constrained) {
        emitCallInstruction("call $memmove(l " + destination.name + ", l " + source.name + ", l " + objectBytes(destination, type) + ")", CallEffects::DeferredCheck);
        return;
    }
    long long size = type->m_tagged ? typeSize(type) : typeLayoutSize(type);
    if (size <= 0) {
        return;
    }
    if (type->m_tagged) {
        // A tagged object's tag is immutable, including through an ancestor view.
        // Parent layouts include their padding; extension fields start after it.
        if (size > 8) {
            std::string target = newTemp();
            std::string value = newTemp();
            line(target + " =l add " + destination.name + ", 8");
            line(value + " =l add " + source.name + ", 8");
            emitCallInstruction("call $memmove(l " + target + ", l " + value + ", l " + std::to_string(size - 8) + ")", CallEffects::DeferredCheck);
        }
        return;
    }
    // Source and destination may refer to overlapping slices of the same array.
    emitCallInstruction("call $memmove(l " + destination.name + ", l " + source.name + ", l " + std::to_string(size) + ")", CallEffects::DeferredCheck);
}

void QbeEmitter::assignInto(const Value& address, Type* type, Expr* value, bool initialize)
{
    if (value == nullptr) {
        return;
    }
    if (type->m_classRoot != nullptr) {
        std::string saved = m_context->m_controllingTag;
        std::string tag = newTemp();
        line(tag + " =l loadl " + address.name);
        m_context->m_controllingTag = tag;
        Value source = emitExpr(value);
        m_context->m_controllingTag = saved;
        emitCallInstruction("call $__ada_tagged_assign(l " + address.name + ", l " + source.name
             + ", l " + m_context->m_temporaryFinalizationChain + ", l " + storageArena(true, true) + ")", CallEffects::CheckException);
        return;
    }
    if (needsFinalization(type)) {
        Expr* operand = value;
        while (operand->kind == ExprKind::Qualified) {
            if (type->kind == TypeKind::Array && operand->type != nullptr
                && (operand->type->constrained || operand->type->m_boundsSymbol != nullptr)) {
                checkArrayShape(withBounds(address, type, nullptr), type,
                                withBounds(Value {}, operand->type, nullptr), operand->type);
            }
            operand = static_cast<QualifiedExpr*>(operand)->operand.get();
        }
        if (initialize && operand->kind == ExprKind::Aggregate) {
            emitAggregateInto(static_cast<AggregateExpr*>(operand), address, type);
            return;
        }
        if (hasLimitedControlledParts(type)) {
            bool temporary = m_context->m_initializingTemporary;
            m_context->m_resultTarget = address.name;
            m_context->m_resultTargetShape = type->kind == TypeKind::Array ? withBounds(address, type, nullptr) : address;
            m_context->m_resultTargetOwner = m_context->m_constructionOwner.empty()
                ? (temporary ? m_context->m_temporaryFinalizationChain : m_context->m_finalizationChain)
                : m_context->m_constructionOwner;
            m_context->m_resultTargetArena = m_context->m_constructionArena.empty()
                ? storageArena(temporary, true) : m_context->m_constructionArena;
            emitExpr(value);
            return;
        }
        Value source = emitExpr(value);
        if (type->kind == TypeKind::Array) {
            checkArrayShape(withBounds(address, type, nullptr), type, source, value->type);
        } else {
            Type* record = baseType(type);
            for (int which = 0; which < record->discriminantCount; ++which) {
                long long fixed = 0;
                if (!discriminantValueOf(type, which, fixed)) {
                    continue;
                }
                const FieldInfo& field = record->fields[which];
                std::string slot = newTemp();
                line(slot + " =l add " + source.name + ", " + std::to_string(field.offset));
                Value actual = loadFrom(Value { slot, 'l' }, field.type);
                std::string same = newTemp();
                std::string good = newLabel("controlledconstraint");
                std::string bad = newLabel("controlledconstraintbad");
                line(same + " =w ceq" + actual.type + " " + actual.name + ", " + std::to_string(fixed));
                branch(Value { same, 'w' }, good, bad);
                label(bad);
                raiseConstraintError();
                label(good);
            }
        }
        if (initialize && type->m_tagged) {
            line("storel " + typeTag(type).name + ", " + address.name);
        }
        copyControlledObject(address, source, type, initialize);
        return;
    }
    if (type != nullptr && type->m_classRoot != nullptr && !initialize) {
        std::string saved = m_context->m_controllingTag;
        std::string tag = newTemp();
        line(tag + " =l loadl " + address.name);
        m_context->m_controllingTag = tag;
        Value source = emitExpr(value);
        m_context->m_controllingTag = saved;
        copyInto(address, source, type);
        return;
    }
    if (initialize && type != nullptr && type->m_tagged) {
        line("storel " + typeTag(type).name + ", " + address.name);
    }
    if (value->kind == ExprKind::Aggregate && type != nullptr && type->kind == TypeKind::Record && !initialize) {
        Value source = emitAggregate(static_cast<AggregateExpr*>(value));
        copyInto(address, source, type);
        return;
    }
    if (value->kind == ExprKind::Aggregate) {
        emitAggregateInto(static_cast<AggregateExpr*>(value), address, type);
        return;
    }
    if (type != nullptr && type->kind == TypeKind::Array) {
        if (type->arrayRank > 1) {
            Value target = withBounds(address, type, nullptr);
            Value source = emitExpr(value);
            checkArrayShape(target, type, source, value->type);
            std::string elementSize = arrayElementSize(target, type);
            std::string size = newTemp();
            emitCallInstruction(size + " =l call $__ada_array_size(l " + target.first + ", l " + target.last + ", l " + elementSize + ")", CallEffects::CheckException);
            emitCallInstruction("call $memmove(l " + target.name + ", l " + source.name + ", l " + size + ")", CallEffects::DeferredCheck);
            return;
        }
        if (!type->constrained) {
            Value source = emitExpr(value);
            Value targetLength = lengthOf(address, type);
            Value sourceLength = lengthOf(source, value->type);
            std::string same = newTemp();
            line(same + " =w ceql " + targetLength.name + ", " + sourceLength.name);
            std::string ok = newLabel("lengthok");
            std::string bad = newLabel("lengthbad");
            branch(Value { same, 'w' }, ok, bad);
            label(bad);
            raiseConstraintError();
            label(ok);
            std::string wide = newTemp();
            std::string size = newTemp();
            line(wide + " =l copy " + targetLength.name);
            emitCallInstruction(size + " =l call $__ada_array_size(l " + address.first + ", l " + address.last + ", l " + std::to_string(typeSize(type->element)) + ")", CallEffects::CheckException);
            emitCallInstruction("call $memmove(l " + address.name + ", l " + source.name + ", l " + size + ")", CallEffects::DeferredCheck);
            return;
        }
        Value source = emitExpr(value);
        long long target = arrayLength(type);
        Value sourceLength = lengthOf(source, value->type);
        if (isLiteralOperand(sourceLength.name)) {
            if (std::stoll(sourceLength.name) != target) {
                m_diagnostics.error(value->location, "the assigned value has a different length");
                return;
            }
        } else {
            // Ada requires the lengths to match, so check them at run time.
            std::string same = newTemp();
            line(same + " =w ceql " + sourceLength.name + ", " + std::to_string(target));
            std::string ok = newLabel("lengthok");
            std::string bad = newLabel("lengthbad");
            branch(Value { same, 'w' }, ok, bad);
            label(bad);
            raiseConstraintError();
            label(ok);
        }
        copyInto(address, source, type);
        return;
    }
    if (isComposite(type)) {
        Value source = emitExpr(value);
        // A constrained record view retains the actual object's discriminants.
        // Check before copying so a failed assignment leaves it intact.
        Type* record = baseType(type);
        for (int which = 0; which < record->discriminantCount; ++which) {
            long long fixed = 0;
            bool known = discriminantValueOf(type, which, fixed);
            if (!known && initialize) {
                continue;
            }
            const FieldInfo& field = record->fields[which];
            Value expected = QbeSupport::constantValue(fixed, qbeClass(field.type));
            if (!known) {
                // Unconstrained formals still designate constrained objects.
                // Mutable discriminants are not part of the supported subset.
                std::string targetSlot = newTemp();
                line(targetSlot + " =l add " + address.name + ", " + std::to_string(field.offset));
                expected = loadFrom(Value { targetSlot, 'l' }, field.type);
            }
            std::string slot = newTemp();
            line(slot + " =l add " + source.name + ", " + std::to_string(field.offset));
            Value discriminant = loadFrom(Value { slot, 'l' }, field.type);
            std::string same = newTemp();
            line(same + " =w ceq" + std::string(1, discriminant.type) + " "
                 + discriminant.name + ", " + expected.name);
            std::string ok = newLabel("recordconstraintok");
            std::string bad = newLabel("recordconstraintbad");
            branch(Value { same, 'w' }, ok, bad);
            label(bad);
            raiseConstraintError();
            label(ok);
        }
        copyInto(address, source, type);
        return;
    }
    Value result = emitExpr(value);
    emitRangeCheck(result, type, value->location);
    storeInto(address, result, type);
}

// Walk the descriptor's immutable parent links; zero terminates the ancestry.
Value QbeEmitter::taggedMembership(const Value& object, Type* target)
{
    target = rootType(target);
    if (target->m_classRoot != nullptr) {
        target = target->m_classRoot;
    }
    std::string cursor = allocScratch(8);
    std::string result = allocScratch(4);
    std::string tag = newTemp();
    line(tag + " =l loadl " + object.name);
    line("storel " + tag + ", " + cursor);
    line("storew 0, " + result);
    std::string loop = newLabel("ancestry");
    std::string compare = newLabel("comparetag");
    std::string parent = newLabel("parenttag");
    std::string found = newLabel("tagfound");
    std::string done = newLabel("ancestrydone");
    jump(loop);
    label(loop);
    std::string current = newTemp();
    std::string nonzero = newTemp();
    line(current + " =l loadl " + cursor);
    line(nonzero + " =w cnel " + current + ", 0");
    branch(Value { nonzero, 'w' }, compare, done);
    label(compare);
    std::string same = newTemp();
    line(same + " =w ceql " + current + ", " + typeTag(target).name);
    branch(Value { same, 'w' }, found, parent);
    label(parent);
    std::string next = newTemp();
    line(next + " =l loadl " + current);
    line("storel " + next + ", " + cursor);
    jump(loop);
    label(found);
    line("storew 1, " + result);
    jump(done);
    label(done);
    std::string answer = newTemp();
    line(answer + " =w loadw " + result);
    return Value { answer, 'w' };
}

Value QbeEmitter::taggedSize(const Value& object)
{
    std::string tag = newTemp();
    std::string slot = newTemp();
    std::string size = newTemp();
    line(tag + " =l loadl " + object.name);
    line(slot + " =l add " + tag + ", 8");
    line(size + " =l loadl " + slot);
    return Value { size, 'l' };
}

void QbeEmitter::checkTagLevel(const Value& object, int level)
{
    emitCallInstruction("call $__ada_tag_check_level(l " + object.name + ", w " + std::to_string(level) + ")", CallEffects::CheckException);
}

void QbeEmitter::emitControlledCall(const Value& object, Type* type, const std::string& operation, const std::string& explicitTag)
{
    type = rootType(type);
    Symbol* primitive = nullptr;
    for (Symbol* slot : type->m_dispatchSlots) {
        if (slot->name == operation && slot->parameters.size() == 1
            && rootType(slot->parameters.front()->type) == type && slot->returnType == nullptr) {
            primitive = slot;
            break;
        }
    }
    if (primitive == nullptr) {
        m_diagnostics.error(m_context->sourceLocation, "missing controlled primitive '" + operation + "'");
        return;
    }
    Symbol* implementation = primitive;
    while (implementation->m_inheritedFrom != nullptr || implementation->m_renamedSubprogram != nullptr) {
        implementation = implementation->m_inheritedFrom != nullptr
            ? implementation->m_inheritedFrom : implementation->m_renamedSubprogram;
    }
    std::string tag = newTemp();
    std::string tableSlot = newTemp();
    std::string table = newTemp();
    std::string entry = newTemp();
    std::string code = newTemp();
    if (explicitTag.empty()) {
        line(tag + " =l loadl " + object.name);
    } else {
        line(tag + " =l copy " + explicitTag);
    }
    line(tableSlot + " =l add " + tag + ", 24");
    line(table + " =l loadl " + tableSlot);
    line(entry + " =l add " + table + ", " + std::to_string(primitive->m_dispatchSlot * 16));
    line(code + " =l loadl " + entry);
    std::string arguments;
    if (implementation->level > 0) {
        std::string linkSlot = newTemp();
        std::string link = newTemp();
        line(linkSlot + " =l add " + entry + ", 8");
        line(link + " =l loadl " + linkSlot);
        arguments = "l " + link + ", ";
    }
    emitCallInstruction("call " + code + "(" + arguments + "l " + object.name + ")", CallEffects::DeferredCheck);
}

void QbeEmitter::emitFinalizer(Type* type)
{
    FunctionContext context;
    context.traceName = type->name + " finalization";
    context.propagateLabel = newLabel("finalizepropagate");
    FunctionContext* saved = m_context;
    m_context = &context;
    emitControlledCall(Value { "%object", 'l' }, type, "finalize");
    emitReturn();
    context.terminated = true;
    finishFunction("export function " + type->m_tagName + ".finalize(l %object)");
    if (!type->isLimited) {
        FunctionContext adjust;
        adjust.traceName = type->name + " adjustment";
        adjust.propagateLabel = newLabel("adjustpropagate");
        m_context = &adjust;
        emitControlledCall(Value { "%object", 'l' }, type, "adjust");
        emitReturn();
        adjust.terminated = true;
        finishFunction("export function " + type->m_tagName + ".adjust(l %object)");
    }
    for (const std::string& operation : { std::string("finalize"), std::string("adjust") }) {
        if (operation == "adjust" && type->isLimited) {
            continue;
        }
        FunctionContext view;
        view.traceName = type->name + " view " + operation;
        view.propagateLabel = newLabel("viewpropagate");
        m_context = &view;
        emitControlledCall(Value { "%object", 'l' }, type, operation, "%tag");
        emitReturn();
        view.terminated = true;
        finishFunction("export function " + type->m_tagName + ".view_" + operation + "(l %object, l %tag)");
    }
    m_context = saved;
}

std::string QbeEmitter::enumCodeTableFor(Type* type)
{
    const Type* base = baseType(type);
    auto found = m_enumCodeTables.find(base);
    if (found != m_enumCodeTables.end()) { return found->second; }
    std::string table = "$." + m_unitTag + ".enum_codes." + std::to_string(m_dataCounter++);
    m_data << "data " << table << " = align 4 { ";
    for (std::size_t i = 0; i < base->m_enumCodes.size(); ++i) {
        m_data << (i == 0 ? "w " : ", w ") << base->m_enumCodes[i];
    }
    m_data << " }\n";
    m_enumCodeTables.emplace(base, table);
    return table;
}

Value QbeEmitter::enumCode(const Value& position, Type* type)
{
    if (type->m_enumCodes.empty()) { return position; }
    std::string result = newTemp();
    emitCallInstruction(result + " =w call $__ada_enum_code(w " + position.name + ", l " + enumCodeTableFor(type)
        + ", w " + std::to_string(type->m_enumCodes.size()) + ")", CallEffects::CheckException);
    return Value { result, 'w' };
}

Value QbeEmitter::enumPosition(const Value& code, Type* type)
{
    if (type->m_enumCodes.empty()) { return code; }
    std::string result = newTemp();
    emitCallInstruction(result + " =w call $__ada_enum_position(w " + code.name + ", l " + enumCodeTableFor(type)
        + ", w " + std::to_string(type->m_enumCodes.size()) + ")", CallEffects::CheckException);
    return Value { result, 'w' };
}

Value QbeEmitter::adaptCompositeStorage(Value actual, Type* actualType, Type* formal, ParameterMode mode,
                                       std::vector<StorageCopyBack>& copies)
{
    if (!isComposite(formal) || (formal->kind == TypeKind::Array && !formal->constrained)
        || typeSize(formal) == typeSize(actualType)
        || (typeSize(formal) == typeLayoutSize(formal) && typeSize(actualType) == typeLayoutSize(actualType))) {
        return actual;
    }
    if (formal->kind == TypeKind::Array) {
        checkArrayShape(withBounds(Value {}, formal, nullptr), formal, actual, actualType);
    }
    Value temporary = withBounds(Value {allocScratch(std::max(1LL, typeSize(formal)), typeAlignment(formal)), 'l'}, formal, nullptr);
    copyInto(temporary, actual, formal);
    if (mode != ParameterMode::In) { copies.push_back({actual, temporary, actualType}); }
    return temporary;
}

void QbeEmitter::emitStorageCopyBacks(const std::vector<StorageCopyBack>& copies)
{
    for (const auto& copy : copies) {
        if (copy.type->kind == TypeKind::Record) { emitRecordConstraintCheck(copy.temporary, copy.type); }
        copyInto(copy.actual, copy.temporary, copy.type);
    }
}
