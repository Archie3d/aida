#include "UnitLoader.h"

#include "Lexer.h"
#include "Parser.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <functional>
#include <sstream>
#include <utility>

namespace
{

// The file a unit lives in: Ada.Text_IO.Integer_IO is ada-text_io-integer_io.
std::string fileKey(const std::string& unitName)
{
    std::string key = toLower(unitName);
    for (char& c : key) {
        if (c == '.') {
            c = '-';
        }
    }
    return key;
}

std::string directoryOf(const std::string& path)
{
    std::size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
        return ".";
    }
    return path.substr(0, slash);
}

// A child unit cannot be looked at before its parent, since it is declared
// inside it.
std::string parentOf(const std::string& unitName)
{
    std::size_t dot = unitName.find_last_of('.');
    if (dot == std::string::npos) {
        return std::string();
    }
    return unitName.substr(0, dot);
}

std::string sourceIdentity(const std::string& path)
{
    std::error_code error;
    auto canonical = std::filesystem::weakly_canonical(path, error);
    return error ? path : canonical.string();
}

std::string declarationName(const Decl& decl)
{
    switch (decl.kind) {
    case DeclKind::PackageSpecification:
        return static_cast<const PackageSpecDecl&>(decl).name;
    case DeclKind::PackageBody:
        return static_cast<const PackageBodyDecl&>(decl).name;
    case DeclKind::SubprogramDeclaration:
        return static_cast<const SubprogramDecl&>(decl).spec.name;
    case DeclKind::SubprogramBody:
        return static_cast<const SubprogramBody&>(decl).spec.name;
    case DeclKind::GenericDeclaration:
        return static_cast<const GenericDecl&>(decl).name;
    case DeclKind::GenericInstantiation:
        return static_cast<const GenericInstantiationDecl&>(decl).name;
    default:
        return {};
    }
}

bool isOptionalCompletion(const Decl& decl)
{
    return (decl.kind == DeclKind::SubprogramDeclaration
            && static_cast<const SubprogramDecl&>(decl).m_renamedName != nullptr)
        || (decl.kind == DeclKind::SubprogramBody
            && static_cast<const SubprogramBody&>(decl).m_isNull);
}

bool isBody(const Decl& decl)
{
    return decl.kind == DeclKind::PackageBody
        || (decl.kind == DeclKind::SubprogramBody && !isOptionalCompletion(decl));
}

bool allowsBody(const Decl& decl)
{
    return decl.kind == DeclKind::PackageSpecification
        || (decl.kind == DeclKind::SubprogramDeclaration && !isOptionalCompletion(decl))
        || (decl.kind == DeclKind::GenericDeclaration
            && !static_cast<const GenericDecl&>(decl).m_hasBody);
}

enum class LibraryKind
{
    Package,
    Procedure,
    Function
};

LibraryKind libraryKind(const Decl& decl)
{
    switch (decl.kind) {
    case DeclKind::SubprogramDeclaration:
        return static_cast<const SubprogramDecl&>(decl).spec.isFunction ? LibraryKind::Function : LibraryKind::Procedure;
    case DeclKind::SubprogramBody:
        return static_cast<const SubprogramBody&>(decl).spec.isFunction ? LibraryKind::Function : LibraryKind::Procedure;
    case DeclKind::GenericDeclaration: {
        const auto& generic = static_cast<const GenericDecl&>(decl);
        return generic.isPackage ? LibraryKind::Package
            : generic.m_isFunction ? LibraryKind::Function : LibraryKind::Procedure;
    }
    case DeclKind::GenericInstantiation: {
        const auto& instance = static_cast<const GenericInstantiationDecl&>(decl);
        return instance.isPackage ? LibraryKind::Package
            : instance.m_isFunction ? LibraryKind::Function : LibraryKind::Procedure;
    }
    default:
        return LibraryKind::Package;
    }
}

std::string partKey(const CompilationUnit& unit)
{
    return unit.partKey;
}

bool declaresGeneric(const DeclList& declarations)
{
    for (const DeclPtr& decl : declarations) {
        if (decl->kind == DeclKind::GenericDeclaration) {
            return true;
        }
        if (decl->kind == DeclKind::PackageSpecification) {
            auto* package = static_cast<PackageSpecDecl*>(decl.get());
            if (declaresGeneric(package->publicPart) || declaresGeneric(package->privatePart)) {
                return true;
            }
        }
    }
    return false;
}

}

UnitLoader::UnitLoader(Diagnostics& diagnostics)
    : m_diagnostics(diagnostics)
{
}

void UnitLoader::addSearchPath(const std::string& directory)
{
    if (directory.empty()) {
        return;
    }
    for (const std::string& existing : m_searchPaths) {
        if (existing == directory) {
            return;
        }
    }
    m_searchPaths.push_back(directory);
}

bool UnitLoader::readFile(const std::string& path, std::string& contents) const
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return false;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    contents = buffer.str();
    return true;
}

std::string UnitLoader::findUnit(const std::string& key, const char* extension) const
{
    auto explicitPath = m_explicitPaths.find(key + extension);
    if (explicitPath != m_explicitPaths.end()) {
        return explicitPath->second;
    }
    for (const std::string& directory : m_searchPaths) {
        std::string candidate = directory + "/" + key + extension;
        std::ifstream probe(candidate, std::ios::binary);
        if (probe) {
            return candidate;
        }
    }
    return std::string();
}

bool UnitLoader::addSource(const std::string& path)
{
    addSearchPath(directoryOf(path));
    std::string file = toLower(std::filesystem::path(path).filename().string());
    auto [entry, inserted] = m_explicitPaths.emplace(file, path);
    if (!inserted && sourceIdentity(entry->second) != sourceIdentity(path)) {
        m_diagnostics.error(SourceLocation {}, "conflicting explicit sources for '" + file
            + "': '" + entry->second + "' and '" + path + "'");
        return false;
    }
    if (!m_sources.contains(sourceIdentity(path))) {
        std::string contents;
        if (!readFile(path, contents)) {
            m_diagnostics.error(SourceLocation {}, "cannot read '" + path + "'");
            return false;
        }
        parseInto(path, contents, toLower(std::filesystem::path(path).stem().string()));
    }
    return true;
}

bool UnitLoader::parseInto(const std::string& path, const std::string& contents, const std::string& key)
{
    int file = m_diagnostics.addFile(path);
    Lexer lexer(contents, file, m_diagnostics);
    Parser parser(lexer.tokenize(), m_diagnostics);
    auto units = parser.parseCompilation();
    auto& source = m_sources[sourceIdentity(path)];
    // Publish every item before following any edge, including forward
    // references within a bundle and references from other explicit sources.
    for (auto& unit : units) {
        unit->fileName = path;
        unit->unitKey = key;
        const Decl& decl = *unit->units.front();
        std::string nameKey = fileKey(declarationName(decl));
        m_items[nameKey].push_back(unit.get());
        registerParts(nameKey);
        source.push_back(unit.get());
        m_units.push_back(std::move(unit));
    }
    return true;
}

void UnitLoader::registerParts(const std::string& key)
{
    const auto& items = m_items.at(key);
    bool hasDeclaration = std::any_of(items.begin(), items.end(), [](const CompilationUnit* unit) {
        const Decl& decl = *unit->units.front();
        return !isBody(decl) && !isOptionalCompletion(decl);
    });
    m_parts.erase(key + ".spec");
    m_parts.erase(key + ".body");
    for (CompilationUnit* unit : items) {
        const Decl& decl = *unit->units.front();
        // A renaming or null procedure is a declaration by itself, but can
        // complete a separate declaration regardless of source argument order.
        unit->isSpec = !isBody(decl) && !(hasDeclaration && isOptionalCompletion(decl));
        unit->partKey = key + (unit->isSpec ? ".spec" : ".body");
        m_parts.emplace(unit->partKey, unit);
    }
}

bool UnitLoader::validateParts()
{
    std::unordered_map<std::string, CompilationUnit*> seen;
    for (const auto& unit : m_units) {
        auto [entry, inserted] = seen.emplace(unit->partKey, unit.get());
        if (!inserted) {
            m_diagnostics.error(unit->units.front()->location, "duplicate library unit part '" + unit->partKey
                + "' (already declared in '" + entry->second->fileName + "')");
            return false;
        }
    }
    for (CompilationUnit* unit : m_discoveredUnits) {
        if (unit->isSpec) {
            continue;
        }
        const Decl& body = *unit->units.front();
        std::string name = declarationName(body);
        auto found = m_parts.find(fileKey(name) + ".spec");
        if (found == m_parts.end()) {
            if (body.kind == DeclKind::PackageBody) {
                m_diagnostics.error(body.location, "package body '" + name + "' has no specification");
                return false;
            }
            continue; // A subprogram body can declare its library unit.
        }
        const Decl& spec = *found->second->units.front();
        if (libraryKind(body) != libraryKind(spec)) {
            m_diagnostics.error(body.location, "library unit kind mismatch for '" + name + "'");
            return false;
        }
        if (!allowsBody(spec)) {
            m_diagnostics.error(body.location, "library unit '" + name + "' is already complete; a body is not allowed");
            return false;
        }
        if (spec.kind == DeclKind::GenericDeclaration && body.kind == DeclKind::SubprogramDeclaration) {
            m_diagnostics.error(body.location, "renaming completion of a generic library unit is not supported");
            return false;
        }
    }
    return true;
}

std::string UnitLoader::sourceKey(const std::string& path) const
{
    auto found = m_sources.find(sourceIdentity(path));
    return found != m_sources.end() && !found->second.empty()
        ? found->second.front()->unitKey : toLower(std::filesystem::path(path).stem().string());
}

bool UnitLoader::loadSource(const std::string& path)
{
    addSearchPath(directoryOf(path));
    std::string identity = sourceIdentity(path);
    if (!m_loadedSources.insert(identity).second) {
        return true;
    }
    if (!m_sources.contains(identity)) {
        std::string contents;
        if (!readFile(path, contents)) {
            m_diagnostics.error(SourceLocation {}, "cannot read '" + path + "'");
            return false;
        }
        parseInto(path, contents, toLower(std::filesystem::path(path).stem().string()));
    }
    // Copy pointers: recursive discovery can grow the source table.
    auto source = m_sources.at(identity);
    for (CompilationUnit* unit : source) {
        const Decl& decl = *unit->units.front();
        std::string name = declarationName(decl);
        std::string key = fileKey(name);
        if (!unit->isSpec || isOptionalCompletion(decl)
            || toLower(std::filesystem::path(path).extension().string()) != ".ads") {
            auto spec = m_parts.find(key + ".spec");
            std::string specPath = spec == m_parts.end() || spec->second == unit
                ? findUnit(key, ".ads") : spec->second->fileName;
            if (!specPath.empty() && sourceIdentity(specPath) != identity) {
                loadPart(specPath, key + ".spec", decl.location);
            }
        }
        std::string parent = parentOf(name);
        if (!parent.empty()) {
            loadUnit(parent, decl.location);
        }
        for (const WithClause& clause : unit->withClauses) {
            for (const std::string& dependency : clause.names) {
                loadUnit(dependency, clause.location);
            }
        }
        if (unit->isSpec && (!m_specificationsOnly || unit->unitKey == m_targetKey
            || declaresGeneric(unit->units))) {
            auto body = m_parts.find(key + ".body");
            std::string bodyPath = body == m_parts.end() ? findUnit(key, ".adb") : body->second->fileName;
            if (!bodyPath.empty() && (body != m_parts.end() || sourceIdentity(bodyPath) != identity)) {
                loadPart(bodyPath, key + ".body", decl.location);
            }
        }
    }
    m_discoveredUnits.insert(m_discoveredUnits.end(), source.begin(), source.end());
    return true;
}

bool UnitLoader::loadPart(const std::string& path, const std::string& identity, const SourceLocation& from)
{
    if (!loadSource(path)) {
        return false;
    }
    const auto& source = m_sources.at(sourceIdentity(path));
    if (std::none_of(source.begin(), source.end(), [&](const CompilationUnit* unit) {
        return unit->partKey == identity;
    }) && !m_diagnostics.hasErrors()) {
        m_diagnostics.error(from, "source '" + path + "' does not declare library unit part '" + identity + "'");
        return false;
    }
    return true;
}

bool UnitLoader::loadUnit(const std::string& name, const SourceLocation& from)
{
    std::string key = fileKey(name);
    std::string specPath = findUnit(key, ".ads");
    std::string bodyPath = findUnit(key, ".adb");
    auto specPart = m_parts.find(key + ".spec");
    auto bodyPart = m_parts.find(key + ".body");
    if (specPart != m_parts.end()) {
        specPath = specPart->second->fileName;
    }
    if (bodyPart != m_parts.end()) {
        bodyPath = bodyPart->second->fileName;
    }
    if (specPath.empty() && bodyPath.empty()) {
        // Standard and System are compiler-defined. Sema diagnoses other
        // missing units after loading has completed.
        return m_parts.contains(key + ".spec") || m_parts.contains(key + ".body");
    }
    if (!specPath.empty() && !loadSource(specPath)) {
        return false;
    }
    bool generic = false;
    if (!specPath.empty()) {
        auto found = m_parts.find(key + ".spec");
        generic = found != m_parts.end() && declaresGeneric(found->second->units);
    }
    bool bodyWanted = !m_specificationsOnly || key == m_targetKey || specPath.empty() || generic;
    if (!bodyPath.empty() && bodyWanted && !loadSource(bodyPath)) {
        return false;
    }
    // Every automatically selected source must name the requested unit, even
    // when a valid specification was already found in another file.
    for (const std::string& selected : { specPath, bodyWanted ? bodyPath : std::string() }) {
        if (selected.empty()) {
            continue;
        }
        const auto& source = m_sources.at(sourceIdentity(selected));
        bool namesUnit = std::any_of(source.begin(), source.end(), [&](const CompilationUnit* unit) {
            return fileKey(declarationName(*unit->units.front())) == key;
        });
        if (!namesUnit && !m_diagnostics.hasErrors()) {
            m_diagnostics.error(from, "source '" + selected + "' does not declare library unit '" + name + "'");
            return false;
        }
        bool explicitSource = std::any_of(m_explicitPaths.begin(), m_explicitPaths.end(), [&](const auto& entry) {
            return sourceIdentity(entry.second) == sourceIdentity(selected);
        });
        bool specification = selected == specPath;
        bool matchesRole = std::any_of(source.begin(), source.end(), [&](const CompilationUnit* unit) {
            return fileKey(declarationName(*unit->units.front())) == key
                && (specification ? unit->isSpec : !unit->isSpec || !allowsBody(*unit->units.front()));
        });
        if (namesUnit && !explicitSource && !matchesRole && !m_diagnostics.hasErrors()) {
            m_diagnostics.error(from, "source '" + selected + "' does not declare library unit part '"
                + key + (specification ? ".spec" : ".body") + "'");
            return false;
        }
    }
    return true;
}

bool UnitLoader::orderUnits()
{
    if (!validateParts()) {
        return false;
    }
    // Explicit files are parsed up front for identity lookup; only reached
    // sources participate in semantic analysis and emission.
    std::unordered_map<CompilationUnit*, std::vector<CompilationUnit*>> dependencies;
    for (CompilationUnit* unit : m_discoveredUnits) {
        unit->dependencies.clear();
        dependencies[unit];
        auto add = [&](const std::string& name, bool ownBody, bool sameSourceAllowed) {
            std::string key = fileKey(name);
            auto found = m_parts.find(key + ".spec");
            if (found == m_parts.end() && !ownBody) {
                found = m_parts.find(key + ".body");
            }
            if (found == m_parts.end() || (sameSourceAllowed && found->second == unit)) {
                return;
            }
            auto& edges = dependencies[unit];
            if (std::find(edges.begin(), edges.end(), found->second) == edges.end()) {
                edges.push_back(found->second);
                unit->dependencies.push_back(partKey(*found->second));
            }
        };
        for (const DeclPtr& decl : unit->units) {
            std::string name = declarationName(*decl);
            if (!unit->isSpec) {
                add(name, true, true);
            }
            std::string parent = parentOf(name);
            if (!parent.empty()) {
                std::string parentKey = fileKey(parent);
                if (!m_parts.contains(parentKey + ".spec")) {
                    m_diagnostics.error(decl->location, "cannot find parent unit '" + parent + "'");
                    return false;
                }
                const Decl& parentDecl = *m_parts.at(parentKey + ".spec")->units.front();
                if (libraryKind(parentDecl) != LibraryKind::Package) {
                    m_diagnostics.error(decl->location, "parent unit '" + parent + "' is not a package");
                    return false;
                }
                add(parent, false, true);
            }
        }
        for (const WithClause& clause : unit->withClauses) {
            for (const std::string& name : clause.names) {
                std::string key = fileKey(name);
                if (!m_parts.contains(key + ".spec") && !m_parts.contains(key + ".body")
                    && key != "standard" && key != "system") {
                    m_diagnostics.error(clause.location, "cannot find the unit '" + name + "'");
                    return false;
                }
                add(name, false, false);
            }
        }
    }
    // Keep semantic dependencies immutable while computing Elaborate_All's
    // closure: pragma ordering edges are not semantic dependence edges.
    const auto semanticDependencies = dependencies;
    auto addEdge = [&](CompilationUnit* client, CompilationUnit* prerequisite) {
        auto& edges = dependencies[client];
        if (std::find(edges.begin(), edges.end(), prerequisite) == edges.end()) {
            edges.push_back(prerequisite);
            client->dependencies.push_back(partKey(*prerequisite));
        }
    };
    auto bodyOf = [&](CompilationUnit* unit) -> CompilationUnit* {
        std::string key = fileKey(declarationName(*unit->units.front()));
        auto found = m_parts.find(key + ".body");
        if (found == m_parts.end()
            || std::find(m_discoveredUnits.begin(), m_discoveredUnits.end(), found->second) == m_discoveredUnits.end()) {
            return nullptr;
        }
        return found->second;
    };
    std::vector<std::pair<CompilationUnit*, CompilationUnit*>> immediateBodies;
    for (CompilationUnit* unit : m_discoveredUnits) {
        // An instance's spec and body already elaborate as one library item.
        if (unit->units.front()->m_elaborateBody
            && unit->units.front()->kind != DeclKind::GenericInstantiation) {
            CompilationUnit* body = bodyOf(unit);
            if (body == nullptr) {
                if (!m_specificationsOnly || unit->unitKey == m_targetKey) {
                    m_diagnostics.error(unit->units.front()->location,
                        "pragma Elaborate_Body requires a body for '"
                        + declarationName(*unit->units.front()) + "'");
                    return false;
                }
            } else {
                immediateBodies.emplace_back(unit, body);
            }
        }
        for (const ElaborationPragma& pragma : unit->elaborationPragmas) {
            for (const std::string& name : pragma.names) {
                std::string key = fileKey(name);
                auto found = m_parts.find(key + ".spec");
                if (found == m_parts.end()) {
                    found = m_parts.find(key + ".body");
                }
                if (found == m_parts.end()) {
                    if (key == "standard" || key == "system") {
                        continue;
                    }
                    m_diagnostics.error(pragma.location, "unknown elaboration pragma unit '" + name + "'");
                    return false;
                }
                std::unordered_set<CompilationUnit*> seen;
                std::function<void(CompilationUnit*)> require = [&](CompilationUnit* required) {
                    if (!seen.insert(required).second) {
                        return;
                    }
                    addEdge(unit, required);
                    CompilationUnit* body = bodyOf(required);
                    if (body != nullptr && body != required) {
                        require(body);
                    }
                    if (pragma.all) {
                        for (CompilationUnit* dependency : semanticDependencies.at(required)) {
                            require(dependency);
                        }
                    }
                };
                require(found->second);
            }
        }
    }
    // Contract each immediate spec/body pair for ordering: all prerequisites
    // of its body must precede its spec, and every client must follow its body.
    // Iterate because immediate pairs may themselves depend on other pairs.
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& [spec, body] : immediateBodies) {
            auto prerequisites = dependencies[body];
            for (CompilationUnit* dependency : prerequisites) {
                if (dependency != spec) {
                    std::size_t before = dependencies[spec].size();
                    addEdge(spec, dependency);
                    changed = changed || dependencies[spec].size() != before;
                }
            }
            for (CompilationUnit* client : m_discoveredUnits) {
                if (client != body && client != spec
                    && std::find(dependencies[client].begin(), dependencies[client].end(), spec)
                        != dependencies[client].end()) {
                    std::size_t before = dependencies[client].size();
                    addEdge(client, body);
                    changed = changed || dependencies[client].size() != before;
                }
            }
        }
    }
    std::unordered_map<CompilationUnit*, int> states;
    std::vector<CompilationUnit*> ordered;
    std::vector<CompilationUnit*> stack;
    std::function<bool(CompilationUnit*)> visit = [&](CompilationUnit* unit) {
        if (states[unit] == 2) {
            return true;
        }
        if (states[unit] == 1) {
            std::string cycle;
            auto start = std::find(stack.begin(), stack.end(), unit);
            for (auto item = start; item != stack.end(); ++item) {
                cycle += partKey(**item) + " -> ";
            }
            cycle += partKey(*unit);
            m_diagnostics.error(unit->units.empty() ? SourceLocation {} : unit->units.front()->location,
                "library dependency cycle: " + cycle);
            return false;
        }
        states[unit] = 1;
        stack.push_back(unit);
        for (CompilationUnit* dependency : dependencies[unit]) {
            if (!visit(dependency)) {
                return false;
            }
        }
        stack.pop_back();
        states[unit] = 2;
        ordered.push_back(unit);
        return true;
    };
    for (CompilationUnit* unit : m_discoveredUnits) {
        if (!visit(unit)) {
            return false;
        }
    }
    for (const auto& [spec, body] : immediateBodies) {
        ordered.erase(std::find(ordered.begin(), ordered.end(), body));
        ordered.insert(std::find(ordered.begin(), ordered.end(), spec) + 1, body);
    }
    m_orderedUnits = std::move(ordered);
    return true;
}

std::vector<LibraryUnit> UnitLoader::libraryUnits() const
{
    std::vector<LibraryUnit> groups;
    std::vector<std::size_t> lastFile;
    std::unordered_map<std::string, std::size_t> index;

    for (std::size_t i = 0; i < m_orderedUnits.size(); ++i) {
        CompilationUnit* unit = m_orderedUnits[i];
        auto found = index.find(unit->unitKey);
        if (found == index.end()) {
            index.emplace(unit->unitKey, groups.size());
            groups.push_back(LibraryUnit { unit->unitKey, { unit } });
            lastFile.push_back(i);
        } else {
            groups[found->second].parts.push_back(unit);
            lastFile[found->second] = i;
        }
    }

    std::vector<std::size_t> order(groups.size());
    for (std::size_t i = 0; i < order.size(); ++i) {
        order[i] = i;
    }
    std::stable_sort(order.begin(), order.end(),
                     [&](std::size_t left, std::size_t right) { return lastFile[left] < lastFile[right]; });

    std::vector<LibraryUnit> result;
    result.reserve(groups.size());
    for (std::size_t position : order) {
        result.push_back(std::move(groups[position]));
    }
    return result;
}

std::vector<CompilationUnit*> UnitLoader::units() const
{
    return m_orderedUnits;
}
