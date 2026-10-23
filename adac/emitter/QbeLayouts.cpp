#include "QbeEmitter.h"

#include <algorithm>
#include <map>

Value QbeEmitter::layoutAddress(Type* type)
{
    Symbol* symbol = type->m_layoutSymbol;
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
    if (type->m_layoutSymbol == nullptr) { return std::to_string(field.offset); }
    Value layout = layoutAddress(type);
    std::string slot = newTemp(), offset = newTemp();
    line(slot + " =l add " + layout.name + ", " + std::to_string((field.index + 1) * 8));
    line(offset + " =l loadl " + slot);
    return offset;
}

Value QbeEmitter::fieldAddress(const Value& object, Type* type, const FieldInfo& field)
{
    if (type->m_layoutSymbol == nullptr && field.offset == 0) { return withBounds(Value {object.name, 'l'}, field.type, nullptr); }
    std::string offset = fieldOffset(type, field);
    Value address {newTemp(), 'l'};
    line(address.name + " =l add " + object.name + ", " + offset);
    return withBounds(address, field.type, nullptr);
}

void QbeEmitter::emitRecordLayout(Type* type)
{
    if (type == nullptr || type->m_layoutSymbol == nullptr || type->m_layoutSymbol->type != type) { return; }
    Symbol* symbol = type->m_layoutSymbol;
    if (!symbol->isGlobal) { symbol->frameOffset = reserveFrame((type->fields.size() + 1) * 8); }
    Value layout = layoutAddress(type);
    std::map<int, std::string> cursors;
    cursors[-1] = type->m_protectedScope != nullptr ? "8" : "0";
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
        std::string offset = append(cursors[field.variant], "0", typeAlignment(field.type));
        std::string end = append(offset, runtimeSize(field.type), 1);
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
    if (type->constrained || type->m_boundsSymbol != nullptr) { return withBounds(pointer, type, nullptr); }
    std::string descriptor = newTemp();
    emitCallInstruction(descriptor + " =l call $__ada_allocation_array_bounds(l " + pointer.name
        + ", w " + std::to_string(type->arrayRank) + ")", CallEffects::CheckException);
    Value view = loadArrayView(descriptor, type->arrayRank, false);
    view.name = pointer.name;
    view.type = 'l';
    return view;
}
