#pragma once

#include <string>
#include <vector>

namespace HostPaths {

#ifdef _WIN32
constexpr char kListSeparator = ';';
#else
constexpr char kListSeparator = ':';
#endif

inline std::string directoryOf(const std::string& path)
{
    std::size_t slash = path.find_last_of("/\\");
    return slash == std::string::npos ? std::string() : path.substr(0, slash);
}

inline std::vector<std::string> splitList(const std::string& text, char separator = kListSeparator)
{
    std::vector<std::string> parts;
    std::size_t start = 0;
    while (start <= text.size()) {
        std::size_t end = text.find(separator, start);
        parts.push_back(text.substr(start, end == std::string::npos ? end : end - start));
        if (end == std::string::npos) { break; }
        start = end + 1;
    }
    return parts;
}

} // namespace HostPaths
