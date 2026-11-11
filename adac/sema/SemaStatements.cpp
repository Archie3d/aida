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
        case StmtKind::Select: {
            auto* select = static_cast<SelectStmt*>(statement.get());
            for (auto& alternative : select->alternatives) { declareStatementNames(alternative.body, scope); }
            declareStatementNames(select->elseBody, scope);
            declareStatementNames(select->abortBody, scope);
            break;
        }
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
    case StmtKind::Requeue: {
        auto* requeue = static_cast<RequeueStmt*>(statement);
        Symbol* source = nullptr;
        if (!m_accepts.empty() && m_accepts.back()->owner == m_currentSubprogram) {
            source = m_accepts.back()->entry;
            requeue->parameters = m_accepts.back()->parameters;
        } else if (m_currentSubprogram != nullptr && m_currentSubprogram->entry
            && m_currentSubprogram->protectedType != nullptr) {
            source = m_currentSubprogram;
            requeue->parameters = source->parameters;
        }
        if (source == nullptr) {
            m_diagnostics.error(requeue->location, "requeue requires an enclosing accept or protected entry body, without an intervening callable body");
            break;
        }
        Symbol* target = nullptr;
        for (Symbol* candidate : expressionNames(requeue->target.get(), scope)) {
            if (!candidate->entry) { continue; }
            bool matches = candidate->parameters.empty();
            if (!matches && candidate->parameters.size() == requeue->parameters.size()) {
                matches = true;
                for (std::size_t i = 0; i < candidate->parameters.size(); ++i) {
                    matches &= SemaSupport::profileTypeMatches(candidate->parameters[i]->type, requeue->parameters[i]->type)
                        && candidate->parameters[i]->mode == requeue->parameters[i]->mode;
                }
            }
            if (!matches) { continue; }
            if (target != nullptr) {
                m_diagnostics.error(requeue->location, "ambiguous requeue target entry");
                target = nullptr;
                break;
            }
            target = candidate;
        }
        if (target == nullptr) {
            m_diagnostics.error(requeue->location, "requeue target must be a parameterless entry or have a type-conformant profile");
            break;
        }
        auto call = std::make_unique<CallExpr>();
        call->location = requeue->location;
        call->form = CallForm::Subprogram;
        call->subprogram = target;
        call->callee = std::move(requeue->target);
        for (std::size_t i = 0; i < target->parameters.size(); ++i) {
            Symbol* formal = requeue->parameters[i];
            if (!SemaSupport::staticallyMatches(target->parameters[i]->type, formal->type)
                || target->parameters[i]->explicitNullExclusion != formal->explicitNullExclusion) {
                m_diagnostics.error(requeue->location, "requeue target profile must be subtype conformant with the enclosing entry");
            }
            auto name = std::make_unique<IdentifierExpr>();
            name->location = requeue->location;
            name->name = formal->displayName;
            name->lower = formal->name;
            name->symbol = formal;
            name->type = formal->type;
            noteReference(formal);
            call->resolvedArguments.push_back(name.get());
            Association argument;
            argument.value = std::move(name);
            call->arguments.push_back(std::move(argument));
        }
        Expr* named = call->callee.get();
        if (named->kind == ExprKind::Call) { named = static_cast<CallExpr*>(named)->callee.get(); }
        if (named->kind == ExprKind::Call) {
            m_diagnostics.error(requeue->location, "requeue has no actual parameter part");
        }
        Expr* object = named->kind == ExprKind::Selected ? static_cast<SelectedExpr*>(named)->prefix.get() : nullptr;
        bindProtectedCall(call.get(), target, object, scope, true);
        bindTaskEntryCall(call.get(), target, scope);
        requeue->internal = target->protectedType != nullptr && call->protectedReceiver != nullptr;
        object = call->taskObject != nullptr ? call->taskObject : call->protectedObject;
        if (object != nullptr) {
            Expr* root = object;
            int level = m_accessibilityLevel;
            bool throughAccess = false;
            for (;;) {
                Type* type = baseType(root->type);
                if (!throughAccess && type != nullptr && type->kind == TypeKind::Access) {
                    // Explicit/implicit dereferences use the access type's
                    // level, rather than the local access variable's level.
                    level = type->accessLevel;
                    throughAccess = true;
                }
                if (root->kind == ExprKind::Selected) { root = static_cast<SelectedExpr*>(root)->prefix.get(); }
                else if (root->kind == ExprKind::Call && static_cast<CallExpr*>(root)->form == CallForm::Indexing) {
                    root = static_cast<CallExpr*>(root)->callee.get();
                } else { break; }
            }
            Symbol* origin = root->kind == ExprKind::Identifier ? static_cast<IdentifierExpr*>(root)->symbol : nullptr;
            bool parameter = !throughAccess && origin != nullptr
                && std::find(requeue->parameters.begin(), requeue->parameters.end(), origin) != requeue->parameters.end();
            if (!throughAccess && origin != nullptr) { level = origin->accessibilityLevel; }
            if (!parameter) {
                bool deep = source->protectedType != nullptr && level > source->accessibilityLevel;
                for (AcceptStmt* accept : m_accepts) {
                    if (accept->owner == m_currentSubprogram && level >= accept->accessibilityLevel) { deep = true; }
                }
                if (deep) { m_diagnostics.error(requeue->location, "requeue target object is too deeply nested for the enclosing entry"); }
            }
        }
        requeue->call = std::move(call);
        break;
    }
    case StmtKind::Abort: {
        auto* abort = static_cast<AbortStmt*>(statement);
        for (auto& name : abort->names) {
            Type* task = baseType(analyzeExpr(name.get(), scope, nullptr));
            Symbol* receiver = nullptr;
            if (task == nullptr || task->taskScope == nullptr) {
                m_diagnostics.error(name->location, "an abort statement requires a task object");
            } else if (isTaskCurrentInstance(name.get(), task, scope)) {
                for (Symbol* owner = m_currentSubprogram; owner != nullptr; owner = owner->owner) {
                    if (owner->taskBody && owner->taskType == task) { receiver = owner->taskIdentity; noteReference(receiver); break; }
                }
            } else {
                auto names = expressionNames(name.get(), scope);
                if (names.size() == 1 && names.front()->kind == SymbolKind::TypeName) {
                    m_diagnostics.error(name->location, "an abort statement requires a task object, not a type name");
                }
            }
            abort->receivers.push_back(receiver);
        }
        break;
    }
    case StmtKind::Select: {
        auto* select = static_cast<SelectStmt*>(statement);
        int accepts = 0, delays = 0, terminates = 0;
        DelayStmt* firstDelay = nullptr;
        for (auto& alternative : select->alternatives) {
            if (alternative.guard != nullptr) {
                Type* guard = analyzeCondition(alternative.guard.get(), scope);
                if (!m_types.isBoolean(guard)) { m_diagnostics.error(alternative.guard->location, "a select guard requires a Boolean condition"); }
                if (select->form != SelectForm::Selective) {
                    m_diagnostics.error(alternative.guard->location, "guards are allowed only on selective accept alternatives");
                }
            }
            Stmt* operation = alternative.operation.get();
            if (operation->kind == StmtKind::Terminate) {
                ++terminates;
                if (!alternative.body.empty()) { m_diagnostics.error(operation->location, "a terminate alternative cannot have following statements"); }
            } else {
                analyzeStatement(operation, scope);
                if (operation->kind == StmtKind::Accept) { ++accepts; }
                else if (operation->kind == StmtKind::Delay) {
                    ++delays;
                    auto* delay = static_cast<DelayStmt*>(operation);
                    if (firstDelay != nullptr && (firstDelay->until != delay->until
                        || (delay->until && firstDelay->calendar != delay->calendar))) {
                        m_diagnostics.error(delay->location, "selective accept delays must use the same relative or absolute time form");
                    }
                    if (firstDelay == nullptr) { firstDelay = delay; }
                } else if (operation->kind != StmtKind::ProcedureCall) {
                    m_diagnostics.error(operation->location, "a select alternative requires an accept, delay, terminate or entry call");
                }
            }
            analyzeStatements(alternative.body, scope);
        }
        if (select->form == SelectForm::Selective) {
            if (m_currentSubprogram == nullptr || !m_currentSubprogram->taskBody || accepts == 0) {
                m_diagnostics.error(select->location, "a selective accept requires an accept alternative in the enclosing task body");
            }
            if (terminates > 1 || (terminates != 0 && (delays != 0 || select->hasElse))
                || (delays != 0 && select->hasElse)) {
                m_diagnostics.error(select->location, "terminate, delay and else alternatives are mutually exclusive in a selective accept");
            }
            for (auto& alternative : select->alternatives) {
                auto kind = alternative.operation->kind;
                if (kind != StmtKind::Accept && kind != StmtKind::Delay && kind != StmtKind::Terminate) {
                    m_diagnostics.error(alternative.operation->location, "a selective accept cannot contain an entry-call alternative");
                }
            }
        } else {
            Stmt* first = select->alternatives.front().operation.get();
            bool asynchronous = select->form == SelectForm::Asynchronous;
            if ((asynchronous && select->alternatives.size() != 1)
                || (select->form == SelectForm::Conditional && select->alternatives.size() != 1)
                || (select->form == SelectForm::Timed && (select->alternatives.size() != 2
                    || select->alternatives.back().operation->kind != StmtKind::Delay))
                || (first->kind != StmtKind::ProcedureCall && !(asynchronous && first->kind == StmtKind::Delay))) {
                m_diagnostics.error(select->location, "invalid timed, conditional or asynchronous select alternatives");
            }
            if (first->kind == StmtKind::ProcedureCall) {
                Expr* call = static_cast<ProcedureCallStmt*>(first)->call.get();
                if (call->implicitCall != nullptr) { call = call->implicitCall.get(); }
                Symbol* entry = call->kind == ExprKind::Call ? static_cast<CallExpr*>(call)->subprogram
                    : call->kind == ExprKind::Selected ? static_cast<SelectedExpr*>(call)->symbol
                    : call->kind == ExprKind::Identifier ? static_cast<IdentifierExpr*>(call)->symbol : nullptr;
                while (entry != nullptr && entry->renamedSubprogram != nullptr) { entry = entry->renamedSubprogram; }
                bool interfaceCall = entry != nullptr && entry->returnType == nullptr && entry->controllingType != nullptr
                    && entry->controllingType->interfaceKind >= InterfaceKind::Limited
                    && call->kind == ExprKind::Call && static_cast<CallExpr*>(call)->dispatching;
                bool implementedEntry = entry != nullptr && entry->concurrentImplementation != nullptr
                    && entry->concurrentImplementation->entry;
                if (entry == nullptr || (!entry->entry && !interfaceCall && !implementedEntry)) {
                    m_diagnostics.error(call->location, "this select form requires an entry call or a supported entry callable view");
                }
            }
        }
        analyzeStatements(select->elseBody, scope);
        if (select->form == SelectForm::Asynchronous) { m_asyncSelects.push_back(m_currentSubprogram); }
        analyzeStatements(select->abortBody, scope);
        if (select->form == SelectForm::Asynchronous) { m_asyncSelects.pop_back(); }
        break;
    }
    case StmtKind::Terminate:
        m_diagnostics.error(statement->location, "terminate is allowed only as a selective accept alternative");
        break;
    case StmtKind::Accept: {
        auto* accept = static_cast<AcceptStmt*>(statement);
        accept->owner = m_currentSubprogram;
        if (std::find(m_asyncSelects.begin(), m_asyncSelects.end(), m_currentSubprogram) != m_asyncSelects.end()) {
            m_diagnostics.error(accept->location, "an accept statement cannot occur in an abortable part of its task body");
        }
        if (m_currentSubprogram == nullptr || !m_currentSubprogram->taskBody) {
            m_diagnostics.error(accept->location, "an accept statement must occur in a task body");
            break;
        }
        Type* task = m_currentSubprogram->taskType;
        Scope* inner = m_symbolTable.createScope(scope);
        Symbol* profile = declareSubprogram(accept->spec, inner, false);
        profile->hasBody = true;
        accept->parameters = profile->parameters;
        Symbol* chosen = nullptr;
        for (Symbol* entry : task->taskEntries) {
            if (entry->name != accept->spec.lower || entry->parameters.size() != profile->parameters.size()) { continue; }
            bool matches = (entry->entryIndexType != nullptr) == (accept->index != nullptr);
            for (std::size_t i = 0; i < entry->parameters.size(); ++i) {
                Symbol* a = entry->parameters[i];
                Symbol* b = profile->parameters[i];
                matches &= a->name == b->name && a->mode == b->mode
                    && a->explicitNullExclusion == b->explicitNullExclusion
                    && SemaSupport::staticallyMatches(a->type, b->type);
            }
            if (!matches) { continue; }
            if (chosen != nullptr) {
                m_diagnostics.error(accept->location, "ambiguous accept entry profile");
                break;
            }
            chosen = entry;
        }
        if (chosen == nullptr) {
            m_diagnostics.error(accept->location, "accept profile must fully conform to an entry of the enclosing task");
            break;
        }
        accept->entry = chosen;
        for (AcceptStmt* active : m_accepts) {
            if (active->entry == chosen && active->owner == m_currentSubprogram) {
                m_diagnostics.error(accept->location, "an accept statement cannot nest another accept for the same entry");
            }
        }
        if (accept->index != nullptr) {
            Type* index = analyzeExpr(accept->index.get(), scope, chosen->entryIndexType);
            if (!typesCompatible(chosen->entryIndexType, index)) {
                m_diagnostics.error(accept->index->location, "accept entry index has an incompatible type");
            }
            adaptUniversal(accept->index.get(), chosen->entryIndexType);
        }
        for (std::size_t i = 0; i < profile->parameters.size(); ++i) {
            Symbol* parameter = profile->parameters[i];
            parameter->byReference = chosen->parameters[i]->byReference;
            parameter->owner = m_currentSubprogram;
            parameter->level = m_currentSubprogram->level;
            parameter->isUplevel = true;
            inner->add(parameter);
            accept->spec.parameters[i].symbol = parameter;
            if (accept->spec.parameters[i].defaultValue != nullptr) {
                Expr* value = accept->spec.parameters[i].defaultValue.get();
                analyzeExpr(value, scope, parameter->type);
                if (!SemaSupport::conformingExpressions(chosen->parameters[i]->defaultExpr, value)) {
                    m_diagnostics.error(accept->spec.parameters[i].location, "accept default must conform to its entry declaration");
                }
            }
        }
        profile->constructScope = inner;
        // The local entry alias supplies Entry_Name.Parameter expanded names.
        profile->entry = true;
        profile->taskType = chosen->taskType;
        profile->taskEntryId = chosen->taskEntryId;
        profile->entryIndexType = chosen->entryIndexType;
        Symbol* caller = m_symbolTable.createSymbol(SymbolKind::Object, "__entry_caller", "entry caller");
        caller->type = m_taskIdType;
        caller->owner = m_currentSubprogram;
        caller->level = m_currentSubprogram->level;
        caller->isConstant = true;
        caller->isUplevel = true;
        accept->callerIdentity = caller;
        accept->accessibilityLevel = ++m_accessibilityLevel;
        m_accepts.push_back(accept);
        declareStatementNames(accept->body, inner);
        analyzeStatements(accept->body, inner);
        analyzeHandlers(accept->handlers, inner);
        m_accepts.pop_back();
        --m_accessibilityLevel;
        break;
    }
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
        if (!delay->until) { adaptUniversal(delay->expression.get(), m_types.durationType()); }
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
        if (!hasProtected(targetType) && !hasTasks(targetType) && hasLimitedParts(targetType)) {
            m_diagnostics.error(assign->location, (hasLimitedControlledParts(targetType) ? "limited controlled objects cannot be assigned" : "limited objects cannot be assigned"));
        }
        Type* valueType = analyzeExpr(assign->value.get(), scope, targetType);
        if (targetType != nullptr && targetType->classRoot != nullptr
            && needsFinalization(valueType) && !needsFinalization(targetType)) {
            m_diagnostics.error(assign->location, "class-wide controlled assignment is not yet supported");
        }
        if (!typesCompatible(targetType, valueType)) {
            m_diagnostics.error(assign->value->location, "the assigned value has an incompatible type"
                + SemaSupport::expectedActualTypes(targetType, valueType));
            if (assign->target->kind == ExprKind::Identifier) {
                Symbol* target = static_cast<IdentifierExpr*>(assign->target.get())->symbol;
                if (target != nullptr) { m_diagnostics.note(target->location, "assignment target is declared here"); }
            }
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
        auto* invocation = static_cast<ProcedureCallStmt*>(statement);
        if (invocation->call->kind == ExprKind::Identifier || invocation->call->kind == ExprKind::Selected) {
            auto names = expressionNames(invocation->call.get(), scope);
            if (std::any_of(names.begin(), names.end(), [](Symbol* symbol) { return symbol->entry && symbol->taskType != nullptr; })) {
                auto call = std::make_unique<CallExpr>();
                call->location = invocation->call->location;
                call->callee = std::move(invocation->call);
                invocation->call = std::move(call);
            }
        }
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
        // Storage ownership can remain with an enclosing callable while a
        // nested package elaborates. The innermost body still governs return.
        Symbol* body = m_statementRegions.empty() ? nullptr : m_statementRegions.front().identifier;
        if (body != nullptr && body->kind == SymbolKind::Package) {
            m_diagnostics.error(statement->location, "a return statement is not allowed in a package body");
            break;
        }
        if (m_currentSubprogram != nullptr && m_currentSubprogram->taskBody
            && (m_accepts.empty() || m_accepts.back()->owner != m_currentSubprogram)) {
            m_diagnostics.error(statement->location, "return statements are not allowed in a task body");
            break;
        }
        auto* returnStatement = static_cast<ReturnStmt*>(statement);
        Type* expected = m_currentSubprogram != nullptr ? m_currentSubprogram->returnType : nullptr;
        if (m_currentSubprogram == nullptr || m_currentSubprogram == elaborationOwner(m_currentUnit)) {
            m_diagnostics.error(returnStatement->location, "a return statement must appear within a subprogram");
            break;
        }
        if (m_currentSubprogram->returnType == nullptr) { m_currentSubprogram->returnStatements.push_back(returnStatement->location); }
        if (!m_extendedReturns.empty() && m_extendedReturns.back() == m_currentSubprogram) {
            if (returnStatement->object != nullptr) {
                m_diagnostics.error(returnStatement->location,
                    "an extended return cannot apply to an enclosing extended return");
            } else if (returnStatement->value != nullptr) {
                m_diagnostics.error(returnStatement->location, "a return inside an extended return cannot have a value");
            }
            break;
        }
        bool acceptReturn = !m_accepts.empty() && m_accepts.back()->owner == m_currentSubprogram;
        if (returnStatement->object != nullptr) {
            if (expected == nullptr) {
                const char* target = acceptReturn ? "an accept statement"
                    : m_currentSubprogram->entry ? "an entry body" : "a procedure body";
                m_diagnostics.error(returnStatement->location,
                    std::string("an extended return cannot apply to ") + target);
            }
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
            if (expected != nullptr && (actual == nullptr || !typesCompatible(expected, actual))) {
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
            Type* valueType = analyzeExpr(returnStatement->value.get(), scope, expected);
            if ((hasLimitedParts(expected) || hasLimitedParts(valueType))
                && !buildsLimitedResult(returnStatement->value.get())) {
                m_diagnostics.error(returnStatement->location, "a limited result must be built in place");
            }
            if (needsFinalization(valueType) && !needsFinalization(expected)) {
                m_diagnostics.error(returnStatement->location, "function results with controlled parts are not yet supported");
            }
            if (expected == nullptr) {
                m_diagnostics.error(returnStatement->location, acceptReturn ? "an accept statement cannot return a value"
                    : m_currentSubprogram->entry ? "an entry body cannot return a value"
                    : "a procedure cannot return a value");
            } else if (!typesCompatible(expected, valueType)) {
                m_diagnostics.error(returnStatement->value->location, "the returned value has an incompatible type"
                    + SemaSupport::expectedActualTypes(expected, valueType));
                m_diagnostics.note(m_currentSubprogram->location, "function is declared here");
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
