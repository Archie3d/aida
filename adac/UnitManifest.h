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
    std::string specDigest;   // Over the specification alone, empty when there is none.
    std::vector<std::string> sources;
    std::vector<std::string> withKeys;
};

// One line per unit, in elaboration order.
bool writeManifest(const std::string& path, const std::vector<UnitRecord>& units);
bool readManifest(const std::string& path, std::vector<UnitRecord>& units);

struct UnitRecordFile
{
    std::string digest;

    // The specification digest of every unit this one was compiled against,
    // the whole closure and not just what it names, so that a change reaching
    // it through another unit is noticed too.
    std::vector<std::pair<std::string, std::string>> dependencies;

    // The mangled name of the main subprogram, when this unit has one.
    std::string mainName;
};

bool writeUnitRecord(const std::string& path, const UnitRecordFile& record);
bool readUnitRecord(const std::string& path, UnitRecordFile& record);
