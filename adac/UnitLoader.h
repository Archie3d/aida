#pragma once

#include "Ast.h"
#include "Diagnostics.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

// Finds the Ada source of a library unit and reads in everything a program
// draws on, so that a `with` clause means what it says rather than obliging the
// caller to name every file on the command line.
//
// A unit is stored under the name GNAT gives it: lower cased, with each dot of
// the name written as a dash, so Ada.Text_IO.Integer_IO lives in
// ada-text_io-integer_io.ads with its body beside it in the .adb file.
// Subunits use the same naming rule for Parent.Name and join the parent's
// object group after expansion at their stubs.
class UnitLoader
{
public:
    explicit UnitLoader(Diagnostics& diagnostics);

    // Reads only the specification of a unit reached through a with clause.
    // The body still comes in when the specification declares a generic, since
    // an instantiation parses the whole of it again.  Compiling one unit at a
    // time needs no more of its dependencies than this.
    void setSpecificationsOnly(bool specificationsOnly) { m_specificationsOnly = specificationsOnly; }

    // The unit being compiled, which is read whole whatever the setting above
    // says, since its body is what there is to generate code from.
    void setTargetUnit(const std::string& key) { m_targetKey = key; }

    void addSearchPath(const std::string& directory);

    // Parse and register explicit paths and item identities before following
    // dependencies, so source argument order cannot hide bundled declarations.
    bool addSource(const std::string& path);

    // Reads a file named on the command line along with everything it withs.
    bool loadSource(const std::string& path);

    // Object-group identity of a registered source, including path aliases.
    std::string sourceKey(const std::string& path) const;

    // Reads a unit by name, doing nothing if it has already been read.
    bool loadUnit(const std::string& name, const SourceLocation& from);

    // Validate and order the discovered specification/body dependency graph.
    bool orderUnits();

    // The units in the order they have to be analysed: whatever a unit draws on
    // comes before it, and the library comes before the program.
    std::vector<CompilationUnit*> units() const;

    // The same files grouped into object-producing library units. Elaboration
    // follows units() instead: specification and body may need other units
    // elaborated between them.
    std::vector<LibraryUnit> libraryUnits() const;

private:
    void expandSubunits(CompilationUnit& unit);
    std::vector<Token> resolveSubunit(CompilationUnit& owner, const std::string& parent,
        const std::string& name, DeclKind kind, const SourceLocation& location);
    bool loadSubunitParent(CompilationUnit& unit);
    void registerParts(const std::string& key);
    bool validateParts();
    bool readFile(const std::string& path, std::string& contents) const;
    std::string findUnit(const std::string& key, const char* extension) const;
    bool loadPart(const std::string& path, const std::string& identity, const SourceLocation& from);
    bool parseInto(const std::string& path, const std::string& contents, const std::string& key);

    Diagnostics& m_diagnostics;
    std::vector<std::string> m_searchPaths;
    std::vector<CompilationUnitPtr> m_units;
    std::vector<CompilationUnit*> m_discoveredUnits;
    std::vector<CompilationUnit*> m_orderedUnits;
    std::unordered_map<std::string, std::string> m_explicitPaths;
    std::unordered_map<std::string, std::vector<CompilationUnit*>> m_sources;
    std::unordered_set<std::string> m_loadedSources;
    std::unordered_map<std::string, CompilationUnit*> m_parts;
    std::unordered_map<std::string, std::vector<CompilationUnit*>> m_items;
    std::unordered_map<std::string, std::vector<CompilationUnit*>> m_subunits;
    std::unordered_set<CompilationUnit*> m_usedSubunits;
    std::unordered_set<CompilationUnit*> m_expandedUnits;
    bool m_specificationsOnly = false;
    std::string m_targetKey;
};
