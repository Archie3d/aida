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

bool writeManifest(const std::string& path, const std::vector<UnitRecord>& units, const std::vector<std::string>& elaborations)
{
    std::ofstream out(path);
    if (!out) {
        return false;
    }
    out << "format 2\n";
    for (const std::string& elaboration : elaborations) {
        out << "elaborate " << elaboration << "\n";
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

bool readManifest(const std::string& path, std::vector<UnitRecord>& units, std::vector<std::string>* elaborations)
{
    std::ifstream in(path);
    if (!in) {
        return false;
    }
    std::string line;
    if (!std::getline(in, line) || line != "format 2") {
        return false;
    }
    while (std::getline(in, line)) {
        std::string keyword;
        std::string rest;
        if (!split(line, keyword, rest)) {
            continue;
        }
        if (keyword == "elaborate") {
            if (elaborations != nullptr) {
                elaborations->push_back(rest);
            }
        } else if (keyword == "unit") {
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
    out << "format 2\n";
    out << "compiler " << record.compilerDigest << "\n";
    out << "ir " << record.irDigest << "\n";
    out << "digest " << record.digest << "\n";
    for (const auto& dependency : record.bodyDependencies) {
        out << "bodydep " << dependency.first << " " << dependency.second << "\n";
    }
    for (const auto& dependency : record.dependencies) {
        out << "dep " << dependency.first << " " << dependency.second << "\n";
    }
    if (!record.mainName.empty()) {
        out << "main " << record.mainName << "\n";
    }
    out << "end\n";
    return static_cast<bool>(out);
}

bool readUnitRecord(const std::string& path, UnitRecordFile& record)
{
    std::ifstream in(path);
    if (!in) {
        return false;
    }
    std::string line;
    if (!std::getline(in, line) || line != "format 2") {
        return false;
    }
    while (std::getline(in, line)) {
        std::string keyword;
        std::string rest;
        if (!split(line, keyword, rest)) {
            continue;
        }
        if (keyword == "end") {
            return !record.digest.empty() && !record.compilerDigest.empty() && !record.irDigest.empty();
        } else if (keyword == "compiler") {
            record.compilerDigest = rest;
        } else if (keyword == "ir") {
            record.irDigest = rest;
        } else if (keyword == "digest") {
            record.digest = rest;
        } else if (keyword == "main") {
            record.mainName = rest;
        } else if (keyword == "dep" || keyword == "bodydep") {
            std::size_t space = rest.find(' ');
            if (space != std::string::npos) {
                auto& dependencies = keyword == "dep" ? record.dependencies : record.bodyDependencies;
                dependencies.emplace_back(rest.substr(0, space), rest.substr(space + 1));
            }
        }
    }
    return false;
}

bool unitRecordMatches(const UnitRecordFile& record, const UnitRecord& unit, const std::vector<UnitRecord>& scanned)
{
    if (record.digest.empty() || record.digest != unit.digest) {
        return false;
    }
    auto matches = [&](const auto& dependencies, bool body) {
        for (const auto& dependency : dependencies) {
            bool found = false;
            for (const UnitRecord& other : scanned) {
                if (other.key == dependency.first) {
                    found = dependency.second == (body ? other.digest : other.specDigest);
                    break;
                }
            }
            if (!found) {
                return false;
            }
        }
        return true;
    };
    return matches(record.dependencies, false) && matches(record.bodyDependencies, true);
}
