#include "QbeEmitter.h"
#include "QbeSupport.h"

using QbeSupport::isUnconstrainedArray;
using QbeSupport::comparisonInstruction;

void QbeEmitter::emitStatements(StmtList& statements)
{
    // Capture the sequence entry state, which dominates forward and backward
    // transfers. A checkpoint at the target itself would not dominate a goto.
    bool hasLabels = std::any_of(statements.begin(), statements.end(), [](const StmtPtr& statement) {
        return !statement->labels.empty();
    });
    if (hasLabels) {
        auto checkpoint = storageCheckpoint();
        for (const StmtPtr& statement : statements) {
            if (!statement->labels.empty()) {
                m_context->gotoTargets[statement.get()] = { newLabel("adalabel"), checkpoint,
                    m_context->blockMasters.size(), m_context->handlerLabels.size(),
                    m_context->returnCleanups.size() };
                m_context->transferOperationDepth[m_context->gotoTargets[statement.get()].label] = m_context->operations.size();
            }
        }
    }
    for (const StmtPtr& statement : statements) {
        if (!statement->labels.empty()) {
            label(m_context->gotoTargets.at(statement.get()).label);
        }
        SourceLocation saved = m_context->sourceLocation;
        m_context->sourceLocation = statement->location;
        auto checkpoint = storageCheckpoint();
        bool completionPoint = statement->kind == StmtKind::Select || statement->kind == StmtKind::Accept
            || statement->kind == StmtKind::Delay || statement->kind == StmtKind::ProcedureCall
            || statement->kind == StmtKind::Abort;
        // Sequences include loop iterations and handler bodies. Labeled
        // statements also poll so a backward goto cannot starve a transfer.
        if (&statement == &statements.front() || !statement->labels.empty() || completionPoint) { emitAsyncPoll(); }
        emitStatement(statement.get());
        if (!m_context->terminated && completionPoint) { emitAsyncPoll(); }
        if (!m_context->terminated) {
            rewindStorage(checkpoint);
        }
        m_context->sourceLocation = saved;
    }
}

void QbeEmitter::emitStatement(Stmt* statement)
{
    ScopedCheckPolicy checks(m_suppressedChecks, statement->suppressedChecks);
    switch (statement->kind) {
    case StmtKind::Requeue: {
        auto* requeue = static_cast<RequeueStmt*>(statement);
        FunctionContext::SelectCallPlan plan;
        plan.target = requeue->call->subprogram;
        plan.requeue = true;
        plan.withAbort = requeue->withAbort;
        plan.internal = requeue->internal;
        auto* saved = m_context->selectCall;
        m_context->selectCall = &plan;
        emitCall(requeue->call.get());
        m_context->selectCall = saved;
        auto accepted = std::find_if(m_context->operations.rbegin(), m_context->operations.rend(),
            [](const auto& operation) { return operation.asynchronous == nullptr; });
        if (accepted != m_context->operations.rend()) { jump(accepted->returnLabel); }
        else { emitReturn(); }
        break;
    }
    case StmtKind::Abort: {
        auto* abort = static_cast<AbortStmt*>(statement);
        std::string targets = allocScratch(abort->names.size() * 8);
        for (std::size_t i = 0; i < abort->names.size(); ++i) {
            Value identity;
            if (abort->receivers[i] != nullptr) { identity = loadFrom(addressOf(abort->receivers[i]), abort->receivers[i]->type); }
            else {
                Value object = emitExpr(abort->names[i].get());
                identity = Value {newTemp(), 'l'};
                emitCallInstruction(identity.name + " =l call $__ada_task_identity(l " + object.name + ")", CallEffects::CheckException);
            }
            std::string slot = newTemp();
            line(slot + " =l add " + targets + ", " + std::to_string(i * 8));
            line("storel " + identity.name + ", " + slot);
        }
        emitCallInstruction("call $__ada_abort_tasks(l %.taskContext, l " + targets + ", w "
            + std::to_string(abort->names.size()) + ")", CallEffects::CheckException);
        break;
    }
    case StmtKind::Select:
        emitSelect(static_cast<SelectStmt*>(statement));
        break;
    case StmtKind::Terminate:
        emitReturn();
        break;
    case StmtKind::Accept: {
        auto* accept = static_cast<AcceptStmt*>(statement);
        Value index {"0", 'l'};
        bool selected = m_context->selectedAccept == accept;
        if (accept->index != nullptr && !selected) {
            index = emitExpr(accept->index.get());
            emitRangeCheck(index, accept->entry->entryIndexType, accept->index->location);
            index = wideBound(index);
        }
        std::string request = selected ? std::exchange(m_context->selectedRequest, "") : newTemp();
        std::string packet = newTemp();
        m_context->selectedAccept = nullptr;
        if (!selected) {
            emitCallInstruction(request + " =l call $__ada_task_entry_accept(l %.taskContext, w "
                + std::to_string(accept->entry->taskEntryId) + ", l " + index.name + ")", CallEffects::CheckException);
        }
        emitCallInstruction(packet + " =l call $__ada_task_entry_arguments(l " + request + ")", CallEffects::RuntimeHook);
        m_context->hasFrame = true;
        accept->callerIdentity->frameOffset = reserveFrame(8);
        std::string caller = newTemp(), callerSlot = newTemp();
        emitCallInstruction(caller + " =l call $__ada_task_entry_caller(l %.taskContext)", CallEffects::RuntimeHook);
        line(callerSlot + " =l add " + m_context->frameTemp + ", " + std::to_string(accept->callerIdentity->frameOffset));
        line("storel " + caller + ", " + callerSlot);
        std::size_t position = 0;
        auto incoming = [&](char type) {
            std::string slot = newTemp(), value = newTemp();
            line(slot + " =l add " + packet + ", " + std::to_string(position++ * 8));
            line(value + " =" + std::string(1, type) + " load" + std::string(1, type) + " " + slot);
            return Value {value, type};
        };
        for (Symbol* parameter : accept->parameters) {
            Type* type = parameter->type;
            bool array = QbeSupport::isUnconstrainedArray(type);
            parameter->frameOffset = reserveFrame(array ? adaArrayLocalSize(type->arrayRank)
                : parameter->byReference ? 8 : typeSize(type), parameter->byReference ? 8 : typeAlignment(type));
            std::string slot = newTemp();
            line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(parameter->frameOffset));
            Value value = incoming(parameter->byReference ? 'l' : qbeClass(type));
            if (parameter->byReference) { line("storel " + value.name + ", " + slot); }
            else { storeInto(Value {slot, 'l'}, value, type); }
            if (needsConstrainedParameter(parameter)) {
                parameter->constrainedOffset = reserveFrame(4, 4);
                Value constrained = incoming('w');
                std::string target = newTemp();
                line(target + " =l add " + m_context->frameTemp + ", " + std::to_string(parameter->constrainedOffset));
                line("storew " + constrained.name + ", " + target);
            }
            if (array) {
                for (int dimension = 0; dimension < type->arrayRank; ++dimension) {
                    Value first = incoming('l'), last = incoming('l');
                    if (dimension == 0) { value.first = first.name; value.last = last.name; }
                    else { value.innerBounds.push_back({first.name, last.name}); }
                    value.lengths.push_back(incoming('l').name);
                    value.strides.push_back(incoming('l').name);
                }
                storeArrayView(slot, value, type, false);
            }
        }
        std::string failed = newLabel("acceptfailed"), done = newLabel("acceptdone"), ready = newLabel("acceptready");
        m_context->operations.push_back({request, done, m_context->blockMasters.size()});
        m_context->transferOperationDepth[done] = m_context->operations.size();
        m_context->transferMasterDepth[done] = m_context->blockMasters.size();
        pushHandler(failed);
        if (accept->handlers.empty()) { emitStatements(accept->body); }
        else {
            std::string dispatch = newLabel("accepthandler"), after = newLabel("accepthandled");
            m_context->handlerStorage[dispatch] = storageCheckpoint();
            pushHandler(dispatch);
            emitStatements(accept->body);
            m_context->handlerLabels.pop_back();
            jump(after);
            emitHandlers(accept->handlers, after, dispatch);
            label(after);
        }
        jump(done);
        m_context->handlerLabels.pop_back();
        m_context->operations.pop_back();
        label(failed);
        emitCallInstruction("call $__ada_task_entry_complete(l %.taskContext, l " + request + ")", CallEffects::DeferredCheck);
        if (!m_context->handlerLabels.empty()) { jump(m_context->handlerLabels.back()); }
        else { m_context->usesPropagate = true; jump(m_context->propagateLabel); }
        label(done);
        emitCallInstruction("call $__ada_task_entry_complete(l %.taskContext, l " + request + ")", CallEffects::CheckException);
        jump(ready);
        label(ready);
        break;
    }
    case StmtKind::Pragma:
        emitPragma(static_cast<PragmaStmt*>(statement)->pragma.get());
        break;
    case StmtKind::Goto: {
        auto* transfer = static_cast<GotoStmt*>(statement);
        const auto target = m_context->gotoTargets.at(transfer->target);
        auto operations = m_context->operations;
        auto masters = m_context->blockMasters;
        auto handlers = m_context->handlerLabels;
        // An abandoned extended return owns registrations in the caller's
        // result chain. Unwind its inner blocks before discarding that result.
        for (std::size_t i = m_context->returnCleanups.size(); i > target.returnDepth; --i) {
            const auto& result = m_context->returnCleanups[i - 1];
            emitOperationExits(result.operationDepth);
            emitBlockExits(result.masterDepth);
            m_context->blockMasters.resize(result.masterDepth);
            m_context->handlerLabels.resize(result.handlerDepth);
            if (!result.taskMark.empty()) {
                emitCallInstruction("call $__ada_task_result_discard(l %.resultMaster, l " + result.taskMark + ", l %.resultGroup)", CallEffects::RuntimeHook);
            }
            emitCallInstruction("call $__ada_finalize_to(l %.resultOwner, l " + result.ownerMark + ")", CallEffects::DeferredCheck);
            emitCallInstruction("call $__ada_array_rewind(l %.resultArena, l " + result.arenaMark + ")", CallEffects::RuntimeHook);
            emitExceptionCheck();
        }
        // Operation exits own their abortable-part masters.
        emitOperationExits(m_context->transferOperationDepth.at(target.label));
        emitBlockExits(target.masterDepth);
        m_context->blockMasters.resize(target.masterDepth);
        m_context->handlerLabels.resize(target.handlerDepth);
        rewindStorage(target.storage);
        jump(target.label);
        m_context->operations = std::move(operations);
        m_context->blockMasters = std::move(masters);
        m_context->handlerLabels = std::move(handlers);
        break;
    }
    case StmtKind::Delay: {
        auto* delay = static_cast<DelayStmt*>(statement);
        Value value = emitExpr(delay->expression.get());
        emitCallInstruction(std::string("call $__ada_") + (delay->calendar ? "calendar_delay_until" : delay->until ? "delay_until" : "delay_relative")
            + "(l %.taskContext, l " + value.name + ")", CallEffects::CheckException);
        break;
    }
    case StmtKind::Null:
        break;

    case StmtKind::Assign: {
        auto* assign = static_cast<AssignStmt*>(statement);
        bool deferred = needsFinalization(assign->target->type);
        std::string failed, done;
        if (deferred) {
            failed = newLabel("assignmentfailed");
            done = newLabel("assignmentdone");
            emitCallInstruction("call $__ada_abort_defer(l %.taskContext)", CallEffects::RuntimeHook, MasterEffects::Independent);
            pushHandler(failed);
        }
        Value address = emitAddress(assign->target.get());
        std::string constrained = assign->target->type->kind == TypeKind::Record
            ? recordConstrained(assign->target.get(), &address).name : "1";
        assignInto(address, assign->target->type, assign->value.get(), false, constrained);
        if (deferred) {
            m_context->handlerLabels.pop_back();
            emitCallInstruction("call $__ada_abort_undefer(l %.taskContext)", CallEffects::RuntimeHook, MasterEffects::Independent);
            jump(done);
            label(failed);
            emitCallInstruction("call $__ada_abort_undefer(l %.taskContext)", CallEffects::RuntimeHook, MasterEffects::Independent);
            emitExceptionCheck();
            jump(done);
            label(done);
            emitAsyncPoll();
        }
        break;
    }

    case StmtKind::ProcedureCall: {
        auto* call = static_cast<ProcedureCallStmt*>(statement);
        emitExpr(call->call.get());
        break;
    }

    case StmtKind::If: {
        auto* ifStatement = static_cast<IfStmt*>(statement);
        std::string end = newLabel("endif");
        for (IfBranch& branchStatement : ifStatement->branches) {
            Value condition = emitExpr(branchStatement.condition.get());
            std::string then = newLabel("then");
            std::string next = newLabel("elsif");
            branch(condition, then, next);
            label(then);
            emitStatements(branchStatement.body);
            jump(end);
            label(next);
        }
        if (ifStatement->hasElse) {
            emitStatements(ifStatement->elseBody);
        }
        jump(end);
        label(end);
        break;
    }

    case StmtKind::Loop: {
        auto* loop = static_cast<LoopStmt*>(statement);
        std::string head = newLabel("loop");
        std::string bodyLabel = newLabel("loopbody");
        std::string exit = newLabel("endloop");
        m_context->loopExits[loop] = exit;
        m_context->loopStorage[loop] = storageCheckpoint();
        m_context->loopHandlerDepth[loop] = m_context->handlerLabels.size();
        m_context->loopReturnDepth[loop] = m_context->returnCleanups.size();
        m_context->transferOperationDepth[exit] = m_context->operations.size();
        m_context->transferMasterDepth[exit] = m_context->blockMasters.size();

        if (loop->loopKind == LoopKind::For) {
            Symbol* variable = loop->variableSymbol;
            long long size = typeSize(variable->type);
            if (size < 1) {
                size = 1;
            }
            if (variable->isUplevel) {
                variable->frameOffset = reserveFrame(size, 1);
            } else {
                std::string slot = "%v." + Unicode::linkerIdentifier(variable->name) + "." + std::to_string(m_tempCounter++);
                m_context->prologue << "    " << slot << " =l " << (size <= 4 ? "alloc4" : "alloc8") << " " << size
                                    << "\n";
                m_context->locals[variable] = slot;
            }

            Value low = emitExpr(loop->rangeLow.get());
            Value high = emitExpr(loop->rangeHigh.get());
            char type = qbeClass(variable->type);
            if (!loop->rangeTypeLower.empty()) {
                std::string nonNull = newTemp();
                std::string check = newLabel("looprangecheck");
                std::string ready = newLabel("looprangeready");
                line(nonNull + " =w " + comparisonInstruction(BinaryOp::GreaterEqual, type, variable->type->modulus != 0)
                    + " " + high.name + ", " + low.name);
                branch(Value { nonNull, 'w' }, check, ready);
                label(check);
                emitRangeCheck(low, variable->type, loop->location);
                emitRangeCheck(high, variable->type, loop->location);
                jump(ready);
                label(ready);
            }
            std::string boundSlot = allocScratch(type == 'l' ? 8 : 4);
            Value address = addressOf(variable);
            storeInto(address, loop->isReverse ? high : low, variable->type);
            line(std::string(type == 'l' ? "storel " : "storew ")
                 + (loop->isReverse ? low.name : high.name) + ", " + boundSlot);

            label(head);
            Value current = loadFrom(addressOf(variable), variable->type);
            std::string bound = newTemp();
            line(bound + " =" + std::string(1, type) + (type == 'l' ? " loadl " : " loadsw ") + boundSlot);
            std::string test = newTemp();
            line(test + " =w " + comparisonInstruction(loop->isReverse ? BinaryOp::GreaterEqual : BinaryOp::LessEqual, type, variable->type->modulus != 0)
                 + " " + current.name + ", " + bound);
            branch(Value { test, 'w' }, bodyLabel, exit);
            label(bodyLabel);
            emitStatements(loop->body);
            Value step = loadFrom(addressOf(variable), variable->type);
            std::string lastIteration = newTemp();
            line(lastIteration + " =w " + comparisonInstruction(BinaryOp::Equal, type) + " " + step.name + ", " + bound);
            std::string advance = newLabel("loopstep");
            branch(Value { lastIteration, 'w' }, exit, advance);
            label(advance);
            std::string updated = newTemp();
            line(updated + " =" + std::string(1, type) + " " + (loop->isReverse ? "sub " : "add ") + step.name
                 + ", 1");
            storeInto(addressOf(variable), Value { updated, type }, variable->type);
            jump(head);
            label(exit);
            break;
        }

        label(head);
        if (loop->loopKind == LoopKind::While) {
            auto conditionStorage = storageCheckpoint();
            Value condition = emitExpr(loop->condition.get());
            rewindStorage(conditionStorage);
            branch(condition, bodyLabel, exit);
            label(bodyLabel);
        }
        emitStatements(loop->body);
        jump(head);
        label(exit);
        break;
    }

    case StmtKind::Exit: {
        auto* exitStatement = static_cast<ExitStmt*>(statement);
        auto it = m_context->loopExits.find(exitStatement->target);
        if (it == m_context->loopExits.end()) {
            break;
        }
        std::string cleanup;
        if (!m_context->finalizationChain.empty() || !m_context->blockMasters.empty()) {
            cleanup = newLabel("exitcleanup");
        }
        std::string destination = cleanup.empty() ? it->second : cleanup;
        std::string next = newLabel("noexit");
        if (exitStatement->condition) {
            Value condition = emitExpr(exitStatement->condition.get());
            branch(condition, destination, next);
        } else {
            jump(destination);
        }
        if (!cleanup.empty()) {
            label(cleanup);
            auto handlers = m_context->handlerLabels;
            auto masters = m_context->blockMasters;
            auto operations = m_context->operations;
            std::size_t depth = m_context->transferMasterDepth.at(it->second);
            for (std::size_t i = m_context->returnCleanups.size();
                i > m_context->loopReturnDepth.at(exitStatement->target); --i) {
                const auto& result = m_context->returnCleanups[i - 1];
                emitOperationExits(result.operationDepth);
                emitBlockExits(result.masterDepth);
                m_context->blockMasters.resize(result.masterDepth);
                m_context->handlerLabels.resize(result.handlerDepth);
                if (!result.taskMark.empty()) {
                    emitCallInstruction("call $__ada_task_result_discard(l %.resultMaster, l " + result.taskMark + ", l %.resultGroup)", CallEffects::RuntimeHook);
                }
                emitCallInstruction("call $__ada_finalize_to(l %.resultOwner, l " + result.ownerMark + ")", CallEffects::DeferredCheck);
                emitCallInstruction("call $__ada_array_rewind(l %.resultArena, l " + result.arenaMark + ")", CallEffects::RuntimeHook);
                emitExceptionCheck();
            }
            emitOperationExits(m_context->transferOperationDepth.at(it->second));
            emitBlockExits(depth);
            m_context->blockMasters.resize(depth);
            m_context->handlerLabels.resize(m_context->loopHandlerDepth.at(exitStatement->target));
            rewindStorage(m_context->loopStorage.at(exitStatement->target));
            m_context->handlerLabels = std::move(handlers);
            jump(it->second);
            m_context->operations = std::move(operations);
            m_context->blockMasters = std::move(masters);
        }
        if (exitStatement->condition) {
            label(next);
        }
        break;
    }

    case StmtKind::Return: {
        auto accepted = std::find_if(m_context->operations.rbegin(), m_context->operations.rend(),
            [](const auto& operation) { return operation.asynchronous == nullptr; });
        if (accepted != m_context->operations.rend()) {
            jump(accepted->returnLabel);
            break;
        }
        auto* returnStatement = static_cast<ReturnStmt*>(statement);
        Type* resultType = m_context->symbol == nullptr ? nullptr : m_context->symbol->returnType;
        if (returnStatement->value == nullptr && returnStatement->object == nullptr
            && !m_context->extendedReturnLabel.empty()) {
            jump(m_context->extendedReturnLabel);
            break;
        }
        std::string savedOwner = m_context->constructionOwner, savedArena = m_context->constructionArena;
        std::string savedLevel = m_context->coextensionLevel;
        if (hasAccessDiscriminants(resultType)) {
            m_context->constructionOwner = "%.resultOwner";
            m_context->constructionArena = "%.resultArena";
            m_context->coextensionLevel = "%.resultAccessLevel";
        }
        auto checkResultAccess = [&]() {
            if (!hasAccessDiscriminants(resultType)) { return; }
            Value result {"%.result", 'l'};
            if (isUnconstrainedArray(resultType)) { result = loadArrayView("%.result", resultType->arrayRank, true); }
            checkAccessDiscriminants(result, resultType, accessLevelFor(m_context->symbol), "%.resultOwner");
        };
        if (returnStatement->object != nullptr && (!isComposite(resultType)
            || (!needsFinalization(resultType) && !hasLimitedParts(resultType)
                && !resultType->tagged && !hasAccessDiscriminants(resultType)))) {
            if (isUnconstrainedArray(resultType) && !m_context->cleansArrayResult) {
                m_context->cleansArrayResult = true;
                m_context->prologue << "    storel 0, %.result\n";
            }
            DeclList declarations;
            declarations.push_back(std::move(returnStatement->object));
            emitLocalDeclarations(declarations);
            returnStatement->object = std::move(declarations.front());
            auto* object = static_cast<ObjectDecl*>(returnStatement->object.get());
            emitExtendedReturnBody(returnStatement);
            auto value = std::make_unique<IdentifierExpr>();
            value->symbol = object->symbols.front();
            value->type = value->symbol->type;
            value->location = returnStatement->location;
            ReturnStmt completion;
            completion.location = returnStatement->location;
            completion.value = std::move(value);
            emitStatement(&completion);
        } else if (returnStatement->object != nullptr) {
            emitControlledResult(nullptr, static_cast<ObjectDecl*>(returnStatement->object.get()), returnStatement);
            checkResultAccess();
            emitReturn();
        } else if (returnStatement->value && isComposite(resultType)) {
            if (needsFinalization(resultType) || hasLimitedParts(resultType) || resultType->tagged) {
                emitControlledResult(returnStatement->value.get());
            } else if (isUnconstrainedArray(resultType)) {
                Value value = emitExpr(returnStatement->value.get());
                value = withBounds(value, returnStatement->value->type, nullptr);
                if (resultType->boundsSymbol != nullptr) {
                    Value target = withBounds(Value {}, resultType, nullptr);
                    checkArrayShape(target, resultType, value, returnStatement->value->type);
                    value.first = target.first;
                    value.last = target.last;
                    value.innerBounds = target.innerBounds;
                }
                std::string elementSize = arrayElementSize(value, resultType);
                emitCallInstruction("call $__ada_array_result(l %.result, l " + value.name + ", l " + value.first
                     + ", l " + value.last + ", l " + elementSize + ")", CallEffects::CheckException);
                storeArrayView("%.result", value, resultType, true);
            } else {
                assignInto(Value { "%.result", 'l' }, resultType, returnStatement->value.get(), true);
            }
            checkResultAccess();
            emitReturn();
        } else if (returnStatement->value) {
            Value value = emitExpr(returnStatement->value.get());
            if (m_context->symbol != nullptr) {
                emitRangeCheck(value, m_context->symbol->returnType, returnStatement->location);
            }
            if (resultType->anonymousAccess) {
                line("storew " + accessLevelFor(resultType) + ", %.accessResultLevel");
            }
            emitReturn(value.name);
        } else {
            checkResultAccess();
            emitReturn();
        }
        m_context->constructionOwner = savedOwner;
        m_context->constructionArena = savedArena;
        m_context->coextensionLevel = savedLevel;
        m_context->terminated = true;
        break;
    }

    case StmtKind::Case: {
        auto* caseStatement = static_cast<CaseStmt*>(statement);
        Value selector = emitExpr(caseStatement->selector.get());
        char type = selector.type;
        std::string end = newLabel("endcase");

        std::vector<std::string> bodyLabels;
        for (std::size_t i = 0; i < caseStatement->alternatives.size(); ++i) {
            bodyLabels.push_back(newLabel("casebody"));
        }

        std::string othersLabel;
        for (std::size_t i = 0; i < caseStatement->alternatives.size(); ++i) {
            CaseAlternative& alternative = caseStatement->alternatives[i];
            if (alternative.isOthers) {
                othersLabel = bodyLabels[i];
                continue;
            }
            std::string match;
            for (const CaseChoice& choice : alternative.choices) {
                std::string test = newTemp();
                if (choice.low == choice.high) {
                    line(test + " =w " + comparisonInstruction(BinaryOp::Equal, type) + " " + selector.name + ", "
                         + scalarText(choice.low));
                } else {
                    std::string lowTest = newTemp();
                    std::string highTest = newTemp();
                    line(lowTest + " =w " + comparisonInstruction(BinaryOp::GreaterEqual, type, caseStatement->selector->type->modulus != 0) + " "
                         + selector.name + ", " + scalarText(choice.low));
                    line(highTest + " =w " + comparisonInstruction(BinaryOp::LessEqual, type, caseStatement->selector->type->modulus != 0) + " " + selector.name
                         + ", " + scalarText(choice.high));
                    line(test + " =w and " + lowTest + ", " + highTest);
                }
                if (match.empty()) {
                    match = test;
                } else {
                    std::string combined = newTemp();
                    line(combined + " =w or " + match + ", " + test);
                    match = combined;
                }
            }
            std::string next = newLabel("nextcase");
            branch(Value { match, 'w' }, bodyLabels[i], next);
            label(next);
        }

        jump(othersLabel.empty() ? end : othersLabel);

        for (std::size_t i = 0; i < caseStatement->alternatives.size(); ++i) {
            label(bodyLabels[i]);
            emitStatements(caseStatement->alternatives[i].body);
            jump(end);
        }
        label(end);
        break;
    }

    case StmtKind::Block: {
        auto* block = static_cast<BlockStmt*>(statement);
        auto checkpoint = storageCheckpoint();
        std::size_t observingCalls = m_context->masterObservingCalls;
        std::string master = allocScratch(kAdaMasterBytes);
        emitCallInstruction("call $__ada_master_enter(l %.taskContext, l " + master + ", w 1)", CallEffects::RuntimeHook, MasterEffects::ScopeHook);
        m_context->blockMasters.push_back({ master, checkpoint });
        emitLocalDeclarations(block->declarations);
        auto activate = [&]() {
            emitCallInstruction("call $__ada_master_activate(l %.taskContext, l " + master + ")", CallEffects::CheckedRuntimeHook, MasterEffects::ScopeHook);
        };
        if (block->handlers.empty()) {
            activate();
            emitStatements(block->body);
        } else {
            std::string dispatch = newLabel("handler");
            std::string after = newLabel("handled");
            m_context->handlerStorage[dispatch] = storageCheckpoint();
            pushHandler(dispatch);
            activate();
            emitStatements(block->body);
            m_context->handlerLabels.pop_back();
            jump(after);
            emitHandlers(block->handlers, after, dispatch);
            label(after);
        }
        if (!m_context->terminated) {
            emitBlockExits(m_context->blockMasters.size() - 1);
        }
        m_context->blockMasters.pop_back();
        if (!m_context->terminated && !checkpoint.finalization.empty()) {
            emitExceptionCheck();
        }
        if (m_context->masterObservingCalls == observingCalls) {
            m_context->elidedBlockMasters.push_back(master);
        }
        break;
    }

    case StmtKind::Raise: {
        auto* raise = static_cast<RaiseStmt*>(statement);
        emitRaise(raise->exceptionSymbol, raise->location, raise->message.get());
        break;
    }
    }
}

void QbeEmitter::emitExtendedReturnBody(ReturnStmt* statement)
{
    std::string savedReturn = m_context->extendedReturnLabel;
    std::string bodyDone = newLabel("returnbodydone");
    m_context->transferMasterDepth[bodyDone] = m_context->blockMasters.size();
    m_context->transferOperationDepth[bodyDone] = m_context->operations.size();
    m_context->extendedReturnLabel = bodyDone;
    if (statement->handlers.empty()) {
        emitStatements(statement->body);
    } else {
        std::string dispatch = newLabel("returnhandler");
        std::string handled = newLabel("returnhandled");
        m_context->handlerStorage[dispatch] = storageCheckpoint();
        pushHandler(dispatch);
        emitStatements(statement->body);
        m_context->handlerLabels.pop_back();
        jump(handled);
        emitHandlers(statement->handlers, handled, dispatch);
        label(handled);
    }
    label(bodyDone);
    m_context->extendedReturnLabel = savedReturn;
}

Value QbeEmitter::emitSelectDeadline(DelayStmt* delay, AdaSelectClock& clock)
{
    if (delay == nullptr) { clock = ADA_SELECT_MONOTONIC; return Value {"0", 'l'}; }
    SourceLocation saved = m_context->sourceLocation;
    m_context->sourceLocation = delay->location;
    Value value = emitExpr(delay->expression.get());
    clock = delay->calendar ? ADA_SELECT_CALENDAR : ADA_SELECT_MONOTONIC;
    if (!delay->until) {
        Value deadline {newTemp(), 'l'};
        emitCallInstruction(deadline.name + " =l call $__ada_select_deadline(l " + value.name
            + ", w " + std::to_string(ADA_SELECT_RELATIVE) + ")", CallEffects::CheckException);
        value = deadline;
    }
    m_context->sourceLocation = saved;
    return value;
}

void QbeEmitter::emitSelect(SelectStmt* select)
{
    if (select->form == SelectForm::Asynchronous) {
        auto& trigger = select->alternatives.front();
        auto checkpoint = storageCheckpoint();
        FunctionContext::SelectCallPlan plan;
        plan.asynchronous = true;
        if (trigger.operation->kind == StmtKind::Delay) {
            AdaSelectClock clock;
            Value deadline = emitSelectDeadline(static_cast<DelayStmt*>(trigger.operation.get()), clock);
            plan.handle = newTemp();
            emitCallInstruction(plan.handle + " =l call $__ada_async_start_delay(l %.taskContext, l " + deadline.name
                + ", w " + std::to_string(clock) + ")", CallEffects::CheckException);
        } else {
            auto* call = static_cast<ProcedureCallStmt*>(trigger.operation.get());
            Expr* expression = call->call.get();
            if (expression->implicitCall != nullptr) { expression = expression->implicitCall.get(); }
            plan.target = expression->kind == ExprKind::Call ? static_cast<CallExpr*>(expression)->subprogram
                : expression->kind == ExprKind::Selected ? static_cast<SelectedExpr*>(expression)->symbol
                : static_cast<IdentifierExpr*>(expression)->symbol;
            auto* saved = m_context->selectCall;
            m_context->selectCall = &plan;
            emitStatement(call);
            m_context->selectCall = saved;
        }
        auto cleanup = std::make_shared<FunctionContext::AsyncCleanup>();
        cleanup->handle = plan.handle;
        cleanup->cleanupLabel = newLabel("asynccleanup");
        cleanup->failureLabel = newLabel("asyncfailed");
        cleanup->resumeSlot = allocScratch(4);
        std::string done = newLabel("asyncdone"), finish = newLabel("asyncfinish");
        cleanup->resumes.push_back(done);
        std::size_t depth = m_context->blockMasters.size();
        std::string master = allocScratch(kAdaMasterBytes);
        emitCallInstruction("call $__ada_master_enter(l %.taskContext, l " + master + ", w 1)", CallEffects::RuntimeHook, MasterEffects::ScopeHook);
        // Keep trigger actual temporaries alive through completion and copy-back.
        auto partStorage = storageCheckpoint();
        m_context->blockMasters.push_back({master, partStorage});
        m_context->operations.push_back({"", "", depth, cleanup});
        pushHandler(cleanup->failureLabel);
        std::string run = newTemp(), body = newLabel("abortablepart");
        emitCallInstruction(run + " =w call $__ada_async_run(l " + plan.handle + ")", CallEffects::RuntimeHook);
        m_context->transferOperationDepth[finish] = m_context->operations.size();
        m_context->transferMasterDepth[finish] = m_context->blockMasters.size();
        branch(Value {run, 'w'}, body, finish);
        label(body);
        emitStatements(select->abortBody);
        jump(finish);
        label(cleanup->failureLabel);
        jump(finish);
        label(finish);
        line("storew 0, " + cleanup->resumeSlot);
        line("jmp " + cleanup->cleanupLabel);
        m_context->terminated = true;
        m_context->handlerLabels.pop_back();
        m_context->operations.pop_back();
        label(cleanup->cleanupLabel);
        std::string matched = newTemp(), interrupted = newLabel("asyncinterrupted"), unwind = newLabel("asyncunwind");
        emitCallInstruction(matched + " =w call $__ada_async_matches(l %.taskContext, l " + plan.handle + ")", CallEffects::RuntimeHook);
        branch(Value {matched, 'w'}, interrupted, unwind);
        label(interrupted);
        line("storew 0, " + cleanup->resumeSlot);
        emitCallInstruction("call $__ada_master_abort(l %.taskContext, l " + master + ")", CallEffects::RuntimeHook);
        jump(unwind);
        label(unwind);
        emitBlockExits(depth);
        m_context->blockMasters.resize(depth);
        // A trigger may complete while awaiting dependent tasks.
        std::string afterAwait = newTemp(), reset = newLabel("asyncresetresume"), leave = newLabel("asyncleave");
        emitCallInstruction(afterAwait + " =w call $__ada_async_matches(l %.taskContext, l " + plan.handle + ")", CallEffects::RuntimeHook);
        branch(Value {afterAwait, 'w'}, reset, leave);
        label(reset);
        line("storew 0, " + cleanup->resumeSlot);
        jump(leave);
        label(leave);
        std::string selected = newTemp();
        emitCallInstruction(selected + " =w call $__ada_async_leave(l %.taskContext, l " + plan.handle + ")", CallEffects::DeferredCheck);
        emitExceptionCheck();
        std::string triggered = newLabel("asynctriggered"), dispatch = newLabel("asyncdispatch");
        branch(Value {selected, 'w'}, triggered, dispatch);
        label(triggered);
        if (plan.copyBack) { plan.copyBack(); }
        rewindStorage(checkpoint);
        emitStatements(trigger.body);
        jump(dispatch);
        label(dispatch);
        std::string resume = newTemp();
        line(resume + " =w loadw " + cleanup->resumeSlot);
        for (std::size_t i = 1; i < cleanup->resumes.size(); ++i) {
            std::string next = newLabel("asyncnextresume"), matches = newTemp();
            line(matches + " =w ceqw " + resume + ", " + std::to_string(i));
            branch(Value {matches, 'w'}, cleanup->resumes[i], next);
            label(next);
        }
        jump(done);
        label(done);
        rewindStorage(checkpoint);
        return;
    }
    std::string done = newLabel("selectdone");
    if (select->form != SelectForm::Selective) {
        auto* call = static_cast<ProcedureCallStmt*>(select->alternatives.front().operation.get());
        Expr* expression = call->call.get();
        if (expression->implicitCall != nullptr) { expression = expression->implicitCall.get(); }
        FunctionContext::SelectCallPlan plan;
        plan.target = expression->kind == ExprKind::Call ? static_cast<CallExpr*>(expression)->subprogram
            : expression->kind == ExprKind::Selected ? static_cast<SelectedExpr*>(expression)->symbol
            : static_cast<IdentifierExpr*>(expression)->symbol;
        plan.mode = select->form == SelectForm::Conditional ? ADA_ENTRY_CONDITIONAL : ADA_ENTRY_TIMED;
        if (select->form == SelectForm::Timed) { plan.delay = static_cast<DelayStmt*>(select->alternatives.back().operation.get()); }
        auto checkpoint = storageCheckpoint();
        auto* saved = m_context->selectCall;
        m_context->selectCall = &plan;
        emitStatement(call);
        m_context->selectCall = saved;
        rewindStorage(checkpoint);
        std::string committed = newLabel("selectcommitted"), cancelled = newLabel("selectcancelled");
        branch(Value {plan.selected, 'w'}, committed, cancelled);
        label(committed);
        emitStatements(select->alternatives.front().body);
        jump(done);
        label(cancelled);
        if (select->form == SelectForm::Conditional) { emitStatements(select->elseBody); }
        else { emitStatements(select->alternatives.back().body); }
        jump(done);
        label(done);
        return;
    }
    std::string table = allocScratch(select->alternatives.size() * kAdaSelectChoiceBytes);
    auto checkpoint = storageCheckpoint();
    for (std::size_t i = 0; i < select->alternatives.size(); ++i) {
        auto& alternative = select->alternatives[i];
        std::string record = newTemp();
        line(record + " =l add " + table + ", " + std::to_string(i * kAdaSelectChoiceBytes));
        std::string flag = newTemp();
        line(flag + " =l add " + record + ", 28");
        Value open {"1", 'w'};
        if (alternative.guard != nullptr) { open = emitExpr(alternative.guard.get()); }
        line("storew " + open.name + ", " + flag);
        std::string evaluate = newLabel("selectopen"), next = newLabel("selectnext");
        branch(open, evaluate, next);
        label(evaluate);
        AdaSelectKind kind = alternative.operation->kind == StmtKind::Accept ? ADA_SELECT_ACCEPT
            : alternative.operation->kind == StmtKind::Delay ? ADA_SELECT_DELAY : ADA_SELECT_TERMINATE;
        line("storew " + std::to_string(kind) + ", " + record);
        auto store = [&](int offset, const std::string& value, char type) {
            std::string slot = newTemp();
            line(slot + " =l add " + record + ", " + std::to_string(offset));
            line(std::string(type == 'w' ? "storew " : "storel ") + value + ", " + slot);
        };
        if (kind == ADA_SELECT_ACCEPT) {
            auto* accept = static_cast<AcceptStmt*>(alternative.operation.get());
            Value index {"0", 'l'};
            if (accept->index != nullptr) {
                index = emitExpr(accept->index.get());
                emitRangeCheck(index, accept->entry->entryIndexType, accept->index->location);
                index = wideBound(index);
            }
            store(4, std::to_string(accept->entry->taskEntryId), 'w');
            store(8, index.name, 'l');
        } else if (kind == ADA_SELECT_DELAY) {
            AdaSelectClock clock;
            Value deadline = emitSelectDeadline(static_cast<DelayStmt*>(alternative.operation.get()), clock);
            store(16, deadline.name, 'l');
            store(24, std::to_string(clock), 'w');
        }
        jump(next);
        label(next);
    }
    rewindStorage(checkpoint);
    std::string requestSlot = allocScratch(8), selected = newTemp();
    line("storel 0, " + requestSlot);
    emitCallInstruction(selected + " =w call $__ada_task_select(l %.taskContext, l " + table
        + ", w " + std::to_string(select->alternatives.size()) + ", w " + (select->hasElse ? "1" : "0")
        + ", l " + requestSlot + ")", CallEffects::CheckException);
    for (std::size_t i = 0; i < select->alternatives.size(); ++i) {
        std::string chosen = newLabel("selectchosen"), next = newLabel("selectchoice");
        Value matches {newTemp(), 'w'};
        line(matches.name + " =w ceqw " + selected + ", " + std::to_string(i));
        branch(matches, chosen, next);
        label(chosen);
        auto& alternative = select->alternatives[i];
        if (alternative.operation->kind == StmtKind::Accept) {
            m_context->selectedAccept = static_cast<AcceptStmt*>(alternative.operation.get());
            m_context->selectedRequest = newTemp();
            line(m_context->selectedRequest + " =l loadl " + requestSlot);
            emitStatement(alternative.operation.get());
        } else if (alternative.operation->kind == StmtKind::Terminate) { emitReturn(); }
        emitStatements(alternative.body);
        jump(done);
        label(next);
    }
    emitStatements(select->elseBody);
    jump(done);
    label(done);
}
