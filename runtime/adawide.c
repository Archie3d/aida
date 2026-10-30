#include "adart.h"
#include "../common/CharacterText.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t readCharacter(const void* text, int index, int width)
{
    if (width == 1) { return ((const unsigned char*)text)[index]; }
    if (width == 2) { uint16_t value; memcpy(&value, (const char*)text + (size_t)index * 2, 2); return value; }
    uint32_t value; memcpy(&value, (const char*)text + (size_t)index * 4, 4); return value;
}

static void writeCharacter(void* text, int index, int width, uint32_t value)
{
    if (width == 1) { ((unsigned char*)text)[index] = (unsigned char)value; }
    else if (width == 2) { uint16_t cell = (uint16_t)value; memcpy((char*)text + (size_t)index * 2, &cell, 2); }
    else { memcpy((char*)text + (size_t)index * 4, &value, 4); }
}

static int decode(const char* text, int length, int* position, uint32_t* value)
{
    if (*position >= length) { return 0; }
    unsigned first = (unsigned char)text[*position];
    unsigned count = first < 128 ? 1 : first >= 0xC2 && first <= 0xDF ? 2
        : first >= 0xE0 && first <= 0xEF ? 3 : first >= 0xF0 && first <= 0xF4 ? 4 : 0;
    if (count == 0 || count > (unsigned)(length - *position)) { return 0; }
    uint32_t code = first & (count == 1 ? 127 : (1u << (7 - count)) - 1);
    for (unsigned i = 1; i < count; ++i) {
        unsigned byte = (unsigned char)text[*position + (int)i];
        if ((byte & 0xC0) != 0x80) { return 0; }
        code = (code << 6) | (byte & 63);
    }
    if ((count == 2 && code < 128) || (count == 3 && code < 0x800) || (count == 4 && code < 0x10000)
        || code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) { return 0; }
    *value = code;
    *position += (int)count;
    return 1;
}

static int encode(char* buffer, uint32_t code)
{
    if (code < 128) { buffer[0] = (char)code; return 1; }
    if (code < 0x800) { buffer[0] = (char)(0xC0 | (code >> 6)); buffer[1] = (char)(0x80 | (code & 63)); return 2; }
    if (code < 0x10000) {
        buffer[0] = (char)(0xE0 | (code >> 12)); buffer[1] = (char)(0x80 | ((code >> 6) & 63));
        buffer[2] = (char)(0x80 | (code & 63)); return 3;
    }
    buffer[0] = (char)(0xF0 | (code >> 18)); buffer[1] = (char)(0x80 | ((code >> 12) & 63));
    buffer[2] = (char)(0x80 | ((code >> 6) & 63)); buffer[3] = (char)(0x80 | (code & 63)); return 4;
}

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
        for (int i = 0; i < length; ++i) { writeCharacter(buffer, i, width, (unsigned char)name[i]); }
    } else {
        writeCharacter(buffer, 0, width, '\''); writeCharacter(buffer, 1, width, code); writeCharacter(buffer, 2, width, '\'');
    }
    writeCharacter(buffer, length, width, 0);
    return length;
}

int __ada_wide_character_value(const void* text, int length, int width, int maximum)
{
    int start = 0, stop = length;
    while (start < stop && readCharacter(text, start, width) == ' ') { ++start; }
    while (stop > start && readCharacter(text, stop - 1, width) == ' ') { --stop; }
    if (stop - start == 3 && readCharacter(text, start, width) == '\'' && readCharacter(text, stop - 1, width) == '\'') {
        uint32_t value = readCharacter(text, start + 1, width);
        if (value <= (uint32_t)maximum && (adaUnicodeProperties(value) & 32) && (value & 0xFFFF) < 0xFFFE) { return (int)value; }
    }
    char name[32];
    if (stop - start < (int)sizeof name) {
        int count = stop - start;
        for (int i = 0; i < count; ++i) {
            uint32_t code = readCharacter(text, start + i, width);
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
        if (!decode(text, length, &position, &code)) { __ada_raise(ADA_CONSTRAINT_ERROR); return 0; }
        if (width == 2 && code > 65535) {
            char escape[16];
            snprintf(escape, sizeof escape, "[\"%08X\"]", code);
            if (capacity - count < 12) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
            for (int i = 0; i < 12; ++i) { writeCharacter(destination, count++, width, (unsigned char)escape[i]); }
            continue;
        }
        if (count >= capacity) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
        writeCharacter(destination, count++, width, code);
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
        uint32_t code = readCharacter(text, i, width);
        if (code > 0x10FFFF || (code >= 0xD800 && code <= 0xDFFF)) { free(bytes); __ada_raise(ADA_CONSTRAINT_ERROR); return; }
        count += encode(bytes + count, code);
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
            if (!decode(names[i], (int)bytes, &position, &code)) { __ada_raise(ADA_CONSTRAINT_ERROR); return 0; }
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
        if (!decode(cursor->text, cursor->length, &cursor->position, &code)) {
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
            if (!decode(left, leftLength, &position, &code)) { code = (unsigned char)left[position++]; }
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
