#include "UnitManifest.h"
#include "Digest.h"
#include "../support/Artifacts.h"

#include <fstream>
#include <sstream>
#include <map>
#include <set>

namespace {

// Every line is a keyword and the rest of the line, which keeps paths with
// spaces in them intact.
bool split(const std::string& line, std::string& keyword, std::string& rest)
{
    std::size_t space = line.find(' ');
    keyword = line.substr(0, space);
    rest = space == std::string::npos ? std::string() : line.substr(space + 1);
    return !keyword.empty();
}

} // namespace

bool writeManifest(const std::string& path, const std::vector<UnitRecord>& units, const std::vector<std::string>& elaborations, const std::vector<std::string>& searchPaths, const std::string& compilerDigest, const Artifacts::FileInputs& inputs)
{
    Artifacts::AtomicFile publication(path);
    return writeManifest(publication.stream(), units, elaborations, searchPaths, compilerDigest, inputs) && publication.commit();
}

bool writeManifest(std::ostream& out, const std::vector<UnitRecord>& units, const std::vector<std::string>& elaborations, const std::vector<std::string>& searchPaths, const std::string& compilerDigest, const Artifacts::FileInputs& inputs)
{
    out << "format 9\n";
    if (!compilerDigest.empty()) { out << "compiler " << compilerDigest << "\n"; }
    for (const auto& [path, fingerprint] : inputs) {
        out << "input " << (fingerprint.empty() ? "-" : fingerprint) << " " << path << "\n";
    }
    for (const std::string& root : searchPaths) { out << "search " << root << "\n"; }
    for (const std::string& elaboration : elaborations) {
        out << "elaborate " << elaboration << "\n";
    }
    for (const UnitRecord& unit : units) {
        out << "unit " << unit.key << "\n";
        if (!unit.interfaceDirectory.empty()) { out << "interface " << unit.interfaceDirectory << "\n"; }
        out << "identity " << unit.sourceIdentity << "\n";
        out << "spec-identity " << unit.specIdentity << "\n";
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
        for (const UnitPartRecord& part : unit.parts) {
            out << "part " << part.key << "\n";
            if (part.category != PackageCategory::Ordinary) {
                out << "category " << static_cast<int>(part.category) << "\n";
            }
            if (part.elaborateBody) {
                out << "immediate-body " << part.key << "\n";
            }
            for (const std::string& dependency : part.dependencies) {
                out << "requires " << dependency << "\n";
            }
            for (const std::string& dependency : part.limitedDependencies) {
                out << "limited " << dependency << "\n";
            }
            out << "part-origin " << part.source << "\n";
            if (part.clientNeedsBody) { out << "client-body yes\n"; }
            for (const std::string& dependency : part.semanticDependencies) { out << "semantic " << dependency << "\n"; }
        }
        out << "end\n";
    }
    out << "end-manifest\n";
    return static_cast<bool>(out);
}

bool readManifest(const std::string& path, std::vector<UnitRecord>& units, std::vector<std::string>* elaborations, std::vector<std::string>* searchPaths, std::string* compilerDigest, Artifacts::FileInputs* inputs)
{
    std::ifstream in(Artifacts::nativePath(path));
    if (!in) {
        return false;
    }
    return readManifest(in, units, elaborations, searchPaths, compilerDigest, inputs);
}

bool readManifest(std::istream& in, std::vector<UnitRecord>& units, std::vector<std::string>* elaborations, std::vector<std::string>* searchPaths, std::string* compilerDigest, Artifacts::FileInputs* inputs)
{
    std::string line;
    if (!std::getline(in, line) || line != "format 9") {
        return false;
    }
    bool inUnit = false;
    bool seenCompiler = false;
    while (std::getline(in, line)) {
        std::string keyword;
        std::string rest;
        if (!split(line, keyword, rest)) {
            continue;
        }
        if (keyword == "end-manifest") {
            return !inUnit && rest.empty() && in.peek() == std::char_traits<char>::eof();
        } else if (keyword == "end") {
            if (!inUnit || !rest.empty() || units.back().digest.empty()
                || units.back().specDigest.empty() || units.back().sources.empty()
                || units.back().sourceIdentity.empty() || units.back().specIdentity.empty()
                || units.back().parts.empty() || units.back().parts.back().source.empty()) {
                return false;
            }
            inUnit = false;
        } else if (rest.empty()) {
            return false;
        } else if (keyword == "compiler" && !inUnit && !seenCompiler) {
            seenCompiler = true;
            if (compilerDigest != nullptr) { *compilerDigest = rest; }
        } else if (keyword == "input" && !inUnit) {
            auto space = rest.find(' ');
            if (space == std::string::npos || space == 0 || space + 1 == rest.size()) { return false; }
            if (inputs != nullptr) { inputs->emplace_back(rest.substr(space + 1), rest.substr(0, space)); }
        } else if (keyword == "search" && !inUnit) {
            if (searchPaths != nullptr) { searchPaths->push_back(rest); }
        } else if (keyword == "elaborate" && !inUnit) {
            if (elaborations != nullptr) {
                elaborations->push_back(rest);
            }
        } else if (keyword == "unit" && !inUnit) {
            units.push_back(UnitRecord {});
            units.back().key = rest;
            inUnit = true;
        } else if (!inUnit) {
            return false;
        } else if (keyword == "interface" && units.back().interfaceDirectory.empty()) {
            units.back().interfaceDirectory = rest;
        } else if (keyword == "identity" && units.back().sourceIdentity.empty()) {
            units.back().sourceIdentity = rest;
        } else if (keyword == "spec-identity" && units.back().specIdentity.empty()) {
            units.back().specIdentity = rest;
        } else if (keyword == "digest" && units.back().digest.empty()) {
            units.back().digest = rest;
        } else if (keyword == "spec" && units.back().specDigest.empty()) {
            units.back().specDigest = rest;
        } else if (keyword == "source") {
            units.back().sources.push_back(rest);
        } else if (keyword == "with") {
            units.back().withKeys.push_back(rest);
        } else if (keyword == "part") {
            if (!units.back().parts.empty() && units.back().parts.back().source.empty()) { return false; }
            units.back().parts.push_back(UnitPartRecord { rest, {} });
        } else if (keyword == "part-origin" && !units.back().parts.empty() && units.back().parts.back().source.empty()) {
            units.back().parts.back().source = rest;
        } else if (keyword == "client-body" && !units.back().parts.empty()
            && !units.back().parts.back().clientNeedsBody && rest == "yes") {
            units.back().parts.back().clientNeedsBody = true;
        } else if (keyword == "semantic" && !units.back().parts.empty()) {
            units.back().parts.back().semanticDependencies.push_back(rest);
        } else if (keyword == "category" && !units.back().parts.empty()
            && units.back().parts.back().category == PackageCategory::Ordinary && (rest == "1" || rest == "2")) {
            units.back().parts.back().category = rest == "2" ? PackageCategory::Pure : PackageCategory::Preelaborate;
        } else if (keyword == "immediate-body" && !units.back().parts.empty()
            && rest == units.back().parts.back().key && !units.back().parts.back().elaborateBody) {
            units.back().parts.back().elaborateBody = true;
        } else if (keyword == "limited" && !units.back().parts.empty()) {
            units.back().parts.back().limitedDependencies.push_back(rest);
        } else if (keyword == "requires" && !units.back().parts.empty()) {
            units.back().parts.back().dependencies.push_back(rest);
        } else {
            return false;
        }
    }
    return false;
}

bool writeUnitRecord(const std::string& path, const UnitRecordFile& record)
{
    Artifacts::AtomicFile publication(path, std::ios::binary);
    std::ostringstream out;
    out << "format 4\n";
    out << "compiler " << record.compilerDigest << "\n";
    out << "ir " << record.irDigest << "\n";
    out << "digest " << record.digest << "\n";
    out << "identity " << record.sourceIdentity << "\n";
    for (const std::string& key : record.replayDependencies) { out << "replay " << key << "\n"; }
    for (const auto& entry : record.interfaceDigests) { out << "interface " << entry.first << " " << entry.second << "\n"; }
    for (const auto& entry : record.dependencySources) { out << "source " << entry.first << " " << entry.second << "\n"; }
    for (const auto& entry : record.bodyDependencies) { out << "bodydep " << entry.first << " " << entry.second << "\n"; }
    for (const auto& entry : record.dependencies) { out << "dep " << entry.first << " " << entry.second << "\n"; }
    if (!record.mainName.empty()) { out << "main " << record.mainName << "\n"; }
    std::string body = out.str();
    publication.stream() << body << "end " << digestOf(body) << "\n";
    return publication.commit();
}

bool readUnitRecord(const std::string& path, UnitRecordFile& record, std::string* fileDigest)
{
    std::ifstream file(Artifacts::nativePath(path), std::ios::binary);
    if (!file) { return false; }
    std::ostringstream contents;
    contents << file.rdbuf();
    if (file.bad()) { return false; }
    std::string data = contents.str();
    std::istringstream in(data);
    std::string line;
    if (!std::getline(in, line) || line != "format 4") { return false; }
    UnitRecordFile result;
    std::set<std::string> fields;
    while (std::getline(in, line)) {
        std::string keyword;
        std::string rest;
        if (!split(line, keyword, rest)) { return false; }
        if (keyword == "end") {
            auto marker = data.rfind("\nend ");
            if (marker == std::string::npos || rest != digestOf(data.substr(0, marker + 1))
                || in.peek() != std::char_traits<char>::eof() || result.digest.empty()
                || result.compilerDigest.empty() || result.irDigest.empty() || result.sourceIdentity.empty()
                || result.interfaceDigests.empty()) { return false; }
            record = std::move(result);
            if (fileDigest != nullptr) { *fileDigest = digestOf(data); }
            return true;
        }
        if (rest.empty()) { return false; }
        if (keyword == "replay") {
            if (!fields.insert(keyword + " " + rest).second) { return false; }
            result.replayDependencies.push_back(rest);
        } else if (keyword == "dep" || keyword == "bodydep" || keyword == "interface" || keyword == "source") {
            std::size_t space = rest.find_last_of(' ');
            if (space == std::string::npos || space == 0 || space + 1 == rest.size()
                || !fields.insert(keyword + " " + rest.substr(0, space)).second) { return false; }
            auto* entries = keyword == "dep" ? &result.dependencies : keyword == "bodydep" ? &result.bodyDependencies
                : keyword == "interface" ? &result.interfaceDigests : &result.dependencySources;
            entries->emplace_back(rest.substr(0, space), rest.substr(space + 1));
        } else {
            if (!fields.insert(keyword).second) { return false; }
            if (keyword == "compiler") { result.compilerDigest = rest; }
            else if (keyword == "ir") { result.irDigest = rest; }
            else if (keyword == "digest") { result.digest = rest; }
            else if (keyword == "identity") { result.sourceIdentity = rest; }
            else if (keyword == "main") { result.mainName = rest; }
            else { return false; }
        }
    }
    return false;
}

std::string interfaceAliasKey(const std::string& partKey)
{
    // Object-group names and declared-part names have separate namespaces.
    // Escape unsafe filename bytes, including operator designators.
    static constexpr char kHex[] = "0123456789abcdef";
    std::string key = ".aida-interfaces/";
    for (unsigned char c : partKey) {
        if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-' || c == '.') {
            key += static_cast<char>(c);
        } else {
            key += '%';
            key += kHex[c >> 4];
            key += kHex[c & 15];
        }
    }
    return key;
}

bool unitArtifactsMatch(const std::string& directory, const UnitRecord& unit, const UnitRecordFile& record)
{
    const std::string& key = unit.key;
    if (record.irDigest.empty() || record.irDigest != digestOfFile(directory + "/" + key + ".ssa")) { return false; }
    std::set<std::string> expected { key };
    for (const auto& part : unit.parts) { expected.insert(interfaceAliasKey(part.key)); }
    std::set<std::string> seen;
    for (const auto& [name, digest] : record.interfaceDigests) {
        if (!expected.contains(name) || !seen.insert(name).second || digest.empty()
            || digest != digestOfFile(directory + "/" + name + ".asi")) { return false; }
    }
    return seen == expected;
}

bool unitRecordMatches(const UnitRecordFile& record, const UnitRecord& unit, const std::vector<UnitRecord>& scanned)
{
    if (record.digest.empty() || record.digest != unit.digest || record.sourceIdentity != unit.sourceIdentity) { return false; }
    std::map<std::string, std::pair<const UnitRecord*, const UnitPartRecord*>> parts;
    std::map<std::string, const UnitRecord*> owners;
    for (const auto& other : scanned) {
        if (!owners.emplace(other.key, &other).second) { return false; }
        for (const auto& part : other.parts) {
            if (!parts.emplace(part.key, std::make_pair(&other, &part)).second) { return false; }
        }
    }
    std::set<std::string> replay(record.replayDependencies.begin(), record.replayDependencies.end());
    std::vector<std::string> pending;
    for (const auto& part : unit.parts) { pending.push_back(part.key); }
    if (parts.contains("ada-io_exceptions.spec")) { pending.push_back("ada-io_exceptions.spec"); }
    std::set<std::string> visited;
    std::map<std::string, bool> expected;
    while (!pending.empty()) {
        std::string key = pending.back();
        pending.pop_back();
        if (!visited.insert(key).second) { continue; }
        auto found = parts.find(key);
        if (found == parts.end()) { return false; }
        const auto& [owner, part] = found->second;
        expected[owner->key] = expected[owner->key] || key.ends_with(".body");
        for (const auto& sibling : owner->parts) {
            if (owner->key == unit.key || replay.contains(owner->key) || sibling.source == part->source
                || (part->clientNeedsBody && sibling.key == key.substr(0, key.size() - 5) + ".body")) {
                pending.push_back(sibling.key);
            }
        }
        if (key.ends_with(".spec") && (owner->key == unit.key || part->clientNeedsBody || replay.contains(owner->key))) {
            std::string body = key.substr(0, key.size() - 5) + ".body";
            if (parts.contains(body)) { pending.push_back(body); }
        }
        pending.insert(pending.end(), part->semanticDependencies.begin(), part->semanticDependencies.end());
    }
    expected.erase(unit.key);
    for (const std::string& key : replay) { if (!expected.contains(key)) { return false; } }
    std::map<std::string, bool> recorded;
    for (const auto* entries : { &record.dependencies, &record.bodyDependencies }) {
        bool body = entries == &record.bodyDependencies;
        for (const auto& [key, digest] : *entries) {
            auto found = owners.find(key);
            if (found == owners.end() || !recorded.emplace(key, body).second
                || digest != (body ? found->second->digest : found->second->specDigest)) { return false; }
        }
    }
    if (recorded != expected || record.dependencySources.size() != expected.size()) { return false; }
    std::set<std::string> provenances;
    for (const auto& [key, identity] : record.dependencySources) {
        if (!expected.contains(key) || !provenances.insert(key).second) { return false; }
        const auto* other = owners.at(key);
        if (identity != (expected.at(key) ? other->sourceIdentity : other->specIdentity)) { return false; }
    }
    return true;
}
