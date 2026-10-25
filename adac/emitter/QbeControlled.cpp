#include "QbeEmitter.h"
#include "QbeSupport.h"

void QbeEmitter::initializeCollection(Type* type)
{
    if (!needsCollection(type) || type->accessParameter || type->accessDiscriminant || type->parentType != nullptr || m_initializedCollections.contains(type)) {
        return;
    }
    m_initializedCollections[type] = true;
    std::string slot = type->collectionName;
    std::string owner = "0";
    std::string arena = "0";
    if (type->collectionOwner != nullptr) {
        type->collectionOffset = reserveFrame(8);
        slot = newTemp();
        line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(type->collectionOffset));
        owner = m_context->finalizationChain;
        arena = storageArena(false, true);
    }
    std::string collection = newTemp();
    emitCallInstruction(collection + " =l call $__ada_collection_create(l " + owner + ", l " + arena + ")", CallEffects::CheckException);
    line("storel " + collection + ", " + slot);
}

Value QbeEmitter::collectionFor(Type* type)
{
    type = rootType(type);
    if (type->anonymousAccess && (type->accessParameter || type->accessDiscriminant)) {
        Value collection {newTemp(), 'l'};
        std::string owner = type->accessParameter ? m_context->temporaryFinalizationChain
            : !m_context->constructionOwner.empty() ? m_context->constructionOwner : m_context->finalizationChain;
        std::string arena = type->accessParameter ? storageArena(true, true)
            : !m_context->constructionArena.empty() ? m_context->constructionArena : storageArena(false, true);
        emitCallInstruction(collection.name + " =l call $__ada_collection_create(l " + owner
            + ", l " + arena + ")", CallEffects::CheckException);
        if (type->accessDiscriminant && !m_context->coextensionLevel.empty()) {
            emitCallInstruction("call $__ada_collection_set_access_level(l " + collection.name
                + ", w " + m_context->coextensionLevel + ")", CallEffects::CheckException);
        }
        return collection;
    }
    // Derived access types share the parent's pool and collection lifetime.
    while (type->parentType != nullptr) {
        type = type->parentType;
    }
    std::string slot = type->collectionName;
    if (type->collectionOwner != nullptr) {
        Value frame = type->collectionOwner == m_context->symbol
            ? Value { m_context->frameTemp, 'l' } : staticLinkFor(type->collectionOwner->level);
        slot = newTemp();
        line(slot + " =l add " + frame.name + ", " + std::to_string(type->collectionOffset));
    }
    Value collection { newTemp(), 'l' };
    line(collection.name + " =l loadl " + slot);
    return collection;
}

void QbeEmitter::emitControlledResult(Expr* expression, ObjectDecl* object, ReturnStmt* extended)
{
    Type* returnType = m_context->symbol->returnType;
    bool wideExtended = object != nullptr && returnType->classRoot != nullptr;
    Type* type = object != nullptr && (wideExtended || returnType->kind == TypeKind::Record)
        ? object->symbols.front()->type : returnType;
    if (type->classRoot != nullptr) {
        if (object != nullptr) { expression = object->initializer.get(); }
        bool limited = hasLimitedParts(type) || hasLimitedParts(expression->type);
        std::string mark = newTemp();
        std::string arenaMark = newTemp();
        line(mark + " =l loadl %.resultOwner");
        line(arenaMark + " =l loadl %.resultArena");
        std::string failed = newLabel("wideresultfailed");
        std::string ready = newLabel("wideresultready");
        pushHandler(failed);
        Value source;
        if (limited && isAggregateExpression(expression)) {
            source = Value { newTemp(), 'l' };
            emitCallInstruction(source.name + " =l call $__ada_array_local(l %.resultArena, l 1, l 1, l "
                + std::to_string(typeSize(expression->type)) + ")", CallEffects::CheckException);
            std::string savedOwner = m_context->constructionOwner, savedArena = m_context->constructionArena;
            m_context->constructionOwner = "%.resultOwner";
            m_context->constructionArena = "%.resultArena";
            prepareControlledObject(source, expression->type);
            assignInto(source, expression->type, expression, true);
            m_context->constructionOwner = savedOwner;
            m_context->constructionArena = savedArena;
        } else {
            if (limited) {
                m_context->resultTargetOwner = "%.resultOwner";
                m_context->resultTargetArena = "%.resultArena";
            }
            source = emitExpr(expression);
        }
        emitRecordConstraintCheck(source, returnType);
        emitCallInstruction("call $__ada_tag_check_accessibility(l " + source.name + ", w "
             + std::to_string(m_context->symbol->accessibilityLevel) + ")", CallEffects::CheckException);
        std::string result = source.name;
        if (!limited) {
            result = newTemp();
            emitCallInstruction(result + " =l call $__ada_tagged_owned_copy(l %.resultOwner, l %.resultArena, l " + source.name + ")", CallEffects::CheckException);
        }
        line("storel " + result + ", %.result");
        if (object != nullptr) {
            Symbol* symbol = object->symbols.front();
            std::string slot;
            if (symbol->isUplevel) {
                symbol->frameOffset = reserveFrame(8);
                slot = newTemp();
                line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->frameOffset));
            } else {
                slot = allocScratch(8);
                m_context->locals[symbol] = slot;
            }
            line("storel " + result + ", " + slot);
            m_context->returnCleanups.push_back({ mark, arenaMark,
                m_context->blockMasters.size(), m_context->handlerLabels.size() - 1 });
            emitExtendedReturnBody(extended);
            m_context->returnCleanups.pop_back();
        }
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
    if (hasLimitedParts(type) && object == nullptr && !isAggregateExpression(expression)) {
        m_context->resultDescriptor = "%.result";
        m_context->resultProvided = "%.resultProvided";
        m_context->resultTargetOwner = "%.resultOwner";
        m_context->resultTargetArena = "%.resultArena";
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

    if (object != nullptr) {
        for (Type* constraint : object->constraintOrder) {
            if (constraint->kind == TypeKind::Record) { emitDiscriminantConstraint(constraint); }
        }
    }
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
    bool limitedAggregate = dynamic && hasLimitedParts(type)
        && aggregate != nullptr && aggregate->kind == ExprKind::Aggregate;
    if (dynamic) {
        if (object != nullptr) {
            Type* actual = object->symbols.front()->type;
            emitTypeBounds(actual, object->location);
            destination = withBounds(Value {}, actual, nullptr);
            if (!destination.hasBounds() && object->subtype->indexLows.empty()) {
                if (limitedAggregate) {
                    destination = prepareArrayAggregate(aggregate, withBounds(Value {}, type, nullptr), type, aggregatePlan);
                } else if (hasLimitedParts(type)) {
                    m_context->resultDescriptor = "%.result";
                    m_context->resultProvided = "%.resultProvided";
                    m_context->resultTargetOwner = "%.resultOwner";
                    m_context->resultTargetArena = "%.resultArena";
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
        if (type->boundsSymbol != nullptr) {
            destination = withBounds(Value {}, type, nullptr);
            checkArrayShape(destination, type, source, expression->type);
        }
    }
    if (!constructed && (dynamic || type->tagged)) {
        std::string size = dynamic ? objectBytes(destination, type) : std::to_string(typeSize(type));
        std::string provided;
        std::string allocate;
        std::string allocated;
        if (hasLimitedParts(type) && (type->tagged || dynamic)) {
            provided = newLabel("resultprovided");
            allocate = newLabel("resultallocate");
            allocated = newLabel("resultallocated");
            branch(Value { hasLimitedParts(returnType) ? "%.resultProvided" : "0", 'w' }, provided, allocate);
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
        initializeDependentStorage(destination, type);
        walkControlled(destination, type, false, [&](const Value& part, Type* partType) {
            if (partType->dependentStorage) { line("storel 0, " + dependentArena(part, partType)); }
            emitCallInstruction("call $__ada_finalization_reserve(l %.resultOwner, l %.resultArena, l " + part.name
                 + ", l " + rootType(partType)->tagName + ".finalize)", CallEffects::CheckException);
        }, false);
    }
    std::string savedOwner = m_context->constructionOwner;
    std::string savedArena = m_context->constructionArena;
    m_context->constructionOwner = "%.resultOwner";
    m_context->constructionArena = "%.resultArena";
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
        m_context->constructionOwner = savedOwner;
        m_context->constructionArena = savedArena;
        m_context->returnCleanups.push_back({ ownerMark, arenaMark,
            m_context->blockMasters.size(), m_context->handlerLabels.size() - 1 });
        emitExtendedReturnBody(extended);
        m_context->returnCleanups.pop_back();
    } else if (limitedAggregate) {
        emitDynamicAggregateInto(static_cast<AggregateExpr*>(aggregate), destination, type, &aggregatePlan);
    } else if (dynamic) {
        copyControlledObject(destination, source, type, true);
    } else {
        assignInto(destination, type, expression, true);
    }
    if (wideExtended) {
        emitCallInstruction("call $__ada_tag_check_accessibility(l " + destination.name + ", w "
             + std::to_string(m_context->symbol->accessibilityLevel) + ")", CallEffects::CheckException);
    }
    m_context->constructionOwner = savedOwner;
    m_context->constructionArena = savedArena;
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
    m_context->finalizationChain = allocScratch(8);
    m_context->temporaryFinalizationChain = allocScratch(8);
    m_context->prologue << "    storel 0, " << m_context->finalizationChain << "\n";
    m_context->prologue << "    storel 0, " << m_context->temporaryFinalizationChain << "\n";
}

// Controlled nodes and records owning component storage need registrations;
// ordinary composites borrow their components' records. Postorder registration
// gives parent-before-child cleanup. Reservation walks include every possible
// arm without reading discriminants; operations on live objects select active arms.
void QbeEmitter::walkControlled(const Value& object, Type* type, bool parentFirst,
                                const std::function<void(const Value&, Type*)>& action, bool activeOnly)
{
    if (!needsFinalization(type)) {
        return;
    }
    if (m_context->initializingStorage && type->tagged) {
        line("storel " + typeTag(type).name + ", " + object.name);
    }
    if (type->protectedScope != nullptr) {
        action(object, type);
        return;
    }
    if (parentFirst && (type->controlled || type->dependentStorage)) {
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
        walkControlled(cell, type->element, parentFirst, action, activeOnly);
        std::string next = newTemp();
        line(next + " =l add " + index + ", 1");
        line("storel " + next + ", " + cursor);
        jump(head);
        label(done);
    } else if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : baseType(type)->fields) {
            if (field.storageType != nullptr || needsFinalization(field.type)) {
                std::string done;
                if (activeOnly && field.variant >= 0) {
                    std::string active = newLabel("controlledvariant");
                    done = newLabel("controlledvariantdone");
                    branch(variantCondition(object, baseType(type), field.variant), active, done);
                    label(active);
                }
                Value component = field.storageType != nullptr ? fieldStorage(object, baseType(type), field)
                    : fieldAddress(object, baseType(type), field);
                walkControlled(component, field.storageType != nullptr ? field.storageType : field.type, parentFirst, action, activeOnly);
                if (!done.empty()) { jump(done); label(done); }
            }
        }
    }
    if (!parentFirst && (type->controlled || type->dependentStorage)) {
        action(object, type);
    }
}

void QbeEmitter::prepareControlledObject(const Value& object, Type* type, bool library)
{
    initializeDependentStorage(object, type);
    walkControlled(object, type, false, [&](const Value& part, Type* partType) {
        if (partType->dependentStorage) { line("storel 0, " + dependentArena(part, partType)); }
        if (library) {
            emitCallInstruction("call $__ada_library_reserve(l " + part.name + ", l " + rootType(partType)->tagName + ".finalize)", CallEffects::CheckException);
            return;
        }
        bool temporary = m_context->initializingTemporary;
        std::string owner = temporary ? m_context->temporaryFinalizationChain : m_context->finalizationChain;
        emitCallInstruction("call $__ada_finalization_reserve(l " + owner + ", l " + storageArena(temporary, true)
             + ", l " + part.name + ", l " + rootType(partType)->tagName + ".finalize)", CallEffects::CheckException);
    }, false);
}

void QbeEmitter::activateControlledObject(const Value& object, Type* type)
{
    if (type->controlled || type->dependentStorage || type->protectedScope != nullptr) {
        emitCallInstruction("call $__ada_controlled_activate(l " + object.name + ", l " + rootType(type)->tagName + ".finalize)", CallEffects::CheckException);
    }
}

void QbeEmitter::adjustControlledObject(const Value& object, Type* type, const std::string& viewTag)
{
    std::string failure = allocScratch(4);
    line("storew 0, " + failure);
    walkControlled(object, type, false, [&](const Value& part, Type* partType) {
        if (partType->dependentStorage) {
            line("storel 0, " + dependentArena(part, partType));
            activateControlledObject(part, partType);
            emitCallInstruction("call " + rootType(partType)->tagName + ".adjust(l " + part.name + ")", CallEffects::CheckException);
            return;
        }
        // Variant alternatives can reuse an address with a different hook.
        // Select the current component's finalizer before adjustment.
        if (viewTag.empty() || part.name != object.name || partType != type) {
            activateControlledObject(part, partType);
        }
        std::string failed = newTemp();
        std::string previous = newTemp();
        std::string combined = newTemp();
        if (!viewTag.empty() && part.name == object.name && partType == type) {
            emitCallInstruction(failed + " =w call $__ada_controlled_adjust_view(l " + part.name + ", l "
                 + rootType(partType)->tagName + ".view_adjust, l " + viewTag + ")", CallEffects::DeferredCheck);
        } else {
            emitCallInstruction(failed + " =w call $__ada_controlled_adjust(l " + part.name + ", l "
                 + rootType(partType)->tagName + ".adjust)", CallEffects::DeferredCheck);
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
                 + rootType(partType)->tagName + ".view_finalize, l " + viewTag + ")", CallEffects::DeferredCheck);
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
        return runtimeSize(type);
    }
    Value array = withBounds(object, type, nullptr);
    std::string size = newTemp();
    std::string elementSize = arrayElementSize(array, type);
    emitCallInstruction(size + " =l call $__ada_array_size(l " + array.first + ", l " + array.last + ", l " + elementSize + ")", CallEffects::CheckException);
    return size;
}

void QbeEmitter::copyControlledObject(const Value& target, const Value& source, Type* type, bool initialize)
{
    std::string viewTag = type->tagged ? typeTag(type).name : "";
    std::string size = objectBytes(target, type);
    auto copy = [&](const Value& to, const Value& from) {
        if (type->tagged) {
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
    // RM 7.6(21): disjoint objects can be assigned without an anonymous
    // adjusted copy. In particular, a failed Adjust then completes the
    // remaining adjustments on the actual target (RM 7.6.1(16)). Compare
    // unsigned distances so neither address-end calculation can overflow.
    std::string forward = newTemp(), backward = newTemp();
    std::string insideForward = newTemp(), insideBackward = newTemp();
    Value overlap { newTemp(), 'w' };
    line(forward + " =l sub " + target.name + ", " + source.name);
    line(backward + " =l sub " + source.name + ", " + target.name);
    line(insideForward + " =w cultl " + forward + ", " + size);
    line(insideBackward + " =w cultl " + backward + ", " + size);
    line(overlap.name + " =w or " + insideForward + ", " + insideBackward);
    std::string snapshotCopy = newLabel("overlappingassign");
    std::string directCopy = newLabel("disjointassign");
    branch(overlap, snapshotCopy, directCopy);
    label(directCopy);
    finalizeControlledObject(target, type, viewTag);
    copy(target, source);
    jump(done);
    label(snapshotCopy);
    // Use the RM's anonymous adjusted copy. It protects aliases, overlapping
    // slices, and an RHS whose resources are shared with the old target.
    Value snapshot = withBounds(target, type, nullptr);
    snapshot.name = newTemp();
    emitCallInstruction(snapshot.name + " =l call $__ada_array_local(l " + storageArena(true, true) + ", l 1, l 1, l " + size + ")", CallEffects::CheckException);
    bool saved = m_context->initializingTemporary;
    m_context->initializingTemporary = true;
    prepareControlledObject(snapshot, type);
    m_context->initializingTemporary = saved;
    if (type->tagged) {
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
        emitCallInstruction("call %visit(l " + part.name + ", l " + root->tagName + ".finalize, l "
             + (root->isLimited ? "0" : root->tagName + ".adjust") + ", l %context)", CallEffects::CheckException);
    };
    std::string reserve = newLabel("partsreserve");
    std::string active = newLabel("partsactive");
    std::string reserving = newTemp();
    line(reserving + " =w ceqw %parentFirst, 2");
    branch(Value { reserving, 'w' }, reserve, active);
    label(reserve);
    walkControlled(Value { "%object", 'l' }, type, false, visit, false);
    emitReturn();
    context.terminated = true;
    label(active);
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
    finishFunction("export function " + type->tagName
                   + ".parts(l %object, l %visit, l %context, w %parentFirst)");
    m_context = saved;
}
