#include "QbeEmitter.h"

void QbeEmitter::initializeFinalization()
{
    m_context->m_finalizationChain = allocScratch(8);
    m_context->m_temporaryFinalizationChain = allocScratch(8);
    m_context->prologue << "    storel 0, " << m_context->m_finalizationChain << "\n";
    m_context->prologue << "    storel 0, " << m_context->m_temporaryFinalizationChain << "\n";
}

// Only controlled nodes need records: plain composites borrow the records of
// their components. Postorder registration gives parent-before-child cleanup.
void QbeEmitter::walkControlled(const Value& object, Type* type, bool parentFirst,
                                const std::function<void(const Value&, Type*)>& action)
{
    if (!needsFinalization(type)) {
        return;
    }
    if (parentFirst && type->m_controlled) {
        action(object, type);
    }
    if (type->kind == TypeKind::Array) {
        Value array = withBounds(object, type, nullptr);
        Value length = lengthOf(array, type);
        std::string count = newTemp();
        line(count + " =l extuw " + length.name);
        std::string elementSize = arrayElementSize(array, type);
        std::string cursor = allocScratch(8);
        line("storel 0, " + cursor);
        std::string head = newLabel("controlledparts");
        std::string body = newLabel("controlledpart");
        std::string done = newLabel("controlledparts_done");
        label(head);
        std::string index = newTemp();
        std::string more = newTemp();
        line(index + " =l loadl " + cursor);
        line(more + " =w cultl " + index + ", " + count);
        branch(Value { more, 'w' }, body, done);
        label(body);
        std::string offset = newTemp();
        Value cell = arrayRow(array, type);
        cell.name = newTemp();
        line(offset + " =l mul " + index + ", " + elementSize);
        line(cell.name + " =l add " + object.name + ", " + offset);
        walkControlled(cell, type->element, parentFirst, action);
        std::string next = newTemp();
        line(next + " =l add " + index + ", 1");
        line("storel " + next + ", " + cursor);
        jump(head);
        label(done);
    } else if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : baseType(type)->fields) {
            if (needsFinalization(field.type)) {
                Value component { newTemp(), 'l' };
                line(component.name + " =l add " + object.name + ", " + std::to_string(field.offset));
                walkControlled(component, field.type, parentFirst, action);
            }
        }
    }
    if (!parentFirst && type->m_controlled) {
        action(object, type);
    }
}

void QbeEmitter::prepareControlledObject(const Value& object, Type* type)
{
    walkControlled(object, type, false, [&](const Value& part, Type* partType) {
        bool temporary = m_context->m_initializingTemporary;
        std::string owner = temporary ? m_context->m_temporaryFinalizationChain : m_context->m_finalizationChain;
        line("call $__ada_finalization_reserve(l " + owner + ", l " + storageArena(temporary, true)
             + ", l " + part.name + ", l " + rootType(partType)->m_tagName + ".finalize)");
        emitExceptionCheck();
    });
}

void QbeEmitter::activateControlledObject(const Value& object, Type* type)
{
    if (type->m_controlled) {
        line("call $__ada_controlled_activate(l " + object.name + ", l " + rootType(type)->m_tagName + ".finalize)");
        emitExceptionCheck();
    }
}

void QbeEmitter::adjustControlledObject(const Value& object, Type* type)
{
    std::string failure = allocScratch(4);
    line("storew 0, " + failure);
    walkControlled(object, type, false, [&](const Value& part, Type* partType) {
        std::string failed = newTemp();
        std::string previous = newTemp();
        std::string combined = newTemp();
        line(failed + " =w call $__ada_controlled_adjust(l " + part.name + ", l "
             + rootType(partType)->m_tagName + ".adjust)");
        line(previous + " =w loadw " + failure);
        line(combined + " =w or " + previous + ", " + failed);
        line("storew " + combined + ", " + failure);
    });
    std::string failed = newTemp();
    line(failed + " =w loadw " + failure);
    std::string bad = newLabel("adjustfailed");
    std::string good = newLabel("adjusted");
    branch(Value { failed, 'w' }, bad, good);
    label(bad);
    line("call $__ada_raise(l $__ada_exc_program_error)");
    emitExceptionCheck();
    jump(good);
    label(good);
}

void QbeEmitter::finalizeControlledObject(const Value& object, Type* type)
{
    std::string failure = allocScratch(4);
    line("storew 0, " + failure);
    walkControlled(object, type, true, [&](const Value& part, Type*) {
        std::string failed = newTemp();
        std::string previous = newTemp();
        std::string combined = newTemp();
        line(failed + " =w call $__ada_controlled_finalize(l " + part.name + ")");
        line(previous + " =w loadw " + failure);
        line(combined + " =w or " + previous + ", " + failed);
        line("storew " + combined + ", " + failure);
    });
    std::string failed = newTemp();
    std::string bad = newLabel("finalizefailed");
    std::string done = newLabel("finalized");
    line(failed + " =w loadw " + failure);
    branch(Value { failed, 'w' }, bad, done);
    label(bad);
    line("call $__ada_raise(l $__ada_exc_program_error)");
    emitExceptionCheck();
    jump(done);
    label(done);
}

std::string QbeEmitter::objectBytes(const Value& object, Type* type)
{
    if (type->kind != TypeKind::Array) {
        return std::to_string(typeSize(type));
    }
    Value array = withBounds(object, type, nullptr);
    std::string size = newTemp();
    std::string elementSize = arrayElementSize(array, type);
    line(size + " =l call $__ada_array_size(w " + array.first + ", w " + array.last + ", l " + elementSize + ")");
    emitExceptionCheck();
    return size;
}

void QbeEmitter::copyControlledObject(const Value& target, const Value& source, Type* type, bool initialize)
{
    if (!initialize && type->m_tagged) {
        std::string tag = newTemp();
        std::string same = newTemp();
        std::string good = newLabel("controlledview");
        std::string bad = newLabel("controlledviewbad");
        line(tag + " =l loadl " + target.name);
        line(same + " =w ceql " + tag + ", " + typeTag(type).name);
        branch(Value { same, 'w' }, good, bad);
        label(bad);
        line("call $__ada_raise(l $__ada_exc_program_error)");
        emitExceptionCheck();
        jump(good);
        label(good);
    }
    std::string size = objectBytes(target, type);
    auto copy = [&](const Value& to, const Value& from) {
        if (type->m_tagged) {
            copyInto(to, from, type);
        } else {
            line("call $memmove(l " + to.name + ", l " + from.name + ", l " + size + ")");
        }
        adjustControlledObject(to, type);
    };
    if (initialize) {
        copy(target, source);
        return;
    }
    std::string same = newTemp();
    std::string done = newLabel("assigned");
    std::string assign = newLabel("controlledassign");
    line(same + " =w ceql " + target.name + ", " + source.name);
    branch(Value { same, 'w' }, done, assign);
    label(assign);
    // Use the RM's anonymous adjusted copy. It protects aliases, overlapping
    // slices, and an RHS whose resources are shared with the old target.
    Value snapshot = withBounds(target, type, nullptr);
    snapshot.name = newTemp();
    line(snapshot.name + " =l call $__ada_array_local(l " + storageArena(true, true) + ", w 1, w 1, l " + size + ")");
    emitExceptionCheck();
    bool saved = m_context->m_initializingTemporary;
    m_context->m_initializingTemporary = true;
    prepareControlledObject(snapshot, type);
    m_context->m_initializingTemporary = saved;
    if (type->m_tagged) {
        line("storel " + typeTag(type).name + ", " + snapshot.name);
    }
    copy(snapshot, source);
    finalizeControlledObject(target, type);
    copy(target, snapshot);
    jump(done);
    label(done);
}
