#include "QbeEmitter.h"
#include "QbeSupport.h"

using QbeSupport::isUnconstrainedArray;
using QbeSupport::isLiteralOperand;

std::string QbeEmitter::allocScratch(long long size)
{
    std::string temp = newTemp();
    const char* instruction = size <= 4 ? "alloc4" : "alloc8";
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
    return { local, temporary, finalization };
}

void QbeEmitter::rewindStorage(const StorageCheckpoint& checkpoint, bool checkException)
{
    if (!checkpoint.m_finalization.empty()) {
        line("call $__ada_finalize_to(l " + m_context->m_finalizationChain + ", l " + checkpoint.m_finalization + ")");
    }
    line("call $__ada_array_rewind(l " + storageArena(true) + ", l " + checkpoint.m_temporary + ")");
    line("call $__ada_array_rewind(l " + storageArena(false) + ", l " + checkpoint.m_local + ")");
    if (!checkpoint.m_finalization.empty() && checkException) {
        emitExceptionCheck();
    }
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
                && isUnconstrainedArray(symbol->type))) {
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
    return withBounds(address, symbol->type, symbol);
}

Value QbeEmitter::loadFrom(const Value& address, Type* type)
{
    char resultType = qbeClass(type);
    std::string temp = newTemp();
    line(temp + " =" + std::string(1, resultType) + " " + qbeLoadInstruction(type) + " " + address.name);
    return Value { temp, resultType };
}

void QbeEmitter::storeInto(const Value& address, const Value& value, Type* type)
{
    line(std::string(qbeStoreInstruction(type)) + " " + value.name + ", " + address.name);
}

void QbeEmitter::copyInto(const Value& destination, const Value& source, Type* type)
{
    if (type->m_classRoot != nullptr) {
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
        line("call $memmove(l " + destination.name + ", l " + source.name + ", l " + size + ")");
        return;
    }
    long long size = typeSize(type);
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
            line("call $memmove(l " + target + ", l " + value + ", l " + std::to_string(size - 8) + ")");
        }
        return;
    }
    // Source and destination may refer to overlapping slices of the same array.
    line("call $memmove(l " + destination.name + ", l " + source.name + ", l " + std::to_string(size) + ")");
}

void QbeEmitter::assignInto(const Value& address, Type* type, Expr* value, bool initialize)
{
    if (value == nullptr) {
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
    if (value->kind == ExprKind::Aggregate && type != nullptr && type->m_tagged && !initialize) {
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
            line(size + " =l call $__ada_array_size(w " + target.first + ", w " + target.last + ", l " + elementSize + ")");
            emitExceptionCheck();
            line("call $memmove(l " + target.name + ", l " + source.name + ", l " + size + ")");
            return;
        }
        if (!type->constrained) {
            Value source = emitExpr(value);
            Value targetLength = lengthOf(address, type);
            Value sourceLength = lengthOf(source, value->type);
            std::string same = newTemp();
            line(same + " =w ceqw " + targetLength.name + ", " + sourceLength.name);
            std::string ok = newLabel("lengthok");
            std::string bad = newLabel("lengthbad");
            branch(Value { same, 'w' }, ok, bad);
            label(bad);
            raiseConstraintError();
            label(ok);
            std::string wide = newTemp();
            std::string size = newTemp();
            line(wide + " =l extuw " + targetLength.name);
            line(size + " =l mul " + wide + ", " + std::to_string(typeSize(type->element)));
            line("call $memmove(l " + address.name + ", l " + source.name + ", l " + size + ")");
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
            line(same + " =w ceqw " + sourceLength.name + ", " + std::to_string(target));
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
            if (!discriminantValueOf(type, which, fixed)) {
                continue;
            }
            const FieldInfo& field = record->fields[which];
            std::string slot = newTemp();
            line(slot + " =l add " + source.name + ", " + std::to_string(field.offset));
            Value discriminant = loadFrom(Value { slot, 'l' }, field.type);
            std::string same = newTemp();
            line(same + " =w ceq" + std::string(1, discriminant.type) + " "
                 + discriminant.name + ", " + std::to_string(fixed));
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
    line("call $__ada_tag_check_level(l " + object.name + ", w " + std::to_string(level) + ")");
    emitExceptionCheck();
}

void QbeEmitter::emitControlledCall(const Value& object, Type* type, const std::string& operation)
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
    line(tag + " =l loadl " + object.name);
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
    line("call " + code + "(" + arguments + "l " + object.name + ")");
}

void QbeEmitter::registerControlledObject(const Value& object, Type* type)
{
    // Fixed-size records live in the activation, so successful Initialize is
    // followed by registration without an intervening allocation that can fail.
    std::string record = allocScratch(24);
    line("call $__ada_finalization_push(l " + m_context->m_finalizationChain + ", l " + record
         + ", l " + object.name + ", l " + rootType(type)->m_tagName + ".finalize)");
}

void QbeEmitter::emitFinalizer(Type* type)
{
    FunctionContext context;
    context.traceName = type->name + " finalization";
    context.propagateLabel = newLabel("finalizepropagate");
    FunctionContext* saved = m_context;
    m_context = &context;
    emitControlledCall(Value { "%object", 'l' }, type, "finalize");
    line("ret");
    context.terminated = true;
    finishFunction("export function " + type->m_tagName + ".finalize(l %object)");
    m_context = saved;
}
