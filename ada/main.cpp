#include "Cache.h"
#include "../support/DiagnosticOptions.h"
#include "BuildState.h"
#include "../support/Artifacts.h"
#include "Toolchain.h"
#include "UnitManifest.h"
#include "WindowsStack.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cwchar>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#ifdef _WIN32
#   ifndef NOMINMAX
#       define NOMINMAX
#   endif
#   include <windows.h>
#endif

namespace {

enum class Stage
{
    IntermediateLanguage,
    Assembly,
    Executable
};

void printUsage()
{
    std::cerr << "AIDA Ada compiler\n"
              << "usage: ada [options] <source> [<source>|<object.o>|<library.a>...]\n"
              << "       ada --clean (-D <directory> | -o <output> | <source>)\n"
              << "\n"
              << "  .o/.a inputs are passed in order to the executable linker\n"
              << "  the last Ada source selects the main program and default output name\n"
              << "\n"
              << "  -o <file>     name of the produced file\n"
              << "  --source <file> treat an input as Ada source regardless of extension\n"
              << "  -D <dir>      where the per unit objects are kept\n"
              << "  --clean       remove -D, or <output>.adaobj from -o/source, and stop\n"
              << "  --force       bypass the whole-build no-op shortcut\n"
              << "  --emit-ir     stop after generating QBE intermediate language\n"
              << "  -S            stop after generating assembly\n"
              << "  -k            keep the intermediate files\n"
              << "  --diagnostic-format=auto|plain|rich  source snippet rendering\n"
              << "  --color=auto|always|never             diagnostic color\n"
              << "  --no-stdlib   leave the predefined environment out of the unit search path\n"
              << "  -v            print each command before running it\n";
}

std::string baseName(const std::string& path)
{
    std::size_t slash = path.find_last_of("/\\");
    std::string name = slash == std::string::npos ? path : path.substr(slash + 1);
    std::size_t dot = name.find_last_of('.');
    return dot == std::string::npos ? name : name.substr(0, dot);
}

// Reuse a unit only when its compiler, semantic inputs and emitted IR match.
bool isCurrent(const std::string& directory, const UnitRecord& unit, const std::vector<UnitRecord>& scanned, const std::string& compilerDigest)
{
    UnitRecordFile record;
    if (!readUnitRecord(directory + "/" + unit.key + ".ali", record)) {
        return false;
    }
    return !compilerDigest.empty() && record.compilerDigest == compilerDigest
        && unitRecordMatches(record, unit, scanned)
        && unitArtifactsMatch(directory, unit, record);
}

bool isCleanupLink([[maybe_unused]] const std::filesystem::path& path, std::filesystem::file_status status)
{
#ifdef _WIN32
    // MinGW's filesystem traversal can follow directory reparse points.
    DWORD attributes = GetFileAttributesW(path.c_str());
    return std::filesystem::is_symlink(status)
        || (attributes != INVALID_FILE_ATTRIBUTES && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0);
#else
    return std::filesystem::is_symlink(status);
#endif
}

void removeArtifactTree(const std::filesystem::path& path, std::error_code& error)
{
#ifdef _WIN32
    DWORD attributes = GetFileAttributesW(path.c_str());
    if (attributes == INVALID_FILE_ATTRIBUTES) {
        DWORD failure = GetLastError();
        if (failure != ERROR_FILE_NOT_FOUND && failure != ERROR_PATH_NOT_FOUND) {
            error = std::error_code(failure, std::system_category());
        }
        return;
    }
    bool directory = (attributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
    if (directory && (attributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
        std::filesystem::directory_iterator entry(path, error), end;
        while (!error && entry != end) {
            removeArtifactTree(entry->path(), error);
            if (error) { return; }
            entry.increment(error);
        }
        if (error) { return; }
    }
    // Remove the link itself, including directory junctions. Never enumerate a
    // reparse point, even when it is nested inside the selected cache tree.
    BOOL removed = directory ? RemoveDirectoryW(path.c_str()) : DeleteFileW(path.c_str());
    if (!removed) { error = std::error_code(GetLastError(), std::system_category()); }
#else
    std::filesystem::remove_all(path, error);
#endif
}

int cleanArtifacts(const std::string& directory)
{
    auto path = Artifacts::nativePath(directory);
    std::error_code error;
    auto status = std::filesystem::symlink_status(path, error);
    if (error && error != std::errc::no_such_file_or_directory && error != std::errc::not_a_directory) {
        std::cerr << "ada: error: cannot inspect cleanup directory: " << error.message() << "\n";
        return 1;
    }
    bool symlink = isCleanupLink(path, status);
    if (!symlink && std::filesystem::exists(status) && !std::filesystem::is_directory(status)) {
        std::cerr << "ada: error: cleanup target is not a directory\n";
        return 1;
    }
    error.clear();
    auto absolute = std::filesystem::absolute(path, error);
    if (error) { std::cerr << "ada: error: cannot resolve cleanup directory\n"; return 1; }
    auto target = std::filesystem::weakly_canonical(absolute, error);
    if (error) { std::cerr << "ada: error: cannot resolve cleanup directory\n"; return 1; }
    if (!symlink) {
        auto current = std::filesystem::current_path(error);
        if (error) { std::cerr << "ada: error: cannot resolve current directory\n"; return 1; }
        for (;;) {
            error.clear();
            if (current == target || (std::filesystem::exists(status) && std::filesystem::equivalent(current, target, error))) {
                std::cerr << "ada: error: refusing to clean the current directory or an ancestor\n";
                return 1;
            }
            if (current == current.parent_path()) { break; }
            current = current.parent_path();
        }
    }
    error.clear();
    auto parent = absolute.parent_path();
    if (!std::filesystem::exists(parent, error) && !error) { return 0; }
    if (error) { std::cerr << "ada: error: cannot inspect cleanup parent: " << error.message() << "\n"; return 1; }
    Artifacts::DirectoryLock lock;
    if (!lock.acquire(directory)) {
        std::cerr << "ada: error: cannot lock cleanup directory: " << lock.error() << "\n";
        return 1;
    }
    status = std::filesystem::symlink_status(path, error);
    symlink = isCleanupLink(path, status);
    if (!symlink && (error == std::errc::no_such_file_or_directory || error == std::errc::not_a_directory)) { return 0; }
    if (error && !symlink) { std::cerr << "ada: error: cannot inspect cleanup target: " << error.message() << "\n"; return 1; }
    error.clear();
    if (!symlink && std::filesystem::exists(status) && !std::filesystem::is_directory(status)) {
        std::cerr << "ada: error: cleanup target became a nondirectory\n";
        return 1;
    }
    removeArtifactTree(path, error);
    if (error) { std::cerr << "ada: error: cannot clean '" << directory << "': " << error.message() << "\n"; return 1; }
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    std::vector<std::string> sources;
    std::vector<std::string> linkerInputs;
    std::string output;
    std::string objectDirectory;
    Stage stage = Stage::Executable;
    bool keepIntermediates = false;
    bool verbose = false;
    bool useLibrary = true;
    bool clean = false;
    bool force = false;
    DiagnosticOptions diagnosticOptions;
    std::vector<std::string> diagnosticArguments;

    for (int i = 1; i < argc; ++i) {
        std::string argument = argv[i];
        if (DiagnosticOptions::recognizes(argument)) {
            if (!diagnosticOptions.parse(argument)) {
                std::cerr << "ada: error: invalid diagnostic option '" << argument << "'\n";
                return 2;
            }
            diagnosticArguments.push_back(argument);
        } else if (argument == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "ada: error: missing file name after '-o'\n";
                return 2;
            }
            output = argv[++i];
        } else if (argument == "--source") {
            if (i + 1 >= argc) {
                std::cerr << "ada: error: missing file name after '--source'\n";
                return 2;
            }
            sources.push_back(argv[++i]);
        } else if (argument == "-D") {
            if (i + 1 >= argc) {
                std::cerr << "ada: error: missing directory after '-D'\n";
                return 2;
            }
            objectDirectory = argv[++i];
        } else if (argument == "--clean") {
            clean = true;
        } else if (argument == "--force") {
            force = true;
        } else if (argument == "--emit-ir") {
            stage = Stage::IntermediateLanguage;
        } else if (argument == "-S") {
            stage = Stage::Assembly;
        } else if (argument == "-k" || argument == "--keep") {
            keepIntermediates = true;
        } else if (argument == "--no-stdlib") {
            useLibrary = false;
        } else if (argument == "-v") {
            verbose = true;
        } else if (argument == "-h" || argument == "--help") {
            printUsage();
            return 0;
        } else if (!argument.empty() && argument[0] == '-') {
            std::cerr << "ada: error: unknown option '" << argument << "'\n";
            return 2;
        } else {
            const std::string extension = std::filesystem::path(argument).extension().string();
            if (extension == ".o" || extension == ".a") {
                linkerInputs.push_back(argument);
            } else {
                sources.push_back(argument);
            }
        }
    }

    if (clean) {
        if (objectDirectory.empty()) {
            if (output.empty() && sources.empty()) {
                std::cerr << "ada: error: --clean needs -D <directory>, -o <output>, or a source\n";
                return 2;
            }
            objectDirectory = (output.empty() ? baseName(sources.back()) : output) + ".adaobj";
        }
        return cleanArtifacts(objectDirectory);
    }
    if (sources.empty()) {
        std::cerr << "ada: error: at least one Ada source is required\n";
        printUsage();
        return 2;
    }
    if (stage != Stage::Executable && !linkerInputs.empty()) {
        std::cerr << "ada: error: .o and .a inputs require executable output (omit -S and --emit-ir)\n";
        return 2;
    }

    Toolchain toolchain;
    toolchain.locateFrom(argv[0]);
    toolchain.setDiagnosticArguments(diagnosticArguments);
    toolchain.setVerbose(verbose);

    // Without -o the name of the last source gives the stem of every product.
    std::string stem = output.empty() ? baseName(sources.back()) : output;
    std::string irFile = stem + ".ssa";
    std::string assemblyFile = stem + ".s";
    if (objectDirectory.empty()) {
        objectDirectory = stem + ".adaobj";
    }
    if (!output.empty()) {
        if (stage == Stage::IntermediateLanguage) { irFile = output; }
        else if (stage == Stage::Assembly) { assemblyFile = output; }
    }
    Artifacts::DirectoryLock buildLock;
    Artifacts::DirectoryLock receiptLock;
    std::string lockDirectory = objectDirectory;
    if (stage != Stage::Executable) {
        auto path = Artifacts::nativePath(stage == Stage::Assembly ? assemblyFile : irFile);
        lockDirectory = Artifacts::pathText(path.parent_path());
        if (lockDirectory.empty()) { lockDirectory = "."; }
    }
    if (!(stage == Stage::IntermediateLanguage && irFile == "-")) {
        // File modes publish under the output lock and also own a receipt in
        // -D. Acquire distinct guards in canonical order to avoid cross-build
        // deadlocks when two invocations exchange these directories.
        std::string error;
        std::string outputGuard = Artifacts::DirectoryLock::pathFor(lockDirectory, error);
        std::string receiptGuard = Artifacts::DirectoryLock::pathFor(objectDirectory, error);
        if (outputGuard.empty() || receiptGuard.empty()) {
            std::cerr << "ada: error: cannot resolve artifact locks: " << error << "\n";
            return 1;
        }
#ifdef _WIN32
        int order = _wcsicmp(Artifacts::nativePath(outputGuard).c_str(), Artifacts::nativePath(receiptGuard).c_str());
#else
        int order = outputGuard.compare(receiptGuard);
#endif
        auto acquireOutput = [&]() { return buildLock.acquire(lockDirectory); };
        auto acquireReceipt = [&]() { return receiptLock.acquire(objectDirectory); };
        bool locked = order < 0 ? acquireOutput() && acquireReceipt()
            : order > 0 ? acquireReceipt() && acquireOutput() : acquireOutput();
        if (!locked) {
            std::cerr << "ada: error: cannot lock artifact directory: " << buildLock.error() << receiptLock.error() << "\n";
            return 1;
        }
        toolchain.setArtifactLock(buildLock.inheritedArgument());
    }
    std::string finalOutput = stage == Stage::Executable ? stem
        : stage == Stage::Assembly ? assemblyFile : irFile;
#ifdef _WIN32
    if (stage == Stage::Executable && Artifacts::nativePath(finalOutput).extension().empty()) { finalOutput += ".exe"; }
#endif
    bool cacheableOutput = !(stage == Stage::IntermediateLanguage && irFile == "-");
    std::vector<std::string> tools { resolveProgram(argv[0]), resolveProgram(toolchain.adac()) };
    if (stage != Stage::IntermediateLanguage) { tools.push_back(resolveProgram(toolchain.qbe())); }
    if (stage == Stage::Executable) { tools.push_back(resolveProgram(toolchain.compiler())); }
    std::string configuration = "AIDA driver state 1";
    auto configure = [&](const std::string& value) { configuration.push_back('\0'); configuration += value; };
    configure(Artifacts::targetIdentity());
    configure(Artifacts::pathText(std::filesystem::current_path()));
    configure(environmentDigest());
    configure(std::to_string(static_cast<int>(stage)));
    configure(useLibrary ? "stdlib" : "no-stdlib");
    configure(keepIntermediates ? "keep" : "discard");
    configure(toolchain.library());
    configure(objectDirectory);
    configure(finalOutput);
    for (const auto& tool : tools) { configure(tool); }
    configure("sources");
    for (const auto& source : sources) { configure(source); }
    configure("linker inputs");
    for (const auto& input : linkerInputs) { configure(input); }
    for (const auto& input : toolchain.runtime()) { configure(input); }
    BuildState state(objectDirectory + "/build.receipt", digestOf(configuration));
    if (cacheableOutput && !force && state.isCurrent()) {
        if (verbose) { std::cerr << "ada: " << finalOutput << " is up to date (no-op build)\n"; }
        return 0;
    }
    Artifacts::FileInputs driverInputs;
    for (const auto& tool : tools) { driverInputs.emplace_back(tool, Artifacts::fileFingerprint(tool)); }
    if (stage == Stage::Executable) {
        for (const auto& input : linkerInputs) { driverInputs.emplace_back(input, Artifacts::fileFingerprint(input)); }
        for (const auto& input : toolchain.runtime()) {
            if (!input.empty() && input.front() != '-') { driverInputs.emplace_back(input, Artifacts::fileFingerprint(input)); }
            else if (input != "-pthread" && input != "-lm" && !input.starts_with("-fsanitize=")) {
                driverInputs.emplace_back("opaque linker option", "-");
            }
        }
    }
    if (cacheableOutput) {
        std::error_code error;
        std::filesystem::create_directories(Artifacts::nativePath(objectDirectory), error);
        if (error) { std::cerr << "ada: error: cannot create artifact directory: " << error.message() << "\n"; return 1; }
    }
    auto completeState = [&](Artifacts::FileInputs inputs, const std::vector<std::string>& products) {
        inputs.insert(inputs.end(), driverInputs.begin(), driverInputs.end());
        if (!state.save(inputs, products) && verbose) {
            std::cerr << "ada: completed build without a reusable no-op receipt\n";
        }
    };

    // The driver knows where the predefined environment ended up, so it tells
    // the front end rather than leaving it to guess.  Dropping it affects the
    // unit search path alone: the run time is still linked, since a program
    // raising Constraint_Error calls into it whether or not it names a
    // predefined unit.
    std::vector<std::string> command = { toolchain.adac() };

    // A program is built one library unit at a time, so that a unit left alone
    // keeps the object it already had.  Asking for the intermediate language or
    // the assembly instead asks for the whole program in one file, which is
    // what there is to look at.
    bool perUnit = stage == Stage::Executable;
    std::unique_ptr<Artifacts::AtomicPath> snapshot;
    if (!perUnit && cacheableOutput) {
        snapshot = std::make_unique<Artifacts::AtomicPath>(objectDirectory + "/inputs.manifest");
        if (!snapshot->valid()) { std::cerr << "ada: error: cannot stage input snapshot\n"; return 1; }
        command.insert(command.end(), { "--build-inputs", snapshot->temporary() });
    }
    if (perUnit) {
        command.push_back("--scan");
        command.push_back("-D");
        command.push_back(objectDirectory);
    } else {
        command.push_back("-o");
        command.push_back(irFile);
    }
    std::vector<std::string> libraryOption;
    if (useLibrary) {
        libraryOption.push_back("--stdlib");
        libraryOption.push_back(toolchain.library());
    } else {
        libraryOption.push_back("--no-stdlib");
    }
    for (const std::string& option : libraryOption) {
        command.push_back(option);
    }
    for (const std::string& source : sources) {
        command.push_back(source);
    }
    if (toolchain.run(command) != 0) {
        return 1;
    }

    if (!perUnit) {
        Artifacts::FileInputs inputs;
        if (snapshot) {
            std::vector<UnitRecord> ignored;
            if (!readManifest(snapshot->temporary(), ignored, nullptr, nullptr, nullptr, &inputs)) {
                std::cerr << "ada: error: cannot read frontend input snapshot\n";
                return 1;
            }
        }
        if (stage == Stage::IntermediateLanguage) {
            if (cacheableOutput) { completeState(inputs, {}); }
            return 0;
        }
        Artifacts::AtomicPath assembly(assemblyFile);
        if (!assembly.valid()) { std::cerr << "ada: error: cannot stage assembly\n"; return 1; }
        std::vector<std::string> qbeCommand = { toolchain.qbe() };
#ifdef _WIN32
        qbeCommand.insert(qbeCommand.end(), { "-t", "amd64_win" });
#endif
        qbeCommand.insert(qbeCommand.end(), { "-o", assembly.temporary(), irFile });
        int result = toolchain.run(qbeCommand);
#ifdef _WIN32
        if (result == 0 && !prepareWindowsAssembly(assembly.temporary())) { result = 1; }
#endif
        if (result != 0) { return 1; }
        if (!Artifacts::inputsMatch(inputs, true) || !Artifacts::inputsMatch(driverInputs, true)) {
            std::cerr << "ada: error: inputs changed during build; retry\n";
            return 1;
        }
        std::string assemblyDigest = digestOfFile(assembly.temporary());
        if (!assembly.commit()) { std::cerr << "ada: error: cannot publish assembly\n"; return 1; }
        if (!keepIntermediates) {
            std::remove(irFile.c_str());
            // The removed intermediate is a product, not a required input.
            inputs.erase(std::remove_if(inputs.begin(), inputs.end(), [&](const auto& input) { return input.first == irFile; }), inputs.end());
        }
        inputs.emplace_back(assemblyFile, Artifacts::fileFingerprint(assemblyFile, &assemblyDigest));
        completeState(inputs, {});
        return 0;
    }

    std::vector<UnitRecord> scanned;
    std::vector<std::string> searchPaths;
    std::string compilerDigest;
    Artifacts::FileInputs buildInputs;
    if (!readManifest(objectDirectory + "/units.manifest", scanned, nullptr, &searchPaths, &compilerDigest, &buildInputs)
        || scanned.empty() || compilerDigest.empty()) {
        std::cerr << "ada: error: nothing was scanned into '" << objectDirectory << "'\n";
        return 1;
    }

    // The unit named last on the command line is the one whose main subprogram
    // the program runs.
    std::string mainUnit = baseName(sources.back());
    for (char& c : mainUnit) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    // Repeated paths and symlinks retain the first registered source group's
    // artifact name, even when an alias names the main source last.
    for (const UnitRecord& unit : scanned) {
        for (const std::string& source : unit.sources) {
            std::error_code error;
            if (std::filesystem::equivalent(source, sources.back(), error) && !error) {
                mainUnit = unit.key;
            }
        }
    }

    // Track specifications and any bodies read for instantiation separately:
    // changes to an ordinary dependency body do not invalidate its clients.
    for (const UnitRecord& unit : scanned) {
        if (!unit.interfaceDirectory.empty()) {
            UnitRecordFile imported;
            std::string metadataDigest;
            const auto& directory = unit.interfaceDirectory;
            if (!readUnitRecord(directory + "/" + unit.key + ".ali", imported, &metadataDigest)
                || imported.compilerDigest != compilerDigest || !unitRecordMatches(imported, unit, scanned)
                || !unitArtifactsMatch(directory, unit, imported)) {
                std::cerr << "ada: error: interface dependency changed during build; retry\n";
                return 1;
            }
            std::vector<std::pair<std::string, std::string>> files { { unit.key + ".ssa", imported.irDigest } };
            for (const auto& [key, digest] : imported.interfaceDigests) { files.emplace_back(key + ".asi", digest); }
            files.emplace_back(unit.key + ".ali", metadataDigest);
            std::vector<std::unique_ptr<Artifacts::AtomicPath>> stagedFiles;
            for (const auto& [name, digest] : files) {
                std::error_code error;
                std::filesystem::create_directories(Artifacts::nativePath(objectDirectory + "/" + name).parent_path(), error);
                auto staged = std::make_unique<Artifacts::AtomicPath>(objectDirectory + "/" + name);
                if (staged->valid()) {
                    std::filesystem::copy_file(Artifacts::nativePath(directory + "/" + name),
                        Artifacts::nativePath(staged->temporary()), error);
                }
                if (!staged->valid() || error || digestOfFile(staged->temporary()) != digest) {
                    std::cerr << "ada: error: cannot snapshot interface artifact '" << name << "'; retry\n";
                    return 1;
                }
                stagedFiles.push_back(std::move(staged));
            }
            for (const auto& file : stagedFiles) {
                if (!file->commit()) { std::cerr << "ada: error: cannot publish imported artifacts\n"; return 1; }
            }
            if (verbose) { std::cerr << "ada: " << unit.key << " uses a compiled semantic interface\n"; }
            continue;
        }
        if (isCurrent(objectDirectory, unit, scanned, compilerDigest)) {
            if (verbose) {
                std::cerr << "ada: " << unit.key << " needs no compiling\n";
            }
            continue;
        }
        std::vector<std::string> compile = { toolchain.adac(), "-c", "-D", objectDirectory };
        for (const std::string& option : libraryOption) {
            compile.push_back(option);
        }
        compile.push_back("--search-roots-only");
        for (const std::string& directory : searchPaths) {
            compile.push_back("-I");
            compile.push_back(directory);
        }
        // Register explicit bundles too: their declared units need not have
        // filenames that library-path lookup can discover.
        for (const std::string& source : sources) {
            compile.push_back(source);
        }
        compile.push_back(unit.sources.back());
        if (toolchain.run(compile) != 0) {
            return 1;
        }
    }

    std::vector<std::string> bind = { toolchain.adac(), "--bind", mainUnit, "-D", objectDirectory };
    if (toolchain.run(bind) != 0) {
        return 1;
    }

    if (stage == Stage::IntermediateLanguage) {
        return 0;
    }

    std::vector<std::string> units;
    for (const UnitRecord& unit : scanned) {
        units.push_back(unit.key);
    }
    units.push_back("__ada_binder");

    std::string objectTools = toolDigest({ toolchain.qbe(), toolchain.compiler() });
#ifdef _WIN32
    // Existing objects may contain unprobed frames, even when their IR and
    // external tools have not changed. Reassemble them under the new contract.
    objectTools += ":windows-stack-probes-v1";
#endif
    Cache cache(objectDirectory, objectTools);

    Artifacts::AtomicPath executableOutput(finalOutput);
    if (!executableOutput.valid()) { std::cerr << "ada: error: cannot stage executable\n"; return 1; }
    std::vector<std::string> link = { toolchain.compiler(), "-o", executableOutput.temporary() };
    for (const std::string& unit : units) {
        std::string base = objectDirectory + "/" + unit;
        if (cache.isCurrent(unit)) {
            if (verbose) {
                std::cerr << "ada: " << unit << " is up to date\n";
            }
            link.push_back(base + ".o");
            continue;
        }

        Artifacts::AtomicPath assembly(base + ".s");
        Artifacts::AtomicPath object(base + ".o");
        if (!assembly.valid() || !object.valid()) { std::cerr << "ada: error: cannot stage native artifacts\n"; return 1; }
        std::vector<std::string> qbeCommand = { toolchain.qbe() };
#ifdef _WIN32
        // qbe's own default target is the ELF sysv ABI even when built on Windows.
        qbeCommand.push_back("-t");
        qbeCommand.push_back("amd64_win");
#endif
        qbeCommand.push_back("-o");
        qbeCommand.push_back(assembly.temporary());
        qbeCommand.push_back(base + ".ssa");
        if (toolchain.run(qbeCommand) != 0) {
            return 1;
        }
#ifdef _WIN32
        if (!prepareWindowsAssembly(assembly.temporary())) {
            return 1;
        }
#endif
        if (toolchain.run({ toolchain.compiler(), "-c", "-o", object.temporary(), assembly.temporary() }) != 0) {
            return 1;
        }
        if (!assembly.commit() || !object.commit() || !cache.record(unit)) {
            std::cerr << "ada: error: cannot publish native artifacts\n";
            return 1;
        }
        link.push_back(base + ".o");
    }

    // Archives follow their users. Preserve external input order, including
    // repetitions, and let external code refer to the Ada runtime as well.
    for (const std::string& file : linkerInputs) {
        link.push_back(file);
    }
    for (const std::string& file : toolchain.runtime()) {
        link.push_back(file);
    }
    if (toolchain.run(link) != 0) { return 1; }
    if (!Artifacts::inputsMatch(buildInputs, true) || !Artifacts::inputsMatch(driverInputs, true)) {
        std::cerr << "ada: error: inputs changed during build; retry\n";
        return 1;
    }
    std::string executableDigest = digestOfFile(executableOutput.temporary());
    if (!executableOutput.commit()) { std::cerr << "ada: error: cannot publish executable\n"; return 1; }
    buildInputs.emplace_back(finalOutput, Artifacts::fileFingerprint(finalOutput, &executableDigest));
    std::vector<std::string> products { objectDirectory + "/units.manifest" };
    bool completeProducts = true;
    for (const auto& unit : scanned) {
        UnitRecordFile record;
        if (!readUnitRecord(objectDirectory + "/" + unit.key + ".ali", record)
            || record.compilerDigest != compilerDigest || !unitRecordMatches(record, unit, scanned)
            || !unitArtifactsMatch(objectDirectory, unit, record)) { completeProducts = false; break; }
        products.push_back(objectDirectory + "/" + unit.key + ".ali");
        for (const auto& [key, digest] : record.interfaceDigests) { products.push_back(objectDirectory + "/" + key + ".asi"); }
    }
    for (const auto& unit : units) {
        completeProducts = completeProducts && cache.isCurrent(unit);
        for (const char* extension : { ".ssa", ".o", ".build" }) { products.push_back(objectDirectory + "/" + unit + extension); }
    }
    if (completeProducts) { completeState(buildInputs, products); }
    else if (verbose) { std::cerr << "ada: artifacts changed before receipt publication; no shortcut recorded\n"; }
    return 0;
}
