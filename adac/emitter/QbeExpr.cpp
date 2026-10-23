#include "QbeEmitter.h"
#include "QbeSupport.h"

#include <cmath>

using QbeSupport::constantValue;
using QbeSupport::realLiteral;
using QbeSupport::isFloatClass;
using QbeSupport::comparisonInstruction;

Value QbeEmitter::emitExpr(Expr* expr)
{
    SourceLocation saved = m_context->sourceLocation;
    if (expr != nullptr) {
        m_context->sourceLocation = expr->location;
    }
    Value result = emitExprValue(expr);
    m_context->sourceLocation = saved;
    return result;
}

Value QbeEmitter::emitExprValue(Expr* expr)
{
    if (expr == nullptr) {
        return Value { "0", 'w' };
    }

    if (expr->fixedInvalid) {
        m_diagnostics.error(expr->location, "fixed-point value exceeds supported exact evaluation or storage limits");
        return Value { "0", 'l' };
    }
    if (expr->implicitCall != nullptr) {
        return emitExpr(expr->implicitCall.get());
    }
    if (expr->isStatic && expr->type != nullptr && expr->type->kind == TypeKind::Fixed) {
        return Value { std::to_string(expr->staticValue), 'l' };
    }
    if (expr->isStatic && isDiscrete(baseType(expr->type))) {
        // Universal named numbers may be wider than every machine type.
        // Refuse an unrepresentable materialization instead of using the
        // deliberately unset machine-value cache as zero.
        if (expr->exactReal.valid && expr->type->modulus != 0) {
            expr->staticValue = static_cast<long long>(expr->exactReal.numerator);
        } else if (expr->exactReal.valid && !expr->exactReal.integer(expr->staticValue)) {
            m_diagnostics.error(expr->location, "static value exceeds supported 64-bit machine storage");
            return Value { "0", qbeClass(expr->type) };
        }
        if (expr->type->modulus != 0
            && (static_cast<__int128>(static_cast<uint64_t>(expr->staticValue)) >= expr->type->modulus)) {
            raiseConstraintError();
        }
        // Check before narrowing the literal to a QBE word. Subtype bounds
        // are checked at value boundaries, not on intermediate expressions.
        if (qbeClass(expr->type) == 'w'
            && (expr->staticValue < -2147483648LL || expr->staticValue > 2147483647LL)) {
            raiseConstraintError();
        }
        return constantValue(expr->staticValue, qbeClass(expr->type));
    }
    if (expr->isStatic && isReal(expr->type)) {
        char type = qbeClass(expr->type);
        return Value { realLiteral(expr->staticReal, type), type };
    }

    switch (expr->kind) {
    case ExprKind::IntegerLiteral:
        return constantValue(static_cast<IntegerLiteralExpr*>(expr)->value, qbeClass(expr->type));
    case ExprKind::CharacterLiteral:
        return constantValue(static_cast<unsigned char>(static_cast<CharacterLiteralExpr*>(expr)->value), 'w');
    case ExprKind::RealLiteral: {
        char type = qbeClass(expr->type);
        return Value { realLiteral(static_cast<RealLiteralExpr*>(expr)->value, type), type };
    }
    case ExprKind::StringLiteral: {
        auto* literal = static_cast<StringLiteralExpr*>(expr);
        return Value { stringData(literal->value), 'l', "1", std::to_string(literal->value.size()) };
    }
    case ExprKind::Null:
        return Value { "0", 'l' };

    case ExprKind::Identifier: {
        auto* identifier = static_cast<IdentifierExpr*>(expr);
        Symbol* symbol = identifier->symbol;
        if (symbol == nullptr) {
            return Value { "0", 'w' };
        }
        if (symbol->kind == SymbolKind::Subprogram) {
            CallExpr call;
            call.location = expr->location;
            call.form = CallForm::Subprogram;
            call.subprogram = symbol;
            call.protectedObject = expr->protectedObject;
            call.protectedReceiver = expr->protectedReceiver;
            call.type = expr->type;
            call.dispatching = expr->tagIndeterminate && expr->type != nullptr && expr->type->classRoot != nullptr;
            call.tagIndeterminate = expr->tagIndeterminate;
            return emitCall(&call);
        }
        if (symbol->kind == SymbolKind::EnumerationLiteral) {
            return constantValue(symbol->enumerationValue, qbeClass(symbol->type));
        }
        if (symbol->kind == SymbolKind::Number) {
            if (isReal(symbol->type)) {
                char type = qbeClass(symbol->type);
                return Value { realLiteral(symbol->staticReal, type), type };
            }
            return constantValue(symbol->staticValue, qbeClass(symbol->type));
        }
        if (symbol->kind == SymbolKind::TypeName && symbol->type->kind == TypeKind::Array) {
            return withBounds(Value {}, symbol->type, nullptr);
        }
        Value address = symbol->protectedField >= 0 ? emitAddress(expr) : addressOf(symbol);
        if (isComposite(symbol->type)) {
            return withBounds(address, symbol->type, symbol->protectedField >= 0 ? nullptr : symbol);
        }
        return loadFrom(address, symbol->type);
    }

    case ExprKind::Selected: {
        auto* selected = static_cast<SelectedExpr*>(expr);
        Symbol* symbol = selected->symbol;
        if (symbol != nullptr && symbol->kind == SymbolKind::Subprogram) {
            CallExpr call;
            call.location = expr->location;
            call.form = CallForm::Subprogram;
            call.subprogram = symbol;
            call.protectedObject = expr->protectedObject;
            call.protectedReceiver = expr->protectedReceiver;
            call.type = expr->type;
            call.dispatching = expr->tagIndeterminate && expr->type != nullptr && expr->type->classRoot != nullptr;
            call.tagIndeterminate = expr->tagIndeterminate;
            return emitCall(&call);
        }
        if (symbol != nullptr && symbol->kind == SymbolKind::EnumerationLiteral) {
            return constantValue(symbol->enumerationValue, qbeClass(symbol->type));
        }
        if (symbol != nullptr && symbol->kind == SymbolKind::TypeName && symbol->type->kind == TypeKind::Array) {
            return withBounds(Value {}, symbol->type, nullptr);
        }
        Value address = emitAddress(expr);
        if (isComposite(expr->type)) {
            return withBounds(address, expr->type, selected->symbol != nullptr && selected->symbol->protectedField >= 0 ? nullptr : selected->symbol);
        }
        return loadFrom(address, expr->type);
    }

    case ExprKind::Allocator:
        return emitAllocator(static_cast<AllocatorExpr*>(expr));

    case ExprKind::Call: {
        auto* call = static_cast<CallExpr*>(expr);
        if (call->operatorExpression) {
            return emitExpr(call->operatorExpression.get());
        }
        if (call->form == CallForm::Subprogram) {
            return emitCall(call);
        }
        if (call->form == CallForm::Slice) {
            return emitSlice(call);
        }
        if (call->form == CallForm::Indexing) {
            Value address = emitAddress(expr);
            if (isComposite(expr->type)) {
                return withBounds(address, expr->type, nullptr);
            }
            return loadFrom(address, expr->type);
        }
        if (call->form == CallForm::Conversion) {
            Expr* operand = call->resolvedArguments.front();
            Value value = emitExpr(operand);
            if (expr->type->kind == TypeKind::Array) {
                Value converted = convertArrayBounds(value, operand->type, expr->type, expr->location);
                std::string bytes = objectBytes(converted, expr->type);
                converted.name = newTemp();
                emitCallInstruction(converted.name + " =l call $__ada_array_local(l " + storageArena(true, true)
                     + ", l 1, l 1, l " + (!hasRuntimeLayout(expr->type) ? std::to_string(typeSize(expr->type)) : bytes) + ")", CallEffects::CheckException);
                if (needsFinalization(expr->type)) {
                    bool saved = m_context->initializingTemporary;
                    m_context->initializingTemporary = true;
                    prepareControlledObject(converted, expr->type);
                    m_context->initializingTemporary = saved;
                    copyControlledObject(converted, value, expr->type, true);
                } else {
                    emitCallInstruction("call $memmove(l " + converted.name + ", l " + value.name + ", l " + bytes + ")", CallEffects::DeferredCheck);
                }
                return converted;
            }
            if (operand->type->classRoot != nullptr && expr->type->tagged) {
                Value valid = taggedMembership(value, expr->type);
                std::string good = newLabel("tagvalid");
                std::string bad = newLabel("taginvalid");
                branch(valid, good, bad);
                label(bad);
                raiseConstraintError();
                label(good);
            }
            if (expr->type->kind == TypeKind::Record && !expr->type->tagged) {
                emitRecordConstraintCheck(value, expr->type);
                if (typeSize(expr->type) != typeSize(operand->type)
                    && (typeSize(expr->type) != typeLayoutSize(expr->type) || typeSize(operand->type) != typeLayoutSize(operand->type))) {
                    Value converted {allocTypeScratch(expr->type), 'l'};
                    copyInto(converted, value, expr->type);
                    return converted;
                }
                return value;
            }
            char from = value.type;
            char to = qbeClass(expr->type);
            if (expr->type->kind == TypeKind::Fixed || operand->type->kind == TypeKind::Fixed) {
                if (operand->type->modulus != 0 || expr->type->modulus != 0) {
                    Value wide = wideBound(value);
                    std::string converted = newTemp();
                    if (operand->type->modulus != 0) {
                        emitCallInstruction(converted + " =l call $__ada_modular_to_fixed(l " + wide.name
                            + ", w " + std::to_string(expr->type->fixedScale) + ")", CallEffects::DeferredCheck);
                    } else {
                        emitCallInstruction(converted + " =l call $__ada_fixed_numeric_operation(w 2, l " + wide.name
                            + ", w " + std::to_string(operand->type->fixedScale) + ", l 1, w 0, w 1)", CallEffects::DeferredCheck);
                    }
                    emitExceptionCheck();
                    Value result { converted, 'l' };
                    emitRangeCheck(result, expr->type, expr->location);
                    if (to == 'w') {
                        std::string narrow = newTemp();
                        line(narrow + " =w copy " + converted);
                        result = Value { narrow, 'w' };
                    }
                    return result;
                }
                std::string result = newTemp();
                int fromBits = operand->type->fixedScale;
                int toBits = expr->type->fixedScale;
                if (isFloatClass(from)) {
                    std::string wide = value.name;
                    if (from == 's') {
                        wide = newTemp();
                        line(wide + " =d exts " + value.name);
                    }
                    emitCallInstruction(result + " =l call $__ada_float_to_fixed(d " + wide + ", w " + std::to_string(toBits) + ")", CallEffects::CheckException);
                } else if (isFloatClass(to)) {
                    emitCallInstruction(result + " =d call $__ada_fixed_real_operation(w 2, l " + value.name
                        + ", w " + std::to_string(fromBits) + ", l 1, w 0, w " + (to == 's' ? "24" : "53") + ")", CallEffects::CheckException);
                    if (to == 's') {
                        std::string narrow = newTemp();
                        line(narrow + " =s truncd " + result);
                        result = narrow;
                    }
                } else {
                    std::string wide = value.name;
                    if (from == 'w') {
                        wide = newTemp();
                        line(wide + " =l extsw " + value.name);
                    }
                    emitCallInstruction(result + " =l call $__ada_fixed_rescale(l " + wide + ", w " + std::to_string(fromBits)
                        + ", w " + std::to_string(toBits) + ")", CallEffects::CheckException);
                }
                Value converted { result, isFloatClass(to) ? to : 'l' };
                emitRangeCheck(converted, expr->type, expr->location);
                if (to == 'w') {
                    std::string narrow = newTemp();
                    line(narrow + " =w copy " + result);
                    return Value { narrow, 'w' };
                }
                return converted;
            }
            if (isDiscrete(operand->type) && isDiscrete(expr->type)
                && (operand->type->modulus != 0 || expr->type->modulus != 0)) {
                return emitDiscreteConversion(value, operand->type, expr->type);
            }
            if (from == to) {
                emitRangeCheck(value, expr->type, expr->location);
                return value;
            }
            std::string temp = newTemp();
            if (isFloatClass(from) && !isFloatClass(to)) {
                // Ada rounds when a real value becomes an integer, while the
                // conversion instructions of the backend truncate.
                std::string wide = value.name;
                if (from == 's') {
                    wide = newTemp();
                    line(wide + " =d exts " + value.name);
                }
                std::string rounded = newTemp();
                emitCallInstruction(rounded + " =l call $" + (expr->type->modulus != 0 ? "__ada_round_to_modular" : "__ada_round_to_integer") + "(d " + wide + ")", CallEffects::CheckException);
                emitRangeCheck(Value { rounded, 'l' }, expr->type, expr->location);
                line(temp + " =" + std::string(1, to) + " copy " + rounded);
            } else {
                std::string instruction;
                if (!isFloatClass(from) && isFloatClass(to)) {
                    instruction = operand->type->modulus != 0
                        ? (from == 'w' ? "uwtof" : "ultof") : (from == 'w' ? "swtof" : "sltof");
                } else if (from == 's' && to == 'd') {
                    instruction = "exts";
                } else if (from == 'd' && to == 's') {
                    instruction = "truncd";
                } else if (from == 'w' && to == 'l') {
                    instruction = "extsw";
                } else {
                    emitRangeCheck(value, expr->type, expr->location);
                    instruction = "copy";
                }
                line(temp + " =" + std::string(1, to) + " " + instruction + " " + value.name);
            }
            Value result { temp, to };
            emitRangeCheck(result, expr->type, expr->location);
            return result;
        }
        m_diagnostics.error(expr->location, "unsupported call form");
        return Value { "0", 'w' };
    }

    case ExprKind::Binary:
        return emitBinary(static_cast<BinaryExpr*>(expr));
    case ExprKind::Unary:
        return emitUnary(static_cast<UnaryExpr*>(expr));
    case ExprKind::Attribute:
        return emitAttribute(static_cast<AttributeExpr*>(expr));
    case ExprKind::Box:
        m_diagnostics.error(expr->location, "box default cannot be emitted as an expression");
        return Value {};
    case ExprKind::Aggregate:
        return emitAggregate(static_cast<AggregateExpr*>(expr));
    case ExprKind::Qualified: {
        Value value = emitExpr(static_cast<QualifiedExpr*>(expr)->operand.get());
        if (expr->type->scalarBoundsSymbol != nullptr || expr->type->modulus != 0 || expr->type->kind == TypeKind::Fixed) {
            emitRangeCheck(value, expr->type, expr->location);
        }
        return value;
    }

    case ExprKind::Membership: {
        auto* membership = static_cast<MembershipExpr*>(expr);
        Value operand = emitExpr(membership->operand.get());
        if (membership->testedTaggedType != nullptr) {
            Type* tested = membership->testedTaggedType;
            Value result { "1", 'w' };
            if (membership->operand->type->classRoot != nullptr) {
                if (tested->classRoot != nullptr) {
                    result = taggedMembership(operand, tested);
                } else {
                    std::string tag = newTemp();
                    result.name = newTemp();
                    line(tag + " =l loadl " + operand.name);
                    line(result.name + " =w ceql " + tag + ", " + typeTag(tested).name);
                }
            }
            if (membership->negated) {
                std::string negated = newTemp();
                line(negated + " =w ceqw " + result.name + ", 0");
                return Value { negated, 'w' };
            }
            return result;
        }
        Value low = emitExpr(membership->low.get());
        Value high = emitExpr(membership->high.get());
        std::string lowTest = newTemp();
        std::string highTest = newTemp();
        std::string combined = newTemp();
        line(lowTest + " =w " + comparisonInstruction(BinaryOp::GreaterEqual, operand.type, membership->operand->type->modulus != 0) + " " + operand.name
             + ", " + low.name);
        line(highTest + " =w " + comparisonInstruction(BinaryOp::LessEqual, operand.type, membership->operand->type->modulus != 0) + " " + operand.name
             + ", " + high.name);
        line(combined + " =w and " + lowTest + ", " + highTest);
        if (!membership->negated) {
            return Value { combined, 'w' };
        }
        std::string negated = newTemp();
        line(negated + " =w ceqw " + combined + ", 0");
        return Value { negated, 'w' };
    }
    }

    return Value { "0", 'w' };
}

Value QbeEmitter::emitAddress(Expr* expr)
{
    if (expr->implicitCall != nullptr) {
        return emitAddress(expr->implicitCall.get());
    }
    switch (expr->kind) {
    case ExprKind::Identifier: {
        auto* identifier = static_cast<IdentifierExpr*>(expr);
        if (identifier->symbol != nullptr
            && (identifier->symbol->kind == SymbolKind::Object
                || identifier->symbol->kind == SymbolKind::Parameter
                || identifier->symbol->kind == SymbolKind::LoopParameter)) {
            if (identifier->symbol->protectedField >= 0 && expr->protectedReceiver != nullptr) {
                Value object = addressOf(expr->protectedReceiver);
                const auto& field = identifier->symbol->protectedType->fields[identifier->symbol->protectedField];
                Value address { newTemp(), 'l' };
                line(address.name + " =l add " + object.name + ", " + fieldOffset(identifier->symbol->protectedType, field));
                return withBounds(address, field.type, nullptr);
            }
            return addressOf(identifier->symbol);
        }
        break;
    }
    case ExprKind::Selected: {
        auto* selected = static_cast<SelectedExpr*>(expr);
        if (selected->symbol != nullptr
            && (selected->symbol->kind == SymbolKind::Object || selected->symbol->kind == SymbolKind::Parameter)) {
            if (selected->symbol->protectedField >= 0 && expr->protectedReceiver != nullptr) {
                Value object = addressOf(expr->protectedReceiver);
                const auto& field = selected->symbol->protectedType->fields[selected->symbol->protectedField];
                Value address { newTemp(), 'l' };
                line(address.name + " =l add " + object.name + ", " + fieldOffset(selected->symbol->protectedType, field));
                return withBounds(address, field.type, nullptr);
            }
            return addressOf(selected->symbol);
        }
        if (selected->isDereference) {
            // The designated object lives where the access value points.
            Value pointer = emitExpr(selected->prefix.get());
            checkNotNull(pointer);
            return expr->type->kind == TypeKind::Array ? allocatedArrayView(pointer, expr->type) : pointer;
        }
        if (selected->fieldIndex >= 0) {
            Value base = emitAddress(selected->prefix.get());
            Type* record = baseType(selected->prefix->type);
            if (record != nullptr && record->kind == TypeKind::Access) {
                base = loadFrom(base, record);
                checkNotNull(base);
                record = record->target;
            }
            record = baseType(record);
            if (selected->checkedVariant >= 0) {
                checkVariant(base, record, selected->checkedVariant);
            }
            return fieldAddress(base, record, record->fields[selected->fieldIndex]);
        }
        break;
    }
    case ExprKind::Call: {
        auto* call = static_cast<CallExpr*>(expr);
        if (call->operatorExpression) {
            return emitExpr(call->operatorExpression.get());
        }
        if (call->form == CallForm::Slice) {
            return emitSlice(call);
        }
        if (call->form == CallForm::Indexing) {
            Value base = emitExpr(call->callee.get());
            Type* array = baseType(call->callee->type);
            if (array != nullptr && array->kind == TypeKind::Access) {
                // The access value is already the address of the array.
                checkNotNull(base);
                array = array->target;
                base = allocatedArrayView(base, array);
            }
            base = withBounds(base, array, nullptr);
            Value address = base;
            Value axis = base;
            Type* dimension = array;
            for (Expr* argument : call->resolvedArguments) {
                Value index = wideBound(emitExpr(argument));
                std::string offset = newTemp(), next = newTemp();
                emitCallInstruction(offset + " =l call $__ada_array_offset(l " + index.name + ", l " + axis.first
                     + ", l " + axis.last + ", l " + arrayElementSize(axis, dimension) + ")", CallEffects::CheckException);
                line(next + " =l add " + address.name + ", " + offset);
                address = Value { next, 'l' };
                axis = arrayRow(axis, dimension);
                dimension = dimension->element;
            }
            return address;
        }
        break;
    }
    default:
        break;
    }

    if (isComposite(baseType(expr->type))) {
        return emitExpr(expr);
    }

    m_diagnostics.error(expr->location, "this expression does not designate an object");
    return Value { "0", 'l' };
}

Value QbeEmitter::emitAllocator(AllocatorExpr* expr)
{
    Type* designated = expr->designated;
    Value bounds, source, initializerBounds;
    bool array = designated->kind == TypeKind::Array;
    bool preparedSource = false;
    ArrayAggregatePlan aggregatePlan;
    AggregateExpr* preparedAggregate = nullptr;
    if (array) {
        bool explicitBounds = designated->constrained || designated->boundsSymbol != nullptr
            || !expr->subtype->indexLows.empty();
        if (designated->constrained || designated->boundsSymbol != nullptr) {
            bounds = withBounds(Value {}, designated, nullptr);
        } else if (!expr->subtype->indexLows.empty()) {
            Type* axis = designated;
            for (int dimension = 0; dimension < designated->arrayRank; ++dimension) {
                Value first = wideBound(emitExpr(expr->subtype->indexLows[dimension].get()));
                Value last = wideBound(emitExpr(expr->subtype->indexHighs[dimension].get()));
                Value nonNull {newTemp(), 'w'};
                line(nonNull.name + " =w csgel " + last.name + ", " + first.name);
                std::string check = newLabel("allocatorboundscheck"), ready = newLabel("allocatorboundsready");
                branch(nonNull, check, ready);
                label(check);
                emitRangeCheck(first, axis->index, expr->location);
                emitRangeCheck(last, axis->index, expr->location);
                jump(ready);
                label(ready);
                if (dimension == 0) { bounds.first = first.name; bounds.last = last.name; }
                else { bounds.innerBounds.push_back({first.name, last.name}); }
                axis = axis->element;
            }
            bounds = arrayView(bounds, designated);
        }
        Expr* aggregate = expr->value.get();
        while (aggregate != nullptr && aggregate->kind == ExprKind::Qualified) {
            aggregate = static_cast<QualifiedExpr*>(aggregate)->operand.get();
        }
        if (aggregate != nullptr && aggregate->kind == ExprKind::Aggregate) {
            preparedAggregate = static_cast<AggregateExpr*>(aggregate);
            bounds = prepareArrayAggregate(aggregate, bounds, designated, aggregatePlan);
        } else if (expr->value != nullptr && hasLimitedControlledParts(designated) && !explicitBounds) {
            Value collection = collectionFor(expr->type);
            std::string pending = newTemp();
            emitCallInstruction(pending + " =l call $__ada_collection_begin(l " + collection.name + ")", CallEffects::CheckException);
            std::string failed = newLabel("arrayallocationfailed"), ready = newLabel("arrayallocationready");
            pushHandler(failed);
            m_context->resultTargetOwner = newTemp();
            m_context->resultTargetArena = newTemp();
            emitCallInstruction(m_context->resultTargetOwner + " =l call $__ada_collection_result_owner(l " + pending + ")", CallEffects::DeferredCheck);
            emitCallInstruction(m_context->resultTargetArena + " =l call $__ada_collection_result_arena(l " + pending + ")", CallEffects::DeferredCheck);
            Value result = emitExpr(expr->value.get());
            if (expr->type->target->boundsSymbol != nullptr || expr->type->target->constrained) {
                checkArrayShape(withBounds(Value {}, expr->type->target, nullptr), expr->type->target, result, designated);
            }
            emitCallInstruction("call $__ada_collection_finish(l " + pending + ", l " + result.name + ")", CallEffects::CheckException);
            std::string descriptor = allocScratch(adaArrayLocalSize(designated->arrayRank));
            line("storel " + result.name + ", " + descriptor);
            storeArrayView(descriptor, result, designated, false);
            emitCallInstruction("call $__ada_allocation_set_array_bounds(l " + result.name + ", l " + descriptor
                + ", w " + std::to_string(designated->arrayRank) + ")", CallEffects::CheckException);
            m_context->handlerLabels.pop_back();
            jump(ready);
            label(failed);
            emitCallInstruction("call $__ada_collection_abort(l " + pending + ")", CallEffects::DeferredCheck);
            if (!m_context->handlerLabels.empty()) { jump(m_context->handlerLabels.back()); }
            else { m_context->usesPropagate = true; jump(m_context->propagateLabel); }
            label(ready);
            return result;
        } else if (expr->value != nullptr && !(hasLimitedControlledParts(designated) && explicitBounds)) {
            source = emitExpr(expr->value.get());
            preparedSource = true;
            if (!explicitBounds) { bounds = source; }
            else { checkArrayShape(bounds, designated, source, expr->value->type); }
        }
        initializerBounds = bounds;
        if (expr->type->target->constrained || expr->type->target->boundsSymbol != nullptr) {
            Value target = withBounds(Value {}, expr->type->target, nullptr);
            checkArrayShape(target, expr->type->target, bounds, designated);
            bounds = target;
            designated = expr->type->target;
        }
    }
    // A static array's Size clause can add storage beyond its components.
    // Dynamic arrays use their evaluated shape; static objects reserve their
    // full storage size, just like records and local array declarations.
    std::string size = array && hasRuntimeLayout(designated) ? objectBytes(bounds, designated)
        : runtimeSize(isScalar(designated) ? expr->type->target : designated);
    if (designated->classRoot != nullptr && hasLimitedControlledParts(designated)) {
        Value collection = collectionFor(expr->type);
        std::string pending = newTemp();
        emitCallInstruction(pending + " =l call $__ada_collection_begin(l " + collection.name + ")", CallEffects::CheckException);
        std::string failed = newLabel("limitedallocationfailed");
        std::string ready = newLabel("limitedallocationready");
        pushHandler(failed);
        m_context->resultTargetOwner = newTemp();
        m_context->resultTargetArena = newTemp();
        emitCallInstruction(m_context->resultTargetOwner + " =l call $__ada_collection_result_owner(l " + pending + ")", CallEffects::DeferredCheck);
        emitCallInstruction(m_context->resultTargetArena + " =l call $__ada_collection_result_arena(l " + pending + ")", CallEffects::DeferredCheck);
        Value source = emitExpr(expr->value.get());
        emitCallInstruction("call $__ada_tag_check_accessibility(l " + source.name + ", w " + std::to_string(expr->type->accessLevel) + ")", CallEffects::CheckException);
        emitCallInstruction("call $__ada_collection_finish(l " + pending + ", l " + source.name + ")", CallEffects::CheckException);
        m_context->handlerLabels.pop_back();
        jump(ready);
        label(failed);
        emitCallInstruction("call $__ada_collection_abort(l " + pending + ")", CallEffects::DeferredCheck);
        if (!m_context->handlerLabels.empty()) {
            jump(m_context->handlerLabels.back());
        } else {
            m_context->usesPropagate = true;
            jump(m_context->propagateLabel);
        }
        label(ready);
        return source;
    }
    if (designated->classRoot != nullptr) {
        Value source = emitExpr(expr->value.get());
        emitCallInstruction("call $__ada_tag_check_accessibility(l " + source.name + ", w " + std::to_string(expr->type->accessLevel) + ")", CallEffects::CheckException);
        Value collection = collectionFor(expr->type);
        Value object { newTemp(), 'l' };
        emitCallInstruction(object.name + " =l call $__ada_tagged_allocation(l " + collection.name + ", l " + source.name + ")", CallEffects::CheckException);
        emitCallInstruction("call $__ada_collection_activate(l " + collection.name + ")", CallEffects::CheckException);
        return object;
    }
    {
        Value collection = collectionFor(expr->type);
        Value object { newTemp(), 'l' };
        emitCallInstruction(object.name + " =l call $__ada_collection_allocate(l " + collection.name
             + ", l " + size + ")", CallEffects::CheckException);
        if (array) { bounds.name = object.name; bounds.type = 'l'; object = bounds; }
        std::string failed = newLabel("allocationfailed");
        std::string ready = newLabel("allocationready");
        pushHandler(failed);
        if (array) {
            std::string descriptor = allocScratch(adaArrayLocalSize(designated->arrayRank));
            line("storel " + object.name + ", " + descriptor);
            storeArrayView(descriptor, object, designated, false);
            emitCallInstruction("call $__ada_allocation_set_array_bounds(l " + object.name + ", l " + descriptor
                + ", w " + std::to_string(designated->arrayRank) + ")", CallEffects::CheckException);
        }
        walkControlled(object, designated, false, [&](const Value& part, Type* partType) {
            emitCallInstruction("call $__ada_allocation_reserve(l " + object.name + ", l " + part.name
                 + ", l " + rootType(partType)->tagName + ".finalize)", CallEffects::CheckException);
        });
        std::string savedTaskCollection = m_context->taskCollection;
        std::string savedTaskGroup = m_context->taskGroup;
        m_context->taskCollection = collection.name;
        m_context->taskGroup = object.name;
        std::string savedOwner = m_context->constructionOwner;
        std::string savedArena = m_context->constructionArena;
        if (hasLimitedControlledParts(designated)) {
            m_context->constructionOwner = newTemp();
            m_context->constructionArena = newTemp();
            emitCallInstruction(m_context->constructionOwner + " =l call $__ada_construction_owner(l " + object.name + ")", CallEffects::DeferredCheck);
            emitCallInstruction(m_context->constructionArena + " =l call $__ada_construction_arena(l " + object.name + ")", CallEffects::DeferredCheck);
        }
        if (expr->value != nullptr && isScalar(designated)) {
            Value value = emitExpr(expr->value.get());
            emitRangeCheck(value, designated, expr->location);
            emitRangeCheck(value, expr->type->target, expr->location);
            storeInto(object, value, expr->type->target);
        } else if (preparedAggregate != nullptr) {
            initializerBounds.name = object.name;
            initializerBounds.type = 'l';
            emitDynamicAggregateInto(preparedAggregate, initializerBounds, expr->designated, &aggregatePlan);
        } else if (preparedSource) {
            checkArrayShape(object, designated, source, expr->value->type);
            if (needsFinalization(designated)) { copyControlledObject(object, source, designated, true); }
            else { copyInto(object, source, designated); }
        } else if (expr->value != nullptr) {
            assignInto(object, designated, expr->value.get(), true);
        } else {
            emitDefaultInit(object, designated);
        }
        m_context->taskCollection = savedTaskCollection;
        m_context->taskGroup = savedTaskGroup;
        m_context->constructionOwner = savedOwner;
        m_context->constructionArena = savedArena;
        if (hasTasks(designated)) {
            emitCallInstruction("call $__ada_task_activate_group(l %.taskContext, l " + collection.name + ", l " + object.name + ")", CallEffects::DeferredCheck);
        } else {
            emitCallInstruction("call $__ada_collection_activate(l " + collection.name + ")", CallEffects::DeferredCheck);
        }
        emitExceptionCheck();
        if (expr->type->target->classRoot != nullptr) {
            // Ownership is already recorded; only accessibility remains to check.
            emitCallInstruction("call $__ada_tag_check_accessibility(l " + object.name + ", w "
                 + std::to_string(expr->type->accessLevel) + ")", CallEffects::CheckException);
        }
        m_context->handlerLabels.pop_back();
        jump(ready);
        label(failed);
        emitCallInstruction("call $__ada_deallocate(l " + object.name + ")", CallEffects::DeferredCheck);
        if (!m_context->handlerLabels.empty()) {
            jump(m_context->handlerLabels.back());
        } else {
            m_context->usesPropagate = true;
            jump(m_context->propagateLabel);
        }
        label(ready);
        return object;
    }
}

// Whether anything inside the type carries a value of its own to start from.
bool QbeEmitter::hasComponentDefaults(Type* type)
{
    Type* base = baseType(type);
    if (base == nullptr) {
        return false;
    }
    if (base->kind == TypeKind::Access || base->taskBody != nullptr || base->protectedScope != nullptr) {
        return true;
    }
    if (base->kind == TypeKind::Array) {
        return hasComponentDefaults(type->element);
    }
    if (base->kind != TypeKind::Record) {
        return false;
    }
    if (base->tagged) {
        return true;
    }
    // A discriminant the subtype fixed is written into the object too, so that
    // reading it back gives what the declaration said.
    long long fixed = 0;
    for (int i = 0; i < base->discriminantCount; ++i) {
        if (discriminantValueOf(type, i, fixed)) {
            return true;
        }
    }
    for (const FieldInfo& field : base->fields) {
        if (field.defaultValue != nullptr || hasComponentDefaults(field.type)) {
            return true;
        }
    }
    return false;
}

// Apply declaration defaults and recursive subtype initialization. Access
// values must be explicitly null even when constructing into reused storage;
// scalar components without defaults retain their unspecified initial value.
void QbeEmitter::emitDefaultInit(const Value& address, Type* type)
{
    Type* base = baseType(type);
    if (base != nullptr && base->kind == TypeKind::Access) {
        storeInto(address, Value { "0", 'l' }, type);
        return;
    }
    if (base == nullptr || !hasComponentDefaults(type)) {
        return;
    }

    if (base->taskBody != nullptr) {
        Symbol* body = base->taskBody;
        Value link = body->level > 0 ? staticLinkFor(body->level - 1) : Value { "0", 'l' };
        Value flag = elaborationFlag(body);
        emitCallInstruction("call $__ada_task_create(l %.taskContext, l " + address.name + ", l " + body->qbeName
             + ", l " + link.name + ", l " + flag.name + ", l " + m_context->taskCollection + ", l "
             + m_context->taskGroup + ")", CallEffects::CheckException);
        return;
    }
    if (type->kind == TypeKind::Array && hasRuntimeLayout(type)) {
        emitArrayFill(address, type, nullptr);
        return;
    }
    if (base->kind == TypeKind::Array) {
        base = type;
        long long count = arrayLength(base);
        long long elementSize = typeSize(base->element);
        for (long long i = 0; i < count; ++i) {
            std::string slot = newTemp();
            line(slot + " =l add " + address.name + ", " + std::to_string(i * elementSize));
            emitDefaultInit(Value { slot, 'l' }, base->element);
        }
        return;
    }

    if (base->tagged) {
        line("storel " + typeTag(base).name + ", " + address.name);
    }
    for (const FieldInfo& field : base->fields) {
        // Every enclosing alternative must be active before a default runs.
        int presence = variantPresence(type, field.variant);
        if (presence == 0) {
            continue;
        }
        std::string next;
        if (presence < 0) {
            std::string active = newLabel("defaultvariant");
            next = newLabel("defaultnextvariant");
            branch(variantCondition(address, base, field.variant), active, next);
            label(active);
        }
        Value slot = fieldAddress(address, base, field);
        long long fixed = 0;
        if (field.isDiscriminant && discriminantValueOf(type, field.index, fixed)) {
            storeInto(slot, constantValue(fixed, qbeClass(field.type)), field.type);
        } else if (field.defaultValue != nullptr) {
            assignInto(slot, field.type, field.defaultValue, true);
        } else {
            emitDefaultInit(slot, field.type);
        }
        if (!next.empty()) {
            jump(next);
            label(next);
        }
    }
    if (base->protectedScope != nullptr) {
        emitCallInstruction("call $__ada_protected_init(l " + address.name + ")", CallEffects::CheckException);
        activateControlledObject(address, type);
    }
    if (base->controlled) {
        emitControlledCall(address, type, "initialize");
        emitExceptionCheck();
        activateControlledObject(address, type);
    }
}
