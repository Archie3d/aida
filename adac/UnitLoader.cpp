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

bool isBody(const Decl& decl)
{
    return decl.kind == DeclKind::PackageBody || decl.kind == DeclKind::SubprogramBody;
}

std::string partKey(const CompilationUnit& unit)
{
    return unit.unitKey + (unit.isSpec ? ".spec" : ".body");
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
    return true;
}

bool UnitLoader::parseInto(const std::string& path, const std::string& contents, const std::string& key, bool spec)
{
    int file = m_diagnostics.addFile(path);
    Lexer lexer(contents, file, m_diagnostics);
    Parser parser(lexer.tokenize(), m_diagnostics);
    CompilationUnitPtr unit = parser.parseCompilation();
    unit->fileName = path;
    unit->unitKey = key;
    unit->isSpec = spec;
    CompilationUnit* current = unit.get();
    m_sources.emplace(sourceIdentity(path), current);

    // Publish identities before reading dependencies. Discovery can encounter a
    // back edge; orderUnits distinguishes legal body edges from spec cycles.
    for (const DeclPtr& decl : current->units) {
        std::string name = declarationName(*decl);
        if (name.empty()) {
            continue;
        }
        std::string identity = fileKey(name) + (isBody(*decl) ? ".body" : ".spec");
        auto [entry, inserted] = m_parts.emplace(identity, current);
        if (!inserted && entry->second != current) {
            m_diagnostics.error(decl->location, "duplicate library unit part '" + identity
                + "' (already declared in '" + entry->second->fileName + "')");
        }
    }

    // An explicit body needs its own spec even if no with clause mentions it.
    if (!spec) {
        std::string specPath = findUnit(key, ".ads");
        if (!specPath.empty()) {
            loadSource(specPath);
        }
    }
    for (const DeclPtr& decl : current->units) {
        std::string parent = parentOf(declarationName(*decl));
        if (!parent.empty()) {
            loadUnit(parent, decl->location);
        }
    }
    for (const WithClause& clause : current->withClauses) {
        for (const std::string& name : clause.names) {
            loadUnit(name, clause.location);
        }
    }
    m_units.push_back(std::move(unit));
    return true;
}

bool UnitLoader::loadSource(const std::string& path)
{
    addSearchPath(directoryOf(path));
    if (m_sources.contains(sourceIdentity(path))) {
        return true;
    }
    std::string contents;
    if (!readFile(path, contents)) {
        m_diagnostics.error(SourceLocation {}, "cannot read '" + path + "'");
        return false;
    }
    std::filesystem::path source(path);
    std::string key = toLower(source.stem().string());
    bool spec = toLower(source.extension().string()) == ".ads";
    if (!parseInto(path, contents, key, spec)) {
        return false;
    }
    if (spec && m_explicitPaths.contains(toLower(source.filename().string()))
        && (!m_specificationsOnly || key == m_targetKey
            || declaresGeneric(m_sources.at(sourceIdentity(path))->units))) {
        std::string body = findUnit(key, ".adb");
        if (!body.empty()) {
            return loadSource(body);
        }
    }
    return true;
}

bool UnitLoader::loadUnit(const std::string& name, const SourceLocation& from)
{
    std::string key = fileKey(name);
    std::string specPath = findUnit(key, ".ads");
    std::string bodyPath = findUnit(key, ".adb");
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
        generic = declaresGeneric(m_sources.at(sourceIdentity(specPath))->units);
    }
    bool bodyWanted = !m_specificationsOnly || key == m_targetKey || specPath.empty() || generic;
    if (!bodyPath.empty() && bodyWanted && !loadSource(bodyPath)) {
        return false;
    }
    // Explicit multi-unit source files keep their historical file-based object
    // names. Library lookup must nevertheless find the declaration requested.
    std::string selected = specPath.empty() ? bodyPath : specPath;
    const CompilationUnit* source = m_sources.at(sourceIdentity(selected));
    bool namesUnit = std::any_of(source->units.begin(), source->units.end(), [&](const DeclPtr& decl) {
        return fileKey(declarationName(*decl)) == key;
    });
    if (!namesUnit && key != m_targetKey && !m_diagnostics.hasErrors()) {
        m_diagnostics.error(from, "source '" + selected + "' does not declare library unit '" + name + "'");
        return false;
    }
    return true;
}

bool UnitLoader::orderUnits()
{
    std::unordered_map<CompilationUnit*, std::vector<CompilationUnit*>> dependencies;
    for (const CompilationUnitPtr& unit : m_units) {
        unit->dependencies.clear();
        auto add = [&](const std::string& name, bool ownBody, bool sameSourceAllowed) {
            std::string key = fileKey(name);
            auto found = m_parts.find(key + ".spec");
            if (found == m_parts.end() && !ownBody) {
                found = m_parts.find(key + ".body");
            }
            if (found == m_parts.end() || (sameSourceAllowed && found->second == unit.get())) {
                return;
            }
            auto& edges = dependencies[unit.get()];
            if (std::find(edges.begin(), edges.end(), found->second) == edges.end()) {
                edges.push_back(found->second);
                unit->dependencies.push_back(partKey(*found->second));
            }
        };
        for (const DeclPtr& decl : unit->units) {
            std::string name = declarationName(*decl);
            if (isBody(*decl)) {
                add(name, true, true);
            }
            std::string parent = parentOf(name);
            if (!parent.empty()) {
                std::string parentKey = fileKey(parent);
                if (!m_parts.contains(parentKey + ".spec")) {
                    m_diagnostics.error(decl->location, "cannot find parent unit '" + parent + "'");
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
                add(name, false, unit->units.size() > 1);
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
    for (const CompilationUnitPtr& unit : m_units) {
        if (!visit(unit.get())) {
            return false;
        }
    }
    std::unordered_map<CompilationUnit*, std::size_t> positions;
    for (std::size_t i = 0; i < ordered.size(); ++i) {
        positions.emplace(ordered[i], i);
    }
    std::sort(m_units.begin(), m_units.end(), [&](const auto& left, const auto& right) {
        return positions.at(left.get()) < positions.at(right.get());
    });
    return true;
}

std::vector<LibraryUnit> UnitLoader::libraryUnits() const
{
    std::vector<LibraryUnit> groups;
    std::vector<std::size_t> lastFile;
    std::unordered_map<std::string, std::size_t> index;

    for (std::size_t i = 0; i < m_units.size(); ++i) {
        CompilationUnit* unit = m_units[i].get();
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
    std::vector<CompilationUnit*> result;
    for (const CompilationUnitPtr& unit : m_units) {
        result.push_back(unit.get());
    }
    return result;
}
