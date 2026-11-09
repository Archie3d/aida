#pragma once

#include "Token.h"
#include "UnitManifest.h"

#include <string>
#include <vector>

// Validated declaration/template replay, not a dump of process-local pointers.
// Each source is a token stream, including configuration pragmas and subunits.
struct InterfaceSource
{
    std::string name;
    std::vector<Token> tokens;
};

struct SemanticInterface
{
    std::string artifactDigest; // Digest of the exact bytes read; not serialized.
    std::string compilerDigest;
    UnitRecord unit;
    std::vector<InterfaceSource> sources;
};

bool writeSemanticInterface(const std::string& path, const SemanticInterface& semanticInterface);
bool readSemanticInterface(const std::string& path, const std::string& compilerDigest,
    SemanticInterface& semanticInterface, std::string& error);
