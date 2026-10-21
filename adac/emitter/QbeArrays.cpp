#include "QbeEmitter.h"
#include "QbeSupport.h"

#include <algorithm>

using QbeSupport::constantValue;
using QbeSupport::isLiteralOperand;

Value QbeEmitter::wideBound(Value value)
{
    if (value.type != 'l') {
        std::string wide = newTemp();
        line(wide + " =l extsw " + value.name);
        value = Value { wide, 'l' };
    }
    return value;
}

Value QbeEmitter::narrowArrayLength(Value value)
{
    std::string narrow = newTemp();
    line(narrow + " =w call $__ada_array_narrow(l " + value.name + ")");
    emitExceptionCheck();
    return Value { narrow, 'w' };
}

Value QbeEmitter::arrayView(Value value, Type* type)
{
    if (!value.hasBounds() || type == nullptr || type->kind != TypeKind::Array) { return value; }
    if (value.strides.size() == static_cast<std::size_t>(type->arrayRank)) { return value; }
    value.lengths.clear();
    value.strides.resize(type->arrayRank);
    Type* cell = type;
    for (int i = 0; i < type->arrayRank; ++i) {
        Value axis { "", 'l', value.axisFirst(i), value.axisLast(i) };
        value.lengths.push_back(lengthOf(axis, nullptr).name);
        cell = cell->element;
    }
    std::string stride = std::to_string(typeSize(cell));
    for (int i = type->arrayRank - 1; i >= 0; --i) {
        value.strides[i] = stride;
        if (i != 0) {
            if (isLiteralOperand(value.lengths[i]) && isLiteralOperand(stride)) {
                __int128 bytes = (__int128)std::stoll(value.lengths[i]) * std::stoll(stride);
                if (bytes <= INT64_MAX) { stride = std::to_string((long long)bytes); continue; }
            }
            std::string bytes = newTemp();
            line(bytes + " =l call $__ada_array_size(l " + value.axisFirst(i) + ", l " + value.axisLast(i)
                 + ", l " + stride + ")");
            emitExceptionCheck();
            stride = bytes;
        }
    }
    return value;
}

Value QbeEmitter::loadArrayView(const std::string& descriptor, int rank, bool result)
{
    Value view;
    for (int dimension = 0; dimension < rank; ++dimension) {
        std::vector<std::string> fields;
        int offset = result ? adaArrayResultFirst(dimension) : adaArrayLocalFirst(dimension);
        for (int field = 0; field < 4; ++field) {
            std::string slot = newTemp(), value = newTemp();
            line(slot + " =l add " + descriptor + ", " + std::to_string(offset + field * AdaArrayBoundBytes));
            line(value + " =l loadl " + slot);
            fields.push_back(value);
        }
        if (dimension == 0) { view.first = fields[0]; view.last = fields[1]; }
        else { view.innerBounds.push_back({ fields[0], fields[1] }); }
        view.lengths.push_back(fields[2]);
        view.strides.push_back(fields[3]);
    }
    return view;
}

void QbeEmitter::storeArrayView(const std::string& descriptor, Value value, Type* type, bool result)
{
    value = arrayView(value, type);
    for (int dimension = 0; dimension < type->arrayRank; ++dimension) {
        int offset = result ? adaArrayResultFirst(dimension) : adaArrayLocalFirst(dimension);
        std::vector<std::string> fields { value.axisFirst(dimension), value.axisLast(dimension),
            value.lengths[dimension], value.strides[dimension] };
        for (int field = 0; field < 4; ++field) {
            std::string slot = newTemp();
            line(slot + " =l add " + descriptor + ", " + std::to_string(offset + field * AdaArrayBoundBytes));
            line("storel " + fields[field] + ", " + slot);
        }
    }
    if (result) {
        std::string bytes = newTemp(), empty = newTemp(), padded = newTemp(), slot = newTemp();
        line(bytes + " =l call $__ada_array_size(l " + value.first + ", l " + value.last + ", l " + value.strides.front() + ")");
        emitExceptionCheck();
        line(empty + " =w ceql " + bytes + ", 0");
        std::string wideEmpty = wideBound(Value { empty, 'w' }).name;
        line(padded + " =l add " + bytes + ", " + wideEmpty);
        line(slot + " =l add " + descriptor + ", " + std::to_string(AdaArrayTransferOffset));
        line("storel " + padded + ", " + slot);
    }
}

void QbeEmitter::emitTypeBounds(Type* type, const SourceLocation& location)
{
    if (type == nullptr || type->m_boundExpressions.empty()) {
        return;
    }
    Symbol* symbol = type->m_boundsSymbol;
    symbol->frameOffset = reserveFrame(adaArrayLocalSize(type->arrayRank));
    Value bounds;
    Type* axis = type;
    for (int dimension = 0; dimension < type->arrayRank; ++dimension) {
        auto [low, high] = type->m_boundExpressions[dimension];
        Value first = low ? emitExpr(low) : constantValue(axis->indexLow, 'l');
        Value last = high ? emitExpr(high) : constantValue(axis->indexHigh, 'l');
        first = wideBound(first);
        last = wideBound(last);
        std::string nonNull = newTemp();
        line(nonNull + " =w csgel " + last.name + ", " + first.name);
        std::string check = newLabel("typeboundscheck");
        std::string ready = newLabel("typeboundsready");
        branch(Value { nonNull, 'w' }, check, ready);
        label(check);
        emitRangeCheck(first, axis->index, location);
        emitRangeCheck(last, axis->index, location);
        jump(ready);
        label(ready);
        if (dimension == 0) { bounds.first = first.name; bounds.last = last.name; }
        else { bounds.innerBounds.push_back({ first.name, last.name }); }
        axis = axis->element;
    }
    std::string descriptor = newTemp();
    line(descriptor + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->frameOffset));
    storeArrayView(descriptor, bounds, type, false);
}

void QbeEmitter::emitDynamicArray(ObjectDecl* object, Symbol* symbol)
{
    Type* type = symbol->type;
    Value bounds;
    bool explicitBounds = type->constrained || !object->subtype->indexLows.empty() || type->m_boundsSymbol != nullptr;
    if (type->constrained) { bounds = withBounds(Value {}, type, nullptr); }
    if (type->m_boundsSymbol != nullptr) {
        bounds = boundsFor(type->m_boundsSymbol);
    } else if (explicitBounds && !type->constrained) {
        Type* axis = type;
        for (int dimension = 0; dimension < type->arrayRank; ++dimension) {
            Value first = emitExpr(object->subtype->indexLows[dimension].get());
            Value last = emitExpr(object->subtype->indexHighs[dimension].get());
            first = wideBound(first);
            last = wideBound(last);
            if (dimension == 0) {
                bounds.first = first.name;
                bounds.last = last.name;
            } else {
                bounds.innerBounds.push_back({ first.name, last.name });
            }
            std::string nonNull = newTemp();
            line(nonNull + " =w csgel " + last.name + ", " + first.name);
            std::string check = newLabel("checkbounds");
            std::string ready = newLabel("boundsready");
            branch(Value { nonNull, 'w' }, check, ready);
            label(check);
            emitRangeCheck(first, axis->index, object->location);
            emitRangeCheck(last, axis->index, object->location);
            jump(ready);
            label(ready);
            axis = axis->element;
        }
    }
    Value source;
    Expr* aggregateExpr = object->initializer.get();
    while (aggregateExpr != nullptr && aggregateExpr->kind == ExprKind::Qualified) {
        aggregateExpr = static_cast<QualifiedExpr*>(aggregateExpr)->operand.get();
    }
    bool aggregate = isAggregateExpression(object->initializer.get())
        && (explicitBounds || needsFinalization(type));
    ArrayAggregatePlan aggregatePlan;
    bool preparedAggregate = aggregate && !explicitBounds;
    if (preparedAggregate) {
        Value context = withBounds(Value {}, object->initializer->type, nullptr);
        bounds = prepareArrayAggregate(aggregateExpr, context, type, aggregatePlan);
    }
    bool constructedResult = object->initializer && !aggregate && hasLimitedControlledParts(type);
    if (object->initializer && !aggregate) {
        if (constructedResult) {
            m_context->m_resultTargetOwner = m_context->m_finalizationChain;
            m_context->m_resultTargetArena = storageArena(false, true);
        }
        source = emitExpr(object->initializer.get());
        if (!explicitBounds) {
            bounds.first = source.first;
            bounds.last = source.last;
            bounds.innerBounds = source.innerBounds;
        }
    }
    if (!bounds.hasBounds()) {
        m_diagnostics.error(object->location, "array object bounds cannot be inferred from this initializer");
        return;
    }
    m_context->arrayArenaUsed = true;
    if (m_context->arrayArena.empty()) {
        m_context->arrayArena = allocScratch(8);
        m_context->prologue << "    storel 0, " << m_context->arrayArena << "\n";
    }
    bounds = arrayView(bounds, type);
    std::string elementSize = arrayElementSize(bounds, type);
    std::string pointer = constructedResult ? source.name : newTemp();
    if (!constructedResult) {
        line(pointer + " =l call $__ada_array_local(l " + m_context->arrayArena + ", l " + bounds.first
             + ", l " + bounds.last + ", l " + elementSize + ")");
        emitExceptionCheck();
    }
    Value address = bounds;
    address.name = pointer;
    address.type = 'l';
    if (symbol->isUplevel) {
        symbol->frameOffset = reserveFrame(adaArrayLocalSize(type->arrayRank));
        std::string slot = newTemp();
        line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->frameOffset));
        line("storel " + pointer + ", " + slot);
        storeArrayView(slot, bounds, type, false);
    } else {
        m_context->locals[symbol] = pointer;
        m_context->bounds[symbol] = bounds;
    }
    if (constructedResult) {
        checkArrayShape(address, type, source, object->initializer->type);
        return;
    }
    prepareControlledObject(address, type);
    if (preparedAggregate) {
        emitDynamicAggregateInto(static_cast<AggregateExpr*>(aggregateExpr), address, type, &aggregatePlan);
    } else if (aggregate) {
        assignInto(address, type, object->initializer.get(), true);
    } else if (object->initializer) {
        checkArrayShape(address, type, source, object->initializer->type);
        std::string size = newTemp();
        line(size + " =l call $__ada_array_size(l " + address.first + ", l " + address.last + ", l " + elementSize + ")");
        emitExceptionCheck();
        if (needsFinalization(type)) {
            copyControlledObject(address, source, type, true);
        } else {
            line("call $memmove(l " + pointer + ", l " + source.name + ", l " + size + ")");
        }
    } else if (hasComponentDefaults(type->element)) {
        emitArrayFill(address, type, nullptr);
    }
}

void QbeEmitter::emitArrayFill(const Value& address, Type* type, Expr* value)
{
    std::string elementSize = arrayElementSize(address, type);
    Value length = lengthOf(address, type);
    std::string wideLength = newTemp();
    line(wideLength + " =l copy " + length.name);
    std::string slot = allocScratch(8);
    line("storel 0, " + slot);
    std::string head = newLabel("arrayfill");
    std::string body = newLabel("arrayfillbody");
    std::string done = newLabel("arrayfilldone");
    label(head);
    std::string index = newTemp();
    std::string test = newTemp();
    line(index + " =l loadl " + slot);
    line(test + " =w csltl " + index + ", " + wideLength);
    branch(Value { test, 'w' }, body, done);
    label(body);
    std::string offset = newTemp();
    std::string element = newTemp();
    line(offset + " =l mul " + index + ", " + elementSize);
    line(element + " =l add " + address.name + ", " + offset);
    Value cell = arrayRow(address, type);
    cell.name = element;
    auto elementStorage = storageCheckpoint();
    if (value != nullptr) {
        assignInto(cell, type->element, value, true);
    } else {
        emitDefaultInit(cell, type->element);
    }
    rewindStorage(elementStorage);
    std::string next = newTemp();
    line(next + " =l add " + index + ", 1");
    line("storel " + next + ", " + slot);
    jump(head);
    label(done);
}

Value QbeEmitter::boundsFor(Symbol* symbol)
{
    if (symbol->owner == m_context->symbol && symbol->frameOffset < 0) {
        auto it = m_context->bounds.find(symbol);
        if (it != m_context->bounds.end()) {
            return it->second;
        }
    }
    Value result;
    if (symbol->frameOffset < 0) {
        return result;
    }
    Value frame = symbol->owner == m_context->symbol ? Value { m_context->frameTemp, 'l' }
                                                     : staticLinkFor(symbol->owner->level);
    std::string descriptor = newTemp();
    line(descriptor + " =l add " + frame.name + ", " + std::to_string(symbol->frameOffset));
    result = loadArrayView(descriptor, symbol->type->arrayRank, false);
    return result;
}

Value QbeEmitter::withBounds(const Value& address, Type* type, Symbol* symbol)
{
    Value result = address;
    if (type == nullptr || type->kind != TypeKind::Array) {
        return result;
    }
    if (type->constrained) {
        result.first = std::to_string(type->indexLow);
        result.last = std::to_string(type->indexHigh);
        result.innerBounds.clear();
        result.strides.clear();
        result.lengths.clear();
        Type* row = type;
        for (int dimension = 1; dimension < type->arrayRank; ++dimension) {
            row = row->element;
            result.innerBounds.push_back({ std::to_string(row->indexLow), std::to_string(row->indexHigh) });
        }
    } else if (symbol != nullptr || (!result.hasBounds() && type->m_boundsSymbol != nullptr)) {
        Value bounds = boundsFor(symbol != nullptr && symbol->kind != SymbolKind::TypeName
                                    ? symbol : type->m_boundsSymbol);
        result.first = bounds.first;
        result.last = bounds.last;
        result.innerBounds = bounds.innerBounds;
        result.strides = bounds.strides;
        result.lengths = bounds.lengths;
    }
    return arrayView(result, type);
}

Value QbeEmitter::arrayRow(const Value& array, Type* type)
{
    Value row { array.name, 'l' };
    if (type->arrayRank > 1 && !array.innerBounds.empty()) {
        row.first = array.innerBounds.front().first;
        row.last = array.innerBounds.front().second;
        row.innerBounds.assign(array.innerBounds.begin() + 1, array.innerBounds.end());
        if (!array.strides.empty()) { row.strides.assign(array.strides.begin() + 1, array.strides.end()); }
        if (!array.lengths.empty()) { row.lengths.assign(array.lengths.begin() + 1, array.lengths.end()); }
    }
    return withBounds(row, type->element, nullptr);
}

std::string QbeEmitter::arrayElementSize(const Value& array, Type* type)
{
    Value view = withBounds(array, type, nullptr);
    if (!view.strides.empty()) { return view.strides.front(); }
    return std::to_string(typeSize(type->element));
}

void QbeEmitter::checkArrayShape(const Value& target, Type* targetType, const Value& source, Type* sourceType)
{
    Value left = withBounds(target, targetType, nullptr);
    Value right = withBounds(source, sourceType, nullptr);
    int rank = targetType->arrayRank;
    for (int dimension = 0; dimension < rank; ++dimension) {
        Value leftLength = lengthOf(left, targetType);
        Value rightLength = lengthOf(right, sourceType);
        std::string same = newTemp();
        std::string ok = newLabel("shapeok");
        std::string bad = newLabel("shapebad");
        line(same + " =w ceql " + leftLength.name + ", " + rightLength.name);
        branch(Value { same, 'w' }, ok, bad);
        label(bad);
        raiseConstraintError();
        label(ok);
        if (dimension + 1 < rank) {
            left = arrayRow(left, targetType);
            right = arrayRow(right, sourceType);
            targetType = targetType->element;
            sourceType = sourceType->element;
        }
    }
}

Value QbeEmitter::lengthOf(const Value& array, Type* type)
{
    if (!array.lengths.empty()) { return Value { array.lengths.front(), 'l' }; }
    if (type != nullptr && type->kind == TypeKind::Array && type->constrained) {
        return constantValue(arrayLength(type), 'l');
    }
    if (!array.hasBounds()) { return constantValue(0, 'l'); }
    if (isLiteralOperand(array.first) && isLiteralOperand(array.last)) {
        __int128 length = (__int128)std::stoll(array.last) - std::stoll(array.first) + 1;
        if (length <= INT64_MAX) { return constantValue(length < 0 ? 0 : (long long)length, 'l'); }
    }
    std::string length = newTemp();
    line(length + " =l call $__ada_array_length(l " + array.first + ", l " + array.last + ")");
    emitExceptionCheck();
    return Value { length, 'l' };
}

Value QbeEmitter::emitSlice(CallExpr* expr)
{
    Value base = emitExpr(expr->callee.get());
    Type* array = expr->callee->type;
    if (array->kind == TypeKind::Access) {
        checkNotNull(base);
        array = array->target;
    }
    base = withBounds(base, array, nullptr);
    Value low = emitExpr(expr->resolvedArguments[0]);
    Value high = emitExpr(expr->resolvedArguments[1]);

    low = wideBound(low);
    high = wideBound(high);
    std::string offset = newTemp(), address = newTemp();
    line(offset + " =l call $__ada_array_slice_offset(l " + low.name + ", l " + high.name
         + ", l " + base.first + ", l " + base.last + ", l " + arrayElementSize(base, array) + ")");
    emitExceptionCheck();
    line(address + " =l add " + base.name + ", " + offset);
    return Value { address, 'l', low.name, high.name };
}

Value QbeEmitter::emitConcatenation(BinaryExpr* expr)
{
    std::vector<Expr*> parts;
    std::vector<Expr*> pending = { expr };
    while (!pending.empty()) {
        Expr* current = pending.back();
        pending.pop_back();
        if (current->kind == ExprKind::Binary
            && static_cast<BinaryExpr*>(current)->op == BinaryOp::Concatenate
            && static_cast<BinaryExpr*>(current)->operatorCall == nullptr) {
            auto* binary = static_cast<BinaryExpr*>(current);
            pending.push_back(binary->right.get());
            pending.push_back(binary->left.get());
            continue;
        }
        parts.push_back(current);
    }

    struct Operand
    {
        Value value;
        Value length;
        bool isCharacter = false;
    };

    std::vector<Operand> operands;
    bool allStatic = true;
    long long staticTotal = 0;

    for (Expr* part : parts) {
        Operand operand;
        operand.value = emitExpr(part);
        operand.isCharacter = part->type == nullptr || part->type->kind != TypeKind::Array;
        if (operand.isCharacter) {
            operand.length = constantValue(1, 'l');
        } else {
            if (typeSize(part->type->element) != 1) {
                m_diagnostics.error(part->location, "'&' is only supported on arrays of characters");
                return Value { "0", 'l' };
            }
            operand.length = lengthOf(operand.value, part->type);
        }
        if (isLiteralOperand(operand.length.name)) {
            if (__builtin_add_overflow(staticTotal, std::stoll(operand.length.name), &staticTotal)) {
                m_diagnostics.error(part->location, "concatenation length exceeds signed 64-bit descriptor capacity");
                return Value { "0", 'l' };
            }
        } else {
            allStatic = false;
        }
        operands.push_back(operand);
    }

    std::string total = std::to_string(staticTotal);
    for (const Operand& operand : operands) {
        if (isLiteralOperand(operand.length.name)) {
            continue;
        }
        total = emitIntegerOperation(0, Value { total, 'l' }, operand.length, 'l').name;
    }

    std::string buffer;
    if (allStatic) {
        buffer = allocScratch(staticTotal > 0 ? staticTotal : 1);
    } else {
        buffer = newTemp();
        line(buffer + " =l call $__ada_array_local(l " + storageArena(true, true) + ", l 1, l " + total + ", l 1)");
        emitExceptionCheck();
    }

    std::string running = buffer;
    long long staticOffset = 0;
    bool dynamic = false;

    for (const Operand& operand : operands) {
        std::string target = running;
        if (!dynamic) {
            target = buffer;
            if (staticOffset > 0) {
                target = newTemp();
                line(target + " =l add " + buffer + ", " + std::to_string(staticOffset));
            }
        }

        if (operand.isCharacter) {
            line("storeb " + operand.value.name + ", " + target);
        } else if (isLiteralOperand(operand.length.name) && std::stoll(operand.length.name) <= m_arrayStackLimit) {
            line("blit " + operand.value.name + ", " + target + ", " + operand.length.name);
        } else {
            std::string bytes = newTemp();
            line(bytes + " =l copy " + operand.length.name);
            line(newTemp() + " =l call $memcpy(l " + target + ", l " + operand.value.name + ", l " + bytes + ")");
        }

        if (!dynamic && isLiteralOperand(operand.length.name)) {
            staticOffset += std::stoll(operand.length.name);
            continue;
        }
        dynamic = true;
        std::string advance = newTemp();
        line(advance + " =l copy " + operand.length.name);
        std::string next = newTemp();
        line(next + " =l add " + target + ", " + advance);
        running = next;
    }

    return Value { buffer, 'l', "1", total };
}

// Conversion changes the descriptor's bounds, never the source descriptor.
Value QbeEmitter::convertArrayBounds(Value value, Type* source, Type* target, const SourceLocation& location)
{
    value = withBounds(value, source, nullptr);
    if (target->constrained || target->m_boundsSymbol != nullptr) {
        Value shape = withBounds(Value {}, target, nullptr);
        checkArrayShape(shape, target, value, source);
        shape.name = value.name;
        return shape;
    }
    Type* axis = target;
    for (int dimension = 0; dimension < target->arrayRank; ++dimension) {
        const auto& first = dimension == 0 ? value.first : value.innerBounds[dimension - 1].first;
        const auto& last = dimension == 0 ? value.last : value.innerBounds[dimension - 1].second;
        std::string wideFirst = newTemp();
        std::string wideLast = newTemp();
        line(wideFirst + " =l copy " + first);
        line(wideLast + " =l copy " + last);
        // Null ranges skip subtype constraints, but their endpoints still
        // must be representable values of the target index's base type.
        Type* indexBase = m_sema.typeTable().scalarBaseType(axis->index);
        emitRangeCheck(Value { wideFirst, 'l' }, indexBase, location);
        emitRangeCheck(Value { wideLast, 'l' }, indexBase, location);
        std::string nonNull = newTemp();
        std::string check = newLabel("conversionbounds");
        std::string ready = newLabel("conversionboundsready");
        line(nonNull + " =w csgel " + last + ", " + first);
        branch(Value { nonNull, 'w' }, check, ready);
        label(check);
        emitRangeCheck(Value { wideFirst, 'l' }, axis->index, location);
        emitRangeCheck(Value { wideLast, 'l' }, axis->index, location);
        jump(ready);
        label(ready);
        axis = axis->element;
    }
    return value;
}

Value QbeEmitter::emitArrayArgument(Expr* argument, ParameterMode mode, std::vector<ArrayCopyBack>& copies)
{
    if (mode == ParameterMode::In || argument->kind != ExprKind::Call
        || static_cast<CallExpr*>(argument)->form != CallForm::Conversion) {
        return emitExpr(argument);
    }
    auto* conversion = static_cast<CallExpr*>(argument);
    Expr* operand = conversion->resolvedArguments.front();
    Value actual = emitExpr(operand);
    Value view = convertArrayBounds(actual, operand->type, argument->type, argument->location);
    // Arrays with by-reference components keep the identity of their elements.
    // Sema only permits these views within the same array type family.
    if (hasPrivateOrTaggedParts(argument->type)) {
        return view;
    }
    ArrayCopyBack copy { actual, view, argument->type, operand->type, argument->location };
    std::string bytes = objectBytes(view, argument->type);
    copy.temporary.name = newTemp();
    line(copy.temporary.name + " =l call $__ada_array_local(l " + storageArena(true, true)
         + ", l 1, l 1, l " + bytes + ")");
    emitExceptionCheck();
    line("call $memmove(l " + copy.temporary.name + ", l " + actual.name + ", l " + bytes + ")");
    copies.push_back(copy);
    return copy.temporary;
}

void QbeEmitter::emitArrayCopyBacks(const std::vector<ArrayCopyBack>& copies)
{
    for (const ArrayCopyBack& copy : copies) {
        Value value = convertArrayBounds(copy.temporary, copy.sourceType, copy.targetType, copy.location);
        checkArrayShape(copy.actual, copy.targetType, value, copy.targetType);
        std::string bytes = objectBytes(copy.actual, copy.targetType);
        line("call $memmove(l " + copy.actual.name + ", l " + value.name + ", l " + bytes + ")");
    }
}
