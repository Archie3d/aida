#include "Sema.h"
#include "SemaSupport.h"

#include <algorithm>

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
            if (candidate->kind == SymbolKind::Package && !candidate->limitedView) {
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
        if (i == 0) {
            for (Symbol* candidate : candidates) {
                if (candidate->limitedView) { enclosing->remove(candidate); }
            }
        }
        m_namePrefix.push_back(symbol->name);
        if (i + 1 == parts.size() || symbol->scope->m_libraryPackage.empty()) {
            symbol->scope->m_libraryPackage = m_currentUnit->partKey.substr(0, m_currentUnit->partKey.rfind('.'));
            std::replace(symbol->scope->m_libraryPackage.begin(), symbol->scope->m_libraryPackage.end(), '-', '.');
        }
        symbol->scope->m_packageName.clear();
        for (const std::string& part : m_namePrefix) {
            if (!symbol->scope->m_packageName.empty()) { symbol->scope->m_packageName += "."; }
            symbol->scope->m_packageName += part;
        }
        if (auto view = m_limitedPackageViews.find(symbol->scope->m_packageName); view != m_limitedPackageViews.end()) {
            view->second->fullPackage = symbol;
        }
        ++pushed;
        parent = symbol;
        enclosing = symbol->scope;
    }

    return symbol;
}

void Sema::analyzePackageSpec(PackageSpecDecl* decl, Scope* scope)
{
    if (!decl->renamesLower.empty()) {
        decl->symbol = declareUnitRenaming(decl->name, decl->lower, decl->renames, decl->renamesLower,
            decl->location, scope, false, true, false);
        return;
    }
    std::size_t pushed = 0;
    Symbol* symbol = declarePackagePath(decl->lower, decl->name, scope, decl->location, pushed);
    decl->symbol = symbol;
    if (symbol->hasSpecification) {
        m_diagnostics.error(decl->location, "'" + decl->name + "' has already been declared in this scope");
        for (std::size_t i = 0; i < pushed; ++i) {
            m_namePrefix.pop_back();
        }
        return;
    }
    symbol->hasSpecification = true;
    symbol->packageDeclaration = decl;

    // What a private type is made of is in reach from here down to the end of
    // the private part, and from the body, but nowhere else.
    m_packages.push_back(symbol);
    Scope* savedSpecScope = m_packageSpecScope;
    bool savedVisiblePart = m_inVisiblePart;
    m_packageSpecScope = symbol->scope;
    m_inVisiblePart = true;
    symbol->scope->m_publicChildPart = symbol->parentPackage != nullptr;
    analyzeDeclarativePart(decl->publicPart, symbol->scope, false);
    for (const DeclPtr& item : decl->publicPart) {
        if (item->kind != DeclKind::Type) { continue; }
        auto* type = static_cast<TypeDecl*>(item.get());
        if (type->definition == nullptr && type->declaredType != nullptr && type->declaredType->isIncomplete) {
            m_diagnostics.error(type->location, "an incomplete declaration in the visible part requires a completion in the same visible part");
        }
    }
    m_inVisiblePart = false;
    bool savedPrivateRegion = symbol->scope->m_privateRegion;
    symbol->scope->m_privateRegion = true;
    symbol->scope->m_publicChildPart = false;
    analyzeDeclarativePart(decl->privatePart, symbol->scope, false);
    symbol->scope->m_privateRegion = savedPrivateRegion;
    for (const DeclPtr& item : decl->privatePart) {
        if (item->kind != DeclKind::Type) { continue; }
        auto* type = static_cast<TypeDecl*>(item.get());
        if (type->declaredType != nullptr && type->declaredType->isIncomplete
            && type->declaredType->incompleteInPrivatePart && !type->declaredType->primitives.empty()) {
            m_diagnostics.error(type->location, "a private incomplete type with primitive declarations must be completed in the private part");
        }
    }
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
            if (operation->pendingControlledResult) {
                Type* result = operation->returnType;
                if (needsFinalization(result) && operation->builtin == BuiltinKind::Runtime) {
                    m_diagnostics.error(operation->location, "imported controlled function results are not yet supported");
                }
                operation->pendingControlledResult = false;
            }
            if (operation->pendingOverride == 1 && !operation->overrides) {
                m_diagnostics.error(operation->location, "subprogram marked overriding does not override an inherited operation");
            } else if (operation->pendingOverride == -1 && operation->overrides) {
                m_diagnostics.error(operation->location, "subprogram marked not overriding overrides an inherited operation");
            }
            operation->pendingOverride = 0;
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
    if (decl->subunit) {
        auto declarations = scope->lookupDeclaration(decl->lower);
        if (named != nullptr && std::find(declarations.begin(), declarations.end(), named) == declarations.end()) {
            m_diagnostics.error(decl->location,
                "package body stub must complete a declaration in the same declarative region");
            return;
        }
    }
    if (named != nullptr && named->kind == SymbolKind::Generic && named->generic != nullptr) {
        if (named->hasBody) {
            m_diagnostics.error(decl->location, "generic '" + named->displayName + "' already has a body");
            return;
        }
        decl->genericBody = named;
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
    if (symbol == nullptr || symbol->kind != SymbolKind::Package || !symbol->hasSpecification) {
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
    bool libraryBody = scope == m_unitScope && m_currentUnit != nullptr
        && std::any_of(m_currentUnit->units.begin(), m_currentUnit->units.end(), [&](const DeclPtr& unit) {
            return unit.get() == decl;
        });
    Scope* bodyScope = libraryBody ? scope : m_symbolTable.createScope(subunitScope(decl->subunit, symbol->scope));
    bodyScope->setCompletionScope(symbol->scope);
    bodyScope->m_privateRegion = true;

    int savedHandlerDepth = m_handlerDepth;
    m_handlerDepth = 0;
    m_packages.push_back(symbol);
    analyzeDeclarativePart(decl->declarations, bodyScope);
    reportMissingBodies(decl->declarations);
    if (symbol->packageDeclaration != nullptr) {
        reportMissingBodies(symbol->packageDeclaration->publicPart);
        reportMissingBodies(symbol->packageDeclaration->privatePart);
    }
    Symbol* savedSubprogram = m_currentSubprogram;
    if (m_currentSubprogram == nullptr) {
        m_currentSubprogram = elaborationOwner(m_currentUnit);
    }
    analyzeBodyStatements(decl->body, decl->handlers, bodyScope, decl->lower, symbol);
    m_currentSubprogram = savedSubprogram;
    m_packages.pop_back();
    m_handlerDepth = savedHandlerDepth;

    for (std::size_t i = 0; i < pushed; ++i) {
        m_namePrefix.pop_back();
    }
}

void Sema::analyzeUseClause(UseDecl& decl, Scope* scope)
{
    for (const std::string& name : decl.namesLower) {
        Symbol* package = lookupName(name, scope);
        if (package != nullptr && package->kind == SymbolKind::Package && package->limitedView) {
            m_diagnostics.error(decl.location, "a limited package view cannot be named in a use clause");
            return;
        }
    }
    Symbol* privateTo = scope == m_packageSpecScope && !m_inVisiblePart && !m_packages.empty()
        ? m_packages.back() : nullptr;
    for (std::size_t i = 0; i < decl.namesLower.size(); ++i) {
        if (decl.typeOnly) {
            Type* type = resolveTypeName(decl.namesLower[i], scope, decl.location);
            if (type != nullptr && checkCompleteView(type, decl.location, "a use type clause")) {
                scope->addUseType(type, decl.all, privateTo);
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
