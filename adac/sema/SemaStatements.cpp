#include "Sema.h"
#include "SemaSupport.h"

#include <utility>

using SemaSupport::isUniversal;
using SemaSupport::adaptUniversal;

namespace {

Symbol* statementIdentifier(Scope* scope, const std::string& name)
{
    // Statement identifiers are declared in the enclosing body/block, even
    // when the construct is nested inside loops or conditional statements.
    for (; scope != nullptr; scope = scope->parent()) {
        for (Symbol* symbol : scope->lookupLocal(name)) {
            if (symbol->kind == SymbolKind::Statement) { return symbol; }
        }
    }
    return nullptr;
}

} // namespace

void Sema::analyzeStatements(StmtList& statements, Scope* scope)
{
    m_statementSequences.push_back(&statements);
    for (const StmtPtr& statement : statements) {
        analyzeStatement(statement.get(), scope);
    }
    m_statementSequences.pop_back();
}

// Statement identifiers are implicitly declared at the end of the enclosing
// block/body declarative part, including identifiers in branches and loops.
void Sema::declareStatementNames(StmtList& statements, Scope* scope)
{
    auto declare = [&](const std::string& name, const std::string& lower,
                       SourceLocation location, Stmt* target, StmtList* sequence) {
        if (!m_statementIdentifiers.insert(lower).second) {
            m_diagnostics.error(location, "duplicate statement identifier '" + name + "' in the same body");
        }
        if (!scope->lookupLocal(lower).empty()) {
            m_diagnostics.error(location, "statement identifier '" + name + "' conflicts with a declaration");
            return;
        }
        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Statement, lower, name);
        symbol->location = location;
        symbol->statement = target;
        symbol->statementSequence = sequence;
        scope->add(symbol);
    };
    for (const StmtPtr& statement : statements) {
        for (const StatementLabel& label : statement->labels) {
            declare(label.name, label.lower, label.location, statement.get(), &statements);
        }
        switch (statement->kind) {
        case StmtKind::If: {
            auto* conditional = static_cast<IfStmt*>(statement.get());
            for (auto& branch : conditional->branches) {
                declareStatementNames(branch.body, scope);
            }
            declareStatementNames(conditional->elseBody, scope);
            break;
        }
        case StmtKind::Case:
            for (auto& alternative : static_cast<CaseStmt*>(statement.get())->alternatives) {
                declareStatementNames(alternative.body, scope);
            }
            break;
        case StmtKind::Loop: {
            auto* loop = static_cast<LoopStmt*>(statement.get());
            if (!loop->labelLower.empty()) {
                declare(loop->label, loop->labelLower, loop->location, nullptr, nullptr);
            }
            declareStatementNames(loop->body, scope);
            break;
        }
        case StmtKind::Return: {
            auto* result = static_cast<ReturnStmt*>(statement.get());
            declareStatementNames(result->body, scope);
            for (auto& handler : result->handlers) {
                declareStatementNames(handler.body, scope);
            }
            break;
        }
        case StmtKind::Block: {
            auto* block = static_cast<BlockStmt*>(statement.get());
            if (!block->label.empty()) {
                declare(block->label, block->label, block->location, nullptr, nullptr);
            }
            break;
        }
        default:
            break;
        }
    }
}

void Sema::analyzeBodyStatements(StmtList& statements, std::vector<ExceptionHandler>& handlers,
                                 Scope* scope, const std::string& name, Symbol* identifier)
{
    auto savedSequences = std::move(m_statementSequences);
    auto savedIdentifiers = std::move(m_statementIdentifiers);
    auto savedRegions = std::move(m_statementRegions);
    m_statementSequences.clear();
    m_statementIdentifiers.clear();
    m_statementRegions = { { name, scope, identifier } };
    declareStatementNames(statements, scope);
    for (auto& handler : handlers) {
        declareStatementNames(handler.body, scope);
    }
    analyzeStatements(statements, scope);
    analyzeHandlers(handlers, scope);
    m_statementSequences = std::move(savedSequences);
    m_statementIdentifiers = std::move(savedIdentifiers);
    m_statementRegions = std::move(savedRegions);
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

void Sema::checkAssignable(Expr* target, Scope* scope, bool allowLimited, bool allowArrayView)
{
    (void)scope;

    if (target->protectedReceiver != nullptr && target->protectedReceiver->mode == ParameterMode::In) {
        Symbol* field = target->kind == ExprKind::Identifier ? static_cast<IdentifierExpr*>(target)->symbol
            : target->kind == ExprKind::Selected ? static_cast<SelectedExpr*>(target)->symbol : nullptr;
        if (field != nullptr && field->protectedField >= 0) {
            m_diagnostics.error(target->location, "a protected function cannot modify its constant current instance");
            return;
        }
    }
    if (target->implicitCall != nullptr
        || (target->kind == ExprKind::Call && static_cast<CallExpr*>(target)->indirect)) {
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

    // Discriminants can change only as part of a whole-object assignment.
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
        if (selected->recordType != nullptr) { record = selected->recordType; }
        if (selected->fieldIndex >= 0 && record != nullptr
            && record->fields[static_cast<std::size_t>(selected->fieldIndex)].isDiscriminant) {
            m_diagnostics.error(target->location, "'" + selected->selector
                                                      + "' is a discriminant and cannot be assigned individually");
            return;
        }
    }

    switch (target->kind) {
    case ExprKind::Identifier: {
        Symbol* symbol = static_cast<IdentifierExpr*>(target)->symbol;
        if (symbol == nullptr) {
            return;
        }
        if (target->type != nullptr && (target->type->tagged || target->type->kind == TypeKind::Array)
            && symbol->kind == SymbolKind::Subprogram) {
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
            && target->type != nullptr && (target->type->tagged || target->type->kind == TypeKind::Array)) {
            m_diagnostics.error(target->location, "the target of an assignment must be a variable");
            return;
        }
        Type* prefix = baseType(selected->prefix->type);
        if (selected->symbol == nullptr && prefix != nullptr && prefix->kind == TypeKind::Access && prefix->accessConstant) {
            m_diagnostics.error(target->location, "a constant access view cannot be modified");
            return;
        }
        if (selected->symbol == nullptr && prefix != nullptr && prefix->kind != TypeKind::Access) {
            checkAssignable(selected->prefix.get(), scope, true);
        }
        return;
    }
    case ExprKind::Call: {
        auto* call = static_cast<CallExpr*>(target);
        if (call->form == CallForm::Conversion && call->type != nullptr
            && call->type->kind == TypeKind::Array) {
            if (!allowArrayView) {
                m_diagnostics.error(target->location,
                    "an array conversion is writable only as an out or in out actual");
                return;
            }
            Expr* operand = call->resolvedArguments.front();
            if (call->type->aliasedElements && !operand->type->aliasedElements) {
                m_diagnostics.error(target->location, "an aliased array view requires aliased source components");
            }
            if (rootType(call->type) != rootType(operand->type)
                && hasPrivateOrTaggedParts(operand->type)) {
                m_diagnostics.error(target->location,
                    "cross-type array views cannot have private or tagged components");
            }
            checkAssignable(operand, scope, true);
            return;
        }
        if (call->form == CallForm::Conversion && call->type != nullptr && call->type->tagged) {
            checkAssignable(call->resolvedArguments.front(), scope, allowLimited);
            return;
        }
        if (call->form == CallForm::Conversion) {
            m_diagnostics.error(target->location, allowArrayView
                ? "writable untagged non-array conversions are not yet supported"
                : "the target of an assignment must be a variable");
            return;
        }
        if (call->form == CallForm::Indexing || call->form == CallForm::Slice) {
            Type* prefix = baseType(call->callee->type);
            if (prefix != nullptr && prefix->kind == TypeKind::Access && prefix->accessConstant) {
                m_diagnostics.error(target->location, "a constant access view cannot be modified");
                return;
            }
            if (prefix != nullptr && prefix->kind != TypeKind::Access) {
                checkAssignable(call->callee.get(), scope, true);
            }
        } else if (target->type != nullptr
            && (target->type->tagged || target->type->kind == TypeKind::Array)) {
            m_diagnostics.error(target->location, "the target of an assignment must be a variable");
        }
        return;
    }
    default:
        m_diagnostics.error(target->location, "the target of an assignment must be a variable");
        return;
    }
}

Type* Sema::analyzeCondition(Expr* expr, Scope* scope)
{
    Type* context = nullptr;
    for (Type* type : expressionTypes(expr, scope)) {
        if (!m_types.isBoolean(type) || !representationVisible(type)) {
            continue;
        }
        if (context != nullptr && rootType(context) != rootType(type)) {
            m_diagnostics.error(expr->location, "ambiguous Boolean condition");
            return nullptr;
        }
        context = type;
    }
    return analyzeExpr(expr, scope, context != nullptr ? context : m_types.booleanType());
}

void Sema::analyzeStatement(Stmt* statement, Scope* scope)
{
    ScopedCheckPolicy checks(m_suppressedChecks, m_suppressedChecks);
    statement->suppressedChecks = activeCheckPermissions();
    switch (statement->kind) {
    case StmtKind::Pragma:
        analyzePragma(static_cast<PragmaStmt*>(statement)->pragma.get(), scope);
        break;
    case StmtKind::Goto: {
        auto* transfer = static_cast<GotoStmt*>(statement);
        std::vector<Symbol*> found;
        std::size_t dot = transfer->lower.rfind('.');
        if (dot == std::string::npos) {
            found = scope->lookup(transfer->lower);
        } else {
            std::string prefix = transfer->lower.substr(0, dot);
            // Expanded names can include several enclosing named regions;
            // anonymous blocks and loops do not contribute a prefix.
            for (std::size_t end = m_statementRegions.size(); end > 0; --end) {
                if (m_statementRegions[end - 1].name.empty()) {
                    continue;
                }
                std::string expanded;
                for (std::size_t start = end; start > 0; --start) {
                    const auto& region = m_statementRegions[start - 1];
                    if (region.name.empty()) {
                        continue;
                    }
                    expanded = region.name + (expanded.empty() ? "" : "." + expanded);
                    if (expanded != prefix) {
                        continue;
                    }
                    // The first prefix must denote this region, rather than
                    // an object, label or another unit hiding its identifier.
                    auto candidates = scope->lookup(region.name);
                    bool visible = std::find(candidates.begin(), candidates.end(), region.identifier) != candidates.end();
                    if (region.name.find('.') != std::string::npos) {
                        visible = lookupName(region.name, scope) == region.identifier;
                    }
                    if (visible) {
                        found = m_statementRegions[end - 1].scope->lookupLocal(transfer->lower.substr(dot + 1));
                    }
                    break;
                }
                if (!found.empty()) {
                    break;
                }
            }
        }
        Symbol* target = found.size() == 1 ? found.front() : nullptr;
        if (target == nullptr || target->kind != SymbolKind::Statement || target->statement == nullptr) {
            m_diagnostics.error(transfer->location, "goto target '" + transfer->name + "' does not denote a label");
        } else if (std::find(m_statementSequences.begin(), m_statementSequences.end(), target->statementSequence)
                   == m_statementSequences.end()) {
            m_diagnostics.error(transfer->location, "goto target '" + transfer->name
                + "' is not in the same or an enclosing sequence of statements");
        } else {
            transfer->target = target->statement;
        }
        break;
    }
    case StmtKind::Delay: {
        auto* delay = static_cast<DelayStmt*>(statement);
        Type* type = analyzeExpr(delay->expression.get(), scope, delay->until ? nullptr : m_types.durationType());
        if (!delay->until && !typesCompatible(m_types.durationType(), type)) {
            m_diagnostics.error(delay->expression->location, "relative delay requires a Standard.Duration value");
        }
        delay->calendar = m_calendarTimeType != nullptr && rootType(type) == m_calendarTimeType;
        if (delay->until && !delay->calendar && (m_realTimeType == nullptr || rootType(type) != m_realTimeType)) {
            m_diagnostics.error(delay->expression->location, "delay until requires an Ada.Real_Time.Time or Ada.Calendar.Time value");
        }
        break;
    }
    case StmtKind::Null:
        break;

    case StmtKind::Assign: {
        auto* assign = static_cast<AssignStmt*>(statement);
        Type* targetType = analyzeExpr(assign->target.get(), scope, nullptr);
        checkAssignable(assign->target.get(), scope);
        if (abstractView(targetType)) {
            m_diagnostics.error(assign->location, "an assignment target cannot have an abstract type");
        }
        if (hasIncompleteParts(targetType)) {
            m_deferredProtectedUses.push_back({targetType, assign->location});
        }
        if (hasProtected(targetType)) {
            m_diagnostics.error(assign->location, "objects containing protected parts cannot be assigned");
        }
        if (hasTasks(targetType)) {
            m_diagnostics.error(assign->location, "objects containing tasks cannot be assigned");
        }
        if (!hasProtected(targetType) && hasLimitedParts(targetType)) {
            m_diagnostics.error(assign->location, (hasLimitedControlledParts(targetType) ? "limited controlled objects cannot be assigned" : "limited objects cannot be assigned"));
        }
        Type* valueType = analyzeExpr(assign->value.get(), scope, targetType);
        if (targetType != nullptr && targetType->classRoot != nullptr
            && needsFinalization(valueType) && !needsFinalization(targetType)) {
            m_diagnostics.error(assign->location, "class-wide controlled assignment is not yet supported");
        }
        if (!typesCompatible(targetType, valueType)) {
            m_diagnostics.error(assign->location, "the assigned value has an incompatible type");
        }
        if (targetType != nullptr && targetType->classRoot != nullptr
            && valueType != nullptr && valueType->classRoot == nullptr && !assign->value->tagIndeterminate) {
            m_diagnostics.error(assign->location, "class-wide assignment requires a dynamically tagged value");
        }
        if (targetType != nullptr && targetType->classRoot != nullptr && assign->value->tagIndeterminate) {
            dispatchIndeterminate(assign->value.get(), targetType->classRoot);
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
                if (type->accessProfile != nullptr) {
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
        if (expression->implicitCall != nullptr) {
            expression = expression->implicitCall.get();
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
            Type* conditionType = analyzeCondition(branch.condition.get(), scope);
            if (!m_types.isBoolean(conditionType) || !representationVisible(conditionType)) {
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
        Symbol* identifier = statementIdentifier(scope, loop->labelLower);
        if (identifier != nullptr) { identifier->constructScope = inner; }

        if (loop->loopKind == LoopKind::While) {
            Type* conditionType = analyzeCondition(loop->condition.get(), scope);
            if (!m_types.isBoolean(conditionType) || !representationVisible(conditionType)) {
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
                validateStatic(loop->rangeLow.get());
                validateStatic(loop->rangeHigh.get());
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
            Type* conditionType = analyzeCondition(exitStatement->condition.get(), scope);
            if (!m_types.isBoolean(conditionType) || !representationVisible(conditionType)) {
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
        if (m_currentSubprogram != nullptr && m_currentSubprogram->taskBody) {
            m_diagnostics.error(statement->location, "return statements are not allowed in a task body");
        }
        auto* returnStatement = static_cast<ReturnStmt*>(statement);
        Type* expected = m_currentSubprogram != nullptr ? m_currentSubprogram->returnType : nullptr;
        if (m_currentSubprogram == nullptr || m_currentSubprogram == elaborationOwner(m_currentUnit)) {
            m_diagnostics.error(returnStatement->location, "a return statement must appear within a subprogram");
            break;
        }
        if (m_currentSubprogram->returnType == nullptr) { m_currentSubprogram->returnStatements.push_back(returnStatement->location); }
        if (!m_extendedReturns.empty() && m_extendedReturns.back() == m_currentSubprogram) {
            if (returnStatement->value != nullptr || returnStatement->object != nullptr) {
                m_diagnostics.error(returnStatement->location, "a return inside an extended return cannot have a value");
            }
            break;
        }
        if (returnStatement->object != nullptr) {
            auto* object = static_cast<ObjectDecl*>(returnStatement->object.get());
            Scope* inner = m_symbolTable.createScope(scope);
            if (expected != nullptr && expected->anonymousAccess
                != (object->subtype->accessDefinition != nullptr)) {
                m_diagnostics.error(object->location,
                    "extended return must use the same kind of subtype indication as the result");
            }
            bool anonymousResult = expected != nullptr && expected->anonymousAccess
                && object->subtype->accessDefinition != nullptr;
            if (anonymousResult) {
                std::size_t count = m_scalarConstraintDeclaration->anonymousAccessTypes.size();
                Type* actual = resolveSubtypeIndication(object->subtype.get(), inner);
                if (!SemaSupport::staticallyMatches(actual, expected)) {
                    m_diagnostics.error(object->location, "extended return access definition must statically match the result subtype");
                }
                m_scalarConstraintDeclaration->anonymousAccessTypes.resize(count);
                object->subtype->resolved = expected;
            }
            analyzeDecl(object, inner);
            Type* actual = object->symbols.empty() ? nullptr : object->symbols.front()->type;
            if (expected == nullptr || actual == nullptr || !typesCompatible(expected, actual)) {
                m_diagnostics.error(returnStatement->location,
                    "extended return requires a compatible specific result subtype");
            }
            if (expected != nullptr && actual != nullptr
                && (isScalar(expected) || (expected->kind == TypeKind::Array && (expected->constrained || expected->boundsSymbol != nullptr))
                    || expected->kind == TypeKind::Access
                    || (expected->kind == TypeKind::Record && hasKnownDiscriminants(expected)))
                && !SemaSupport::staticallyMatches(actual, expected)) {
                m_diagnostics.error(returnStatement->location, "extended return subtype must statically match the result subtype");
            }
            if (!object->symbols.empty() && expected != nullptr && isComposite(expected)
                && (needsFinalization(expected) || hasLimitedParts(expected) || expected->tagged || hasAccessDiscriminants(expected))) {
                object->symbols.front()->objectReference = true;
            }
            m_extendedReturns.push_back(m_currentSubprogram);
            analyzeStatements(returnStatement->body, inner);
            analyzeHandlers(returnStatement->handlers, inner);
            m_extendedReturns.pop_back();
            break;
        }
        if (returnStatement->value) {
            if (baseType(expected) == m_exceptionOccurrenceType) {
                m_diagnostics.error(returnStatement->location, "exception occurrence results are not yet supported");
            }
            Type* valueType = analyzeExpr(returnStatement->value.get(), scope, expected);
            if ((hasLimitedParts(expected) || hasLimitedParts(valueType))
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
        Symbol* identifier = statementIdentifier(scope, block->label);
        if (identifier != nullptr) { identifier->constructScope = inner; }
        analyzeDeclarativePart(block->declarations, inner);
        reportMissingBodies(block->declarations);
        m_statementRegions.push_back({ block->label, inner, identifier });
        declareStatementNames(block->body, inner);
        for (auto& handler : block->handlers) {
            declareStatementNames(handler.body, inner);
        }
        analyzeStatements(block->body, inner);
        analyzeHandlers(block->handlers, inner);
        m_statementRegions.pop_back();
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
    // RM 5.4(4): the selector expects any discrete type. Discover its
    // interpretations before binding; choices get the resolved type later
    // and must not be used to choose between discrete interpretations.
    Type* selectorContext = nullptr;
    for (Type* candidate : expressionTypes(statement->selector.get(), scope)) {
        if (!isDiscrete(candidate) || !representationVisible(candidate)) { continue; }
        if (selectorContext != nullptr && rootType(selectorContext) != rootType(candidate)) {
            m_diagnostics.error(statement->selector->location, "ambiguous case selector: multiple discrete types match");
            return;
        }
        if (selectorContext == nullptr) { selectorContext = candidate; }
    }
    Type* selectorType = analyzeExpr(statement->selector.get(), scope, selectorContext);
    if (selectorType != nullptr && selectorType->kind == TypeKind::UniversalInteger) {
        selectorType = m_types.integerType();
        adaptUniversal(statement->selector.get(), selectorType);
        validateStatic(statement->selector.get());
    }

    if (selectorType != nullptr && selectorType->scalarBoundsSymbol != nullptr) {
        selectorType = m_types.scalarBaseType(selectorType);
    }
    bool discrete = selectorType != nullptr && isDiscrete(selectorType) && representationVisible(selectorType);
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
