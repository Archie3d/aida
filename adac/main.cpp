#include "Binder.h"
#include "Diagnostics.h"
#include "Lexer.h"
#include "QbeEmitter.h"
#include "Sema.h"
#include "SemanticInterface.h"
#include "UnitLoader.h"
#include "UnitManifest.h"

#include "Digest.h"
#include "../support/HostPaths.h"
#include "../support/Artifacts.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#ifdef _WIN32
#   include <cstdlib>
#   include <direct.h>
#else
#   include <climits>
#endif

#ifndef ADA_LIBRARY_DEFAULT_PATH
#define ADA_LIBRARY_DEFAULT_PATH ""
#endif

#ifndef ADA_INSTALL_LIBDIR
#define ADA_INSTALL_LIBDIR "lib"
#endif

namespace {

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
              << "  -I <dir>        another directory for sources or compiled .asi interfaces\n"
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

// Select the entry point from the requested object group, never from a
// dependency that happens to contain a parameterless procedure.
Symbol* mainIn(const LibraryUnit& unit)
{
    Symbol* main = nullptr;
    for (CompilationUnit* part : unit.parts) {
        for (const DeclPtr& declaration : part->units) {
            Symbol* candidate = nullptr;
            if (declaration->kind == DeclKind::SubprogramBody) {
                candidate = static_cast<SubprogramBody*>(declaration.get())->symbol;
            } else if (declaration->kind == DeclKind::SubprogramDeclaration) {
                auto* renaming = static_cast<SubprogramDecl*>(declaration.get());
                if (!renaming->renamingExpansion.empty()) {
                    candidate = renaming->symbol;
                }
            }
            if (candidate != nullptr && candidate->returnType == nullptr && candidate->parameters.empty()) {
                main = candidate;
            }
        }
    }
    return main;
}

// What the driver has to know about a unit to tell whether anything has
// reached it: its own text, and the declarations another unit compiles against.
UnitRecord describe(const LibraryUnit& unit, const UnitLoader& loader)
{
    if (const UnitRecord* imported = loader.importedRecord(unit.key)) { return *imported; }
    UnitRecord record;
    record.key = unit.key;

    std::string text;
    for (CompilationUnit* part : unit.parts) {
        for (const std::string& source : part->subunitSources) {
            if (std::find(record.sources.begin(), record.sources.end(), source) == record.sources.end()) {
                record.sources.push_back(source);
            }
        }
        if (std::find(record.sources.begin(), record.sources.end(), part->fileName) == record.sources.end()) {
            record.sources.push_back(part->fileName);
        }
        record.parts.push_back(UnitPartRecord {
            part->partKey, part->dependencies, part->units.front()->elaborateBody
                && part->units.front()->kind != DeclKind::GenericInstantiation, part->limitedDependencies, part->category,
                part->fileName, part->semanticDependencies, part->clientNeedsBody });
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
    // Source digests must be independent of how many items a file contains
    // and of interleaving with dependency bodies during graph traversal.
    std::stable_sort(record.sources.begin(), record.sources.end(), [](const auto& left, const auto& right) {
        return toLower(std::filesystem::path(left).extension().string()) == ".ads"
            && toLower(std::filesystem::path(right).extension().string()) != ".ads";
    });
    std::string specText;
    std::string identity;
    std::string specIdentity;
    for (const std::string& source : record.sources) {
        std::string contents = loader.sourceDigest(source);
        text += contents;
        std::string sourceIdentity = loader.sourceProvenance(source);
        sourceIdentity.push_back('\0');
        identity += sourceIdentity;
        if (std::any_of(unit.parts.begin(), unit.parts.end(), [&](const CompilationUnit* part) {
            return part->fileName == source && part->isSpec;
        })) {
            specText += contents;
            specIdentity += sourceIdentity;
        }
    }
    record.sourceIdentity = digestOf(identity);
    record.specIdentity = specIdentity.empty() ? record.sourceIdentity : digestOf(specIdentity);
    record.digest = digestOf(text);
    if (!specText.empty()) {
        record.specDigest = digestOf(specText);
    }

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
    for (const std::string& path : HostPaths::splitList(text)) {
        loader.addSearchPath(path);
    }
}

bool directoryExists(const std::string& path)
{
    struct stat information;
    return !path.empty() && stat(path.c_str(), &information) == 0 && S_ISDIR(information.st_mode);
}

// A name invoked without a slash was found on PATH, and that is the only clue
// to where adac was installed.
std::string alongPath(const std::string& name)
{
    const char* path = std::getenv("PATH");
    if (path == nullptr) {
        return std::string();
    }
    for (const std::string& directory : HostPaths::splitList(path)) {
        std::string candidate = (directory.empty() ? "." : directory) + "/" + name;
        if (access(candidate.c_str(), X_OK) == 0) {
            return candidate;
        }
#ifdef _WIN32
        candidate += ".exe";
        if (access(candidate.c_str(), X_OK) == 0) {
            return candidate;
        }
#endif
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
    std::string directory = HostPaths::directoryOf(resolvedPath(found));
    if (directory.empty()) {
        return std::string();
    }
    std::string candidate = directory + "/../" ADA_INSTALL_LIBDIR "/ada/adainclude";
    return directoryExists(candidate) ? candidate : std::string();
}

} // namespace

int main(int argc, char** argv)
{
    std::string buildInputsOutput;
    std::string inheritedLock;
    bool strictSearchRoots = false;
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
        } else if (argument == "--build-inputs") {
            if (++i >= argc) { std::cerr << "adac: error: missing input snapshot filename\n"; return 2; }
            buildInputsOutput = argv[i];
        } else if (argument == "--artifact-lock") {
            if (++i >= argc) { std::cerr << "adac: error: missing artifact lock handle\n"; return 2; }
            inheritedLock = argv[i];
        } else if (argument == "--search-roots-only") {
            strictSearchRoots = true;
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

    if (output.empty() && artifacts.empty() && !inputs.empty()) { output = defaultOutputName(inputs.back()); }
    std::string lockDirectory = artifacts;
    if (lockDirectory.empty() && !output.empty() && output != "-") {
        lockDirectory = HostPaths::directoryOf(output);
        if (lockDirectory.empty()) { lockDirectory = "."; }
    }
    Artifacts::DirectoryLock buildLock;
    if (!lockDirectory.empty() && !buildLock.acquire(lockDirectory, inheritedLock)) {
        std::cerr << "adac: error: cannot lock artifact directory: " << buildLock.error() << "\n";
        return 1;
    }
    if (lockDirectory.empty() && !inheritedLock.empty()) {
        std::cerr << "adac: error: inherited artifact lock needs a file output or -D\n";
        return 2;
    }

    std::string executable = argv[0];
    if (executable.find_first_of("/\\") == std::string::npos) {
        executable = alongPath(executable);
    }
    const std::string compilerPath = resolvedPath(executable);
    const std::string compilerDigest = digestOfFile(compilerPath);

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
        std::string orderError = validateElaborationOrder(units, keys);
        if (!orderError.empty()) {
            std::cerr << "adac: error: " << orderError << "\n";
            return 1;
        }

        UnitRecordFile record;
        bool foundMain = false;
        for (const UnitRecord& unit : units) {
            UnitRecordFile compiled;
            if (!readUnitRecord(artifacts + "/" + unit.key + ".ali", compiled)
                || compiled.compilerDigest != compilerDigest
                || !unitRecordMatches(compiled, unit, units)
                || !unitArtifactsMatch(artifacts, unit, compiled)) {
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
        Artifacts::AtomicFile publication(path);
        auto& binder = publication.stream();
        if (!binder) {
            std::cerr << "adac: error: cannot write '" << path << "'\n";
            return 2;
        }
        emitBinder(keys, record.mainName, binder);
        if (!publication.commit()) { std::cerr << "adac: error: cannot publish binder\n"; return 2; }
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
    loader.setCompilerDigest(compilerDigest);
    auto inputSnapshot = [&]() {
        auto inputs = loader.inputSnapshot();
        inputs.emplace_back(executable, Artifacts::fileFingerprint(executable, &compilerDigest));
        inputs.emplace_back(compilerPath, Artifacts::fileFingerprint(compilerPath, &compilerDigest));
        return inputs;
    };

    loader.setImplicitSourcePaths(!strictSearchRoots);
    if (strictSearchRoots || singleUnit) {
        for (const std::string& directory : includes) { loader.addSearchPath(directory); }
    }
    if (!strictSearchRoots) {
        for (const std::string& path : inputs) {
            loader.addSearchPath(HostPaths::directoryOf(path).empty() ? "." : HostPaths::directoryOf(path));
        }
        for (const std::string& directory : includes) { loader.addSearchPath(directory); }
        addSeparatedPaths(loader, std::getenv("ADA_INCLUDE_PATH"));
        addSeparatedPaths(loader, library.c_str());
    }

    // Source-based dependency bodies stay unread unless needed for generics.
    // Imported replay interfaces retain their full checked dependency closure.
    loader.setSpecificationsOnly(singleUnit);

    for (const std::string& path : inputs) {
        if (!loader.addSource(path)) {
            return 1;
        }
    }

    // The run time raises its exceptions by number, so the package declaring
    // them is part of every program whether or not it was asked for.
    loader.loadUnit("Ada.IO_Exceptions", SourceLocation {});

    if (singleUnit) {
        // The target is an explicit source, not necessarily a conventionally
        // named library unit. Its declarations drive counterpart discovery.
        loader.setTargetUnit(loader.sourceKey(inputs.back()));
        if (!loader.loadSource(inputs.back())) {
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

    if (!loader.orderUnits()) {
        return 1;
    }

    std::vector<LibraryUnit> libraryUnits = loader.libraryUnits();
    std::vector<std::string> elaborations;
    for (CompilationUnit* part : loader.units()) {
        elaborations.push_back(part->partKey);
    }

    std::vector<CompilationUnit*> units = loader.units();

    Sema sema(diagnostics);
    sema.prepareLimitedViews(units);
    for (CompilationUnit* unit : units) {
        sema.analyze(*unit);
    }
    sema.finishAnalysis(!singleUnit);
    if (diagnostics.hasErrors()) {
        return 1;
    }

    std::vector<UnitRecord> semanticInputs;
    for (const LibraryUnit& unit : libraryUnits) { semanticInputs.push_back(describe(unit, loader)); }
    for (const UnitRecord& unit : semanticInputs) {
        if (unit.interfaceDirectory.empty()) { continue; }
        UnitRecordFile compiled;
        std::string base = unit.interfaceDirectory + "/" + unit.key;
        std::string metadataDigest;
        if (!readUnitRecord(base + ".ali", compiled, &metadataDigest) || compiled.compilerDigest != compilerDigest
            || !unitRecordMatches(compiled, unit, semanticInputs)
            || !unitArtifactsMatch(unit.interfaceDirectory, unit, compiled)) {
            std::cerr << "adac: error: missing or stale semantic interface dependencies for unit '" << unit.key << "'\n";
            return 1;
        }
        loader.recordArtifactInput(base + ".ali", metadataDigest);
        loader.recordArtifactInput(base + ".ssa", compiled.irDigest);
        for (const auto& [key, digest] : compiled.interfaceDigests) {
            loader.recordArtifactInput(unit.interfaceDirectory + "/" + key + ".asi", digest);
        }
    }
    if (!loader.validateInputs()) { return 1; }

    if (scanOnly) {
        if (artifacts.empty() || !ensureDirectory(artifacts)) {
            std::cerr << "adac: error: '--scan' needs the directory named by '-D'\n";
            return 2;
        }
        std::vector<UnitRecord> records;
        for (const LibraryUnit& unit : libraryUnits) {
            records.push_back(describe(unit, loader));
        }
        if (!writeManifest(artifacts + "/" + manifestName, records, elaborations, loader.searchPaths(), compilerDigest, inputSnapshot())) {
            std::cerr << "adac: error: cannot write '" << artifacts << "/" << manifestName << "'\n";
            return 2;
        }
        return 0;
    }

    Symbol* main = nullptr;
    for (const LibraryUnit& unit : libraryUnits) {
        if (unit.key == loader.sourceKey(inputs.back())) {
            main = mainIn(unit);
        }
    }

    QbeEmitter emitter(sema, diagnostics);

    // Compiling one unit emits that unit and nothing else: whatever it draws on
    // was read to make sense of it, and is emitted when its own turn comes.
    if (singleUnit) {
        const LibraryUnit* target = nullptr;
        for (const LibraryUnit& unit : libraryUnits) {
            if (unit.key == loader.sourceKey(inputs.back())) {
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
        Artifacts::AtomicFile publication(path);
        auto& stream = publication.stream();
        if (!stream) {
            std::cerr << "adac: error: cannot write '" << path << "'\n";
            return 2;
        }
        emitter.emitUnit(*target, stream);

        UnitRecordFile record;
        if (!publication.finish() || diagnostics.hasErrors()) {
            std::cerr << "adac: error: could not compile unit '" << target->key << "'\n";
            return 1;
        }
        record.compilerDigest = compilerDigest;
        record.irDigest = digestOfFile(publication.temporary());
        record.digest = describe(*target, loader).digest;
        record.sourceIdentity = describe(*target, loader).sourceIdentity;
        for (const LibraryUnit& unit : libraryUnits) {
            if (unit.key != target->key) {
                bool readsBody = false;
                for (CompilationUnit* part : unit.parts) {
                    readsBody = readsBody || !part->isSpec;
                }
                const auto input = describe(unit, loader);
                record.dependencySources.emplace_back(unit.key, readsBody ? input.sourceIdentity : input.specIdentity);
                if (loader.importedRecord(unit.key) != nullptr) { record.replayDependencies.push_back(unit.key); }
                if (readsBody) {
                    record.bodyDependencies.emplace_back(unit.key, describe(unit, loader).digest);
                } else {
                    record.dependencies.emplace_back(unit.key, describe(unit, loader).specDigest);
                }
            }
        }
        if (Symbol* main = mainIn(*target)) {
            record.mainName = main->qbeName;
        }
        SemanticInterface semanticInterface;
        semanticInterface.compilerDigest = compilerDigest;
        semanticInterface.unit = describe(*target, loader);
        semanticInterface.unit.interfaceDirectory.clear();
        semanticInterface.sources = loader.interfaceSources(*target);
        std::vector<std::string> interfaceKeys = { target->key };
        for (const CompilationUnit* part : target->parts) {
            std::string key = interfaceAliasKey(part->partKey);
            if (std::find(interfaceKeys.begin(), interfaceKeys.end(), key) == interfaceKeys.end()) {
                interfaceKeys.push_back(key);
            }
        }
        std::error_code directoryError;
        auto aliasDirectory = Artifacts::nativePath(artifacts + "/" + interfaceAliasKey("placeholder")).parent_path();
        std::filesystem::create_directories(aliasDirectory, directoryError);
        if (directoryError) { std::cerr << "adac: error: cannot create interface index directory\n"; return 2; }
        std::vector<std::unique_ptr<Artifacts::AtomicPath>> interfaces;
        for (const std::string& key : interfaceKeys) {
            auto staged = std::make_unique<Artifacts::AtomicPath>(artifacts + "/" + key + ".asi");
            if (!staged->valid() || !writeSemanticInterface(staged->temporary(), semanticInterface)) {
                std::cerr << "adac: error: cannot stage semantic interface for '" << key << "'\n";
                return 2;
            }
            record.interfaceDigests.emplace_back(key, digestOfFile(staged->temporary()));
            interfaces.push_back(std::move(staged));
        }
        Artifacts::AtomicPath metadata(artifacts + "/" + target->key + ".ali");
        if (!metadata.valid() || !writeUnitRecord(metadata.temporary(), record)) {
            std::cerr << "adac: error: cannot stage compiled unit metadata\n";
            return 2;
        }
        if (!loader.validateInputs()) { return 1; }
        if (!publication.commit()) { std::cerr << "adac: error: cannot publish unit IR\n"; return 2; }
        for (const auto& stagedInterface : interfaces) {
            if (!stagedInterface->commit()) { std::cerr << "adac: error: cannot publish unit interface\n"; return 2; }
        }
        // Publish the complete group's checksummed commit record last.
        if (!metadata.commit()) { std::cerr << "adac: error: cannot publish compiled unit metadata\n"; return 2; }
        return diagnostics.hasErrors() ? 1 : 0;
    }

    if (!artifacts.empty()) {
        if (!ensureDirectory(artifacts)) {
            std::cerr << "adac: error: cannot use '" << artifacts << "' as an output directory\n";
            return 2;
        }

        std::vector<UnitRecord> records;
        std::vector<std::unique_ptr<Artifacts::AtomicFile>> outputs;
        for (const LibraryUnit& unit : libraryUnits) {
            auto staged = std::make_unique<Artifacts::AtomicFile>(artifacts + "/" + unit.key + ".ssa");
            emitter.emitUnit(unit, staged->stream());
            if (!staged->finish()) { std::cerr << "adac: error: cannot stage unit IR\n"; return 2; }
            outputs.push_back(std::move(staged));
            records.push_back(describe(unit, loader));
        }
        auto binder = std::make_unique<Artifacts::AtomicFile>(artifacts + "/" + binderName + ".ssa");
        emitBinder(elaborations, main == nullptr ? std::string() : main->qbeName, binder->stream());
        if (!binder->finish()) { std::cerr << "adac: error: cannot stage binder\n"; return 2; }
        outputs.push_back(std::move(binder));
        Artifacts::AtomicPath manifest(artifacts + "/" + manifestName);
        if (!manifest.valid() || !writeManifest(manifest.temporary(), records, elaborations, loader.searchPaths(), compilerDigest, inputSnapshot())) {
            std::cerr << "adac: error: cannot stage manifest\n";
            return 2;
        }
        if (!loader.validateInputs()) { return 1; }
        for (const auto& file : outputs) {
            if (!file->commit()) { std::cerr << "adac: error: cannot publish IR\n"; return 2; }
        }
        if (!manifest.commit()) { std::cerr << "adac: error: cannot publish manifest\n"; return 2; }
    } else {
        std::ostream* stream = &std::cout;
        std::unique_ptr<Artifacts::AtomicFile> file;
        if (output != "-") {
            file = std::make_unique<Artifacts::AtomicFile>(output);
            if (!file->stream()) { std::cerr << "adac: error: cannot stage '" << output << "'\n"; return 2; }
            stream = &file->stream();
        }
        for (const LibraryUnit& unit : libraryUnits) { emitter.emitUnit(unit, *stream); }
        emitBinder(elaborations, main == nullptr ? std::string() : main->qbeName, *stream);
        if (!loader.validateInputs()) { return 1; }
        std::string outputDigest;
        if (file) {
            if (!file->finish()) { std::cerr << "adac: error: cannot finish output\n"; return 2; }
            outputDigest = digestOfFile(file->temporary());
            if (!file->commit()) { std::cerr << "adac: error: cannot publish '" << output << "'\n"; return 2; }
        }
        if (!buildInputsOutput.empty()) {
            auto inputs = inputSnapshot();
            if (file) { inputs.emplace_back(output, Artifacts::fileFingerprint(output, &outputDigest)); }
            if (!writeManifest(buildInputsOutput, {}, {}, loader.searchPaths(), compilerDigest, inputs)) {
                std::cerr << "adac: error: cannot publish input snapshot\n";
                return 2;
            }
        }
    }

    return diagnostics.hasErrors() ? 1 : 0;
}
