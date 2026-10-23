#include "QbeEmitter.h"
#include "QbeSupport.h"

#include <algorithm>
#include <map>

Value QbeEmitter::layoutAddress(Type* type)
{
    Symbol* symbol = type->layoutSymbol;
    if (symbol->isGlobal) { return Value {symbol->qbeName, 'l'}; }
    Value frame = symbol->owner == m_context->symbol ? Value {m_context->frameTemp, 'l'}
        : staticLinkFor(symbol->owner->level);
    Value slot {newTemp(), 'l'};
    line(slot.name + " =l add " + frame.name + ", " + std::to_string(symbol->frameOffset));
    return slot;
}

std::string QbeEmitter::runtimeSize(Type* type)
{
    if (!hasRuntimeLayout(type)) { return std::to_string(typeSize(type)); }
    if (type->kind == TypeKind::Record) {
        std::string size = newTemp();
        line(size + " =l loadl " + layoutAddress(type).name);
        return size;
    }
    Value view = withBounds(Value {}, type, nullptr);
    std::string size = newTemp();
    emitCallInstruction(size + " =l call $__ada_array_size(l " + view.first + ", l " + view.last
        + ", l " + arrayElementSize(view, type) + ")", CallEffects::CheckException);
    return size;
}

std::string QbeEmitter::allocTypeScratch(Type* type)
{
    if (!hasRuntimeLayout(type)) { return allocScratch(std::max(1LL, typeSize(type)), typeAlignment(type)); }
    std::string size = runtimeSize(type);
    std::string slot = newTemp();
    emitCallInstruction(slot + " =l call $__ada_array_local(l " + storageArena(true, true)
        + ", l 1, l 1, l " + size + ")", CallEffects::CheckException);
    return slot;
}

std::string QbeEmitter::fieldOffset(Type* type, const FieldInfo& field)
{
    if (type->layoutSymbol == nullptr) { return std::to_string(field.offset); }
    Value layout = layoutAddress(type);
    std::string slot = newTemp(), offset = newTemp();
    line(slot + " =l add " + layout.name + ", " + std::to_string((field.index + 1) * 8));
    line(offset + " =l loadl " + slot);
    return offset;
}

Value QbeEmitter::fieldStorage(const Value& object, Type* type, const FieldInfo& field)
{
    if (type->layoutSymbol == nullptr && field.offset == 0) { return Value {object.name, 'l'}; }
    Value address {newTemp(), 'l'};
    std::string offset;
    if (type->dependentStorage && type->layoutSymbol != nullptr) {
        std::string slot = newTemp(), layout = newTemp(), fieldSlot = newTemp();
        offset = newTemp();
        line(slot + " =l add " + object.name + ", 8");
        line(layout + " =l loadl " + slot);
        line(fieldSlot + " =l add " + layout + ", " + std::to_string((field.index + 1) * 8));
        line(offset + " =l loadl " + fieldSlot);
    } else { offset = fieldOffset(type, field); }
    line(address.name + " =l add " + object.name + ", " + offset);
    return address;
}

Value QbeEmitter::fieldAddress(const Value& object, Type* type, const FieldInfo& field)
{
    Value address = fieldStorage(object, type, field);
    if (field.dependentBounds) {
        Value view = loadArrayView(address.name, field.type->arrayRank, false);
        view.name = newTemp();
        line(view.name + " =l loadl " + address.name);
        return view;
    }
    return withBounds(address, field.type, nullptr);
}

void QbeEmitter::initializeDependentField(const Value& object, Type* type, const FieldInfo& field)
{
    Value view;
    Type* axis = field.type;
    Value captured = boundsFor(field.type->boundsSymbol);
    Type* captureType = field.type->boundsSymbol->type;
    for (int i = 0; i < field.type->arrayRank; ++i) {
        const auto& expressions = field.type->boundExpressions[i];
        Value first = captureType->boundExpressions[i].first != nullptr ? Value {captured.axisFirst(i), 'l'}
            : wideBound(emitExpr(expressions.first));
        Value last = captureType->boundExpressions[i].second != nullptr ? Value {captured.axisLast(i), 'l'}
            : wideBound(emitExpr(expressions.second));
        Type* indexBase = m_sema.typeTable().scalarBaseType(axis->index);
        emitRangeCheck(first, indexBase, expressions.first->location);
        emitRangeCheck(last, indexBase, expressions.second->location);
        std::string nonNull = newTemp(), check = newLabel("componentbounds"), ready = newLabel("componentboundsready");
        line(nonNull + " =w csgel " + last.name + ", " + first.name);
        branch(Value {nonNull, 'w'}, check, ready);
        label(check);
        emitRangeCheck(first, axis->index, expressions.first->location);
        emitRangeCheck(last, axis->index, expressions.second->location);
        jump(ready);
        label(ready);
        if (i == 0) { view.first = first.name; view.last = last.name; }
        else { view.innerBounds.push_back({first.name, last.name}); }
        axis = axis->element;
    }
    view = arrayView(view, field.type);
    view.name = newTemp();
    emitCallInstruction(view.name + " =l call $__ada_array_local(l " + object.name + ", l " + view.first
        + ", l " + view.last + ", l " + view.strides.front() + ")", CallEffects::CheckException);
    Value descriptor = fieldStorage(object, type, field);
    line("storel " + view.name + ", " + descriptor.name);
    storeArrayView(descriptor.name, view, field.type, false);
}

void QbeEmitter::cloneDependentStorage(const Value& object, Type* type)
{
    for (const FieldInfo& field : baseType(type)->fields) {
        if (!field.dependentBounds) { continue; }
        std::string done;
        if (field.variant >= 0) {
            std::string active = newLabel("clonestorage");
            done = newLabel("clonestoragenext");
            branch(variantCondition(object, baseType(type), field.variant), active, done);
            label(active);
        }
        Value source = fieldAddress(object, baseType(type), field);
        std::string bytes = objectBytes(source, field.type);
        std::string data = newTemp();
        emitCallInstruction(data + " =l call $__ada_array_local(l " + object.name + ", l 1, l 1, l "
            + bytes + ")", CallEffects::CheckException);
        emitCallInstruction("call $memmove(l " + data + ", l " + source.name + ", l " + bytes + ")", CallEffects::DeferredCheck);
        line("storel " + data + ", " + fieldStorage(object, baseType(type), field).name);
        if (!done.empty()) { jump(done); label(done); }
    }
}

void QbeEmitter::emitRecordLayout(Type* type)
{
    if (type == nullptr || type->layoutSymbol == nullptr || type->layoutSymbol->type != type) { return; }
    Symbol* symbol = type->layoutSymbol;
    if (!symbol->isGlobal) { symbol->frameOffset = reserveFrame((type->fields.size() + 1) * 8); }
    Value layout = layoutAddress(type);
    std::map<int, std::string> cursors;
    cursors[-1] = type->dependentStorage ? "16" : type->protectedScope != nullptr ? "8" : "0";
    std::string maximum = allocScratch(8);
    line("storel " + cursors[-1] + ", " + maximum);
    auto append = [&](const std::string& offset, const std::string& size, long long alignment) {
        std::string result = newTemp();
        emitCallInstruction(result + " =l call $__ada_layout_append(l " + offset + ", l " + size
            + ", l " + std::to_string(alignment) + ")", CallEffects::CheckException);
        return result;
    };
    // Alternatives start at their parent's cursor and share storage. Their
    // children retain that cursor independently; total size covers every arm.
    for (const FieldInfo& field : type->fields) {
        if (!cursors.contains(field.variant)) {
            std::vector<int> missing;
            int variant = field.variant;
            while (!cursors.contains(variant)) {
                missing.push_back(variant);
                variant = type->variants[variant].parent;
            }
            for (auto at = missing.rbegin(); at != missing.rend(); ++at) {
                cursors[*at] = cursors[type->variants[*at].parent];
            }
        }
        long long alignment = field.dependentBounds ? 8 : typeAlignment(field.type);
        std::string size = field.dependentBounds ? std::to_string(adaArrayLocalSize(field.type->arrayRank))
            : runtimeSize(field.type);
        std::string offset = append(cursors[field.variant], "0", alignment);
        std::string end = append(offset, size, 1);
        cursors[field.variant] = end;
        std::string slot = newTemp();
        line(slot + " =l add " + layout.name + ", " + std::to_string((field.index + 1) * 8));
        line("storel " + offset + ", " + slot);
        std::string previous = newTemp(), larger = newTemp();
        line(previous + " =l loadl " + maximum);
        line(larger + " =w cugtl " + end + ", " + previous);
        std::string update = newLabel("layoutgrow"), ready = newLabel("layoutready");
        branch(Value {larger, 'w'}, update, ready);
        label(update);
        line("storel " + end + ", " + maximum);
        jump(ready);
        label(ready);
    }
    std::string size = newTemp();
    line(size + " =l loadl " + maximum);
    size = append(size, "0", typeAlignment(type));
    line("storel " + size + ", " + layout.name);
}

Value QbeEmitter::allocatedArrayView(Value pointer, Type* type)
{
    if (type->constrained || type->boundsSymbol != nullptr) { return withBounds(pointer, type, nullptr); }
    std::string descriptor = newTemp();
    emitCallInstruction(descriptor + " =l call $__ada_allocation_array_bounds(l " + pointer.name
        + ", w " + std::to_string(type->arrayRank) + ")", CallEffects::CheckException);
    Value view = loadArrayView(descriptor, type->arrayRank, false);
    view.name = pointer.name;
    view.type = 'l';
    return view;
}

Value QbeEmitter::discriminantConstraint(Type* type, int index)
{
    long long value = 0;
    if (discriminantValueOf(type, index, value)) {
        return QbeSupport::constantValue(value, qbeClass(baseType(type)->fields[index].type));
    }
    for (Type* walk = type; walk != nullptr; walk = walk->isSubtype ? walk->base : nullptr) {
        if (walk->discriminantConstraintSource != nullptr) {
            return discriminantConstraint(walk->discriminantConstraintSource, index);
        }
        if (walk->discriminantRecord != nullptr && walk->kind == TypeKind::Record) {
            if (!m_context->discriminantObjects.contains(walk->discriminantRecord)) { return Value {}; }
            if (walk->discriminantPerObject[index]) {
                return emitExpr(walk->discriminantExpressions[index]);
            }
        }
        Symbol* symbol = walk->discriminantSymbol;
        if (symbol == nullptr) { continue; }
        Value address {symbol->qbeName, 'l'};
        if (walk->discriminantPerEvaluation) {
            address = m_context->discriminantBounds.at(walk);
        } else if (!symbol->isGlobal) {
            Value frame = symbol->owner == m_context->symbol ? Value {m_context->frameTemp, 'l'}
                : staticLinkFor(symbol->owner->level);
            address.name = newTemp();
            line(address.name + " =l add " + frame.name + ", " + std::to_string(symbol->frameOffset));
        }
        std::string slot = newTemp();
        line(slot + " =l add " + address.name + ", " + std::to_string(index * 8));
        return loadFrom(Value {slot, 'l'}, baseType(type)->fields[index].type);
    }
    return Value {};
}

void QbeEmitter::emitDiscriminantConstraint(Type* type)
{
    Symbol* symbol = type->discriminantSymbol;
    if (!symbol->isGlobal && !type->discriminantPerEvaluation) {
        symbol->frameOffset = reserveFrame(type->discriminantCount * 8);
    }
    Value address {symbol->qbeName, 'l'};
    if (type->discriminantPerEvaluation) {
        address.name = allocScratch(type->discriminantCount * 8);
        m_context->discriminantBounds[type] = address;
    } else if (!symbol->isGlobal) {
        address.name = newTemp();
        line(address.name + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->frameOffset));
    }
    for (int i = 0; i < type->discriminantCount; ++i) {
        Expr* expression = type->discriminantExpressions[i];
        Type* fieldType = baseType(type)->fields[i].type;
        if (!type->discriminantPerObject.empty() && type->discriminantPerObject[i]) { continue; }
        Value value = emitExpr(expression);
        emitRangeCheck(value, fieldType, expression->location);
        std::string slot = newTemp();
        line(slot + " =l add " + address.name + ", " + std::to_string(i * 8));
        storeInto(Value {slot, 'l'}, value, fieldType);
    }
}

void QbeEmitter::initializeDependentStorage(const Value& object, Type* type)
{
    if (!hasDependentStorage(type)) { return; }
    walkControlled(object, type, true, [&](const Value& part, Type* partType) {
        if (!partType->dependentStorage) { return; }
        line("storel 0, " + part.name);
        std::string slot = newTemp();
        line(slot + " =l add " + part.name + ", 8");
        line("storel " + (partType->layoutSymbol ? layoutAddress(partType).name : "0") + ", " + slot);
    });
}
