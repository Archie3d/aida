#include "Cache.h"
#include "Toolchain.h"
#include "UnitManifest.h"

#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace
{

enum class Stage
{
    IntermediateLanguage,
    Assembly,
    Executable
};

void printUsage()
{
    std::cerr << "AIDA Ada compiler\n"
              << "usage: ada [options] <source> [<source>...]\n"
              << "\n"
              << "  -o <file>     name of the produced file\n"
              << "  -D <dir>      where the per unit objects are kept\n"
              << "  --clean       remove that directory and stop\n"
              << "  --emit-ir     stop after generating QBE intermediate language\n"
              << "  -S            stop after generating assembly\n"
              << "  -k            keep the intermediate files\n"
              << "  --no-stdlib   leave the predefined environment out of the unit search path\n"
              << "  -v            print each command before running it\n";
}

std::string baseName(const std::string& path)
{
    std::size_t slash = path.find_last_of('/');
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
        && !record.irDigest.empty()
        && record.irDigest == digestOfFile(directory + "/" + unit.key + ".ssa");
}

}

int main(int argc, char** argv)
{
    std::vector<std::string> sources;
    std::string output;
    std::string objectDirectory;
    Stage stage = Stage::Executable;
    bool keepIntermediates = false;
    bool verbose = false;
    bool useLibrary = true;
    bool clean = false;

    for (int i = 1; i < argc; ++i) {
        std::string argument = argv[i];
        if (argument == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "ada: error: missing file name after '-o'\n";
                return 2;
            }
            output = argv[++i];
        } else if (argument == "-D") {
            if (i + 1 >= argc) {
                std::cerr << "ada: error: missing directory after '-D'\n";
                return 2;
            }
            objectDirectory = argv[++i];
        } else if (argument == "--clean") {
            clean = true;
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
            sources.push_back(argument);
        }
    }

    if (sources.empty()) {
        printUsage();
        return 2;
    }

    Toolchain toolchain;
    toolchain.locateFrom(argv[0]);
    toolchain.setVerbose(verbose);

    // Without -o the name of the last source gives the stem of every product.
    std::string stem = output.empty() ? baseName(sources.back()) : output;
    std::string irFile = stem + ".ssa";
    std::string assemblyFile = stem + ".s";
    if (objectDirectory.empty()) {
        objectDirectory = stem + ".adaobj";
    }
    if (clean) {
        std::error_code ignored;
        std::filesystem::remove_all(objectDirectory, ignored);
        return 0;
    }
    if (!output.empty()) {
        if (stage == Stage::IntermediateLanguage) {
            irFile = output;
        } else if (stage == Stage::Assembly) {
            assemblyFile = output;
        }
    }

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
        if (stage == Stage::IntermediateLanguage) {
            return 0;
        }
        std::vector<std::string> qbeCommand = { toolchain.qbe() };
#ifdef _WIN32
        // qbe's own default target is the ELF sysv ABI even when built on Windows.
        qbeCommand.push_back("-t");
        qbeCommand.push_back("amd64_win");
#endif
        qbeCommand.push_back("-o");
        qbeCommand.push_back(assemblyFile);
        qbeCommand.push_back(irFile);
        int result = toolchain.run(qbeCommand);
        if (!keepIntermediates) {
            std::remove(irFile.c_str());
        }
        return result == 0 ? 0 : 1;
    }

    std::vector<UnitRecord> scanned;
    if (!readManifest(objectDirectory + "/units.manifest", scanned) || scanned.empty()) {
        std::cerr << "ada: error: nothing was scanned into '" << objectDirectory << "'\n";
        return 1;
    }

    // The unit named last on the command line is the one whose main subprogram
    // the program runs.
    std::string mainUnit = baseName(sources.back());
    for (char& c : mainUnit) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    // Track specifications and any bodies read for instantiation separately:
    // changes to an ordinary dependency body do not invalidate its clients.
    const std::string compilerDigest = programDigest(toolchain.adac());
    for (const UnitRecord& unit : scanned) {
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
        for (const std::string& source : sources) {
            compile.push_back("-I");
            auto directory = std::filesystem::path(source).parent_path();
            compile.push_back(directory.empty() ? "." : directory.string());
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

    Cache cache(objectDirectory, toolDigest({ toolchain.qbe(), toolchain.compiler() }));

    std::vector<std::string> link = { toolchain.compiler(), "-o", stem };
    for (const std::string& unit : units) {
        std::string base = objectDirectory + "/" + unit;
        if (cache.isCurrent(unit)) {
            if (verbose) {
                std::cerr << "ada: " << unit << " is up to date\n";
            }
            link.push_back(base + ".o");
            continue;
        }

        std::vector<std::string> qbeCommand = { toolchain.qbe() };
#ifdef _WIN32
        // qbe's own default target is the ELF sysv ABI even when built on Windows.
        qbeCommand.push_back("-t");
        qbeCommand.push_back("amd64_win");
#endif
        qbeCommand.push_back("-o");
        qbeCommand.push_back(base + ".s");
        qbeCommand.push_back(base + ".ssa");
        if (toolchain.run(qbeCommand) != 0) {
            return 1;
        }
        if (toolchain.run({ toolchain.compiler(), "-c", "-o", base + ".o", base + ".s" }) != 0) {
            return 1;
        }
        cache.record(unit);
        link.push_back(base + ".o");
    }

    for (const std::string& file : toolchain.runtime()) {
        link.push_back(file);
    }
    return toolchain.run(link) == 0 ? 0 : 1;
}
