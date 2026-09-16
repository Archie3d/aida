#include "Binder.h"
#include "Diagnostics.h"
#include "Lexer.h"
#include "QbeEmitter.h"
#include "Sema.h"
#include "UnitLoader.h"
#include "UnitManifest.h"

#include "Digest.h"

#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#ifdef _WIN32
#   include <cstdlib>
#else
#   include <climits>
#endif

#ifndef ADA_LIBRARY_DEFAULT_PATH
#define ADA_LIBRARY_DEFAULT_PATH ""
#endif

#ifndef ADA_INSTALL_LIBDIR
#define ADA_INSTALL_LIBDIR "lib"
#endif

namespace
{

void printUsage()
{
    std::cerr << "AIDA Ada compiler to QBE IR\n"
              << "usage: adac [options] <source> [<source>...]\n"
              << "\n"
              << "  -o <file>       name of the produced file, '-' for standard output\n"
              << "  -D <dir>        write one file per library unit into this directory\n"
              << "  -c              compile the named source alone, against the\n"
              << "                  specifications of what it draws on\n"
              << "  --scan          only work out which units the program is made of\n"
              << "  --bind <unit>   only write the entry point, for the program whose\n"
              << "                  main subprogram that unit holds\n"
              << "  -I <dir>        another directory to look for units in\n"
              << "  --stdlib <dir>  where the predefined environment lives\n"
              << "  --no-stdlib     leave the predefined environment out altogether\n";
}

// The name a unit's intermediate language is filed under, which is also what
// the object built from it is named after.
const char* const manifestName = "units.manifest";
const char* const binderName = "__ada_binder";

bool ensureDirectory(const std::string& path)
{
    struct stat information;
    if (stat(path.c_str(), &information) == 0) {
        return S_ISDIR(information.st_mode);
    }
#ifdef _WIN32
    return _mkdir(path.c_str()) == 0;
#else
    return mkdir(path.c_str(), 0777) == 0;
#endif
}

std::string defaultOutputName(const std::string& path)
{
    std::size_t dot = path.find_last_of('.');
    std::size_t slash = path.find_last_of("/\\");
    if (dot != std::string::npos && (slash == std::string::npos || dot > slash)) {
        return path.substr(0, dot) + ".ssa";
    }
    return path + ".ssa";
}

// The loader names a unit after its file, so a source on the command line says
// which unit it belongs to without anything having been read.
std::string unitKeyOf(const std::string& path)
{
    std::size_t slash = path.find_last_of("/\\");
    std::string base = slash == std::string::npos ? path : path.substr(slash + 1);
    std::size_t dot = base.find_last_of('.');
    return toLower(dot == std::string::npos ? base : base.substr(0, dot));
}

// What the driver has to know about a unit to tell whether anything has
// reached it: its own text, and the declarations another unit compiles against.
UnitRecord describe(const LibraryUnit& unit)
{
    UnitRecord record;
    record.key = unit.key;

    std::string text;
    for (CompilationUnit* part : unit.parts) {
        record.sources.push_back(part->fileName);
        std::string contents = digestOfFile(part->fileName);
        text += contents;
        if (part->isSpec) {
            record.specDigest = contents;
        }
        for (const WithClause& clause : part->withClauses) {
            for (const std::string& name : clause.names) {
                std::string key = toLower(name);
                for (char& c : key) {
                    if (c == '.') {
                        c = '-';
                    }
                }
                record.withKeys.push_back(key);
            }
        }
    }
    record.digest = digestOf(text);

    // A unit with no specification is compiled against its body, so that is
    // what a change to it has to be measured against.
    if (record.specDigest.empty()) {
        record.specDigest = record.digest;
    }
    return record;
}

// Several directories travel in one string, separated the way a search path is.
void addSeparatedPaths(UnitLoader& loader, const char* text)
{
    if (text == nullptr) {
        return;
    }
    std::string list = text;
    std::size_t start = 0;
    while (start <= list.size()) {
        std::size_t separator = list.find(':', start);
        std::size_t end = separator == std::string::npos ? list.size() : separator;
        loader.addSearchPath(list.substr(start, end - start));
        if (separator == std::string::npos) {
            break;
        }
        start = separator + 1;
    }
}

bool directoryExists(const std::string& path)
{
    struct stat information;
    return !path.empty() && stat(path.c_str(), &information) == 0 && S_ISDIR(information.st_mode);
}

std::string directoryOf(const std::string& path)
{
    std::size_t slash = path.find_last_of("/\\");
    if (slash == std::string::npos) {
        return std::string();
    }
    return path.substr(0, slash);
}

// A name invoked without a slash was found on PATH, and that is the only clue
// to where adac was installed.
std::string alongPath(const std::string& name)
{
    const char* path = std::getenv("PATH");
    if (path == nullptr) {
        return std::string();
    }
    std::string list = path;
    std::size_t start = 0;
    while (start <= list.size()) {
        std::size_t separator = list.find(':', start);
        std::size_t end = separator == std::string::npos ? list.size() : separator;
        std::string candidate = list.substr(start, end - start) + "/" + name;
        if (access(candidate.c_str(), X_OK) == 0) {
            return candidate;
        }
        if (separator == std::string::npos) {
            break;
        }
        start = separator + 1;
    }
    return std::string();
}

// Symbolic links are followed, so a link on PATH pointing into an install
// leads back to the install rather than to wherever the link happens to sit.
std::string resolvedPath(const std::string& path)
{
#ifdef _WIN32
    char resolved[_MAX_PATH];
    if (_fullpath(resolved, path.c_str(), _MAX_PATH) != nullptr) {
        return resolved;
    }
#else
    char resolved[PATH_MAX];
    if (realpath(path.c_str(), resolved) != nullptr) {
        return resolved;
    }
#endif
    return path;
}

// An install has bin/adac beside lib/ada/adainclude.  Run in place, in a
// build tree or from an unrelated directory, adac has no such neighbour and
// falls back to whatever ADA_LIBRARY_DEFAULT_PATH names instead.
std::string libraryBesideExecutable(const std::string& executablePath)
{
    std::string found = executablePath.find_first_of("/\\") == std::string::npos
        ? alongPath(executablePath)
        : executablePath;
    std::string directory = directoryOf(resolvedPath(found));
    if (directory.empty()) {
        return std::string();
    }
    std::string candidate = directory + "/../" ADA_INSTALL_LIBDIR "/ada/adainclude";
    return directoryExists(candidate) ? candidate : std::string();
}

}

int main(int argc, char** argv)
{
    std::vector<std::string> inputs;
    std::vector<std::string> includes;
    std::string output;
    std::string artifacts;
    std::string bindMainUnit;
    std::string library = ADA_LIBRARY_DEFAULT_PATH;
    bool librarySelected = false;
    bool singleUnit = false;
    bool scanOnly = false;
    bool bindOnly = false;

    for (int i = 1; i < argc; ++i) {
        std::string argument = argv[i];
        if (argument == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "adac: error: missing file name after '-o'\n";
                return 2;
            }
            output = argv[++i];
        } else if (argument == "-D") {
            if (i + 1 >= argc) {
                std::cerr << "adac: error: missing directory after '-D'\n";
                return 2;
            }
            artifacts = argv[++i];
        } else if (argument == "-c") {
            singleUnit = true;
        } else if (argument == "--scan") {
            scanOnly = true;
        } else if (argument == "--bind") {
            if (i + 1 >= argc) {
                std::cerr << "adac: error: missing unit after '--bind'\n";
                return 2;
            }
            bindMainUnit = argv[++i];
            bindOnly = true;
        } else if (argument == "-I") {
            if (i + 1 >= argc) {
                std::cerr << "adac: error: missing directory after '-I'\n";
                return 2;
            }
            includes.push_back(argv[++i]);
        } else if (argument.size() > 2 && argument.compare(0, 2, "-I") == 0) {
            includes.push_back(argument.substr(2));
        } else if (argument == "--stdlib") {
            if (i + 1 >= argc) {
                std::cerr << "adac: error: missing directory after '--stdlib'\n";
                return 2;
            }
            library = argv[++i];
            librarySelected = true;
        } else if (argument == "--no-stdlib") {
            library.clear();
            librarySelected = true;
        } else if (argument == "-h" || argument == "--help") {
            printUsage();
            return 0;
        } else if (!argument.empty() && argument[0] == '-' && argument != "-") {
            std::cerr << "adac: error: unknown option '" << argument << "'\n";
            return 2;
        } else {
            inputs.push_back(argument);
        }
    }

    std::string executable = argv[0];
    if (executable.find_first_of("/\\") == std::string::npos) {
        executable = alongPath(executable);
    }
    const std::string compilerDigest = digestOfFile(resolvedPath(executable));

    if (bindOnly) {
        if (artifacts.empty()) {
            std::cerr << "adac: error: '--bind' needs the directory named by '-D'\n";
            return 2;
        }
        std::vector<UnitRecord> units;
        std::vector<std::string> keys;
        if (!readManifest(artifacts + "/" + manifestName, units, &keys) || units.empty() || keys.empty()) {
            std::cerr << "adac: error: cannot read '" << artifacts << "/" << manifestName << "'\n";
            return 2;
        }

        UnitRecordFile record;
        bool foundMain = false;
        for (const UnitRecord& unit : units) {
            UnitRecordFile compiled;
            if (!readUnitRecord(artifacts + "/" + unit.key + ".ali", compiled)
                || compiled.compilerDigest != compilerDigest
                || !unitRecordMatches(compiled, unit, units)
                || compiled.irDigest.empty()
                || compiled.irDigest != digestOfFile(artifacts + "/" + unit.key + ".ssa")) {
                std::cerr << "adac: error: missing or stale compiled unit '" << unit.key << "'\n";
                return 1;
            }
            if (unit.key == bindMainUnit) {
                record = compiled;
                foundMain = true;
            }
        }
        if (!foundMain || record.mainName.empty()) {
            std::cerr << "adac: error: no main subprogram in unit '" << bindMainUnit << "'\n";
            return 1;
        }

        std::string path = artifacts + "/" + binderName + ".ssa";
        std::ofstream binder(path);
        if (!binder) {
            std::cerr << "adac: error: cannot write '" << path << "'\n";
            return 2;
        }
        emitBinder(keys, record.mainName, binder);
        return 0;
    }

    if (inputs.empty()) {
        printUsage();
        return 2;
    }
    if (output.empty() && artifacts.empty()) {
        output = defaultOutputName(inputs.back());
    }

    // Neither --stdlib nor --no-stdlib was given, so adac is on its own: an
    // installed adac looks beside itself before falling back to the path
    // settled at build time.
    if (!librarySelected) {
        std::string beside = libraryBesideExecutable(argv[0]);
        if (!beside.empty()) {
            library = beside;
        }
    }

    Diagnostics diagnostics;
    UnitLoader loader(diagnostics);

    // A unit is looked for beside the source that asked for it first, then
    // where the command line says, then the environment, then in the library
    // that came with the compiler.
    // The driver supplies the scan's ordered search roots through -I when
    // compiling one unit; keep those ahead of the target's own directory.
    if (singleUnit) {
        for (const std::string& directory : includes) {
            loader.addSearchPath(directory);
        }
    }
    for (const std::string& path : inputs) {
        std::size_t slash = path.find_last_of("/\\");
        loader.addSearchPath(slash == std::string::npos ? "." : path.substr(0, slash));
    }
    for (const std::string& directory : includes) {
        loader.addSearchPath(directory);
    }
    addSeparatedPaths(loader, std::getenv("ADA_INCLUDE_PATH"));
    addSeparatedPaths(loader, library.c_str());

    // Compiling one unit needs no more of what it draws on than the
    // declarations, so the bodies of its dependencies stay unread.
    loader.setSpecificationsOnly(singleUnit);

    // The run time raises its exceptions by number, so the package declaring
    // them is part of every program whether or not it was asked for.
    loader.loadUnit("Ada.IO_Exceptions", SourceLocation {});

    if (singleUnit) {
        // The unit is read by name rather than by file, so that a body arrives
        // together with the specification it completes.
        std::string key = unitKeyOf(inputs.back());
        std::string name = key;
        for (char& c : name) {
            if (c == '-') {
                c = '.';
            }
        }
        loader.setTargetUnit(key);
        if (!loader.loadUnit(name, SourceLocation {})) {
            std::cerr << "adac: error: cannot find the unit '" << inputs.back() << "'\n";
            return 2;
        }
    } else {
        for (const std::string& path : inputs) {
            if (!loader.loadSource(path)) {
                return 2;
            }
        }
    }
    if (diagnostics.hasErrors()) {
        return 1;
    }

    std::vector<LibraryUnit> libraryUnits = loader.libraryUnits();
    std::vector<std::string> elaborations;
    for (CompilationUnit* part : loader.units()) {
        elaborations.push_back(part->unitKey + (part->isSpec ? ".spec" : ".body"));
    }

    if (scanOnly) {
        if (artifacts.empty() || !ensureDirectory(artifacts)) {
            std::cerr << "adac: error: '--scan' needs the directory named by '-D'\n";
            return 2;
        }
        std::vector<UnitRecord> records;
        for (const LibraryUnit& unit : libraryUnits) {
            records.push_back(describe(unit));
        }
        if (!writeManifest(artifacts + "/" + manifestName, records, elaborations)) {
            std::cerr << "adac: error: cannot write '" << artifacts << "/" << manifestName << "'\n";
            return 2;
        }
        return 0;
    }

    std::vector<CompilationUnit*> units = loader.units();

    Sema sema(diagnostics);
    for (CompilationUnit* unit : units) {
        sema.analyze(*unit);
    }
    if (diagnostics.hasErrors()) {
        return 1;
    }

    QbeEmitter emitter(sema, diagnostics);

    // Compiling one unit emits that unit and nothing else: whatever it draws on
    // was read to make sense of it, and is emitted when its own turn comes.
    if (singleUnit) {
        const LibraryUnit* target = nullptr;
        for (const LibraryUnit& unit : libraryUnits) {
            if (unit.key == unitKeyOf(inputs.back())) {
                target = &unit;
            }
        }
        if (target == nullptr) {
            std::cerr << "adac: error: '" << inputs.back() << "' is not a library unit\n";
            return 2;
        }
        if (artifacts.empty() || !ensureDirectory(artifacts)) {
            std::cerr << "adac: error: '-c' needs the directory named by '-D'\n";
            return 2;
        }

        std::string path = artifacts + "/" + target->key + ".ssa";
        std::ofstream stream(path);
        if (!stream) {
            std::cerr << "adac: error: cannot write '" << path << "'\n";
            return 2;
        }
        emitter.emitUnit(*target, stream);

        UnitRecordFile record;
        stream.close();
        if (!stream || diagnostics.hasErrors()) {
            std::cerr << "adac: error: could not compile unit '" << target->key << "'\n";
            return 1;
        }
        record.compilerDigest = compilerDigest;
        record.irDigest = digestOfFile(path);
        record.digest = describe(*target).digest;
        for (const LibraryUnit& unit : libraryUnits) {
            if (unit.key != target->key) {
                bool readsBody = false;
                for (CompilationUnit* part : unit.parts) {
                    readsBody = readsBody || !part->isSpec;
                }
                if (readsBody) {
                    record.bodyDependencies.emplace_back(unit.key, describe(unit).digest);
                } else {
                    record.dependencies.emplace_back(unit.key, describe(unit).specDigest);
                }
            }
        }
        // Only a main declared by this unit belongs in its record.
        for (CompilationUnit* part : target->parts) {
            for (const DeclPtr& declaration : part->units) {
                if (declaration->kind == DeclKind::SubprogramBody) {
                    auto* body = static_cast<SubprogramBody*>(declaration.get());
                    if (body->symbol != nullptr && !body->spec.isFunction && body->spec.parameters.empty()) {
                        record.mainName = body->symbol->qbeName;
                    }
                }
            }
        }
        if (!writeUnitRecord(artifacts + "/" + target->key + ".ali", record)) {
            std::cerr << "adac: error: cannot write '" << artifacts << "/" << target->key << ".ali'\n";
            return 2;
        }
        return diagnostics.hasErrors() ? 1 : 0;
    }

    if (!artifacts.empty()) {
        if (!ensureDirectory(artifacts)) {
            std::cerr << "adac: error: cannot use '" << artifacts << "' as an output directory\n";
            return 2;
        }

        std::vector<UnitRecord> records;
        for (const LibraryUnit& unit : libraryUnits) {
            std::string path = artifacts + "/" + unit.key + ".ssa";
            std::ofstream stream(path);
            if (!stream) {
                std::cerr << "adac: error: cannot write '" << path << "'\n";
                return 2;
            }
            emitter.emitUnit(unit, stream);
            records.push_back(describe(unit));
        }

        std::string binderPath = artifacts + "/" + binderName + ".ssa";
        std::ofstream binder(binderPath);
        if (!binder) {
            std::cerr << "adac: error: cannot write '" << binderPath << "'\n";
            return 2;
        }
        Symbol* main = sema.mainSubprogram();
        emitBinder(elaborations, main == nullptr ? std::string() : main->qbeName, binder);

        // The driver reads this rather than guessing what was produced, so a
        // unit that stops being part of the program stops being linked too.
        if (!writeManifest(artifacts + "/" + manifestName, records, elaborations)) {
            std::cerr << "adac: error: cannot write '" << artifacts << "/" << manifestName << "'\n";
            return 2;
        }
    } else {
        std::ostream* stream = &std::cout;
        std::ofstream file;
        if (output != "-") {
            file.open(output);
            if (!file) {
                std::cerr << "adac: error: cannot write '" << output << "'\n";
                return 2;
            }
            stream = &file;
        }
        for (const LibraryUnit& unit : libraryUnits) {
            emitter.emitUnit(unit, *stream);
        }
        Symbol* main = sema.mainSubprogram();
        emitBinder(elaborations, main == nullptr ? std::string() : main->qbeName, *stream);
    }

    return diagnostics.hasErrors() ? 1 : 0;
}
