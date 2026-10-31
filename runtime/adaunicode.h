/* Native text cells and strict UTF-8 codecs shared by the runtime. */
#ifndef ADA_UNICODE_H
#define ADA_UNICODE_H
#include <stdint.h>
#include <string.h>

static inline uint32_t adaTextRead(const void* text, int index, int width)
{
    if (width == 1) { return ((const unsigned char*)text)[index]; }
    if (width == 2) { uint16_t value; memcpy(&value, (const char*)text + (size_t)index * 2, 2); return value; }
    uint32_t value; memcpy(&value, (const char*)text + (size_t)index * 4, 4); return value;
}

static inline void adaTextWrite(void* text, int index, int width, uint32_t value)
{
    if (width == 1) { ((unsigned char*)text)[index] = (unsigned char)value; }
    else if (width == 2) { uint16_t cell = (uint16_t)value; memcpy((char*)text + (size_t)index * 2, &cell, 2); }
    else { memcpy((char*)text + (size_t)index * 4, &value, 4); }
}

static inline int adaUtf8Decode(const char* text, int length, int* position, uint32_t* value)
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

static inline int adaUtf8Encode(char* buffer, uint32_t code)
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

#endif
