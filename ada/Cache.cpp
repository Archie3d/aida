#include "Cache.h"
#include "../support/Artifacts.h"

#include <filesystem>
#include <cstdlib>
#include <unistd.h>
#include <fstream>
#include <sstream>
#include <utility>

std::string programDigest(const std::string& program)
{
    if (program.find_first_of("/\\") != std::string::npos) {
        return digestOfFile(program);
    }
    const char* environment = std::getenv("PATH");
    std::istringstream paths(environment == nullptr ? "" : environment);
    std::string directory;
    while (std::getline(paths, directory, ':')) {
        std::string candidate = (directory.empty() ? "." : directory) + "/" + program;
        if (access(candidate.c_str(), X_OK) == 0) {
            return digestOfFile(candidate);
        }
    }
    return {};
}

std::string toolDigest(const std::vector<std::string>& programs)
{
    std::string description;
    for (const std::string& program : programs) {
        std::error_code ignored;
        auto size = std::filesystem::file_size(program, ignored);
        auto written = std::filesystem::last_write_time(program, ignored);
        description += program;
        description += ":" + std::to_string(static_cast<unsigned long long>(size));
        description += ":" + std::to_string(static_cast<unsigned long long>(written.time_since_epoch().count()));
        description += "\n";
    }
    return digestOf(description);
}

Cache::Cache(std::string directory, std::string stamp)
    : m_directory(std::move(directory))
    , m_stamp(std::move(stamp))
{
}

bool Cache::isCurrent(const std::string& unit) const
{
    std::ifstream record(Artifacts::nativePath(m_directory + "/" + unit + ".build"));
    std::string format;
    std::string intermediate;
    std::string stamp;
    std::string object;
    std::string end;
    if (!std::getline(record, format) || format != "format 2" || !std::getline(record, intermediate)
        || !std::getline(record, stamp) || !std::getline(record, object) || !std::getline(record, end)
        || end != "end" || record.peek() != std::char_traits<char>::eof()) { return false; }
    return !intermediate.empty() && !object.empty() && stamp == m_stamp
        && intermediate == digestOfFile(m_directory + "/" + unit + ".ssa")
        && object == digestOfFile(m_directory + "/" + unit + ".o");
}

bool Cache::record(const std::string& unit) const
{
    Artifacts::AtomicFile publication(m_directory + "/" + unit + ".build");
    publication.stream() << "format 2\n" << digestOfFile(m_directory + "/" + unit + ".ssa") << "\n" << m_stamp
        << "\n" << digestOfFile(m_directory + "/" + unit + ".o") << "\nend\n";
    return publication.commit();
}
