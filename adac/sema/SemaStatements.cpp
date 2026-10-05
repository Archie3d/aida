#include "Sema.h"
#include "SemaSupport.h"

#include <utility>

using SemaSupport::isUniversal;
using SemaSupport::adaptUniversal;

void Sema::analyzeStatements(StmtList& statements, Scope* scope)
{
    for (const StmtPtr& statement : statements) {
        analyzeStatement(statement.get(), scope);
    }
}

void Sema::analyzeHandlers(std::vector<ExceptionHandler>& handlers, Scope* scope)
{
    std::unordered_map<std::string, ExceptionHandler*> covered;
    for (ExceptionHandler& handler : handlers) {
        if (handler.isOthers && &handler != &handlers.back()) {
            m_diagnostics.error(handler.location, "others must appear in the last handler");
        }
        if (handler.isOthers && !handler.names.empty()) {
            m_diagnostics.error(handler.location, "others must be the only choice of a handler");
        }
        for (std::size_t i = 0; i < handler.namesLower.size(); ++i) {
            Symbol* symbol = lookupName(handler.namesLower[i], scope);
            if (symbol == nullptr || symbol->kind != SymbolKind::Exception) {
                m_diagnostics.error(handler.location, "unknown exception '" + handler.names[i] + "'");
                continue;
            }
            handler.exceptions.push_back(symbol);
            auto [previous, inserted] = covered.emplace(symbol->exceptionObject, &handler);
            if (!inserted && previous->second != &handler) {
                m_diagnostics.error(handler.location, "exception '" + handler.names[i]
                    + "' is already covered by an earlier handler");
            }
        }
        Scope* inner = m_symbolTable.createScope(scope);
        if (!handler.choiceName.empty()) {
            Symbol* choice = m_symbolTable.createSymbol(SymbolKind::Object, handler.choiceLower, handler.choiceName);
            choice->type = m_exceptionOccurrenceType;
            choice->isConstant = true;
            choice->location = handler.choiceLocation;
            choice->owner = m_currentSubprogram;
            choice->level = m_currentSubprogram != nullptr ? m_currentSubprogram->level : 0;
            inner->add(choice);
            handler.choiceSymbol = choice;
        }
        ++m_handlerDepth;
        analyzeStatements(handler.body, inner);
        --m_handlerDepth;
    }
}

void Sema::checkAssignable(Expr* target, Scope* scope, bool allowLimited)
{
    (void)scope;

    if (target->m_implicitCall != nullptr
        || (target->kind == ExprKind::Call && static_cast<CallExpr*>(target)->m_indirect)) {
        m_diagnostics.error(target->location, "the target of an assignment must be a variable");
        return;
    }

    // A limited private type is not copied outside the package that declared
    // it; whatever it takes to make one is that package's to offer.
    Type* type = baseType(target->type);
    if (!allowLimited && type != nullptr && type->isLimited && type->privateTo != nullptr
        && !withinPackage(type->privateTo)) {
        m_diagnostics.error(target->location, "'" + type->name + "' is limited private, so a value of it cannot "
                                                  + "be assigned outside '" + type->privateTo->displayName + "'");
        return;
    }

    // A discriminant is fixed when the object is declared and stays that way,
    // since the components the value has were settled by it.
    if (target->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(target);
        if (selected->symbol != nullptr && selected->symbol->isConstant) {
            m_diagnostics.error(target->location, "'" + selected->symbol->displayName + "' cannot be assigned to");
            return;
        }
        Type* record = baseType(selected->prefix->type);
        if (record != nullptr && record->kind == TypeKind::Access) {
            record = baseType(record->target);
        }
        if (selected->fieldIndex >= 0 && record != nullptr
            && record->fields[static_cast<std::size_t>(selected->fieldIndex)].isDiscriminant) {
            m_diagnostics.error(target->location, "'" + selected->selector
                                                      + "' is a discriminant, fixed when the object was declared");
            return;
        }
    }

    switch (target->kind) {
    case ExprKind::Identifier: {
        Symbol* symbol = static_cast<IdentifierExpr*>(target)->symbol;
        if (symbol == nullptr) {
            return;
        }
        if (target->type != nullptr && target->type->m_tagged && symbol->kind == SymbolKind::Subprogram) {
            m_diagnostics.error(target->location, "the target of an assignment must be a variable");
            return;
        }
        if (symbol->kind == SymbolKind::Number || symbol->kind == SymbolKind::LoopParameter
            || (symbol->kind == SymbolKind::Object && symbol->isConstant)
            || (symbol->kind == SymbolKind::Parameter && symbol->mode == ParameterMode::In)) {
            m_diagnostics.error(target->location, "'" + symbol->displayName + "' cannot be assigned to");
        }
        return;
    }
    case ExprKind::Selected: {
        auto* selected = static_cast<SelectedExpr*>(target);
        if (selected->symbol != nullptr && selected->symbol->kind == SymbolKind::Subprogram
            && target->type != nullptr && target->type->m_tagged) {
            m_diagnostics.error(target->location, "the target of an assignment must be a variable");
            return;
        }
        Type* prefix = baseType(selected->prefix->type);
        if (selected->symbol == nullptr && prefix != nullptr && prefix->kind != TypeKind::Access) {
            checkAssignable(selected->prefix.get(), scope, true);
        }
        return;
    }
    case ExprKind::Call: {
        auto* call = static_cast<CallExpr*>(target);
        if (call->form == CallForm::Conversion && call->type != nullptr && call->type->m_tagged) {
            checkAssignable(call->resolvedArguments.front(), scope, allowLimited);
            return;
        }
        if (call->form == CallForm::Indexing || call->form == CallForm::Slice) {
            Type* prefix = baseType(call->callee->type);
            if (prefix != nullptr && prefix->kind != TypeKind::Access) {
                checkAssignable(call->callee.get(), scope, true);
            }
        } else if (target->type != nullptr && target->type->m_tagged) {
            m_diagnostics.error(target->location, "the target of an assignment must be a variable");
        }
        return;
    }
    default:
        m_diagnostics.error(target->location, "the target of an assignment must be a variable");
        return;
    }
}

void Sema::analyzeStatement(Stmt* statement, Scope* scope)
{
    switch (statement->kind) {
    case StmtKind::Null:
        break;

    case StmtKind::Assign: {
        auto* assign = static_cast<AssignStmt*>(statement);
        Type* targetType = analyzeExpr(assign->target.get(), scope, nullptr);
        checkAssignable(assign->target.get(), scope);
        if (hasLimitedControlledParts(targetType)) {
            m_diagnostics.error(assign->location, "limited controlled objects cannot be assigned");
        }
        Type* valueType = analyzeExpr(assign->value.get(), scope, targetType);
        if (targetType != nullptr && targetType->m_classRoot != nullptr
            && needsFinalization(valueType) && !needsFinalization(targetType)) {
            m_diagnostics.error(assign->location, "class-wide controlled assignment is not yet supported");
        }
        if (!typesCompatible(targetType, valueType)) {
            m_diagnostics.error(assign->location, "the assigned value has an incompatible type");
        }
        if (targetType != nullptr && targetType->m_classRoot != nullptr
            && valueType != nullptr && valueType->m_classRoot == nullptr && !assign->value->m_tagIndeterminate) {
            m_diagnostics.error(assign->location, "class-wide assignment requires a dynamically tagged value");
        }
        if (targetType != nullptr && targetType->m_classRoot != nullptr && assign->value->m_tagIndeterminate) {
            dispatchIndeterminate(assign->value.get(), targetType->m_classRoot);
        }
        adaptUniversal(assign->value.get(), targetType);
        break;
    }

    case StmtKind::ProcedureCall: {
        auto* call = static_cast<ProcedureCallStmt*>(statement);
        int errorsBefore = m_diagnostics.errorCount();
        Expr* expression = call->call.get();
        if (expression->kind == ExprKind::Identifier || expression->kind == ExprKind::Selected) {
            for (Type* type : expressionTypes(expression, scope)) {
                if (type->m_accessProfile != nullptr) {
                    auto indirect = std::make_unique<CallExpr>();
                    indirect->location = expression->location;
                    indirect->callee = std::move(call->call);
                    call->call = std::move(indirect);
                    expression = call->call.get();
                    break;
                }
            }
        }
        Type* result = analyzeExpr(expression, scope, m_types.voidType());
        if (m_diagnostics.errorCount() != errorsBefore) {
            break;
        }
        if (expression->m_implicitCall != nullptr) {
            expression = expression->m_implicitCall.get();
        }
        Symbol* target = nullptr;
        if (expression->kind == ExprKind::Identifier) {
            target = static_cast<IdentifierExpr*>(expression)->symbol;
        } else if (expression->kind == ExprKind::Selected) {
            target = static_cast<SelectedExpr*>(expression)->symbol;
        } else if (expression->kind == ExprKind::Call) {
            target = static_cast<CallExpr*>(expression)->subprogram;
        }
        bool streamProcedure = expression->kind == ExprKind::Attribute && result == m_types.voidType();
        if (!streamProcedure && (target == nullptr || target->kind != SymbolKind::Subprogram
                                 || target->returnType != nullptr)) {
            m_diagnostics.error(expression->location, "a procedure call statement requires a procedure");
        }
        break;
    }

    case StmtKind::If: {
        auto* ifStatement = static_cast<IfStmt*>(statement);
        for (IfBranch& branch : ifStatement->branches) {
            Type* conditionType = analyzeExpr(branch.condition.get(), scope, m_types.booleanType());
            if (!m_types.isBoolean(conditionType)) {
                m_diagnostics.error(branch.condition->location, "condition must be of type Boolean");
            }
            analyzeStatements(branch.body, scope);
        }
        analyzeStatements(ifStatement->elseBody, scope);
        break;
    }

    case StmtKind::Loop: {
        auto* loop = static_cast<LoopStmt*>(statement);
        Scope* inner = m_symbolTable.createScope(scope);

        if (loop->loopKind == LoopKind::While) {
            Type* conditionType = analyzeExpr(loop->condition.get(), scope, m_types.booleanType());
            if (!m_types.isBoolean(conditionType)) {
                m_diagnostics.error(loop->condition->location, "condition must be of type Boolean");
            }
        } else if (loop->loopKind == LoopKind::For) {
            Type* variableType = nullptr;
            if (!loop->rangeTypeLower.empty()) {
                variableType = resolveTypeName(loop->rangeTypeLower, scope, loop->location);
            }
            if (loop->rangeLow && loop->rangeHigh) {
                Type* lowType = analyzeExpr(loop->rangeLow.get(), scope, variableType);
                Type* highType = analyzeExpr(loop->rangeHigh.get(), scope, variableType);
                if (variableType == nullptr) {
                    variableType = isUniversal(lowType) ? highType : lowType;
                    if (isUniversal(variableType)) {
                        variableType = m_types.integerType();
                    }
                }
                adaptUniversal(loop->rangeLow.get(), variableType);
                adaptUniversal(loop->rangeHigh.get(), variableType);
            } else if (variableType != nullptr) {
                loop->rangeLow = scalarBoundExpr(variableType, true, loop->location);
                loop->rangeHigh = scalarBoundExpr(variableType, false, loop->location);
            }
            if (variableType == nullptr) {
                variableType = m_types.integerType();
            }

            Symbol* variable = m_symbolTable.createSymbol(SymbolKind::LoopParameter, loop->variableLower,
                                                          loop->variableName);
            variable->type = variableType;
            variable->isConstant = true;
            variable->location = loop->location;
            variable->owner = m_currentSubprogram;
            variable->level = m_currentSubprogram != nullptr ? m_currentSubprogram->level : 0;
            variable->isGlobal = m_currentSubprogram == nullptr;
            inner->add(variable);
            loop->variableSymbol = variable;
        }

        m_loops.push_back(loop);
        analyzeStatements(loop->body, inner);
        m_loops.pop_back();
        break;
    }

    case StmtKind::Exit: {
        auto* exitStatement = static_cast<ExitStmt*>(statement);
        if (exitStatement->condition) {
            Type* conditionType = analyzeExpr(exitStatement->condition.get(), scope, m_types.booleanType());
            if (!m_types.isBoolean(conditionType)) {
                m_diagnostics.error(exitStatement->condition->location, "condition must be of type Boolean");
            }
        }
        if (m_loops.empty()) {
            m_diagnostics.error(exitStatement->location, "exit statement outside of a loop");
            break;
        }
        if (exitStatement->labelLower.empty()) {
            exitStatement->target = m_loops.back();
        } else {
            for (auto it = m_loops.rbegin(); it != m_loops.rend(); ++it) {
                if ((*it)->labelLower == exitStatement->labelLower) {
                    exitStatement->target = *it;
                    break;
                }
            }
            if (exitStatement->target == nullptr) {
                m_diagnostics.error(exitStatement->location, "unknown loop label '" + exitStatement->label + "'");
                exitStatement->target = m_loops.back();
            }
        }
        break;
    }

    case StmtKind::Return: {
        auto* returnStatement = static_cast<ReturnStmt*>(statement);
        Type* expected = m_currentSubprogram != nullptr ? m_currentSubprogram->returnType : nullptr;
        if (m_currentSubprogram == nullptr) {
            m_diagnostics.error(returnStatement->location, "a return statement must appear within a subprogram");
            break;
        }
        if (!m_extendedReturns.empty() && m_extendedReturns.back() == m_currentSubprogram) {
            if (returnStatement->value != nullptr || returnStatement->m_object != nullptr) {
                m_diagnostics.error(returnStatement->location, "a return inside an extended return cannot have a value");
            }
            break;
        }
        if (returnStatement->m_object != nullptr) {
            auto* object = static_cast<ObjectDecl*>(returnStatement->m_object.get());
            Scope* inner = m_symbolTable.createScope(scope);
            analyzeObjectDecl(object, inner);
            Type* actual = object->symbols.empty() ? nullptr : object->symbols.front()->type;
            if (expected == nullptr || actual == nullptr || !typesCompatible(expected, actual)
                || !needsFinalization(expected) || actual->m_classRoot != nullptr) {
                m_diagnostics.error(returnStatement->location,
                    "extended return requires the specific controlled result subtype");
            }
            if (expected != nullptr && actual != nullptr
                && ((expected->kind == TypeKind::Array && expected->constrained)
                    || (expected->kind == TypeKind::Record && hasKnownDiscriminants(expected)))
                && !SemaSupport::staticallyMatches(actual, expected)) {
                m_diagnostics.error(returnStatement->location, "extended return subtype must statically match the result subtype");
            }
            if (!object->symbols.empty()) {
                object->symbols.front()->m_objectReference = true;
            }
            m_extendedReturns.push_back(m_currentSubprogram);
            analyzeStatements(returnStatement->m_body, inner);
            analyzeHandlers(returnStatement->m_handlers, inner);
            m_extendedReturns.pop_back();
            break;
        }
        if (returnStatement->value) {
            if (baseType(expected) == m_exceptionOccurrenceType) {
                m_diagnostics.error(returnStatement->location, "exception occurrence results are not yet supported");
            }
            Type* valueType = analyzeExpr(returnStatement->value.get(), scope, expected);
            if ((hasLimitedControlledParts(expected) || hasLimitedControlledParts(valueType))
                && !buildsLimitedResult(returnStatement->value.get())) {
                m_diagnostics.error(returnStatement->location, "a limited result must be built in place");
            }
            if (needsFinalization(valueType) && !needsFinalization(expected)) {
                m_diagnostics.error(returnStatement->location, "function results with controlled parts are not yet supported");
            }
            if (expected == nullptr) {
                m_diagnostics.error(returnStatement->location, "a procedure cannot return a value");
            } else if (!typesCompatible(expected, valueType)) {
                m_diagnostics.error(returnStatement->location, "the returned value has an incompatible type");
            }
            adaptUniversal(returnStatement->value.get(), expected);
        } else if (expected != nullptr) {
            m_diagnostics.error(returnStatement->location, "a function must return a value");
        }
        break;
    }

    case StmtKind::Case:
        analyzeCaseStatement(static_cast<CaseStmt*>(statement), scope);
        break;

    case StmtKind::Block: {
        auto* block = static_cast<BlockStmt*>(statement);
        ++m_accessibilityLevel;
        Scope* inner = m_symbolTable.createScope(scope);
        analyzeDeclarativePart(block->declarations, inner);
        reportMissingBodies(block->declarations);
        analyzeStatements(block->body, inner);
        analyzeHandlers(block->handlers, inner);
        --m_accessibilityLevel;
        break;
    }

    case StmtKind::Raise: {
        auto* raise = static_cast<RaiseStmt*>(statement);
        if (raise->lower.empty()) {
            if (m_handlerDepth == 0) {
                m_diagnostics.error(raise->location, "a bare raise must appear within an exception handler, not an enclosed body");
            }
            break;
        }
        Symbol* symbol = lookupName(raise->lower, scope);
        if (symbol == nullptr || symbol->kind != SymbolKind::Exception) {
            m_diagnostics.error(raise->location, "unknown exception '" + raise->name + "'");
            break;
        }
        raise->exceptionSymbol = symbol;
        if (raise->message) {
            Type* messageType = analyzeExpr(raise->message.get(), scope, m_types.stringType());
            if (!typesCompatible(m_types.stringType(), messageType)) {
                m_diagnostics.error(raise->message->location, "an exception message must be of type String");
            }
        }
        break;
    }
    }
}

// Ada asks a case to account for every value its selector can take, each of
// them exactly once.  That is what lets a case compile into a plain choice
// with no run time check behind it and nothing to fall through to.
void Sema::analyzeCaseStatement(CaseStmt* statement, Scope* scope)
{
    Type* selectorType = analyzeExpr(statement->selector.get(), scope, nullptr);
    if (isUniversal(selectorType)) {
        selectorType = m_types.integerType();
        adaptUniversal(statement->selector.get(), selectorType);
    }

    if (selectorType != nullptr && selectorType->m_scalarBoundsSymbol != nullptr) {
        selectorType = m_types.scalarBaseType(selectorType);
    }
    bool discrete = selectorType != nullptr && isDiscrete(selectorType);
    if (selectorType != nullptr && !discrete) {
        m_diagnostics.error(statement->selector->location,
                            "a case selector has to be discrete, and '" + selectorType->name + "' is not");
    }

    // What the choices have accounted for so far, which is both how a value
    // covered twice is noticed and how the gaps are found at the end.
    std::vector<CaseChoice> covered;
    bool sawOthers = false;

    for (CaseAlternative& alternative : statement->alternatives) {
        if (sawOthers) {
            m_diagnostics.error(alternative.location, "'others' has to be the last alternative of a case");
        }
        if (alternative.isOthers) {
            sawOthers = true;
            analyzeStatements(alternative.body, scope);
            continue;
        }

        for (std::size_t i = 0; i < alternative.choiceLows.size(); ++i) {
            CaseChoice choice;
            if (!resolveChoice(alternative.choiceLows[i].get(), alternative.choiceHighs[i].get(), selectorType,
                               covered, scope, choice)) {
                continue;
            }
            covered.push_back(choice);
            alternative.choices.push_back(choice);
        }

        analyzeStatements(alternative.body, scope);
    }

    if (!sawOthers && discrete) {
        reportUncovered(covered, selectorType, statement->location, "case");
    }
}
