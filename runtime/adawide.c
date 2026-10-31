#include "adart.h"
#include "adaunicode.h"
#include "../common/CharacterText.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int __ada_wide_character_image(void* buffer, int capacity, int value, int width)
{
    uint32_t code = (uint32_t)value;
    uint32_t limit = width == 1 ? 255 : width == 2 ? 65535 : INT_MAX;
    const char* name = adaControlName(code);
    char hex[16];
    if (name == NULL && (code > limit || !(adaUnicodeProperties(code) & 32) || (code & 0xFFFF) >= 0xFFFE)) {
        snprintf(hex, sizeof hex, "Hex_%08X", code);
        name = hex;
    }
    int length = name != NULL ? (int)strlen(name) : 3;
    if (capacity <= length) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
    if (name != NULL) {
        for (int i = 0; i < length; ++i) { adaTextWrite(buffer, i, width, (unsigned char)name[i]); }
    } else {
        adaTextWrite(buffer, 0, width, '\''); adaTextWrite(buffer, 1, width, code); adaTextWrite(buffer, 2, width, '\'');
    }
    adaTextWrite(buffer, length, width, 0);
    return length;
}

int __ada_wide_character_value(const void* text, int length, int width, int maximum)
{
    int start = 0, stop = length;
    while (start < stop && adaTextRead(text, start, width) == ' ') { ++start; }
    while (stop > start && adaTextRead(text, stop - 1, width) == ' ') { --stop; }
    if (stop - start == 3 && adaTextRead(text, start, width) == '\'' && adaTextRead(text, stop - 1, width) == '\'') {
        uint32_t value = adaTextRead(text, start + 1, width);
        if (value <= (uint32_t)maximum && (adaUnicodeProperties(value) & 32) && (value & 0xFFFF) < 0xFFFE) { return (int)value; }
    }
    char name[32];
    if (stop - start < (int)sizeof name) {
        int count = stop - start;
        for (int i = 0; i < count; ++i) {
            uint32_t code = adaTextRead(text, start + i, width);
            if (code > 127) { __ada_raise(ADA_CONSTRAINT_ERROR); return 0; }
            name[i] = code >= 'a' && code <= 'z' ? (char)(code - ('a' - 'A')) : (char)code;
        }
        name[count] = 0;
        for (uint32_t code = 0; code <= 159; ++code) {
            const char* control = adaControlName(code);
            if (control != NULL && strcmp(name, control) == 0 && code <= (uint32_t)maximum) { return (int)code; }
        }
        if (count == 12 && memcmp(name, "HEX_", 4) == 0) {
            uint32_t value = 0;
            for (int i = 4; i < 12; ++i) {
                int digit = name[i] >= '0' && name[i] <= '9' ? name[i] - '0' : name[i] >= 'A' && name[i] <= 'F' ? name[i] - 'A' + 10 : -1;
                if (digit < 0) { __ada_raise(ADA_CONSTRAINT_ERROR); return 0; }
                value = (value << 4) | (unsigned)digit;
            }
            uint32_t limit = width == 1 ? 255 : width == 2 ? 65535 : INT_MAX;
            if (value <= (uint32_t)maximum && adaControlName(value) == NULL
                && (value > limit || !(adaUnicodeProperties(value) & 32) || (value & 0xFFFF) >= 0xFFFE)) { return (int)value; }
        }
    }
    __ada_raise(ADA_CONSTRAINT_ERROR);
    return 0;
}

int __ada_wide_character_width(int low, int high, int width)
{
    return adaCharacterWidth(low, high, width);
}

int __ada_image_widen(void* destination, int capacity, const char* text, int length, int width)
{
    int position = 0, count = 0;
    while (position < length) {
        uint32_t code;
        if (!adaUtf8Decode(text, length, &position, &code)) { __ada_raise(ADA_CONSTRAINT_ERROR); return 0; }
        if (width == 2 && code > 65535) {
            char escape[16];
            snprintf(escape, sizeof escape, "[\"%08X\"]", code);
            if (capacity - count < 12) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
            for (int i = 0; i < 12; ++i) { adaTextWrite(destination, count++, width, (unsigned char)escape[i]); }
            continue;
        }
        if (count >= capacity) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
        adaTextWrite(destination, count++, width, code);
    }
    return count;
}

void __ada_wide_to_utf8(void* result, const void* text, int length, int width)
{
    if (length < 0 || (size_t)length > SIZE_MAX / 4) { __ada_raise(ADA_STORAGE_ERROR); return; }
    char* bytes = malloc(length == 0 ? 1 : (size_t)length * 4);
    if (bytes == NULL) { __ada_raise(ADA_STORAGE_ERROR); return; }
    int64_t count = 0;
    for (int i = 0; i < length; ++i) {
        uint32_t code = adaTextRead(text, i, width);
        // Wide_Image spells supplementary identifier characters as twelve
        // ASCII cells. Wide_Value must undo that spelling before name lookup.
        if (width == 2 && code == '[' && length - i >= 12
            && adaTextRead(text, i + 1, width) == '"'
            && adaTextRead(text, i + 10, width) == '"'
            && adaTextRead(text, i + 11, width) == ']') {
            uint32_t escaped = 0;
            int valid = 1;
            for (int j = 2; j < 10; ++j) {
                uint32_t cell = adaTextRead(text, i + j, width);
                int digit = cell >= '0' && cell <= '9' ? (int)(cell - '0')
                    : cell >= 'A' && cell <= 'F' ? (int)(cell - 'A' + 10)
                    : cell >= 'a' && cell <= 'f' ? (int)(cell - 'a' + 10) : -1;
                if (digit < 0) { valid = 0; break; }
                escaped = (escaped << 4) | (unsigned)digit;
            }
            if (valid && escaped > 65535 && escaped <= 0x10FFFF) {
                code = escaped;
                i += 11;
            }
        }
        if (code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) { free(bytes); __ada_raise(ADA_CONSTRAINT_ERROR); return; }
        count += adaUtf8Encode(bytes + count, code);
    }
    if (count > INT_MAX) { free(bytes); __ada_raise(ADA_STORAGE_ERROR); return; }
    __ada_array_result(result, bytes, 1, count, 1);
    free(bytes);
}

static void exceptionName(void* result, const char* text, int length, int width)
{
    if (length < 0 || length > INT_MAX / 3 || (size_t)length > SIZE_MAX / ((unsigned)width * 3)) { __ada_raise(ADA_STORAGE_ERROR); return; }
    void* data = malloc(length == 0 ? 1 : (size_t)length * (unsigned)width * 3);
    if (data == NULL) { __ada_raise(ADA_STORAGE_ERROR); return; }
    int count = __ada_image_widen(data, length * 3, text, length, width);
    if (__ada_exception == NULL) { __ada_array_result(result, data, 1, count, width); }
    free(data);
}

void __ada_utf8_wide(void* result, const char* text, int length) { exceptionName(result, text, length, 2); }
void __ada_utf8_wide_wide(void* result, const char* text, int length) { exceptionName(result, text, length, 4); }

int __ada_width_enum_text(int low, int high, const char** names, int count, int width)
{
    if (width == 1) { return __ada_width_enum(low, high, names, count); }
    if (low > high) { return 0; }
    if (low < 0 || high >= count) { __ada_raise(ADA_CONSTRAINT_ERROR); return 0; }
    int result = 0;
    for (int i = low; i <= high; ++i) {
        size_t bytes = strlen(names[i]);
        if (bytes > INT_MAX / 3) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
        int position = 0, length = 0;
        while (position < (int)bytes) {
            uint32_t code;
            if (!adaUtf8Decode(names[i], (int)bytes, &position, &code)) { __ada_raise(ADA_CONSTRAINT_ERROR); return 0; }
            length += width == 2 && code > 65535 ? 12 : 1;
        }
        if (length > result) { result = length; }
    }
    return result;
}

typedef struct
{
    const char* text;
    int length;
    int position;
    uint32_t pending[3];
    unsigned count;
    unsigned next;
    int fold;
} UnicodeCursor;

static int nextCharacter(UnicodeCursor* cursor, uint32_t* value)
{
    for (;;) {
        if (cursor->next < cursor->count) { *value = cursor->pending[cursor->next++]; return 1; }
        if (cursor->position >= cursor->length) { return 0; }
        uint32_t code;
        if (!adaUtf8Decode(cursor->text, cursor->length, &cursor->position, &code)) {
            code = (unsigned char)cursor->text[cursor->position++]; // Character-valued Latin-1 input.
        }
        if (!cursor->fold) { *value = code; return 1; }
        if (adaUnicodeProperties(code) & 8) { continue; }
        size_t low = 0, high = sizeof(kAdaUnicodeUpper) / sizeof(kAdaUnicodeUpper[0]);
        while (low < high) {
            size_t middle = low + (high - low) / 2;
            if (kAdaUnicodeUpper[middle].code < code) { low = middle + 1; }
            else { high = middle; }
        }
        if (low < sizeof(kAdaUnicodeUpper) / sizeof(kAdaUnicodeUpper[0]) && kAdaUnicodeUpper[low].code == code) {
            memcpy(cursor->pending, kAdaUnicodeUpper[low].upper, sizeof cursor->pending);
            cursor->count = kAdaUnicodeUpper[low].length;
            cursor->next = 0;
        } else { *value = code; return 1; }
    }
}

int __ada_identifier_equal(const char* left, int leftLength, const char* right, int rightLength, int fold)
{
    if (fold) {
        int position = 0, first = 1, connector = 0;
        while (position < leftLength) {
            uint32_t code;
            if (!adaUtf8Decode(left, leftLength, &position, &code)) { code = (unsigned char)left[position++]; }
            unsigned flags = adaUnicodeProperties(code);
            if (first && !(flags & 1)) { return 0; }
            first = 0;
            if (flags & 8) { continue; }
            if (!(flags & 3) || ((flags & 4) && connector)) { return 0; }
            connector = (flags & 4) != 0;
        }
        if (first || connector) { return 0; }
    }
    UnicodeCursor a = { .text = left, .length = leftLength, .fold = fold };
    UnicodeCursor b = { .text = right, .length = rightLength, .fold = fold };
    for (;;) {
        uint32_t x, y;
        int hasLeft = nextCharacter(&a, &x), hasRight = nextCharacter(&b, &y);
        if (hasLeft != hasRight) { return 0; }
        if (!hasLeft) { return 1; }
        if (x != y) { return 0; }
    }
}
