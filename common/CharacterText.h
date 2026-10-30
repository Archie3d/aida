/* Shared Ada character image/width policy; Unicode 4.0 properties. */
#ifndef ADA_CHARACTER_TEXT_H
#define ADA_CHARACTER_TEXT_H
#include "UnicodeData.h"
#include <limits.h>
#include <stddef.h>
#include <string.h>

static inline unsigned adaUnicodeProperties(uint32_t code)
{
    size_t low = 0, high = sizeof(kAdaUnicodeRanges) / sizeof(kAdaUnicodeRanges[0]);
    while (low < high) {
        size_t middle = low + (high - low) / 2;
        const AdaUnicodeRange* range = &kAdaUnicodeRanges[middle];
        if (code < range->low) { high = middle; }
        else if (code > range->high) { low = middle + 1; }
        else { return range->flags; }
    }
    return 32;
}

static inline const char* adaControlName(uint32_t value)
{
    static const char* const kC0[] = { "NUL", "SOH", "STX", "ETX", "EOT", "ENQ", "ACK", "BEL",
        "BS", "HT", "LF", "VT", "FF", "CR", "SO", "SI", "DLE", "DC1", "DC2", "DC3", "DC4", "NAK",
        "SYN", "ETB", "CAN", "EM", "SUB", "ESC", "FS", "GS", "RS", "US" };
    static const char* const kC1[] = { "DEL", "RESERVED_128", "RESERVED_129", "BPH", "NBH", "RESERVED_132",
        "NEL", "SSA", "ESA", "HTS", "HTJ", "VTS", "PLD", "PLU", "RI", "SS2", "SS3", "DCS", "PU1", "PU2",
        "STS", "CCH", "MW", "SPA", "EPA", "SOS", "RESERVED_153", "SCI", "CSI", "ST", "OSC", "PM", "APC" };
    return value < 32 ? kC0[value] : value >= 127 && value <= 159 ? kC1[value - 127] : NULL;
}

static inline int adaCharacterWidth(int low, int high, int width)
{
    if (low > high) { return 0; }
    uint32_t limit = width == 1 ? 255 : width == 2 ? 65535 : INT_MAX;
    if ((uint32_t)high > limit) { return 12; }
    int result = 3;
    for (uint32_t code = 0; code <= 159; ++code) {
        if (code < (uint32_t)low || code > (uint32_t)high) { continue; }
        const char* name = adaControlName(code);
        if (name != NULL && (int)strlen(name) > result) { result = (int)strlen(name); }
    }
    for (size_t i = 0; i < sizeof(kAdaUnicodeRanges) / sizeof(kAdaUnicodeRanges[0]); ++i) {
        const AdaUnicodeRange* range = &kAdaUnicodeRanges[i];
        if (!(range->flags & 32) && range->high > 255 && range->low <= (uint32_t)high && range->high >= (uint32_t)low) { return 12; }
    }
    uint64_t next = ((uint32_t)low & ~65535u) | 65534u;
    if (next < (uint32_t)low) { next += 65536; }
    return next <= (uint32_t)high ? 12 : result;
}

#endif
