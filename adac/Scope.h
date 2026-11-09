#pragma once

#include "Symbol.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// A declaration region and its Ada visibility rules: local overload sets,
// enclosing regions, use clauses, private/limited views and library context.
// Scopes refer to symbols; SymbolTable owns both symbols and scopes.
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
    int immediateDepth(const Scope* declaration) const;
    bool usesType(Type* type) const;
    std::vector<Type*> operatorTypes(const Scope* from) const;

    void setParent(Scope* scope) { m_parent = scope; }
    Scope* parent() const { return m_parent; }
    bool isRegion(const Scope* scope) const { return scope != nullptr && (this == scope || m_origin == scope); }
    void addLibraryVisibility(const std::string& name, bool privateWith = false);
    void addLimitedLibraryVisibility(const std::string& name, Symbol* view, bool privateWith);
    Symbol* limitedLibraryView(const std::string& name) const;
    bool privateVisible(Symbol* package) const;
    bool libraryVisible(const std::string& name) const;
    void setCompletionScope(Scope* scope) { m_completionScope = scope; }
    Scope* completionScope() const { return m_completionScope; }
    std::vector<Symbol*> lookupDeclaration(const std::string& name) const;
    const std::unordered_map<std::string, std::vector<Symbol*>>& declarations() const { return m_symbols; }

    bool isPrivateRegion() const { return m_privateRegion; }
    void setPrivateRegion(bool value) { m_privateRegion = value; }
    const std::string& packageName() const { return m_packageName; }
    void setPackageName(const std::string& name) { m_packageName = name; }
    const std::string& libraryPackage() const { return m_libraryPackage; }
    void setLibraryPackage(const std::string& name) { m_libraryPackage = name; }
    bool isContextClause() const { return m_contextClause; }
    void setContextClause(bool value) { m_contextClause = value; }
    bool isGenericBindingScope() const { return m_genericBindings; }
    void setGenericBindingScope(bool value) { m_genericBindings = value; }
    bool isLibraryRegistry() const { return m_isLibraryRegistry; }
    void setLibraryRegistry(bool value) { m_isLibraryRegistry = value; }
    bool isPublicChildPart() const { return m_publicChildPart; }
    void setPublicChildPart(bool value) { m_publicChildPart = value; }

private:
    bool m_privateRegion = false; // Private parts/bodies can use private-with visibility.
    std::string m_packageName; // Dotted path including nested packages.
    std::string m_libraryPackage; // Owning library unit, shared by its nested scopes.
    bool m_contextClause = false; // Hide use-clause visibility while resolving context items.
    bool m_genericBindings = false; // Region containing an instance's formal-to-actual bindings.
    bool m_isLibraryRegistry = false; // Registry scope; its own lookups bypass library-context filtering.
    bool m_publicChildPart = false; // Boundary preventing access to ancestor private declarations.

    friend class SymbolTable;
    Scope* m_parent = nullptr;
    const Scope* m_origin = nullptr;
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
    Scope* createScope(Scope* parent, const Scope* declarations);
    const std::vector<std::unique_ptr<Symbol>>& symbols() const { return m_symbols; }

private:
    std::vector<std::unique_ptr<Symbol>> m_symbols;
    std::vector<std::unique_ptr<Scope>> m_scopes;
};
