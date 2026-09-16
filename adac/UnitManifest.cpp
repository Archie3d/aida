#include "UnitManifest.h"

#include <fstream>
#include <sstream>

namespace
{

// Every line is a keyword and the rest of the line, which keeps paths with
// spaces in them intact.
bool split(const std::string& line, std::string& keyword, std::string& rest)
{
    std::size_t space = line.find(' ');
    keyword = line.substr(0, space);
    rest = space == std::string::npos ? std::string() : line.substr(space + 1);
    return !keyword.empty();
}

}

bool writeManifest(const std::string& path, const std::vector<UnitRecord>& units)
{
    std::ofstream out(path);
    if (!out) {
        return false;
    }
    for (const UnitRecord& unit : units) {
        out << "unit " << unit.key << "\n";
        out << "digest " << unit.digest << "\n";
        if (!unit.specDigest.empty()) {
            out << "spec " << unit.specDigest << "\n";
        }
        for (const std::string& source : unit.sources) {
            out << "source " << source << "\n";
        }
        for (const std::string& key : unit.withKeys) {
            out << "with " << key << "\n";
        }
        out << "end\n";
    }
    return true;
}

bool readManifest(const std::string& path, std::vector<UnitRecord>& units)
{
    std::ifstream in(path);
    if (!in) {
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        std::string keyword;
        std::string rest;
        if (!split(line, keyword, rest)) {
            continue;
        }
        if (keyword == "unit") {
            units.push_back(UnitRecord {});
            units.back().key = rest;
        } else if (units.empty()) {
            continue;
        } else if (keyword == "digest") {
            units.back().digest = rest;
        } else if (keyword == "spec") {
            units.back().specDigest = rest;
        } else if (keyword == "source") {
            units.back().sources.push_back(rest);
        } else if (keyword == "with") {
            units.back().withKeys.push_back(rest);
        }
    }
    return true;
}

bool writeUnitRecord(const std::string& path, const UnitRecordFile& record)
{
    std::ofstream out(path);
    if (!out) {
        return false;
    }
    out << "digest " << record.digest << "\n";
    for (const auto& dependency : record.dependencies) {
        out << "dep " << dependency.first << " " << dependency.second << "\n";
    }
    if (!record.mainName.empty()) {
        out << "main " << record.mainName << "\n";
    }
    return true;
}

bool readUnitRecord(const std::string& path, UnitRecordFile& record)
{
    std::ifstream in(path);
    if (!in) {
        return false;
    }
    std::string line;
    while (std::getline(in, line)) {
        std::string keyword;
        std::string rest;
        if (!split(line, keyword, rest)) {
            continue;
        }
        if (keyword == "digest") {
            record.digest = rest;
        } else if (keyword == "main") {
            record.mainName = rest;
        } else if (keyword == "dep") {
            std::size_t space = rest.find(' ');
            if (space != std::string::npos) {
                record.dependencies.emplace_back(rest.substr(0, space), rest.substr(space + 1));
            }
        }
    }
    return true;
}
