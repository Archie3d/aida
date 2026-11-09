#include "BuildState.h"

#include "Digest.h"
#include "../support/HostPaths.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#ifndef _WIN32
#   include <unistd.h>
extern char** environ;
#endif

namespace {

constexpr std::size_t kMaxReceiptBytes = 32 * 1024 * 1024;

} // namespace

BuildState::BuildState(std::string path, std::string configuration)
    : m_path(std::move(path))
    , m_configuration(std::move(configuration))
{
}

bool BuildState::isCurrent() const
{
    std::error_code error;
    auto size = std::filesystem::file_size(Artifacts::nativePath(m_path), error);
    if (error || size > kMaxReceiptBytes) { return false; }
    std::ifstream file(Artifacts::nativePath(m_path), std::ios::binary);
    std::ostringstream contents;
    contents << file.rdbuf();
    if (!file || file.bad()) { return false; }
    std::string data = contents.str();
    std::istringstream in(data);
    std::string line;
    if (!std::getline(in, line) || line != "AIDA build receipt 1"
        || !std::getline(in, line) || line != "configuration " + m_configuration) { return false; }
    Artifacts::FileInputs inputs;
    std::set<std::string> paths;
    while (std::getline(in, line)) {
        if (line.starts_with("end ")) {
            auto marker = data.rfind("\nend ");
            return marker != std::string::npos && line.substr(4) == digestOf(data.substr(0, marker + 1))
                && in.peek() == std::char_traits<char>::eof() && !inputs.empty() && Artifacts::inputsMatch(inputs);
        }
        if (!line.starts_with("file ")) { return false; }
        auto space = line.find(' ', 5);
        if (space == std::string::npos || space == 5 || space + 1 == line.size()) { return false; }
        std::string path = line.substr(space + 1);
        if (!paths.insert(path).second) { return false; }
        inputs.emplace_back(std::move(path), line.substr(5, space - 5));
    }
    return false;
}

bool BuildState::save(const Artifacts::FileInputs& inputs, const std::vector<std::string>& products) const
{
    std::map<std::string, std::string> files;
    auto append = [&](const std::string& path, const std::string& fingerprint) {
        if (path.empty() || path.find_first_of("\n\r") != std::string::npos || path.find('\0') != std::string::npos
            || fingerprint.empty() || fingerprint == "-") { return false; }
        auto [entry, inserted] = files.emplace(path, fingerprint);
        return inserted || entry->second == fingerprint;
    };
    for (const auto& [path, fingerprint] : inputs) { if (!append(path, fingerprint)) { return false; } }
    for (const std::string& path : products) {
        std::error_code error;
        if (!std::filesystem::is_regular_file(Artifacts::nativePath(path), error) || error
            || !append(path, Artifacts::fileFingerprint(path))) { return false; }
    }
    Artifacts::FileInputs snapshot(files.begin(), files.end());
    if (!Artifacts::inputsMatch(snapshot)) { return false; }
    std::ostringstream out;
    out << "AIDA build receipt 1\nconfiguration " << m_configuration << "\n";
    for (const auto& [path, fingerprint] : files) { out << "file " << fingerprint << " " << path << "\n"; }
    std::string body = out.str();
    if (body.size() > kMaxReceiptBytes - 64) { return false; }
    Artifacts::AtomicFile publication(m_path, std::ios::binary);
    publication.stream() << body << "end " << digestOf(body) << "\n";
    return publication.commit();
}

std::string environmentDigest()
{
    std::vector<std::string> values;
#ifdef _WIN32
    char** entries = _environ;
#else
    char** entries = environ;
#endif
    if (entries != nullptr) {
        for (; *entries != nullptr; ++entries) {
            std::string entry = *entries;
            if (entry.starts_with("_=") || entry.starts_with("SHLVL=")) { continue; }
            values.push_back(std::move(entry));
        }
    }
    std::sort(values.begin(), values.end());
    std::string text;
    for (const auto& value : values) { text += value; text.push_back('\0'); }
    // Persist the digest, never environment values or credentials.
    return digestOf(text);
}

std::string resolveProgram(const std::string& program)
{
    auto present = [](const std::string& path) {
        std::error_code error;
        bool regular = std::filesystem::is_regular_file(Artifacts::nativePath(path), error) && !error;
#ifdef _WIN32
        return regular;
#else
        return regular && access(path.c_str(), X_OK) == 0;
#endif
    };
    if (program.find_first_of("/\\") != std::string::npos) { return program; }
    std::vector<std::string> directories;
#ifdef _WIN32
    directories.push_back(".");
#endif
    const char* path = std::getenv("PATH");
    if (path != nullptr) {
        auto entries = HostPaths::splitList(path);
        directories.insert(directories.end(), entries.begin(), entries.end());
    }
    for (const auto& directory : directories) {
        std::string candidate = (directory.empty() ? "." : directory) + "/" + program;
        if (present(candidate)) { return candidate; }
#ifdef _WIN32
        if (present(candidate + ".exe")) { return candidate + ".exe"; }
#endif
    }
    return program;
}
