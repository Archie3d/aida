#include "Sema.h"
#include "Parser.h"
#include "SemaSupport.h"

#include <algorithm>

namespace
{

Symbol* parameterBinding(Scope* scope, const GenericFormal& parameter)
{
    auto symbols = scope->lookupLocal(parameter.lower);
    if (symbols.size() == 1) { return symbols.front(); }
    SourceLocation location = parameter.kind == GenericFormalKind::SubprogramFormal
        ? parameter.subprogramTokens.front().location : parameter.location;
    for (Symbol* symbol : symbols) {
        if (symbol->location.file == location.file && symbol->location.line == location.line
            && symbol->location.column == location.column) { return symbol; }
    }
    return nullptr;
}

Symbol* packageEntity(Symbol* symbol)
{
    while (symbol != nullptr && symbol->renamedUnit != nullptr) { symbol = symbol->renamedUnit; }
    return symbol;
}

Symbol* actualEntity(Symbol* symbol)
{
    for (;;) {
        Symbol* next = symbol == nullptr ? nullptr : symbol->genericActualEntity != nullptr
            ? symbol->genericActualEntity : symbol->renamedSubprogram;
        if (next == nullptr || next == symbol) { return packageEntity(symbol); }
        symbol = next;
    }
}

Symbol* namedEntity(Expr* expression)
{
    if (expression == nullptr) { return nullptr; }
    if (expression->kind == ExprKind::Identifier) { return static_cast<IdentifierExpr*>(expression)->symbol; }
    if (expression->kind == ExprKind::Selected) { return static_cast<SelectedExpr*>(expression)->symbol; }
    return nullptr;
}

Expr* actualExpression(Expr* expression)
{
    for (int depth = 0; depth < 64; ++depth) {
        Symbol* symbol = namedEntity(expression);
        Expr* next = symbol == nullptr ? nullptr : symbol->genericObject ? symbol->genericActualExpression
            : symbol->objectReference ? symbol->sharedAlias : nullptr;
        if (next == nullptr || next == expression) { break; }
        expression = next;
    }
    return expression;
}

bool staticString(Expr* expression, std::string& value, int depth = 0)
{
    if (expression == nullptr || depth > 64 || expression->staticFailure != StaticFailure::None) { return false; }
    if (expression->kind == ExprKind::StringLiteral) {
        value = static_cast<StringLiteralExpr*>(expression)->value;
        return true;
    }
    if (expression->kind == ExprKind::CharacterLiteral) {
        value.assign(1, static_cast<CharacterLiteralExpr*>(expression)->value);
        return true;
    }
    Type* type = expression->type;
    if (type == nullptr || type->kind != TypeKind::Array || type->boundsSymbol != nullptr
        || type->index == nullptr || type->index->scalarBoundsSymbol != nullptr || type->index->genericFormal) { return false; }
    if (Symbol* symbol = namedEntity(expression)) {
        return symbol->isConstant && staticString(symbol->constantInitializer, value, depth + 1);
    }
    if (!type->constrained) { return false; }
    if (expression->kind == ExprKind::Qualified) {
        return staticString(static_cast<QualifiedExpr*>(expression)->operand.get(), value, depth + 1);
    }
    if (expression->kind == ExprKind::Binary) {
        auto* binary = static_cast<BinaryExpr*>(expression);
        std::string right;
        if (binary->op != BinaryOp::Concatenate || binary->operatorCall != nullptr
            || !staticString(binary->left.get(), value, depth + 1) || !staticString(binary->right.get(), right, depth + 1)) { return false; }
        value += right;
        return true;
    }
    if (expression->kind == ExprKind::Call) {
        auto* call = static_cast<CallExpr*>(expression);
        if (call->form == CallForm::Conversion && call->arguments.size() == 1) {
            return staticString(call->arguments.front().value.get(), value, depth + 1);
        }
        if (call->form == CallForm::Slice && call->arguments.size() == 1) {
            Expr* low = call->arguments.front().value.get();
            Expr* high = call->arguments.front().high.get();
            if (low == nullptr || high == nullptr || !low->isStatic || !high->isStatic
                || !staticString(call->callee.get(), value, depth + 1)) { return false; }
            long long first = call->callee->type->indexLow;
            if (low->staticValue > high->staticValue) { value.clear(); return true; }
            if (low->staticValue < first || high->staticValue - first >= static_cast<long long>(value.size())) { return false; }
            value = value.substr(static_cast<std::size_t>(low->staticValue - first),
                                 static_cast<std::size_t>(high->staticValue - low->staticValue + 1));
            return true;
        }
    }
    return false;
}

}

void Sema::applyGenericUses(GenericDecl* generic, std::size_t beforeFormal, Scope* scope, DeclList& owned)
{
    for (const GenericDecl::UseClause& use : generic->formalUses) {
        if (use.beforeFormal != beforeFormal) { continue; }
        Parser parser(use.tokens, m_diagnostics);
        DeclList declarations = parser.parseDeclarations();
        analyzeDeclarativePart(declarations, scope);
        for (DeclPtr& declaration : declarations) { owned.push_back(std::move(declaration)); }
    }
}

Symbol* Sema::declareFormalPackage(const GenericFormal& formal, Scope* bindings, DeclList& owned)
{
    Parser parser(formal.packageTokens, m_diagnostics);
    DeclList declarations;
    declarations.push_back(parser.parseFormalPackageInstantiation());
    auto* instance = static_cast<GenericInstantiationDecl*>(declarations.front().get());
    Symbol* generic = packageEntity(lookupName(instance->genericLower, bindings));
    if (generic == nullptr || generic->kind != SymbolKind::Generic || !generic->generic->isPackage) {
        m_diagnostics.error(formal.location, "a formal package template must denote a generic package");
        return nullptr;
    }
    auto source = m_genericContracts.find(generic->generic);
    if (source == m_genericContracts.end() || !source->second->valid) { return nullptr; }
    if (m_instantiationDepth > 16) {
        m_diagnostics.error(formal.location, "formal package contracts nest too deeply");
        return nullptr;
    }
    if (!bindings->lookupLocal(formal.lower).empty()) {
        m_diagnostics.error(formal.location, "'" + formal.name + "' has already been declared in this scope");
        return nullptr;
    }
    instance->packageOthers = instance->packageOthers || formal.packageBox;
    std::vector<Expr*> actuals;
    if (!associateGenericActuals(instance, generic, actuals)) { return nullptr; }
    bool partial = std::any_of(instance->packageMatches.begin(), instance->packageMatches.end(), [](bool match) { return !match; });
    if (!partial) {
        analyzeGenericInstantiation(instance, bindings);
        Symbol* view = instance->symbol;
        if (view != nullptr) { view->formalPackage = true; view->packageMatches = instance->packageMatches; }
        owned.push_back(std::move(declarations.front()));
        return view;
    }

    // Every partial view gets its own formal types and checked package
    // specification. Sharing the template's contract would equate P.T and Q.T.
    Parser cloneParser(generic->generic->specificationTokens, m_diagnostics);
    DeclList clones = cloneParser.parseDeclarations();
    if (clones.size() != 1 || clones.front()->kind != DeclKind::GenericDeclaration) { return nullptr; }
    auto* clone = static_cast<GenericDecl*>(clones.front().get());
    clone->symbol = m_symbolTable.createSymbol(SymbolKind::Generic, clone->lower, clone->name);
    clone->symbol->generic = clone;
    ++m_instantiationDepth;
    checkGenericContract(clone, source->second->environment, instance, generic, bindings);
    --m_instantiationDepth;
    auto* contract = m_genericContracts.at(clone).get();
    Symbol* specification = nullptr;
    for (Symbol* candidate : contract->bindings->lookupLocal(contract->unitName)) {
        if (candidate->kind == SymbolKind::Package) { specification = candidate; break; }
    }
    if (specification == nullptr || !contract->valid) {
        owned.push_back(std::move(clones.front()));
        owned.push_back(std::move(declarations.front()));
        return nullptr;
    }
    Symbol* view = m_symbolTable.createSymbol(SymbolKind::Package, formal.lower, formal.name);
    view->location = formal.location;
    view->scope = m_symbolTable.createScope(bindings, specification->scope);
    view->instanceTemplate = generic;
    view->instanceBindings = contract->bindings;
    view->instanceSpecification = specification;
    view->packageMatches = instance->packageMatches;
    view->formalPackage = true;
    view->hasSpecification = true;
    // RM 12.7(10): expose only declarations not required to match.
    for (std::size_t i = 0; i < clone->formals.size(); ++i) {
        if (view->packageMatches[i]) { continue; }
        if (Symbol* symbol = parameterBinding(contract->bindings, clone->formals[i])) {
            view->scope->add(symbol);
            view->exposedFormals.push_back(symbol);
            if (symbol->kind == SymbolKind::Subprogram && symbol->name == "=") {
                for (Symbol* complement : contract->bindings->lookupLocal("/=")) {
                    if (complement->negatedEquality == symbol) {
                        view->scope->add(complement);
                        view->exposedFormals.push_back(complement);
                    }
                }
            }
            if (symbol->kind == SymbolKind::TypeName) {
                for (Symbol* primitive : symbol->type->primitives) {
                    auto local = contract->bindings->lookupLocal(primitive->name);
                    if (std::find(local.begin(), local.end(), primitive) == local.end()) { continue; }
                    auto present = view->scope->lookupLocal(primitive->name);
                    if (std::find(present.begin(), present.end(), primitive) == present.end()) {
                        view->scope->add(primitive);
                        view->exposedFormals.push_back(primitive);
                    }
                }
            }
        }
    }
    bindings->add(view);
    owned.push_back(std::move(clones.front()));
    owned.push_back(std::move(declarations.front()));
    return view;
}

void Sema::mapPackageMembers(Symbol* formal, Symbol* actual, std::unordered_map<Symbol*, Symbol*>& copies)
{
    if (auto found = copies.find(formal); found != copies.end() && found->second == actual) { return; }
    copies[formal] = actual;
    if (formal->scope == nullptr || actual->scope == nullptr) { return; }
    for (const auto& [name, originals] : formal->scope->declarations()) {
        auto candidates = actual->scope->lookupLocal(name);
        std::unordered_set<Symbol*> used;
        for (Symbol* original : originals) {
            for (Symbol* candidate : candidates) {
                if (used.contains(candidate) || original->kind != candidate->kind
                    || contractKey(original->location) != contractKey(candidate->location)) { continue; }
                used.insert(candidate);
                if (original->kind == SymbolKind::Package) { mapPackageMembers(original, candidate, copies); }
                else { copies[original] = candidate; }
                for (std::size_t i = 0; i < std::min(original->parameters.size(), candidate->parameters.size()); ++i) {
                    copies[original->parameters[i]] = candidate->parameters[i];
                }
                break;
            }
        }
    }
}

bool Sema::bindFormalPackage(GenericInstantiationDecl* decl, const GenericFormal& formal, Expr* actual,
                              Scope* bindings, Scope* scope, GenericContract* contract, std::size_t firstSymbol)
{
    Symbol* supplied = nullptr;
    for (Symbol* candidate : expressionNames(actual, scope)) {
        if (candidate->kind == SymbolKind::Package && supplied == nullptr) { supplied = packageEntity(candidate); }
        else { supplied = nullptr; break; }
    }
    if (formal.packageView == nullptr) { return false; }
    Symbol* expectedTemplate = formal.packageView->instanceTemplate;
    auto* savedReplay = m_replayContract;
    auto* savedRecord = m_recordContract;
    std::size_t savedFirst = m_instanceFirstSymbol;
    m_replayContract = contract;
    m_recordContract = nullptr;
    m_instanceFirstSymbol = firstSymbol;
    auto savedCopies = m_instanceCopies;
    m_instanceCopies.insert(decl->packageCopies.begin(), decl->packageCopies.end());
    expectedTemplate = instanceSymbol(expectedTemplate);
    m_instanceCopies = std::move(savedCopies);
    m_replayContract = savedReplay;
    m_recordContract = savedRecord;
    m_instanceFirstSymbol = savedFirst;
    if (supplied == nullptr || supplied->instanceTemplate == nullptr || supplied->instanceTemplate != expectedTemplate) {
        m_diagnostics.error(actual->location, "actual for formal package '" + formal.name + "' must be an instance of its generic template");
        return false;
    }
    if (std::any_of(formal.packageView->packageMatches.begin(), formal.packageView->packageMatches.end(), [](bool match) { return match; })) {
        Scope* matching = m_symbolTable.createScope(bindings);
        // Rebind the complete list/defaults to preceding outer actuals. This
        // specification is for matching only and emits no new package instance.
        Symbol* expected = declareFormalPackage(formal, matching, decl->formalTypes);
        if (expected == nullptr) { return false; }
        bool matches = true;
        std::size_t index = 0;
        for (const GenericFormal& parameter : expectedTemplate->generic->formals) {
            if (!expected->packageMatches[index++]) { continue; }
            Symbol* a = parameterBinding(supplied->instanceBindings, parameter);
            Symbol* b = parameterBinding(expected->instanceBindings, parameter);
            if (a == nullptr || b == nullptr) { matches = false; break; }
            bool same = false;
            if (parameter.kind == GenericFormalKind::TypeFormal) {
                same = SemaSupport::staticallyMatches(a->type, b->type);
            } else if (parameter.kind == GenericFormalKind::ObjectFormal && parameter.mode == ParameterMode::In) {
                Expr* left = actualExpression(a->genericActualExpression);
                Expr* right = actualExpression(b->genericActualExpression);
                Symbol* leftName = left != nullptr ? namedEntity(left) : a->genericObject && a->isConstant ? a : nullptr;
                Symbol* rightName = right != nullptr ? namedEntity(right) : b->genericObject && b->isConstant ? b : nullptr;
                same = leftName != nullptr && leftName == rightName && leftName->isConstant;
                if (left != nullptr && right != nullptr) {
                    same = same || (left->kind == ExprKind::Null && right->kind == ExprKind::Null);
                    std::string leftString, rightString;
                    if (m_types.isString(left->type) && m_types.isString(right->type)
                        && staticString(left, leftString) && staticString(right, rightString)) {
                        same = leftString == rightString;
                    }
                    if (left->isStatic && right->isStatic && left->staticFailure == StaticFailure::None
                        && right->staticFailure == StaticFailure::None) {
                        if (isReal(left->type)) { same = left->staticReal == right->staticReal; }
                        else if (isScalar(left->type)) { same = left->staticValue == right->staticValue; }
                        else { same = SemaSupport::conformingExpressions(left, right); }
                    }
                }
            } else if (parameter.kind == GenericFormalKind::ObjectFormal) {
                same = namedEntity(actualExpression(a->genericActualExpression))
                    == namedEntity(actualExpression(b->genericActualExpression));
                same = same && namedEntity(actualExpression(a->genericActualExpression)) != nullptr;
            } else if (parameter.kind == GenericFormalKind::PackageFormal) {
                same = (a->packageActual != nullptr ? a->packageActual : a) == (b->packageActual != nullptr ? b->packageActual : b);
            } else {
                Symbol* left = actualEntity(a);
                Symbol* right = actualEntity(b);
                same = left != nullptr && left == right;
                if (left != nullptr && right != nullptr && !left->genericIntrinsic.empty()
                    && left->genericIntrinsic == right->genericIntrinsic) {
                    same = left->intrinsicPrefix == right->intrinsicPrefix
                        && SemaSupport::profileTypeMatches(a->returnType, b->returnType)
                        && a->parameters.size() == b->parameters.size();
                    for (std::size_t i = 0; same && i < a->parameters.size(); ++i) {
                        same = SemaSupport::profileTypeMatches(a->parameters[i]->type, b->parameters[i]->type);
                    }
                }
                if (a->genericActualEntity == nullptr && b->genericActualEntity == nullptr
                    && a->genericActualExpression != nullptr && b->genericActualExpression != nullptr) {
                    same = SemaSupport::conformingExpressions(a->genericActualExpression, b->genericActualExpression);
                }
                Expr* leftExpression = a->genericActualExpression;
                Expr* rightExpression = b->genericActualExpression;
                if (leftExpression != nullptr && rightExpression != nullptr
                    && leftExpression->kind == ExprKind::Attribute && rightExpression->kind == ExprKind::Attribute) {
                    auto* leftAttribute = static_cast<AttributeExpr*>(leftExpression);
                    auto* rightAttribute = static_cast<AttributeExpr*>(rightExpression);
                    same = leftAttribute->lower == rightAttribute->lower
                        && baseType(leftAttribute->prefixType) == baseType(rightAttribute->prefixType);
                }
            }
            if (!same) { matches = false; break; }
        }
        if (!matches) {
            m_diagnostics.error(actual->location, "actual parameters do not match the contract of formal package '" + formal.name + "'");
            return false;
        }
    }
    Symbol* view = m_symbolTable.createSymbol(SymbolKind::Package, formal.lower, formal.name);
    view->location = formal.location;
    view->scope = m_symbolTable.createScope(bindings, supplied->instanceSpecification->scope);
    view->instanceTemplate = supplied->instanceTemplate;
    view->instanceBindings = supplied->instanceBindings;
    view->instanceSpecification = supplied->instanceSpecification;
    view->packageMatches = formal.packageView->packageMatches;
    view->packageActual = supplied->packageActual != nullptr ? supplied->packageActual : supplied;
    view->formalPackage = true;
    view->hasSpecification = true;
    for (Symbol* original : formal.packageView->exposedFormals) {
        for (Symbol* candidate : supplied->instanceBindings->lookupLocal(original->name)) {
            if (candidate->kind != original->kind || candidate->parameters.size() != original->parameters.size()
                || contractKey(candidate->location) != contractKey(original->location)) { continue; }
            auto present = view->scope->lookupLocal(candidate->name);
            if (std::find(present.begin(), present.end(), candidate) == present.end()) {
                view->scope->add(candidate);
                view->exposedFormals.push_back(candidate);
            }
        }
    }
    bindings->add(view);
    mapPackageMembers(formal.packageView, view, decl->packageCopies);
    return true;
}
