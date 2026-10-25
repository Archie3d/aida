#include "Scope.h"

#include <algorithm>

Scope::Scope(Scope* parent)
    : m_parent(parent)
{
}

void Scope::add(Symbol* symbol)
{
    m_symbols[symbol->name].push_back(symbol);
}

void Scope::remove(Symbol* symbol)
{
    std::erase(m_symbols[symbol->name], symbol);
}

void Scope::addUseType(Type* type, bool all, Symbol* privateTo)
{
    m_useTypes.push_back({ rootType(type), all });
    if (!m_privateUseTypes.contains(rootType(type)) || privateTo == nullptr) {
        m_privateUseTypes[rootType(type)] = privateTo;
    }
}

void Scope::addUseScope(Scope* scope, Symbol* privateTo)
{
    if (!m_privateUseScopes.contains(scope) || privateTo == nullptr) {
        m_privateUseScopes[scope] = privateTo;
    }
    for (Scope* existing : m_useScopes) {
        if (existing == scope) {
            return;
        }
    }
    m_useScopes.push_back(scope);
}

std::vector<Symbol*> Scope::lookupLocal(const std::string& name) const
{
    auto it = m_symbols.find(name);
    if (it == m_symbols.end()) {
        return {};
    }
    return it->second;
}

void Scope::addLibraryVisibility(const std::string& name, bool privateWith)
{
    (privateWith ? m_privateLibraries : m_visibleLibraries).insert(name);
}

void Scope::addLimitedLibraryVisibility(const std::string& name, Symbol* view, bool privateWith)
{
    (privateWith ? m_privateLimitedLibraries : m_limitedLibraries)[name] = view;
}

bool Scope::libraryVisible(const std::string& name) const
{
    bool privateRegion = false;
    for (const Scope* scope = this; scope != nullptr; scope = scope->m_parent) {
        privateRegion = privateRegion || scope->m_privateRegion;
        if (scope->m_isLibraryRegistry || scope->m_visibleLibraries.contains(name)
            || scope->m_limitedLibraries.contains(name)
            || (privateRegion && (scope->m_privateLibraries.contains(name) || scope->m_privateLimitedLibraries.contains(name)))) {
            return true;
        }
    }
    return false;
}

Symbol* Scope::limitedLibraryView(const std::string& name) const
{
    bool privateRegion = false;
    Symbol* limited = nullptr;
    for (const Scope* scope = this; scope != nullptr; scope = scope->m_parent) {
        privateRegion = privateRegion || scope->m_privateRegion;
        if (scope->m_isLibraryRegistry || scope->m_visibleLibraries.contains(name)
            || (privateRegion && scope->m_privateLibraries.contains(name))) { return nullptr; }
        if (auto found = scope->m_limitedLibraries.find(name); found != scope->m_limitedLibraries.end()) {
            limited = found->second;
        }
        if (privateRegion) {
            if (auto found = scope->m_privateLimitedLibraries.find(name); found != scope->m_privateLimitedLibraries.end()) {
                limited = found->second;
            }
        }
    }
    return limited;
}

std::vector<Symbol*> Scope::lookupDeclaration(const std::string& name) const
{
    auto result = lookupLocal(name);
    if (m_completionScope != nullptr) {
        auto spec = m_completionScope->lookupLocal(name);
        result.insert(result.end(), spec.begin(), spec.end());
    }
    return result;
}

bool Scope::privateVisible(Symbol* package) const
{
    if (package == nullptr) {
        return true;
    }
    for (const Scope* enclosing = this; enclosing != nullptr; enclosing = enclosing->m_parent) {
        if (enclosing == package->scope) {
            return true;
        }
        if (enclosing->m_publicChildPart) {
            return false;
        }
    }
    return false;
}

std::vector<Symbol*> Scope::lookupVisibleLocal(const std::string& name, const Scope* from) const
{
    if (!m_packageName.empty()) {
        if (Symbol* limited = from->limitedLibraryView(m_packageName + "." + name)) { return {limited}; }
    }
    auto symbols = lookupLocal(name);
    std::erase_if(symbols, [&](Symbol* symbol) {
        if (!symbol->libraryName.empty() && !from->libraryVisible(symbol->libraryName)) { return true; }
        return !from->privateVisible(symbol->privatePrimitiveTo);
    });
    // RM 8.3: when a limited package view is visible, package renamings
    // denote that same view, including renamings of nested packages.
    for (Symbol*& symbol : symbols) {
        if (symbol->kind != SymbolKind::Package || symbol->scope == nullptr) { continue; }
        Symbol* original = symbol->renamedUnit;
        if (original != nullptr && original->limitedView) {
            bool immediate = false;
            for (const Scope* scope = from; scope != nullptr; scope = scope->m_parent) {
                immediate = immediate || scope == symbol->declarationScope;
            }
            const std::string& library = original->scope->m_libraryPackage;
            if (!immediate && !from->libraryVisible(library)) { symbol = nullptr; continue; }
            if (from->limitedLibraryView(library) == nullptr && original->fullPackage != nullptr) {
                symbol = original->fullPackage;
                continue;
            }
        }
        const std::string& owner = symbol->scope->m_libraryPackage;
        Symbol* limited = from->limitedLibraryView(owner.empty() ? symbol->libraryName : owner);
        if (limited == nullptr) { continue; }
        std::string path = symbol->scope->m_packageName;
        std::size_t start = owner.size();
        while (limited != nullptr && start < path.size()) {
            ++start;
            std::size_t dot = path.find('.', start);
            auto nested = limited->scope->lookupLocal(path.substr(start, dot - start));
            limited = nested.size() == 1 && nested.front()->kind == SymbolKind::Package ? nested.front() : nullptr;
            start = dot == std::string::npos ? path.size() : dot;
        }
        symbol = limited;
    }
    std::erase(symbols, nullptr);
    return symbols;
}

namespace {

bool isOverloadable(const Symbol* symbol)
{
    return symbol->kind == SymbolKind::Subprogram || symbol->kind == SymbolKind::EnumerationLiteral;
}

bool sameProfile(const Symbol* left, const Symbol* right)
{
    if (left->kind != right->kind) {
        const Symbol* literal = left->kind == SymbolKind::EnumerationLiteral ? left : right;
        const Symbol* subprogram = left->kind == SymbolKind::Subprogram ? left : right;
        return literal->kind == SymbolKind::EnumerationLiteral && subprogram->kind == SymbolKind::Subprogram
            && subprogram->parameters.empty() && rootType(literal->type) == rootType(subprogram->returnType);
    }
    if (left->kind == SymbolKind::EnumerationLiteral) {
        return rootType(left->type) == rootType(right->type);
    }
    if (left->kind != SymbolKind::Subprogram || left->parameters.size() != right->parameters.size()
        || rootType(left->returnType) != rootType(right->returnType)) {
        return false;
    }
    for (std::size_t i = 0; i < left->parameters.size(); ++i) {
        if (rootType(left->parameters[i]->type) != rootType(right->parameters[i]->type)) {
            return false;
        }
    }
    return true;
}

}  // namespace

// A name that cannot be overloaded is hidden by the innermost declaration of
// it. Overloadable names gather distinct profiles from enclosing scopes;
// an inner declaration hides an outer declaration of the same profile.
std::vector<Symbol*> Scope::lookup(const std::string& name) const
{
    std::vector<Symbol*> candidates;
    std::vector<Symbol*> imported;
    bool stopped = false;
    bool allowUse = true;
    for (const Scope* enclosing = this; enclosing != nullptr; enclosing = enclosing->m_parent) {
        allowUse = allowUse && !enclosing->m_contextClause;
    }
    for (const Scope* scope = this; scope != nullptr; scope = scope->m_parent) {
        for (Scope* used : scope->m_useScopes) {
            auto privacy = scope->m_privateUseScopes.find(used);
            if (!allowUse || (privacy != scope->m_privateUseScopes.end() && !privateVisible(privacy->second))) {
                continue;
            }
            auto symbols = used->lookupVisibleLocal(name, this);
            imported.insert(imported.end(), symbols.begin(), symbols.end());
        }
        for (const auto& [type, all] : scope->m_useTypes) {
            auto privacy = scope->m_privateUseTypes.find(type);
            if (!allowUse || (privacy != scope->m_privateUseTypes.end() && !privateVisible(privacy->second))) {
                continue;
            }
            for (Symbol* symbol : type->primitives) {
                if (symbol->name == name && (all || !operatorSymbol(name).empty())) {
                    if (privateVisible(symbol->privatePrimitiveTo)) {
                        imported.push_back(symbol);
                    }
                }
            }
        }
        if (stopped) {
            continue;
        }
        auto level = scope->lookupVisibleLocal(name, this);
        std::size_t innerCount = candidates.size();
        for (Symbol* symbol : level) {
            bool hidden = false;
            for (std::size_t i = 0; i < innerCount; ++i) {
                if (!isOverloadable(candidates[i]) || !isOverloadable(symbol)
                    || sameProfile(candidates[i], symbol)) {
                    hidden = true;
                    break;
                }
            }
            if (!hidden && std::find(candidates.begin(), candidates.end(), symbol) == candidates.end()) {
                candidates.push_back(symbol);
            }
            stopped = stopped || !isOverloadable(symbol);
        }
    }
    const auto direct = candidates;
    for (Symbol* symbol : imported) {
        bool hidden = false;
        for (Symbol* local : direct) {
            if (!isOverloadable(local) || !isOverloadable(symbol) || sameProfile(local, symbol)) {
                hidden = true;
                break;
            }
        }
        if (!hidden && std::find(candidates.begin(), candidates.end(), symbol) == candidates.end()) {
            candidates.push_back(symbol);
        }
    }

    return candidates;
}

Symbol* SymbolTable::createSymbol(SymbolKind kind, const std::string& lowerName, const std::string& displayName)
{
    auto symbol = std::make_unique<Symbol>();
    symbol->kind = kind;
    symbol->name = lowerName;
    symbol->displayName = displayName;
    m_symbols.push_back(std::move(symbol));
    return m_symbols.back().get();
}

Scope* SymbolTable::createScope(Scope* parent)
{
    m_scopes.push_back(std::make_unique<Scope>(parent));
    return m_scopes.back().get();
}
