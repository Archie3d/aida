#include "QbeEmitter.h"
#include "QbeSupport.h"

void QbeEmitter::initializeCollection(Type* type)
{
    if (!needsCollection(type) || type->m_parentType != nullptr || m_initializedCollections.contains(type)) {
        return;
    }
    m_initializedCollections[type] = true;
    std::string slot = type->m_collectionName;
    std::string owner = "0";
    std::string arena = "0";
    if (type->m_collectionOwner != nullptr) {
        type->m_collectionOffset = reserveFrame(8);
        slot = newTemp();
        line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(type->m_collectionOffset));
        owner = m_context->m_finalizationChain;
        arena = storageArena(false, true);
    }
    std::string collection = newTemp();
    emitCallInstruction(collection + " =l call $__ada_collection_create(l " + owner + ", l " + arena + ")", CallEffects::CheckException);
    line("storel " + collection + ", " + slot);
}

Value QbeEmitter::collectionFor(Type* type)
{
    type = rootType(type);
    // Derived access types share the parent's pool and collection lifetime.
    while (type->m_parentType != nullptr) {
        type = type->m_parentType;
    }
    std::string slot = type->m_collectionName;
    if (type->m_collectionOwner != nullptr) {
        Value frame = type->m_collectionOwner == m_context->symbol
            ? Value { m_context->frameTemp, 'l' } : staticLinkFor(type->m_collectionOwner->level);
        slot = newTemp();
        line(slot + " =l add " + frame.name + ", " + std::to_string(type->m_collectionOffset));
    }
    Value collection { newTemp(), 'l' };
    line(collection.name + " =l loadl " + slot);
    return collection;
}

void QbeEmitter::emitControlledResult(Expr* expression, ObjectDecl* object, ReturnStmt* extended)
{
    Type* returnType = m_context->symbol->returnType;
    bool wideExtended = object != nullptr && returnType->m_classRoot != nullptr;
    Type* type = wideExtended ? object->symbols.front()->type : returnType;
    if (type->m_classRoot != nullptr) {
        bool limited = hasLimitedControlledParts(type) || hasLimitedControlledParts(expression->type);
        std::string mark = newTemp();
        std::string arenaMark = newTemp();
        line(mark + " =l loadl %.resultOwner");
        line(arenaMark + " =l loadl %.resultArena");
        std::string failed = newLabel("wideresultfailed");
        std::string ready = newLabel("wideresultready");
        pushHandler(failed);
        if (limited) {
            m_context->m_resultTargetOwner = "%.resultOwner";
            m_context->m_resultTargetArena = "%.resultArena";
        }
        Value source = emitExpr(expression);
        emitCallInstruction("call $__ada_tag_check_accessibility(l " + source.name + ", w "
             + std::to_string(m_context->symbol->m_accessibilityLevel) + ")", CallEffects::CheckException);
        std::string result = source.name;
        if (!limited) {
            result = newTemp();
            emitCallInstruction(result + " =l call $__ada_tagged_owned_copy(l %.resultOwner, l %.resultArena, l " + source.name + ")", CallEffects::CheckException);
        }
        line("storel " + result + ", %.result");
        m_context->handlerLabels.pop_back();
        jump(ready);
        label(failed);
        emitCallInstruction("call $__ada_finalize_to(l %.resultOwner, l " + mark + ")", CallEffects::DeferredCheck);
        emitCallInstruction("call $__ada_array_rewind(l %.resultArena, l " + arenaMark + ")", CallEffects::RuntimeHook);
        if (!m_context->handlerLabels.empty()) {
            jump(m_context->handlerLabels.back());
        } else {
            m_context->usesPropagate = true;
            jump(m_context->propagateLabel);
        }
        label(ready);
        return;
    }
    if (hasLimitedControlledParts(type) && object == nullptr && !isAggregateExpression(expression)) {
        m_context->m_resultDescriptor = "%.result";
        m_context->m_resultProvided = "%.resultProvided";
        m_context->m_resultTargetOwner = "%.resultOwner";
        m_context->m_resultTargetArena = "%.resultArena";
        emitExpr(expression);
        return;
    }
    // Construct in caller-owned storage. Registrations are installed before
    // adjustment, so propagation (including a failing local finalizer) never
    // leaves an unowned result. A caught construction failure discards just
    // this attempt before the function's handler can retry the return.
    std::string ownerMark = newTemp();
    std::string arenaMark = newTemp();
    line(ownerMark + " =l loadl %.resultOwner");
    line(arenaMark + " =l loadl %.resultArena");
    std::string failed = newLabel("resultfailed");
    std::string ready = newLabel("resultready");
    pushHandler(failed);

    Value destination { "%.result", 'l' };
    Value source;
    bool dynamic = QbeSupport::isUnconstrainedArray(type);
    bool constructed = false;
    bool preparedSource = false;
    ArrayAggregatePlan aggregatePlan;
    Expr* aggregate = object != nullptr ? object->initializer.get() : expression;
    while (aggregate != nullptr && aggregate->kind == ExprKind::Qualified) {
        aggregate = static_cast<QualifiedExpr*>(aggregate)->operand.get();
    }
    bool limitedAggregate = dynamic && hasLimitedControlledParts(type)
        && aggregate != nullptr && aggregate->kind == ExprKind::Aggregate;
    if (dynamic) {
        if (object != nullptr) {
            Type* actual = object->symbols.front()->type;
            emitTypeBounds(actual, object->location);
            destination = withBounds(Value {}, actual, nullptr);
            if (!destination.hasBounds() && object->subtype->indexLows.empty()) {
                if (limitedAggregate) {
                    destination = prepareArrayAggregate(aggregate, withBounds(Value {}, type, nullptr), type, aggregatePlan);
                } else if (hasLimitedControlledParts(type)) {
                    m_context->m_resultDescriptor = "%.result";
                    m_context->m_resultProvided = "%.resultProvided";
                    m_context->m_resultTargetOwner = "%.resultOwner";
                    m_context->m_resultTargetArena = "%.resultArena";
                    destination = emitExpr(object->initializer.get());
                    constructed = true;
                } else {
                    source = emitExpr(object->initializer.get());
                    destination = source;
                    preparedSource = true;
                }
            } else if (!destination.hasBounds()) {
                Type* axis = actual;
                for (int dimension = 0; dimension < actual->arrayRank; ++dimension) {
                    Value first = emitExpr(object->subtype->indexLows[dimension].get());
                    Value last = emitExpr(object->subtype->indexHighs[dimension].get());
                    first = wideBound(first);
                    last = wideBound(last);
                    if (dimension == 0) {
                        destination.first = first.name;
                        destination.last = last.name;
                    } else {
                        destination.innerBounds.push_back({ first.name, last.name });
                    }
                    std::string check = newLabel("returnboundscheck");
                    std::string readyBounds = newLabel("returnboundsready");
                    Value nonNull { newTemp(), 'w' };
                    line(nonNull.name + " =w csgel " + last.name + ", " + first.name);
                    branch(nonNull, check, readyBounds);
                    label(check);
                    emitRangeCheck(first, axis->index, object->location);
                    emitRangeCheck(last, axis->index, object->location);
                    jump(readyBounds);
                    label(readyBounds);
                    axis = axis->element;
                }
            }
        } else if (limitedAggregate) {
            destination = prepareArrayAggregate(aggregate, withBounds(Value {}, type, nullptr), type, aggregatePlan);
        } else {
            source = emitExpr(expression);
            source = withBounds(source, expression->type, nullptr);
            destination = source;
        }
        if (type->m_boundsSymbol != nullptr) {
            destination = withBounds(Value {}, type, nullptr);
            checkArrayShape(destination, type, source, expression->type);
        }
    }
    if (!constructed && (dynamic || type->m_tagged)) {
        std::string size = dynamic ? objectBytes(destination, type) : std::to_string(typeSize(type));
        std::string provided;
        std::string allocate;
        std::string allocated;
        if (hasLimitedControlledParts(type) && (type->m_tagged || dynamic)) {
            provided = newLabel("resultprovided");
            allocate = newLabel("resultallocate");
            allocated = newLabel("resultallocated");
            branch(Value { hasLimitedControlledParts(returnType) ? "%.resultProvided" : "0", 'w' }, provided, allocate);
            label(allocate);
        }
        destination.name = newTemp();
        emitCallInstruction(destination.name + " =l call $__ada_array_local(l %.resultArena, l 1, l 1, l " + size + ")", CallEffects::CheckException);
        line("storel " + destination.name + ", %.result");
        if (!provided.empty()) {
            jump(allocated);
            label(provided);
            if (dynamic) {
                Value target = loadArrayView("%.result", type->arrayRank, true);
                checkArrayShape(target, type, destination, type);
            }
            jump(allocated);
            label(allocated);
            destination.name = newTemp();
            line(destination.name + " =l loadl %.result");
        }
    }
    if (dynamic) {
        storeArrayView("%.result", destination, type, true);
    }
    if (!constructed) {
        walkControlled(destination, type, false, [&](const Value& part, Type* partType) {
            emitCallInstruction("call $__ada_finalization_reserve(l %.resultOwner, l %.resultArena, l " + part.name
                 + ", l " + rootType(partType)->m_tagName + ".finalize)", CallEffects::CheckException);
        });
    }
    std::string savedOwner = m_context->m_constructionOwner;
    std::string savedArena = m_context->m_constructionArena;
    m_context->m_constructionOwner = "%.resultOwner";
    m_context->m_constructionArena = "%.resultArena";
    if (object != nullptr) {
        Symbol* symbol = object->symbols.front();
        if (dynamic) {
            m_context->bounds[symbol] = destination;
        }
        if (symbol->isUplevel) {
            symbol->frameOffset = reserveFrame(dynamic ? adaArrayLocalSize(type->arrayRank) : 8);
            std::string slot = newTemp();
            line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->frameOffset));
            line("storel " + destination.name + ", " + slot);
            if (dynamic) {
                storeArrayView(slot, destination, type, false);
            }
        } else {
            std::string slot = allocScratch(8);
            m_context->locals[symbol] = slot;
            line("storel " + destination.name + ", " + slot);
        }
        if (!constructed) {
            if (preparedSource) {
                copyControlledObject(destination, source, type, true);
            } else if (limitedAggregate && object->subtype->indexLows.empty()) {
                emitDynamicAggregateInto(static_cast<AggregateExpr*>(aggregate), destination, type, &aggregatePlan);
            } else if (object->initializer != nullptr) {
                assignInto(destination, type, object->initializer.get(), true);
            } else {
                emitDefaultInit(destination, type);
            }
        }
        m_context->m_constructionOwner = savedOwner;
        m_context->m_constructionArena = savedArena;
        std::string savedReturn = m_context->m_extendedReturnLabel;
        std::string bodyDone = newLabel("returnbodydone");
        m_context->m_transferMasterDepth[bodyDone] = m_context->m_blockMasters.size();
        m_context->m_extendedReturnLabel = bodyDone;
        if (extended->m_handlers.empty()) {
            emitStatements(extended->m_body);
        } else {
            std::string dispatch = newLabel("returnhandler");
            std::string handled = newLabel("returnhandled");
            m_context->handlerStorage[dispatch] = storageCheckpoint();
            pushHandler(dispatch);
            emitStatements(extended->m_body);
            m_context->handlerLabels.pop_back();
            jump(handled);
            emitHandlers(extended->m_handlers, handled, dispatch);
            label(handled);
        }
        label(bodyDone);
        m_context->m_extendedReturnLabel = savedReturn;
    } else if (limitedAggregate) {
        emitDynamicAggregateInto(static_cast<AggregateExpr*>(aggregate), destination, type, &aggregatePlan);
    } else if (dynamic) {
        copyControlledObject(destination, source, type, true);
    } else {
        assignInto(destination, type, expression, true);
    }
    if (wideExtended) {
        emitCallInstruction("call $__ada_tag_check_accessibility(l " + destination.name + ", w "
             + std::to_string(m_context->symbol->m_accessibilityLevel) + ")", CallEffects::CheckException);
    }
    m_context->m_constructionOwner = savedOwner;
    m_context->m_constructionArena = savedArena;
    m_context->handlerLabels.pop_back();
    jump(ready);
    label(failed);
    emitCallInstruction("call $__ada_finalize_to(l %.resultOwner, l " + ownerMark + ")", CallEffects::DeferredCheck);
    emitCallInstruction("call $__ada_array_rewind(l %.resultArena, l " + arenaMark + ")", CallEffects::RuntimeHook);
    if (!m_context->handlerLabels.empty()) {
        jump(m_context->handlerLabels.back());
    } else {
        m_context->usesPropagate = true;
        jump(m_context->propagateLabel);
    }
    label(ready);
}

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
    if (type->m_protectedScope != nullptr) {
        action(object, type);
        return;
    }
    if (parentFirst && type->m_controlled) {
        action(object, type);
    }
    if (type->kind == TypeKind::Array) {
        Value array = withBounds(object, type, nullptr);
        Value length = lengthOf(array, type);
        std::string count = newTemp();
        line(count + " =l copy " + length.name);
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

void QbeEmitter::prepareControlledObject(const Value& object, Type* type, bool library)
{
    walkControlled(object, type, false, [&](const Value& part, Type* partType) {
        if (library) {
            emitCallInstruction("call $__ada_library_reserve(l " + part.name + ", l " + rootType(partType)->m_tagName + ".finalize)", CallEffects::CheckException);
            return;
        }
        bool temporary = m_context->m_initializingTemporary;
        std::string owner = temporary ? m_context->m_temporaryFinalizationChain : m_context->m_finalizationChain;
        emitCallInstruction("call $__ada_finalization_reserve(l " + owner + ", l " + storageArena(temporary, true)
             + ", l " + part.name + ", l " + rootType(partType)->m_tagName + ".finalize)", CallEffects::CheckException);
    });
}

void QbeEmitter::activateControlledObject(const Value& object, Type* type)
{
    if (type->m_controlled || type->m_protectedScope != nullptr) {
        emitCallInstruction("call $__ada_controlled_activate(l " + object.name + ", l " + rootType(type)->m_tagName + ".finalize)", CallEffects::CheckException);
    }
}

void QbeEmitter::adjustControlledObject(const Value& object, Type* type, const std::string& viewTag)
{
    std::string failure = allocScratch(4);
    line("storew 0, " + failure);
    walkControlled(object, type, false, [&](const Value& part, Type* partType) {
        std::string failed = newTemp();
        std::string previous = newTemp();
        std::string combined = newTemp();
        if (!viewTag.empty() && part.name == object.name && partType == type) {
            emitCallInstruction(failed + " =w call $__ada_controlled_adjust_view(l " + part.name + ", l "
                 + rootType(partType)->m_tagName + ".view_adjust, l " + viewTag + ")", CallEffects::DeferredCheck);
        } else {
            emitCallInstruction(failed + " =w call $__ada_controlled_adjust(l " + part.name + ", l "
                 + rootType(partType)->m_tagName + ".adjust)", CallEffects::DeferredCheck);
        }
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
    emitCallInstruction("call $__ada_raise(l $__ada_exc_program_error)", CallEffects::CheckException);
    jump(good);
    label(good);
}

void QbeEmitter::finalizeControlledObject(const Value& object, Type* type, const std::string& viewTag)
{
    std::string failure = allocScratch(4);
    line("storew 0, " + failure);
    walkControlled(object, type, true, [&](const Value& part, Type* partType) {
        std::string failed = newTemp();
        std::string previous = newTemp();
        std::string combined = newTemp();
        if (!viewTag.empty() && part.name == object.name && partType == type) {
            emitCallInstruction(failed + " =w call $__ada_controlled_finalize_view(l " + part.name + ", l "
                 + rootType(partType)->m_tagName + ".view_finalize, l " + viewTag + ")", CallEffects::DeferredCheck);
        } else {
            emitCallInstruction(failed + " =w call $__ada_controlled_finalize(l " + part.name + ")", CallEffects::DeferredCheck);
        }
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
    emitCallInstruction("call $__ada_raise(l $__ada_exc_program_error)", CallEffects::CheckException);
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
    emitCallInstruction(size + " =l call $__ada_array_size(l " + array.first + ", l " + array.last + ", l " + elementSize + ")", CallEffects::CheckException);
    return size;
}

void QbeEmitter::copyControlledObject(const Value& target, const Value& source, Type* type, bool initialize)
{
    std::string viewTag = type->m_tagged ? typeTag(type).name : "";
    std::string size = objectBytes(target, type);
    auto copy = [&](const Value& to, const Value& from) {
        if (type->m_tagged) {
            copyInto(to, from, type);
        } else {
            emitCallInstruction("call $memmove(l " + to.name + ", l " + from.name + ", l " + size + ")", CallEffects::DeferredCheck);
        }
        adjustControlledObject(to, type, viewTag);
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
    emitCallInstruction(snapshot.name + " =l call $__ada_array_local(l " + storageArena(true, true) + ", l 1, l 1, l " + size + ")", CallEffects::CheckException);
    bool saved = m_context->m_initializingTemporary;
    m_context->m_initializingTemporary = true;
    prepareControlledObject(snapshot, type);
    m_context->m_initializingTemporary = saved;
    if (type->m_tagged) {
        line("storel " + typeTag(type).name + ", " + snapshot.name);
    }
    copy(snapshot, source);
    finalizeControlledObject(target, type, viewTag);
    copy(target, snapshot);
    jump(done);
    label(done);
}

// Concrete tag descriptors expose their controlled parts without depending on
// the caller's static view. Hook thunks recover lexical links from object tags.
void QbeEmitter::emitControlledParts(Type* type)
{
    FunctionContext context;
    context.traceName = type->name + " controlled parts";
    context.propagateLabel = newLabel("partspropagate");
    FunctionContext* saved = m_context;
    m_context = &context;
    auto visit = [&](const Value& part, Type* partType) {
        Type* root = rootType(partType);
        emitCallInstruction("call %visit(l " + part.name + ", l " + root->m_tagName + ".finalize, l "
             + (root->isLimited ? "0" : root->m_tagName + ".adjust") + ", l %context)", CallEffects::CheckException);
    };
    std::string parent = newLabel("partsparent");
    std::string child = newLabel("partschild");
    branch(Value { "%parentFirst", 'w' }, parent, child);
    label(parent);
    walkControlled(Value { "%object", 'l' }, type, true, visit);
    emitReturn();
    context.terminated = true;
    label(child);
    walkControlled(Value { "%object", 'l' }, type, false, visit);
    emitReturn();
    context.terminated = true;
    finishFunction("export function " + type->m_tagName
                   + ".parts(l %object, l %visit, l %context, w %parentFirst)");
    m_context = saved;
}
