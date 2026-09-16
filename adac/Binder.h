#pragma once

#include <ostream>
#include <string>
#include <vector>

// A unit key spells a file name, where QBE wants an identifier.  Internal
// symbols carry the tag so that no two units name the same one.
std::string unitTag(const std::string& key);

// The symbol elaborating a library unit.  The dot keeps it clear of both
// mangled Ada names and run time symbols.
std::string elaborationName(const std::string& key);

// Writes the entry point: every unit is elaborated in turn, and the main
// subprogram runs once they all succeeded.  This belongs to no unit, and needs
// nothing but the names, so it is written straight out rather than going
// through the emitter.
void emitBinder(const std::vector<std::string>& units, const std::string& mainName, std::ostream& out);
