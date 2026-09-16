#include "Cache.h"

#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

namespace
{

std::string hexadecimal(unsigned long long value)
{
    static const char digits[] = "0123456789abcdef";
    std::string text(16, '0');
    for (int i = 15; i >= 0; --i) {
        text[i] = digits[value & 0xf];
        value >>= 4;
    }
    return text;
}

}

std::string digestOf(const std::string& text)
{
    unsigned long long hash = 14695981039346656037ull;
    for (unsigned char c : text) {
        hash ^= c;
        hash *= 1099511628211ull;
    }
    return hexadecimal(hash);
}

std::string digestOfFile(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return std::string();
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return digestOf(buffer.str());
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
