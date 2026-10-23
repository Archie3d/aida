#include "QbeEmitter.h"
#include "QbeSupport.h"

#include <algorithm>
#include <limits>

using QbeSupport::isUnconstrainedArray;

// Multidimensional aggregates determine every index range before evaluating
// component values. Keep choice operands so repeated rows do not reevaluate
// their bounds while filling storage.
Value QbeEmitter::prepareArrayAggregate(Expr* expr, const Value& context, Type* type, ArrayAggregatePlan& plan)
{
    auto require = [&](const std::string& condition) {
        std::string ok = newLabel("aggregateshapeok");
        std::string bad = newLabel("aggregateshapebad");
        branch(Value { condition, 'w' }, ok, bad);
        label(bad);
        raiseConstraintError();
        label(ok);
    };
    Value indexBounds = scalarBounds(type->index);
    std::string first = context.hasBounds() ? context.first
        : wideBound(Value { indexBounds.first, indexBounds.type }).name;
    std::string last;
    AggregateExpr* aggregate = nullptr;
    bool others = false;
    if (expr->kind == ExprKind::StringLiteral) {
        auto* literal = static_cast<StringLiteralExpr*>(expr);
        last = newTemp();
        last = emitIntegerOperation(0, Value { first, 'l' }, Value { std::to_string(static_cast<long long>(literal->value.size()) - 1), 'l' }, 'l').name;
    } else if (expr->kind == ExprKind::Aggregate) {
        aggregate = static_cast<AggregateExpr*>(expr);
        std::vector<std::pair<std::string, std::string>> ranges;
        long long positional = 0;
        auto choiceValue = [&](Expr* choice) {
            Value value = emitExpr(choice);
            emitRangeCheck(value, m_sema.typeTable().scalarBaseType(type->index), choice->location);
            plan.choices[choice] = value;
            return wideBound(value).name;
        };
        for (AggregateComponent& component : aggregate->components) {
            if (component.isOthers) {
                others = true;
            } else if (component.choiceLows.empty()) {
                ++positional;
            } else {
                for (std::size_t i = 0; i < component.choiceLows.size(); ++i) {
                    std::string low = choiceValue(component.choiceLows[i].get());
                    std::string high = component.choiceHighs[i] ? choiceValue(component.choiceHighs[i].get()) : low;
                    ranges.push_back({ low, high });
                }
            }
        }
        if (others) {
            if (!context.hasBounds()) {
                m_diagnostics.error(expr->location, "others requires bounds from the aggregate context");
                return Value {};
            }
            last = context.last;
            for (const auto& range : ranges) {
                std::string low = newTemp();
                std::string high = newTemp();
                std::string valid = newTemp();
                line(low + " =w csgel " + range.first + ", " + first);
                line(high + " =w cslel " + range.second + ", " + last);
                line(valid + " =w and " + low + ", " + high);
                require(valid);
            }
            if (positional != 0) {
                std::string end = newTemp();
                std::string valid = newTemp();
                end = emitIntegerOperation(0, Value { first, 'l' }, Value { std::to_string(positional - 1), 'l' }, 'l').name;
                line(valid + " =w cslel " + end + ", " + last);
                require(valid);
            }
        } else if (ranges.empty()) {
            last = newTemp();
            last = emitIntegerOperation(0, Value { first, 'l' }, Value { std::to_string(positional - 1), 'l' }, 'l').name;
        } else if (ranges.size() == 1) {
            first = ranges.front().first;
            last = ranges.front().second;
        } else {
            // Sema requires multiple choices to be static and contiguous.
            long long low = std::numeric_limits<long long>::max();
            long long high = std::numeric_limits<long long>::min();
            for (AggregateComponent& component : aggregate->components) {
                for (std::size_t i = 0; i < component.choiceLows.size(); ++i) {
                    low = std::min(low, component.choiceLows[i]->staticValue);
                    high = std::max(high, component.choiceHighs[i]
                        ? component.choiceHighs[i]->staticValue : component.choiceLows[i]->staticValue);
                }
            }
            first = std::to_string(low);
            last = std::to_string(high);
        }
    } else {
        m_diagnostics.error(expr->location, "multidimensional aggregate requires nested subaggregates");
        return Value {};
    }
    emitRangeCheck(Value { first, 'l' }, m_sema.typeTable().scalarBaseType(type->index), expr->location);
    emitRangeCheck(Value { last, 'l' }, m_sema.typeTable().scalarBaseType(type->index), expr->location);
    std::string nonNull = newTemp();
    std::string check = newLabel("aggregateindexbounds");
    std::string ready = newLabel("aggregateindexready");
    line(nonNull + " =w csgel " + last + ", " + first);
    branch(Value { nonNull, 'w' }, check, ready);
    label(check);
    emitRangeCheck(Value { first, 'l' }, type->index, expr->location);
    emitRangeCheck(Value { last, 'l' }, type->index, expr->location);
    jump(ready);
    label(ready);
    Value shape { "", 'l', first, last };
    if (context.hasBounds()) {
        // Compare normalized lengths using wide arithmetic, before any cells
        // are evaluated. Named aggregates may slide to the target's bounds.
        auto length = [&](const std::string& low, const std::string& high) {
            return lengthOf(Value { "", 'l', low, high }, nullptr).name;
        };
        std::string actual = length(first, last);
        std::string targetFirst = context.first;
        std::string targetLast = context.last;
        std::string wanted = length(targetFirst, targetLast);
        std::string same = newTemp();
        line(same + " =w ceql " + actual + ", " + wanted);
        require(same);
    }
    if (type->arrayRank > 1 && aggregate != nullptr) {
        Value rowContext = arrayRow(context, type);
        for (AggregateComponent& component : aggregate->components) {
            Value row = prepareArrayAggregate(component.value.get(), rowContext, type->element, plan);
            if (!row.hasBounds()) {
                return Value {};
            }
            std::vector<std::pair<std::string, std::string>> dimensions { { row.first, row.last } };
            dimensions.insert(dimensions.end(), row.innerBounds.begin(), row.innerBounds.end());
            if (shape.innerBounds.empty()) {
                shape.innerBounds = dimensions;
            } else {
                for (std::size_t i = 0; i < dimensions.size(); ++i) {
                    std::string low = newTemp();
                    std::string high = newTemp();
                    std::string same = newTemp();
                    line(low + " =w ceql " + shape.innerBounds[i].first + ", " + dimensions[i].first);
                    line(high + " =w ceql " + shape.innerBounds[i].second + ", " + dimensions[i].second);
                    line(same + " =w and " + low + ", " + high);
                    require(same);
                }
            }
        }
    }
    plan.shapes[expr] = shape;
    return shape;
}

Value QbeEmitter::emitDynamicAggregateInto(AggregateExpr* expr, const Value& address, Type* type,
                                          ArrayAggregatePlan* plan)
{
    if (type->arrayRank > 1 && plan == nullptr) {
        ArrayAggregatePlan prepared;
        Value shape = prepareArrayAggregate(expr, address, type, prepared);
        if (!shape.hasBounds()) {
            return Value { "0", 'l' };
        }
        return emitDynamicAggregateInto(expr, address, type, &prepared);
    }
    bool inferred = !address.hasBounds();
    if (inferred) {
        for (const AggregateComponent& component : expr->components) {
            if (component.isOthers) {
                m_diagnostics.error(expr->location, "others requires bounds from the aggregate context");
                return Value { "0", 'l', "1", "0" };
            }
        }
    }
    struct Choice
    {
        std::string low;
        std::string high;
        Expr* value;
    };
    std::vector<Choice> choices;
    Expr* others = nullptr;
    bool named = false;
    long long position = 0;
    auto widen = [&](const Value& value) {
        if (value.type == 'l') {
            return value.name;
        }
        std::string result = newTemp();
        line(result + " =l extsw " + value.name);
        return result;
    };
    Value indexBounds = scalarBounds(type->index);
    std::string first = inferred ? widen(Value { indexBounds.first, indexBounds.type })
                                 : widen(Value { address.first, 'l' });
    std::string last = inferred ? first : widen(Value { address.last, 'l' });
    for (AggregateComponent& component : expr->components) {
        if (component.isOthers) {
            others = component.value.get();
        } else if (component.choiceLows.empty()) {
            std::string index = newTemp();
            index = emitIntegerOperation(0, Value { first, 'l' }, Value { std::to_string(position++), 'l' }, 'l').name;
            choices.push_back({ index, index, component.value.get() });
        } else {
            named = true;
            for (std::size_t i = 0; i < component.choiceLows.size(); ++i) {
                Value lowValue = plan != nullptr ? plan->choices.at(component.choiceLows[i].get())
                    : emitExpr(component.choiceLows[i].get());
                emitRangeCheck(lowValue, m_sema.typeTable().scalarBaseType(type->index), component.choiceLows[i]->location);
                std::string low = widen(lowValue);
                std::string high = low;
                if (component.choiceHighs[i]) {
                    Value highValue = plan != nullptr ? plan->choices.at(component.choiceHighs[i].get())
                        : emitExpr(component.choiceHighs[i].get());
                    emitRangeCheck(highValue, m_sema.typeTable().scalarBaseType(type->index), component.choiceHighs[i]->location);
                    high = widen(highValue);
                }
                choices.push_back({ low, high, component.value.get() });
            }
        }
    }
    // Named aggregates without others have their own bounds, then slide to
    // the target. Sema verifies static choices are contiguous and disjoint.
    if (named && others == nullptr && !choices.empty()) {
        first = choices.front().low;
        last = choices.front().high;
        if (choices.size() > 1) {
            long long low = std::numeric_limits<long long>::max();
            long long high = std::numeric_limits<long long>::min();
            for (AggregateComponent& component : expr->components) {
                for (std::size_t i = 0; i < component.choiceLows.size(); ++i) {
                    low = std::min(low, component.choiceLows[i]->staticValue);
                    high = std::max(high, component.choiceHighs[i]
                        ? component.choiceHighs[i]->staticValue : component.choiceLows[i]->staticValue);
                }
            }
            first = std::to_string(low);
            last = std::to_string(high);
        }
    } else if (!named && others == nullptr) {
        last = newTemp();
        last = emitIntegerOperation(0, Value { first, 'l' }, Value { std::to_string(position - 1), 'l' }, 'l').name;
    }
    std::string wideLength;
    if (!inferred) {
        Value length = lengthOf(address, type);
        wideLength = newTemp();
        line(wideLength + " =l copy " + length.name);
    }
    // Check size and explicit choices before evaluating any component value.
    auto require = [&](const std::string& condition) {
        std::string good = newLabel("aggregateok");
        std::string bad = newLabel("aggregatebad");
        branch(Value { condition, 'w' }, good, bad);
        label(bad);
        raiseConstraintError();
        label(good);
    };
    std::string nonNull = newTemp();
    line(nonNull + " =w csgel " + last + ", " + first);
    std::string normalized = lengthOf(Value { "", 'l', first, last }, nullptr).name;
    std::string same = newTemp();
    if (inferred) {
        wideLength = normalized;
    } else {
        line(same + " =w ceql " + normalized + ", " + wideLength);
        require(same);
    }
    // Source bounds must belong to the index subtype before sliding, even
    // when the destination is constrained. Null ranges have no index check.
    emitRangeCheck(Value { first, 'l' }, m_sema.typeTable().scalarBaseType(type->index), expr->location);
    emitRangeCheck(Value { last, 'l' }, m_sema.typeTable().scalarBaseType(type->index), expr->location);
    std::string check = newLabel("aggregatebounds");
    std::string ready = newLabel("aggregateboundsready");
    branch(Value { nonNull, 'w' }, check, ready);
    label(check);
    emitRangeCheck(Value { first, 'l' }, type->index, expr->location);
    emitRangeCheck(Value { last, 'l' }, type->index, expr->location);
    jump(ready);
    label(ready);
    if (others != nullptr) {
        for (const Choice& choice : choices) {
            std::string low = newTemp();
            std::string high = newTemp();
            std::string valid = newTemp();
            line(low + " =w csgel " + choice.low + ", " + first);
            line(high + " =w cslel " + choice.high + ", " + last);
            line(valid + " =w and " + low + ", " + high);
            require(valid);
        }
    }
    std::string resultFirst = inferred ? first : address.first;
    std::string resultLast = inferred ? last : address.last;
    Value resultShape = inferred && plan != nullptr ? plan->shapes.at(expr) : address;
    std::string elementSize = arrayElementSize(resultShape, type);
    std::string buffer = address.name;
    if (buffer.empty() || !needsFinalization(type)) {
        buffer = newTemp();
        emitCallInstruction(buffer + " =l call $__ada_array_local(l " + storageArena(true, true)
             + ", l " + resultFirst + ", l " + resultLast + ", l " + elementSize + ")", CallEffects::CheckException);
        if (needsFinalization(type)) {
            Value object { buffer, 'l', resultFirst, resultLast };
            object.innerBounds = resultShape.innerBounds;
            prepareControlledObject(object, type);
        }
    }
    std::string slot = allocScratch(8);
    line("storel 0, " + slot);
    std::string head = newLabel("aggregatefill");
    std::string body = newLabel("aggregatebody");
    std::string next = newLabel("aggregatenext");
    std::string done = newLabel("aggregatedone");
    label(head);
    std::string offset = newTemp();
    std::string test = newTemp();
    line(offset + " =l loadl " + slot);
    line(test + " =w csltl " + offset + ", " + wideLength);
    branch(Value { test, 'w' }, body, done);
    label(body);
    std::string index = newTemp();
    std::string scaled = newTemp();
    std::string element = newTemp();
    line(index + " =l add " + first + ", " + offset);
    line(scaled + " =l mul " + offset + ", " + elementSize);
    line(element + " =l add " + buffer + ", " + scaled);
    auto fill = [&](Expr* value) {
        auto checkpoint = storageCheckpoint();
        Value cell = arrayRow(resultShape, type);
        cell.name = element;
        if (type->arrayRank > 1 && value->kind == ExprKind::Aggregate) {
            emitDynamicAggregateInto(static_cast<AggregateExpr*>(value), cell, type->element, plan);
        } else {
            assignInto(cell, type->element, value, true);
        }
        rewindStorage(checkpoint);
        jump(next);
    };
    for (const Choice& choice : choices) {
        std::string low = newTemp();
        std::string high = newTemp();
        std::string match = newTemp();
        line(low + " =w csgel " + index + ", " + choice.low);
        line(high + " =w cslel " + index + ", " + choice.high);
        line(match + " =w and " + low + ", " + high);
        std::string selected = newLabel("aggregatechoice");
        std::string following = newLabel("aggregatechoice_next");
        branch(Value { match, 'w' }, selected, following);
        label(selected);
        fill(choice.value);
        label(following);
    }
    if (others != nullptr) {
        fill(others);
    } else {
        raiseConstraintError();
    }
    label(next);
    std::string incremented = newTemp();
    line(incremented + " =l add " + offset + ", 1");
    line("storel " + incremented + ", " + slot);
    jump(head);
    label(done);
    std::string size = newTemp();
    line(size + " =l mul " + wideLength + ", " + elementSize);
    if (!address.name.empty() && !needsFinalization(type)) {
        emitCallInstruction("call $memmove(l " + address.name + ", l " + buffer + ", l " + size + ")", CallEffects::DeferredCheck);
    }
    Value result { buffer, 'l', resultFirst, resultLast };
    result.innerBounds = resultShape.innerBounds;
    return result;
}

void QbeEmitter::emitAggregateInto(AggregateExpr* expr, const Value& address, Type* type)
{
    Type* target = type != nullptr && type->tagged ? rootType(type) : type;
    if (target == nullptr) {
        return;
    }

    if (target->kind == TypeKind::Record) {
        if (expr->m_ancestor != nullptr) {
            if (expr->m_ancestorIsType) {
                emitDefaultInit(address, expr->m_ancestorType);
            } else {
                if (needsFinalization(expr->m_ancestorType)) {
                    assignInto(address, expr->m_ancestorType, expr->m_ancestor.get(), true);
                } else {
                    Value ancestor = emitExpr(expr->m_ancestor.get());
                    copyInto(address, ancestor, expr->m_ancestorType);
                }
            }
        }
        if (target->tagged && !target->controlled) {
            line("storel " + typeTag(target).name + ", " + address.name);
        }
        if (target->controlled && expr->m_ancestor != nullptr && expr->m_ancestorType->controlled) {
            activateControlledObject(address, expr->m_ancestorType);
        }
        for (std::size_t i = 0; i < target->fields.size() && i < expr->resolvedFields.size(); ++i) {
            const FieldInfo& field = target->fields[i];
            Value component = fieldAddress(address, target, field);
            Expr* value = expr->resolvedFields[i];
            if (value != nullptr && value->kind == ExprKind::Box && field.defaultValue != nullptr) {
                value = field.defaultValue;
            }
            assignInto(component, field.type, value, true);
        }
        if (target->controlled) {
            line("storel " + typeTag(target).name + ", " + address.name);
            activateControlledObject(address, target);
        }
        return;
    }

    if (target->kind != TypeKind::Array) {
        m_diagnostics.error(expr->location, "an aggregate requires an array or record type");
        return;
    }

    // Static and dynamic targets share choice validation, sliding, and
    // temporary construction. Never write unchecked source indexes directly
    // into a constrained destination.
    emitDynamicAggregateInto(expr, withBounds(address, target, nullptr), target);
}

Value QbeEmitter::emitAggregate(AggregateExpr* expr)
{
    bool saved = m_context->initializingTemporary;
    m_context->initializingTemporary = true;
    Value result;
    if (isUnconstrainedArray(expr->type)) {
        result = emitDynamicAggregateInto(expr, withBounds(Value {}, expr->type, nullptr), expr->type);
    } else {
        std::string buffer = allocTypeScratch(expr->type);
        result = withBounds(Value { buffer, 'l' }, expr->type, nullptr);
        prepareControlledObject(result, expr->type);
        emitAggregateInto(expr, result, expr->type);
    }
    m_context->initializingTemporary = saved;
    return result;
}
