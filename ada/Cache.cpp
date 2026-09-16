#include "Cache.h"

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
    std::ifstream object(m_directory + "/" + unit + ".o", std::ios::binary);
    if (!object) {
        return false;
    }

    std::ifstream record(m_directory + "/" + unit + ".build");
    if (!record) {
        return false;
    }
    std::string intermediate;
    std::string stamp;
    std::getline(record, intermediate);
    std::getline(record, stamp);

    return !intermediate.empty() && stamp == m_stamp
        && intermediate == digestOfFile(m_directory + "/" + unit + ".ssa");
}

void Cache::record(const std::string& unit) const
{
    std::ofstream out(m_directory + "/" + unit + ".build");
    out << digestOfFile(m_directory + "/" + unit + ".ssa") << "\n" << m_stamp << "\n";
}
