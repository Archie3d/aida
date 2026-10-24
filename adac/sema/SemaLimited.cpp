#include "Sema.h"

#include <algorithm>
#include <functional>

void Sema::prepareLimitedViews(const std::vector<CompilationUnit*>& units)
{
    std::unordered_map<std::string, PackageSpecDecl*> packages;
    std::vector<std::string> requested;
    for (CompilationUnit* unit : units) {
        for (const DeclPtr& declaration : unit->units) {
            if (declaration->kind == DeclKind::PackageSpecification) {
                auto* package = static_cast<PackageSpecDecl*>(declaration.get());
                if (package->renamesLower.empty()) { packages[package->lower] = package; }
            }
        }
        for (const WithClause& clause : unit->withClauses) {
            if (!clause.limited) { continue; }
            for (const std::string& name : clause.namesLower) {
                for (std::size_t end = name.find('.');; end = name.find('.', end + 1)) {
                    requested.push_back(name.substr(0, end));
                    if (end == std::string::npos) { break; }
                }
            }
        }
    }
    std::sort(requested.begin(), requested.end());
    requested.erase(std::unique(requested.begin(), requested.end()), requested.end());
    auto tagged = [](TypeDecl* type) {
        return type->taggedIncomplete || (type->definition != nullptr
            && (type->definition->tagged || type->definition->extension));
    };
    std::function<Symbol*(PackageSpecDecl*, const std::string&, const std::string&)> build;
    build = [&](PackageSpecDecl* declaration, const std::string& name, const std::string& owner) {
        auto* package = m_symbolTable.createSymbol(SymbolKind::Package, declaration->lower, declaration->name);
        package->limitedView = true;
        package->privateUnit = declaration->privateUnit;
        package->scope = m_symbolTable.createScope(nullptr); // Limited views have no context clause.
        package->scope->m_packageName = name;
        package->scope->m_libraryPackage = owner;
        package->location = declaration->location;
        m_limitedPackageViews[name] = package;
        for (const DeclPtr& item : declaration->publicPart) {
            if (item->kind == DeclKind::PackageSpecification) {
                auto* nested = static_cast<PackageSpecDecl*>(item.get());
                if (nested->renamesLower.empty()) {
                    package->scope->add(build(nested, name + "." + nested->lower, owner));
                }
                continue;
            }
            if (item->kind != DeclKind::Type) { continue; }
            auto* typeDecl = static_cast<TypeDecl*>(item.get());
            auto existing = package->scope->lookupLocal(typeDecl->lower);
            Type* full = existing.empty() ? nullptr : rootType(existing.front()->type);
            if (full == nullptr) {
                full = m_types.create(TypeKind::Record, typeDecl->name);
                full->isIncomplete = true;
                full->tagged = tagged(typeDecl);
                full->privateViewTagged = full->tagged;
                Type* view = m_types.create(TypeKind::Record, typeDecl->name);
                view->base = full;
                view->isIncomplete = true;
                view->limitedView = true;
                view->limitedPackage = owner;
                view->isLimited = true;
                view->tagged = full->tagged;
                view->privateViewTagged = full->tagged;
                m_limitedTypes.push_back(view);
                Symbol* named = m_symbolTable.createSymbol(SymbolKind::TypeName, typeDecl->lower, typeDecl->name);
                named->location = typeDecl->location;
                named->type = view;
                package->scope->add(named);
            }
            if (tagged(typeDecl)) {
                full->tagged = true;
                auto names = package->scope->lookupLocal(typeDecl->lower);
                names.front()->type->tagged = true;
                names.front()->type->privateViewTagged = true;
            }
            m_limitedCompletions[typeDecl] = full;
        }
        return package;
    };
    for (const std::string& name : requested) {
        auto found = packages.find(name);
        if (found == packages.end()) { continue; } // The loader diagnoses invalid limited targets.
        Symbol* view = build(found->second, name, name);
        view->name = name.substr(name.rfind('.') == std::string::npos ? 0 : name.rfind('.') + 1);
        view->libraryName = name;
        m_limitedPackages[name] = view;
        auto dot = name.rfind('.');
        if (dot != std::string::npos && m_limitedPackages.contains(name.substr(0, dot))) {
            m_limitedPackages.at(name.substr(0, dot))->scope->add(view);
        }
    }
}

// Incomplete designated views become usable only where their completion is
// visible, independently of whether another unit has already been analyzed.
Type* Sema::completeView(Type* type, Scope* scope)
{
    if (type == nullptr || !type->limitedView) { return type; }
    Symbol* package = lookupName(type->limitedPackage, scope);
    if (package != nullptr && package->kind == SymbolKind::Package && !package->limitedView) {
        return rootType(type);
    }
    return type;
}

bool Sema::checkCompleteView(Type* type, const SourceLocation& location, const char* use)
{
    Type* view = baseType(type);
    if (view == nullptr || (!view->limitedView && !(view->isIncomplete && view->privateTo == nullptr))) { return true; }
    m_diagnostics.error(location, std::string(use) + " requires a complete type view");
    return false;
}

void Sema::finishLimitedViews()
{
    // Semantic visibility checks are finished. Emission needs the completed
    // layouts behind legal access types and tagged incomplete formal profiles.
    for (Type* view : m_limitedTypes) {
        Type* full = rootType(view);
        if (full == nullptr || full->isIncomplete) {
            m_diagnostics.error(SourceLocation {}, "limited type view has no completed definition");
            continue;
        }
        *view = *full;
        view->base = full;
    }
}
