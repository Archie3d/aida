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
        emitStatement(statement.get());
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
    case StmtKind::Pragma:
        emitPragma(static_cast<PragmaStmt*>(statement)->pragma.get());
        break;
    case StmtKind::Goto: {
        auto* transfer = static_cast<GotoStmt*>(statement);
        const auto target = m_context->gotoTargets.at(transfer->target);
        auto masters = m_context->blockMasters;
        auto handlers = m_context->handlerLabels;
        // An abandoned extended return owns registrations in the caller's
        // result chain. Unwind its inner blocks before discarding that result.
        for (std::size_t i = m_context->returnCleanups.size(); i > target.returnDepth; --i) {
            const auto& result = m_context->returnCleanups[i - 1];
            emitBlockExits(result.masterDepth);
            m_context->blockMasters.resize(result.masterDepth);
            m_context->handlerLabels.resize(result.handlerDepth);
            emitCallInstruction("call $__ada_finalize_to(l %.resultOwner, l " + result.ownerMark + ")", CallEffects::DeferredCheck);
            emitCallInstruction("call $__ada_array_rewind(l %.resultArena, l " + result.arenaMark + ")", CallEffects::RuntimeHook);
            emitExceptionCheck();
        }
        emitBlockExits(target.masterDepth);
        m_context->blockMasters.resize(target.masterDepth);
        m_context->handlerLabels.resize(target.handlerDepth);
        rewindStorage(target.storage);
        jump(target.label);
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
        Value address = emitAddress(assign->target.get());
        std::string constrained = assign->target->type->kind == TypeKind::Record
            ? recordConstrained(assign->target.get(), &address).name : "1";
        assignInto(address, assign->target->type, assign->value.get(), false, constrained);
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
            if (variable->type->scalarBoundsSymbol != nullptr) {
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
            std::size_t depth = m_context->transferMasterDepth.at(it->second);
            for (std::size_t i = m_context->returnCleanups.size();
                i > m_context->loopReturnDepth.at(exitStatement->target); --i) {
                const auto& result = m_context->returnCleanups[i - 1];
                emitBlockExits(result.masterDepth);
                m_context->blockMasters.resize(result.masterDepth);
                m_context->handlerLabels.resize(result.handlerDepth);
                emitCallInstruction("call $__ada_finalize_to(l %.resultOwner, l " + result.ownerMark + ")", CallEffects::DeferredCheck);
                emitCallInstruction("call $__ada_array_rewind(l %.resultArena, l " + result.arenaMark + ")", CallEffects::RuntimeHook);
                emitExceptionCheck();
            }
            emitBlockExits(depth);
            m_context->blockMasters.resize(depth);
            m_context->handlerLabels.resize(m_context->loopHandlerDepth.at(exitStatement->target));
            rewindStorage(m_context->loopStorage.at(exitStatement->target));
            m_context->handlerLabels = std::move(handlers);
            jump(it->second);
            m_context->blockMasters = std::move(masters);
        }
        if (exitStatement->condition) {
            label(next);
        }
        break;
    }

    case StmtKind::Return: {
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
        std::string master = allocScratch(8);
        emitCallInstruction("call $__ada_master_enter(l %.taskContext, l " + master + ", w 1)", CallEffects::RuntimeHook);
        m_context->blockMasters.push_back({ master, checkpoint });
        emitLocalDeclarations(block->declarations);
        emitCallInstruction("call $__ada_master_activate(l %.taskContext, l " + master + ")", CallEffects::CheckedRuntimeHook);
        if (block->handlers.empty()) {
            emitStatements(block->body);
        } else {
            std::string dispatch = newLabel("handler");
            std::string after = newLabel("handled");
            m_context->handlerStorage[dispatch] = storageCheckpoint();
            pushHandler(dispatch);
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
