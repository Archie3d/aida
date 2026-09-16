#pragma once

#include <string>
#include <vector>

// What a scan found out about one library unit, and what a compiled unit
// records about the sources it was built from.  Between them the driver can
// tell which units a change has reached without analysing anything itself.

struct UnitRecord
{
    std::string key;
    std::string digest;       // Over every source of the unit.
    std::string specDigest;   // Over the specification, or the whole unit if no spec exists.
    std::vector<std::string> sources;
    std::vector<std::string> withKeys;
};

// Unit metadata plus the independent specification/body elaboration sequence.
bool writeManifest(const std::string& path, const std::vector<UnitRecord>& units, const std::vector<std::string>& elaborations);
bool readManifest(const std::string& path, std::vector<UnitRecord>& units, std::vector<std::string>* elaborations = nullptr);

struct UnitRecordFile
{
    std::string digest;
    std::string compilerDigest;
    std::string irDigest;

    // Full-unit digests for dependencies whose bodies were read (e.g. generics).
    std::vector<std::pair<std::string, std::string>> bodyDependencies;

    // Specification digests for dependencies whose bodies were not read,
    // the whole closure and not just what it names, so that a change reaching
    // it through another unit is noticed too.
    std::vector<std::pair<std::string, std::string>> dependencies;

    // The mangled name of the main subprogram, when this unit has one.
    std::string mainName;
};

bool writeUnitRecord(const std::string& path, const UnitRecordFile& record);
bool readUnitRecord(const std::string& path, UnitRecordFile& record);

// Validate all recorded semantic inputs against a fresh scan.
bool unitRecordMatches(const UnitRecordFile& record, const UnitRecord& unit, const std::vector<UnitRecord>& scanned);
