#include "QbeEmitter.h"
#include "QbeSupport.h"
#include "../sema/SemaSupport.h"

using QbeSupport::isUnconstrainedArray;
using QbeSupport::isLiteralOperand;

long long QbeEmitter::reserveFrame(long long size, long long alignment)
{
    __int128 offset = ((__int128)m_context->frameSize + alignment - 1) / alignment * alignment;
    if (size < 0 || offset + size > INT64_MAX) {
        m_diagnostics.error(m_context->sourceLocation, "activation frame exceeds signed 64-bit storage capacity");
        return 0;
    }
    if (offset + size > kFrameStackLimit) {
        m_diagnostics.error(m_context->sourceLocation, "activation frame exceeds backend stack capacity");
        return 0;
    }
    m_context->frameSize = (long long)(offset + size);
    return (long long)offset;
}

std::string QbeEmitter::allocScratch(long long size, long long alignment)
{
    std::string temp = newTemp();
    if (size > kArrayStackLimit) {
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
    if (!m_context->finalizationChain.empty()) {
        finalization = newTemp();
        line(finalization + " =l loadl " + m_context->finalizationChain);
    }
    std::string temporaryFinalization;
    if (!m_context->temporaryFinalizationChain.empty()) {
        temporaryFinalization = newTemp();
        line(temporaryFinalization + " =l loadl " + m_context->temporaryFinalizationChain);
    }
    return { local, temporary, finalization, temporaryFinalization };
}

void QbeEmitter::rewindStorage(const StorageCheckpoint& checkpoint, bool checkException)
{
    if (!checkpoint.temporaryFinalization.empty()) {
        emitCallInstruction("call $__ada_finalize_to(l " + m_context->temporaryFinalizationChain
             + ", l " + checkpoint.temporaryFinalization + ")", CallEffects::DeferredCheck);
    }
    if (!checkpoint.finalization.empty()) {
        emitCallInstruction("call $__ada_finalize_to(l " + m_context->finalizationChain + ", l " + checkpoint.finalization + ")", CallEffects::DeferredCheck);
    }
    emitCallInstruction("call $__ada_array_rewind(l " + storageArena(true) + ", l " + checkpoint.temporary + ")", CallEffects::RuntimeHook);
    emitCallInstruction("call $__ada_array_rewind(l " + storageArena(false) + ", l " + checkpoint.local + ")", CallEffects::RuntimeHook);
    if (!checkpoint.finalization.empty() && checkException) {
        emitExceptionCheck();
    }
}

void QbeEmitter::emitBlockExits(std::size_t depth)
{
    // Emit cleanup for this control-flow edge without changing lexical state:
    // another branch may still execute inside these masters. Finalize each
    // inner scope before awaiting the next outer scope.
    for (std::size_t i = m_context->blockMasters.size(); i > depth; --i) {
        const auto& master = m_context->blockMasters[i - 1];
        emitCallInstruction("call $__ada_master_await(l %.taskContext, l " + master.record + ")", CallEffects::RuntimeHook);
        rewindStorage(master.storage, false);
        emitCallInstruction("call $__ada_master_leave(l %.taskContext, l " + master.record + ")", CallEffects::RuntimeHook);
    }
}

void QbeEmitter::pushHandler(const std::string& label)
{
    m_context->handlerLabels.push_back(label);
    m_context->transferMasterDepth[label] = m_context->blockMasters.size();
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
    if (symbol->currentInstance != nullptr) {
        Value object = m_context->discriminantObjects.at(symbol->currentInstance);
        if (object.accessLevel.empty()) {
            object.accessLevel = m_context->coextensionLevel.empty() ? currentAccessLevel() : m_context->coextensionLevel;
        }
        return object;
    }
    if (symbol->discriminantRecord != nullptr) {
        Type* record = symbol->discriminantRecord;
        return fieldAddress(m_context->discriminantObjects.at(record), record, record->fields[symbol->discriminantIndex]);
    }
    Value address;
    if (symbol->isGlobal) {
        address = Value { symbol->qbeName, 'l' };
        if (symbol->kind == SymbolKind::Object && hasRuntimeLayout(symbol->type) && !symbol->objectReference) {
            std::string pointer = newTemp();
            line(pointer + " =l loadl " + address.name);
            address.name = pointer;
        }
    } else if (symbol->frameOffset >= 0) {
        Value frame = symbol->owner == m_context->symbol
                          ? Value { m_context->frameTemp, 'l' }
                          : staticLinkFor(symbol->owner->level);
        address = Value { newTemp(), 'l' };
        line(address.name + " =l add " + frame.name + ", " + std::to_string(symbol->frameOffset));
        if ((symbol->kind == SymbolKind::Parameter && symbol->byReference)
            || (symbol->kind == SymbolKind::Object && !symbol->objectReference
                && (hasRuntimeLayout(symbol->type)
                    || (symbol->type->kind == TypeKind::Array && typeSize(symbol->type) > kArrayStackLimit)))) {
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
    if (symbol->objectReference || symbol->classWideObject) {
        std::string pointer = newTemp();
        line(pointer + " =l loadl " + address.name);
        address.name = pointer;
    }
    address.shared = sharedSymbolKind(symbol) != 0;
    return withBounds(address, symbol->type, symbol);
}

Value QbeEmitter::loadFrom(const Value& address, Type* type)
{
    char resultType = qbeClass(type);
    std::string temp = newTemp();
    if (address.shared || sharedTypeKind(type) != 0) {
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
    if (resultType == 'l' && typeSize(type) < 8 && (address.shared || sharedTypeKind(type) != 0)) {
        std::string wide = newTemp();
        line(wide + " =l " + (scalarStorageSigned(type) ? "extsw " : "extuw ") + temp);
        temp = wide;
    } else if (resultType == 'w' && typeSize(type) == 8) {
        std::string narrow = newTemp();
        line(narrow + " =w copy " + temp);
        temp = narrow;
    }
    Value result = enumPosition(Value { temp, resultType }, type);
    if (type->kind == TypeKind::Access && (type->accessParameter || type->accessDiscriminant)) {
        std::string slot = newTemp();
        result.accessLevel = newTemp();
        line(slot + " =l add " + address.name + ", 8");
        line(result.accessLevel + " =w loadsw " + slot);
        if (type->accessDiscriminant) {
            std::string ownerSlot = newTemp();
            result.accessOwner = newTemp();
            line(ownerSlot + " =l add " + address.name + ", 16");
            line(result.accessOwner + " =l loadl " + ownerSlot);
        }
    }
    return result;
}

void QbeEmitter::storeInto(const Value& address, const Value& position, Type* type)
{
    Value value = enumCode(position, type);
    if (type->kind == TypeKind::Access && (type->accessParameter || type->accessDiscriminant)) {
        std::string slot = newTemp();
        line(slot + " =l add " + address.name + ", 8");
        line("storew " + (position.accessLevel.empty() ? (position.name == "0" ? "0" : accessLevelFor(type)) : position.accessLevel) + ", " + slot);
        if (type->accessDiscriminant) {
            std::string ownerSlot = newTemp();
            line(ownerSlot + " =l add " + address.name + ", 16");
            line("storel " + (position.accessOwner.empty() ? "0" : position.accessOwner) + ", " + ownerSlot);
        }
    }
    if (value.type == 'w' && typeSize(type) == 8) {
        std::string wide = newTemp();
        line(wide + " =l " + (scalarStorageSigned(type) ? "extsw " : "extuw ") + value.name);
        value = Value {wide, 'l'};
    } else if (value.type == 'l' && typeSize(type) < 8) {
        std::string narrow = newTemp();
        line(narrow + " =w copy " + value.name);
        value = Value {narrow, 'w'};
    }
    if (address.shared || sharedTypeKind(type) != 0) {
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
    if (type->classRoot != nullptr) {
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
    if (hasRuntimeLayout(type)) {
        std::string size = objectBytes(destination, type);
        if (type->tagged) {
            std::string target = newTemp(), value = newTemp(), payload = newTemp();
            line(target + " =l add " + destination.name + ", 8");
            line(value + " =l add " + source.name + ", 8");
            line(payload + " =l sub " + size + ", 8");
            emitCallInstruction("call $memmove(l " + target + ", l " + value + ", l " + payload + ")", CallEffects::DeferredCheck);
        } else {
            emitCallInstruction("call $memmove(l " + destination.name + ", l " + source.name + ", l " + size + ")", CallEffects::DeferredCheck);
        }
        return;
    }
    long long size = type->tagged ? typeSize(type) : typeLayoutSize(type);
    if (size <= 0) {
        return;
    }
    if (type->tagged) {
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

Value QbeEmitter::recordConstrained(Expr* object, const Value* evaluated)
{
    if (object == nullptr || hasLimitedParts(object->type) || !hasDefaultDiscriminants(object->type)
        || hasDiscriminantConstraint(object->type)) {
        return Value { "1", 'w' };
    }
    Symbol* symbol = object->kind == ExprKind::Identifier ? static_cast<IdentifierExpr*>(object)->symbol
        : object->kind == ExprKind::Selected ? static_cast<SelectedExpr*>(object)->symbol : nullptr;
    if (symbol != nullptr) {
        if (symbol->sharedAlias != nullptr) {
            return recordConstrained(symbol->sharedAlias, evaluated);
        }
        if (symbol->isConstant || (symbol->kind == SymbolKind::Parameter && symbol->mode == ParameterMode::In)) {
            return Value { "1", 'w' };
        }
        if (symbol->kind == SymbolKind::Parameter && needsConstrainedParameter(symbol)) {
            Value frame = symbol->owner == m_context->symbol
                ? Value { m_context->frameTemp, 'l' } : staticLinkFor(symbol->owner->level);
            std::string slot = newTemp(), result = newTemp();
            line(slot + " =l add " + frame.name + ", " + std::to_string(symbol->constrainedOffset));
            line(result + " =w loadw " + slot);
            return Value { result, 'w' };
        }
        return Value { "0", 'w' };
    }
    if (object->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(object);
        if (selected->isDereference) {
            Type* access = selected->prefix->type;
            if (access->generalAccess && !access->accessConstant) {
                Value pointer = evaluated != nullptr ? *evaluated : emitExpr(selected->prefix.get());
                checkNotNull(pointer);
                std::string result = newTemp();
                emitCallInstruction(result + " =w call $__ada_access_constrained(l " + pointer.name + ", l "
                    + stringData(rootType(access->target)->accessIdentity) + ")", CallEffects::CheckException);
                return Value {result, 'w'};
            }
            return Value { "1", 'w' }; // Pool objects and constant views are constrained.
        }
        return Value { SemaSupport::objectView(object) == 1 ? "1" : "0", 'w' };
    }
    if (object->kind == ExprKind::Call && static_cast<CallExpr*>(object)->form == CallForm::Indexing) {
        return Value { SemaSupport::objectView(object) == 1 ? "1" : "0", 'w' };
    }
    return Value { "1", 'w' };
}

void QbeEmitter::checkRecordAssignment(const Value& address, const Value& source, Type* type,
                                      bool initialize, const std::string& constrained)
{
    std::string done;
    if (!initialize && constrained != "0" && constrained != "1") {
        std::string check = newLabel("recordconstraint");
        done = newLabel("recordconstraintdone");
        branch(Value { constrained, 'w' }, check, done);
        label(check);
    }
    Type* record = baseType(type);
    for (int which = 0; which < record->discriminantCount; ++which) {
        Value expected = discriminantConstraint(type, which);
        bool known = !expected.name.empty();
        if (!known && (initialize || constrained == "0")) {
            continue;
        }
        const FieldInfo& field = discriminantField(record, which);
        if (!known) {
            std::string targetSlot = newTemp();
            line(targetSlot + " =l add " + address.name + ", " + fieldOffset(record, field));
            expected = loadFrom(Value { targetSlot, 'l' }, field.type);
        }
        std::string slot = newTemp();
        line(slot + " =l add " + source.name + ", " + fieldOffset(record, field));
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
    if (!done.empty()) {
        jump(done);
        label(done);
    }
}

void QbeEmitter::assignInto(const Value& address, Type* type, Expr* value, bool initialize,
                           const std::string& constrained)
{
    if (value == nullptr) {
        return;
    }
    if (value->kind == ExprKind::Box) {
        emitDefaultInit(address, type);
        return;
    }
    if (type->classRoot != nullptr) {
        std::string saved = m_context->controllingTag;
        std::string tag = newTemp();
        line(tag + " =l loadl " + address.name);
        m_context->controllingTag = tag;
        Value source = emitExpr(value);
        m_context->controllingTag = saved;
        emitRecordConstraintCheck(source, type);
        emitCallInstruction("call $__ada_tagged_assign(l " + address.name + ", l " + source.name
             + ", l " + m_context->temporaryFinalizationChain + ", l " + storageArena(true, true) + ")", CallEffects::CheckException);
        return;
    }
    if (needsFinalization(type) || hasLimitedParts(type)) {
        Expr* operand = value;
        while (operand->kind == ExprKind::Qualified) {
            if (type->kind == TypeKind::Array && operand->type != nullptr
                && (operand->type->constrained || operand->type->boundsSymbol != nullptr)) {
                checkArrayShape(withBounds(address, type, nullptr), type,
                                withBounds(Value {}, operand->type, nullptr), operand->type);
            }
            operand = static_cast<QualifiedExpr*>(operand)->operand.get();
        }
        if (initialize && operand->kind == ExprKind::Aggregate) {
            emitAggregateInto(static_cast<AggregateExpr*>(operand), address, type);
            return;
        }
        if (hasLimitedParts(type)) {
            bool temporary = m_context->initializingTemporary;
            m_context->resultTarget = address.name;
            m_context->resultTargetShape = type->kind == TypeKind::Array ? withBounds(address, type, nullptr) : address;
            m_context->resultTargetOwner = m_context->constructionOwner.empty()
                ? (temporary ? m_context->temporaryFinalizationChain : m_context->finalizationChain)
                : m_context->constructionOwner;
            m_context->resultTargetArena = m_context->constructionArena.empty()
                ? storageArena(temporary, true) : m_context->constructionArena;
            Value result = emitExpr(value);
            if (type->kind == TypeKind::Record) { checkRecordAssignment(address, result, type, true, constrained); }
            return;
        }
        Value source = emitExpr(value);
        if (type->kind == TypeKind::Array) {
            checkArrayShape(withBounds(address, type, nullptr), type, source, value->type);
        } else {
            checkRecordAssignment(address, source, type, initialize, constrained);
        }
        if (initialize && type->tagged) {
            line("storel " + typeTag(type).name + ", " + address.name);
        }
        copyControlledObject(address, source, type, initialize);
        return;
    }
    if (type != nullptr && type->classRoot != nullptr && !initialize) {
        std::string saved = m_context->controllingTag;
        std::string tag = newTemp();
        line(tag + " =l loadl " + address.name);
        m_context->controllingTag = tag;
        Value source = emitExpr(value);
        m_context->controllingTag = saved;
        copyInto(address, source, type);
        return;
    }
    if (initialize && type != nullptr && type->tagged) {
        line("storel " + typeTag(type).name + ", " + address.name);
    }
    if (value->kind == ExprKind::Aggregate
        && (initialize || type == nullptr || type->kind != TypeKind::Record)) {
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
            emitCallInstruction(size + " =l call $__ada_array_size(l " + address.first + ", l " + address.last + ", l " + arrayElementSize(address, type) + ")", CallEffects::CheckException);
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
        checkRecordAssignment(address, source, type, initialize, constrained);
        copyInto(address, source, type);
        return;
    }
    Value result = emitExpr(value);
    emitRangeCheck(result, type, value->location);
    storeInto(address, result, type);
}

// Membership includes the physical parent chain and transitive interface views.
Value QbeEmitter::taggedMembership(const Value& object, Type* target)
{
    Type* subtype = target;
    target = rootType(target);
    if (target->classRoot != nullptr) {
        target = target->classRoot;
    }
    std::string tag = newTemp(), answer = newTemp();
    line(tag + " =l loadl " + object.name);
    emitCallInstruction(answer + " =w call $__ada_tag_has_ancestor(l " + tag + ", l "
        + typeTag(target).name + ")", CallEffects::DeferredCheck);
    if (hasDiscriminantConstraint(subtype)) {
        std::string check = newLabel("membershipconstraint"), done = newLabel("membershipdone");
        std::string result = allocScratch(4);
        line("storew " + answer + ", " + result);
        branch(Value {answer, 'w'}, check, done);
        label(check);
        Value matches = recordConstraintMatches(object, subtype);
        line("storew " + matches.name + ", " + result);
        jump(done);
        label(done);
        answer = newTemp();
        line(answer + " =w loadw " + result);
    }
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
    for (Symbol* slot : type->dispatchSlots) {
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
    while (implementation->inheritedFrom != nullptr || implementation->renamedSubprogram != nullptr) {
        implementation = implementation->inheritedFrom != nullptr
            ? implementation->inheritedFrom : implementation->renamedSubprogram;
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
    line(entry + " =l add " + table + ", " + std::to_string(primitive->dispatchSlot * 16));
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
    finishFunction("export function " + type->tagName + ".finalize(l %object)");
    if (!type->isLimited) {
        FunctionContext adjust;
        adjust.traceName = type->name + " adjustment";
        adjust.propagateLabel = newLabel("adjustpropagate");
        m_context = &adjust;
        emitControlledCall(Value { "%object", 'l' }, type, "adjust");
        emitReturn();
        adjust.terminated = true;
        finishFunction("export function " + type->tagName + ".adjust(l %object)");
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
        finishFunction("export function " + type->tagName + ".view_" + operation + "(l %object, l %tag)");
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
    for (std::size_t i = 0; i < base->enumCodes.size(); ++i) {
        m_data << (i == 0 ? "w " : ", w ") << base->enumCodes[i];
    }
    m_data << " }\n";
    m_enumCodeTables.emplace(base, table);
    return table;
}

Value QbeEmitter::enumCode(const Value& position, Type* type)
{
    if (type->enumCodes.empty()) { return position; }
    std::string result = newTemp();
    emitCallInstruction(result + " =w call $__ada_enum_code(w " + position.name + ", l " + enumCodeTableFor(type)
        + ", w " + std::to_string(type->enumCodes.size()) + ")", CallEffects::CheckException);
    return Value { result, 'w' };
}

Value QbeEmitter::enumPosition(const Value& code, Type* type)
{
    if (type->enumCodes.empty()) { return code; }
    std::string result = newTemp();
    emitCallInstruction(result + " =w call $__ada_enum_position(w " + code.name + ", l " + enumCodeTableFor(type)
        + ", w " + std::to_string(type->enumCodes.size()) + ")", CallEffects::CheckException);
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
    Value temporary = withBounds(Value {allocTypeScratch(formal), 'l'}, formal, nullptr);
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

std::string QbeEmitter::currentAccessLevel()
{
    std::string level = newTemp();
    emitCallInstruction(level + " =w call $__ada_current_access_level()", CallEffects::CheckException);
    return level;
}

std::string QbeEmitter::accessLevelFor(Type* type)
{
    type = rootType(type);
    while (type->parentType != nullptr) { type = type->parentType; }
    if ((type->accessProfile != nullptr && !type->accessProtected) || type->collectionOwner == nullptr) { return "0"; }
    Value collection = collectionFor(type);
    std::string level = newTemp();
    emitCallInstruction(level + " =w call $__ada_collection_access_level(l " + collection.name + ")", CallEffects::DeferredCheck);
    return level;
}

std::string QbeEmitter::accessLevelFor(Symbol* symbol)
{
    if (symbol == nullptr || symbol->owner == nullptr || symbol->isGlobal) { return "0"; }
    Value frame = symbol->owner == m_context->symbol ? Value {m_context->frameTemp, 'l'} : staticLinkFor(symbol->owner->level);
    std::string slot = newTemp(), level = newTemp();
    line(slot + " =l add " + frame.name + ", " + std::to_string(symbol->accessibilityOffset));
    line(level + " =w loadsw " + slot);
    return level;
}

void QbeEmitter::captureAccessLevel(Symbol* symbol)
{
    if (!symbol->needsAccessibility || symbol->owner == nullptr || symbol->isGlobal) { return; }
    if (symbol->accessibilityOffset < 0) { symbol->accessibilityOffset = reserveFrame(4, 4); }
    std::string slot = newTemp();
    line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->accessibilityOffset));
    line("storew " + currentAccessLevel() + ", " + slot);
}
