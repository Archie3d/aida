#pragma once

#include <fstream>
#include <sstream>
#include <string>

// A 64 bit FNV-1a digest written as hexadecimal.  Content is what decides
// whether something is out of date: a file rewritten with the same text is no
// change at all.
inline std::string digestOf(const std::string& text)
{
    unsigned long long hash = 14695981039346656037ull;
    for (unsigned char c : text) {
        hash ^= c;
        hash *= 1099511628211ull;
    }

    static const char digits[] = "0123456789abcdef";
    std::string result(16, '0');
    for (int i = 15; i >= 0; --i) {
        result[i] = digits[hash & 0xf];
        hash >>= 4;
    }
    return result;
}

// Empty when the file cannot be read, which counts as out of date.
inline std::string digestOfFile(const std::string& path)
{
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return std::string();
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return digestOf(buffer.str());
}
