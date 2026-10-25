#pragma once

#include "Symbol.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Scope
{
public:
    explicit Scope(Scope* parent = nullptr);

    void add(Symbol* symbol);
    void addUseScope(Scope* scope, Symbol* privateTo = nullptr);
    void addUseType(Type* type, bool all, Symbol* privateTo = nullptr);
    void remove(Symbol* symbol);

    std::vector<Symbol*> lookup(const std::string& name) const;
    std::vector<Symbol*> lookupLocal(const std::string& name) const;
    std::vector<Symbol*> lookupVisibleLocal(const std::string& name, const Scope* from) const;

    void setParent(Scope* scope) { m_parent = scope; }
    Scope* parent() const { return m_parent; }
    void addLibraryVisibility(const std::string& name, bool privateWith = false);
    void addLimitedLibraryVisibility(const std::string& name, Symbol* view, bool privateWith);
    Symbol* limitedLibraryView(const std::string& name) const;
    bool privateVisible(Symbol* package) const;
    bool libraryVisible(const std::string& name) const;
    void setCompletionScope(Scope* scope) { m_completionScope = scope; }
    std::vector<Symbol*> lookupDeclaration(const std::string& name) const;
    bool m_privateRegion = false;
    std::string m_packageName;
    std::string m_libraryPackage;
    bool m_contextClause = false;
    bool m_isLibraryRegistry = false;
    bool m_publicChildPart = false;

private:
    Scope* m_parent = nullptr;
    Scope* m_completionScope = nullptr;
    std::unordered_set<std::string> m_visibleLibraries;
    std::unordered_set<std::string> m_privateLibraries;
    std::unordered_map<std::string, Symbol*> m_limitedLibraries;
    std::unordered_map<std::string, Symbol*> m_privateLimitedLibraries;
    std::unordered_map<std::string, std::vector<Symbol*>> m_symbols;
    std::vector<Scope*> m_useScopes;
    std::unordered_map<Scope*, Symbol*> m_privateUseScopes;
    std::unordered_map<Type*, Symbol*> m_privateUseTypes;
    std::vector<std::pair<Type*, bool>> m_useTypes;
};

// Owns every symbol and scope created while analysing a compilation.
class SymbolTable
{
public:
    Symbol* createSymbol(SymbolKind kind, const std::string& lowerName, const std::string& displayName);
    Scope* createScope(Scope* parent);
    const std::vector<std::unique_ptr<Symbol>>& symbols() const { return m_symbols; }

private:
    std::vector<std::unique_ptr<Symbol>> m_symbols;
    std::vector<std::unique_ptr<Scope>> m_scopes;
};
