#include "Sema.h"
#include "SemaSupport.h"

using SemaSupport::splitDottedName;

// A library unit written as Ada.Text_IO is a child declared inside Ada, so each
// part of the name is walked, and made if it has not been seen, before the unit
// itself.  Every part joins m_namePrefix, which is what gives the child's
// subprograms names of their own.
Symbol* Sema::declarePackagePath(const std::string& lower, const std::string& displayName, Scope* scope,
                                 const SourceLocation& location, std::size_t& pushed)
{
    std::vector<std::string> parts = splitDottedName(lower);
    std::vector<std::string> display = splitDottedName(displayName);

    Scope* enclosing = scope;
    Symbol* symbol = nullptr;
    Symbol* parent = nullptr;
    pushed = 0;

    for (std::size_t i = 0; i < parts.size(); ++i) {
        // A parent is looked for wherever it may be, a child only in its parent.
        std::vector<Symbol*> candidates =
            i == 0 && parts.size() > 1 ? enclosing->lookup(parts[i]) : enclosing->lookupDeclaration(parts[i]);
        symbol = nullptr;
        if (i + 1 == parts.size()) {
            for (Symbol* candidate : candidates) {
                if (candidate->kind != SymbolKind::Package) {
                    m_diagnostics.error(location, "'" + displayName + "' has already been declared in this scope");
                    break;
                }
            }
        }
        for (Symbol* candidate : candidates) {
            if (candidate->kind == SymbolKind::Package) {
                symbol = candidate;
                break;
            }
        }
        if (symbol == nullptr) {
            symbol = m_symbolTable.createSymbol(SymbolKind::Package, parts[i],
                                                i < display.size() ? display[i] : parts[i]);
            symbol->scope = m_symbolTable.createScope(i + 1 == parts.size() && scope == m_unitScope ? scope : enclosing);
            symbol->location = location;
            symbol->parentPackage = parent;
            enclosing->add(symbol);
        }
        m_namePrefix.push_back(symbol->name);
        ++pushed;
        parent = symbol;
        enclosing = symbol->scope;
    }

    return symbol;
}

void Sema::analyzePackageSpec(PackageSpecDecl* decl, Scope* scope)
{
    std::size_t pushed = 0;
    Symbol* symbol = declarePackagePath(decl->lower, decl->name, scope, decl->location, pushed);
    decl->symbol = symbol;
    if (symbol->m_hasSpecification) {
        m_diagnostics.error(decl->location, "'" + decl->name + "' has already been declared in this scope");
        for (std::size_t i = 0; i < pushed; ++i) {
            m_namePrefix.pop_back();
        }
        return;
    }
    symbol->m_hasSpecification = true;
    symbol->m_packageDeclaration = decl;

    // What a private type is made of is in reach from here down to the end of
    // the private part, and from the body, but nowhere else.
    m_packages.push_back(symbol);
    Scope* savedSpecScope = m_packageSpecScope;
    bool savedVisiblePart = m_inVisiblePart;
    m_packageSpecScope = symbol->scope;
    m_inVisiblePart = true;
    symbol->scope->m_publicChildPart = symbol->parentPackage != nullptr;
    analyzeDeclarativePart(decl->publicPart, symbol->scope, false);
    m_inVisiblePart = false;
    symbol->scope->m_publicChildPart = false;
    analyzeDeclarativePart(decl->privatePart, symbol->scope, false);
    // A private type can acquire inherited operations only at its completion.
    // Check indicators and result restrictions on earlier declarations once
    // the ancestry and controlled lifetime requirements are known.
    for (DeclList* part : { &decl->publicPart, &decl->privatePart }) {
        for (const DeclPtr& item : *part) {
            Symbol* operation = nullptr;
            if (item->kind == DeclKind::SubprogramDeclaration) {
                operation = static_cast<SubprogramDecl*>(item.get())->symbol;
            } else if (item->kind == DeclKind::Type) {
                Type* type = static_cast<TypeDecl*>(item.get())->declaredType;
                operation = type != nullptr ? type->accessProfile : nullptr;
            }
            if (operation == nullptr) {
                continue;
            }
            if (hasProtected(operation->returnType)) {
                m_diagnostics.error(operation->location, "function results containing protected objects are not yet supported");
            }
            if (hasTasks(operation->returnType)) {
                m_diagnostics.error(operation->location, "function results containing tasks are not yet supported");
            }
            if (operation->m_pendingControlledResult) {
                Type* result = operation->returnType;
                if (needsFinalization(result) && operation->builtin == BuiltinKind::Runtime) {
                    m_diagnostics.error(operation->location, "imported controlled function results are not yet supported");
                }
                operation->m_pendingControlledResult = false;
            }
            if (operation->m_pendingOverride == 1 && !operation->m_overrides) {
                m_diagnostics.error(operation->location, "subprogram marked overriding does not override an inherited operation");
            } else if (operation->m_pendingOverride == -1 && operation->m_overrides) {
                m_diagnostics.error(operation->location, "subprogram marked not overriding overrides an inherited operation");
            }
            operation->m_pendingOverride = 0;
        }
    }
    m_packageSpecScope = savedSpecScope;
    m_inVisiblePart = savedVisiblePart;
    m_packages.pop_back();

    reportIncompleteTypes(decl->publicPart);
    reportIncompleteTypes(decl->privatePart);

    // Every promise the visible part made has to have been kept by now.
    for (const DeclPtr& item : decl->publicPart) {
        if (item->kind != DeclKind::Object) {
            continue;
        }
        auto* object = static_cast<ObjectDecl*>(item.get());
        for (Symbol* constant : object->symbols) {
            if (constant->awaitsValue) {
                m_diagnostics.error(item->location, "'" + constant->displayName + "' is never given a value");
            }
        }
    }

    adoptLibraryUnit(decl, symbol);

    for (std::size_t i = 0; i < pushed; ++i) {
        m_namePrefix.pop_back();
    }
}

void Sema::analyzePackageBody(PackageBodyDecl* decl, Scope* scope)
{
    // Attach a separate generic body and check its contract before any instance.
    Symbol* named = lookupName(decl->lower, scope);
    if (named != nullptr && named->kind == SymbolKind::Generic && named->generic != nullptr) {
        if (named->hasBody) {
            m_diagnostics.error(decl->location, "generic '" + named->displayName + "' already has a body");
            return;
        }
        decl->m_genericBody = named;
        named->hasBody = true;
        std::vector<Token>& tokens = named->generic->tokens;
        Token endOfFile = tokens.back();
        tokens.pop_back();
        tokens.insert(tokens.end(), decl->tokens.begin(), decl->tokens.end());
        tokens.push_back(endOfFile);
        checkGenericContract(named->generic, scope);
        return;
    }

    Symbol* symbol = lookupName(decl->lower, scope);
    if (symbol == nullptr || symbol->kind != SymbolKind::Package || !symbol->m_hasSpecification) {
        m_diagnostics.error(decl->location, "package body '" + decl->name + "' has no specification");
        return;
    }
    if (symbol->hasBody) {
        m_diagnostics.error(decl->location, "package '" + decl->name + "' already has a body");
        return;
    }
    symbol->hasBody = true;
    decl->symbol = symbol;
    std::size_t pushed = 0;
    for (const std::string& part : splitDottedName(decl->lower)) {
        m_namePrefix.push_back(part);
        ++pushed;
    }
    Scope* bodyScope = scope == m_unitScope ? scope : m_symbolTable.createScope(symbol->scope);
    bodyScope->setCompletionScope(symbol->scope);

    int savedHandlerDepth = m_handlerDepth;
    m_handlerDepth = 0;
    m_packages.push_back(symbol);
    analyzeDeclarativePart(decl->declarations, bodyScope);
    reportMissingBodies(decl->declarations);
    if (symbol->m_packageDeclaration != nullptr) {
        reportMissingBodies(symbol->m_packageDeclaration->publicPart);
        reportMissingBodies(symbol->m_packageDeclaration->privatePart);
    }
    Symbol* savedSubprogram = m_currentSubprogram;
    if (m_currentSubprogram == nullptr) {
        m_currentSubprogram = elaborationOwner(m_currentUnit);
    }
    analyzeStatements(decl->body, bodyScope);
    analyzeHandlers(decl->handlers, bodyScope);
    m_currentSubprogram = savedSubprogram;
    m_packages.pop_back();
    m_handlerDepth = savedHandlerDepth;

    for (std::size_t i = 0; i < pushed; ++i) {
        m_namePrefix.pop_back();
    }
}

void Sema::analyzeUseClause(UseDecl& decl, Scope* scope)
{
    Symbol* privateTo = scope == m_packageSpecScope && !m_inVisiblePart && !m_packages.empty()
        ? m_packages.back() : nullptr;
    for (std::size_t i = 0; i < decl.namesLower.size(); ++i) {
        if (decl.m_typeOnly) {
            Type* type = resolveTypeName(decl.namesLower[i], scope, decl.location);
            if (type != nullptr) {
                scope->addUseType(type, decl.m_all, privateTo);
            }
            continue;
        }
        Symbol* symbol = lookupName(decl.namesLower[i], scope);
        if (symbol == nullptr) {
            m_diagnostics.error(decl.location, "unknown package '" + decl.names[i] + "' in use clause");
            continue;
        }
        if (symbol->kind == SymbolKind::Package && symbol->scope != nullptr) {
            scope->addUseScope(symbol->scope, privateTo);
        } else {
            m_diagnostics.error(decl.location, "'" + decl.names[i] + "' is not a package in use clause");
        }
    }
}

// Whether the package a private type belongs to is one of those being analysed
// right now.  A child unit counts, since it is analysed inside its parent.
bool Sema::withinPackage(Symbol* package) const
{
    if (m_unitScope != nullptr && m_unitScope->m_publicChildPart && !m_unitScope->privateVisible(package)) {
        return false;
    }
    for (Symbol* open : m_packages) {
        if (open->scope != nullptr && open->scope->privateVisible(package)) {
            return true;
        }
    }
    return false;
}

// The type as its user may see it: outside the package that declared it, a
// private type is a name and nothing more.
bool Sema::representationVisible(Type* type) const
{
    Type* base = baseType(type);
    return base == nullptr || base->privateTo == nullptr || withinPackage(base->privateTo);
}

bool Sema::taggedViewVisible(Type* type) const
{
    Type* base = baseType(type);
    return base != nullptr && base->tagged
        && (base->privateViewTagged || representationVisible(base));
}

bool Sema::checkNotPrivate(Type* type, const SourceLocation& location, const char* what)
{
    Type* base = baseType(type);
    if (representationVisible(base)) {
        return true;
    }
    m_diagnostics.error(location, std::string(what) + " of '" + base->name + "', whose representation '"
                                      + base->privateTo->displayName + "' keeps to itself");
    return false;
}
