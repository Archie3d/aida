#include "UnitLoader.h"

#include "Lexer.h"
#include "Parser.h"

#include <algorithm>
#include <fstream>
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
    for (const std::string& directory : m_searchPaths) {
        std::string candidate = directory + "/" + key + extension;
        std::ifstream probe(candidate, std::ios::binary);
        if (probe) {
            return candidate;
        }
    }
    return std::string();
}

bool UnitLoader::parseInto(const std::string& path, const std::string& contents, const std::string& key, bool isSpec)
{
    int file = m_diagnostics.addFile(path);
    Lexer lexer(contents, file, m_diagnostics);
    Parser parser(lexer.tokenize(), m_diagnostics);
    CompilationUnitPtr unit = parser.parseCompilation();
    unit->fileName = path;
    unit->unitKey = key;
    unit->isSpec = isSpec;

    // Whatever this unit draws on is read first, so that it lands ahead of the
    // unit naming it and Sema meets a declaration before any use of it.
    for (const WithClause& clause : unit->withClauses) {
        for (const std::string& name : clause.names) {
            loadUnit(name, clause.location);
        }
    }

    m_units.push_back(std::move(unit));
    return true;
}

bool UnitLoader::loadSource(const std::string& path)
{
    // A program finds its own units beside it.
    addSearchPath(directoryOf(path));

    if (m_states.find(path) != m_states.end()) {
        return true;
    }
    m_states.emplace(path, State::Loaded);

    // The file name gives the unit away, so a source named here is not read a
    // second time on account of something withing it.
    std::string base = path.substr(directoryOf(path) == "." && path.find('/') == std::string::npos
                                       ? 0
                                       : directoryOf(path).size() + 1);
    std::size_t dot = base.find_last_of('.');
    std::string key = toLower(dot == std::string::npos ? base : base.substr(0, dot));
    if (dot != std::string::npos) {
        m_states.emplace(key, State::Loaded);
    }

    std::string contents;
    if (!readFile(path, contents)) {
        m_diagnostics.error(SourceLocation {}, "cannot read '" + path + "'");
        return false;
    }

    bool isSpec = dot != std::string::npos && toLower(base.substr(dot)) == ".ads";
    return parseInto(path, contents, key, isSpec);
}

bool UnitLoader::loadUnit(const std::string& name, const SourceLocation& from)
{
    std::string key = fileKey(name);

    auto seen = m_states.find(key);
    if (seen != m_states.end()) {
        // A unit already being read is one that withs its way back to itself.
        if (seen->second == State::Loading) {
            m_diagnostics.error(from, "'" + name + "' depends on itself");
            return false;
        }
        return true;
    }

    std::string parent = parentOf(name);
    if (!parent.empty() && !loadUnit(parent, from)) {
        return false;
    }

    std::string specPath = findUnit(key, ".ads");
    std::string bodyPath = findUnit(key, ".adb");
    if (specPath.empty() && bodyPath.empty()) {
        // Standard and System are declared by the compiler and have no file to
        // read.  Sema reports the with clause if the name turns out to stand
        // for no unit at all.
        return false;
    }

    m_states.emplace(key, State::Loading);

    std::string contents;
    if (!specPath.empty()) {
        m_states.emplace(specPath, State::Loaded);
        if (!readFile(specPath, contents)) {
            m_diagnostics.error(from, "cannot read '" + specPath + "'");
            return false;
        }
        parseInto(specPath, contents, key, true);
    }

    // The specification is in place, so a body that draws on something naming
    // this unit back is no longer a circle.
    m_states[key] = State::Loaded;

    // A generic is kept as the tokens it was written with, and an instantiation
    // parses them again, so a specification declaring one is not complete
    // without its body.
    bool bodyWanted = !m_specificationsOnly || key == m_targetKey || specPath.empty() || m_units.empty()
        || declaresGeneric(m_units.back()->units);

    if (!bodyPath.empty() && bodyWanted) {
        m_states.emplace(bodyPath, State::Loaded);
        if (!readFile(bodyPath, contents)) {
            m_diagnostics.error(from, "cannot read '" + bodyPath + "'");
            return false;
        }
        parseInto(bodyPath, contents, key, false);
    }

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
