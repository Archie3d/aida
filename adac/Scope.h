#pragma once

#include "Symbol.h"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class Scope
{
public:
    explicit Scope(Scope* parent = nullptr)
        : m_parent(parent)
    {
    }

    void add(Symbol* symbol);
    void addUseScope(Scope* scope);
    void addUseType(Type* type, bool all);
    void remove(Symbol* symbol);

    std::vector<Symbol*> lookup(const std::string& name) const;
    std::vector<Symbol*> lookupLocal(const std::string& name) const;
    std::vector<Symbol*> lookupVisibleLocal(const std::string& name, const Scope* from) const;

    Scope* parent() const { return m_parent; }

private:
    Scope* m_parent = nullptr;
    std::unordered_map<std::string, std::vector<Symbol*>> m_symbols;
    std::vector<Scope*> m_useScopes;
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
