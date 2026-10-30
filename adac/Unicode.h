#pragma once

#include "../common/UnicodeData.h"

#include <cstddef>
#include <string>
#include <string_view>

namespace Unicode
{

constexpr unsigned kIdentifierStart = 1;
constexpr unsigned kIdentifierExtend = 2;
constexpr unsigned kConnector = 4;
constexpr unsigned kFormat = 8;
constexpr unsigned kSpace = 16;
constexpr unsigned kGraphic = 32;

inline unsigned properties(uint32_t code)
{
    std::size_t low = 0, high = sizeof(kAdaUnicodeRanges) / sizeof(kAdaUnicodeRanges[0]);
    while (low < high) {
        std::size_t middle = low + (high - low) / 2;
        const auto& range = kAdaUnicodeRanges[middle];
        if (code < range.low) { high = middle; }
        else if (code > range.high) { low = middle + 1; }
        else { return range.flags; }
    }
    return kGraphic; // Unassigned cells are characters in Ada 2005.
}

inline bool decode(std::string_view text, std::size_t position, uint32_t& code, std::size_t& count)
{
    count = 1;
    code = 0xFFFD;
    if (position >= text.size()) { return false; }
    unsigned first = static_cast<unsigned char>(text[position]);
    if (first < 0x80) { code = first; return true; }
    unsigned length = first >= 0xC2 && first <= 0xDF ? 2 : first >= 0xE0 && first <= 0xEF ? 3
        : first >= 0xF0 && first <= 0xF4 ? 4 : 0;
    if (length == 0 || text.size() - position < length) { return false; }
    uint32_t value = first & ((1u << (7 - length)) - 1);
    for (unsigned i = 1; i < length; ++i) {
        unsigned byte = static_cast<unsigned char>(text[position + i]);
        if ((byte & 0xC0) != 0x80) { return false; }
        value = (value << 6) | (byte & 0x3F);
    }
    if ((length == 2 && value < 0x80) || (length == 3 && value < 0x800)
        || (length == 4 && value < 0x10000) || value > 0x10FFFF || (value >= 0xD800 && value <= 0xDFFF)) { return false; }
    code = value;
    count = length;
    return true;
}

inline std::string encode(uint32_t code)
{
    std::string result;
    if (code < 0x80) { result += static_cast<char>(code); }
    else if (code < 0x800) {
        result += static_cast<char>(0xC0 | (code >> 6));
        result += static_cast<char>(0x80 | (code & 0x3F));
    } else if (code < 0x10000) {
        result += static_cast<char>(0xE0 | (code >> 12));
        result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (code & 0x3F));
    } else {
        result += static_cast<char>(0xF0 | (code >> 18));
        result += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
        result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
        result += static_cast<char>(0x80 | (code & 0x3F));
    }
    return result;
}

inline std::u32string characters(std::string_view text)
{
    std::u32string result;
    for (std::size_t position = 0; position < text.size();) {
        uint32_t code;
        std::size_t count;
        decode(text, position, code, count);
        result.push_back(code);
        position += count;
    }
    return result;
}

inline std::string utf8(std::u32string_view text)
{
    std::string result;
    for (uint32_t code : text) { result += encode(code); }
    return result;
}

inline std::string linkerIdentifier(std::string_view text)
{
    bool ascii = true;
    for (unsigned char byte : text) { ascii &= byte < 128; }
    if (ascii) { return std::string(text); }
    constexpr char kHex[] = "0123456789abcdef";
    std::string result = "unicode.";
    for (unsigned char byte : text) { result += kHex[byte >> 4]; result += kHex[byte & 15]; }
    return result;
}

inline std::string identifier(std::string_view text, bool canonical = true)
{
    std::string result;
    auto append = [&](uint32_t code) { result += encode(canonical && code >= 'A' && code <= 'Z' ? code + ('a' - 'A') : code); };
    for (uint32_t code : characters(text)) {
        if (properties(code) & kFormat) { continue; }
        std::size_t low = 0, high = sizeof(kAdaUnicodeUpper) / sizeof(kAdaUnicodeUpper[0]);
        while (low < high) {
            std::size_t middle = low + (high - low) / 2;
            if (kAdaUnicodeUpper[middle].code < code) { low = middle + 1; }
            else { high = middle; }
        }
        if (low < sizeof(kAdaUnicodeUpper) / sizeof(kAdaUnicodeUpper[0]) && kAdaUnicodeUpper[low].code == code) {
            const auto& mapping = kAdaUnicodeUpper[low];
            for (unsigned i = 0; i < mapping.length; ++i) { append(mapping.upper[i]); }
        } else { append(code); }
    }
    return result;
}

}
