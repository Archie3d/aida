#include <stdint.h>

int8_t aida_abi_signed_byte(int8_t value)
{
    return value == INT8_MIN ? -125 : 0;
}

uint8_t aida_abi_unsigned_byte(uint8_t value)
{
    return value == UINT8_MAX ? 253 : 0;
}

int16_t aida_abi_signed_half(int16_t value)
{
    return value == INT16_MIN ? -32765 : 0;
}

uint16_t aida_abi_unsigned_half(uint16_t value)
{
    return value == UINT16_MAX ? 65533 : 0;
}

int32_t aida_abi_signed_word(int32_t value)
{
    return value == INT32_MIN ? INT32_MIN + 1 : 0;
}

uint32_t aida_abi_unsigned_word(uint32_t value)
{
    return value == UINT32_MAX ? UINT32_MAX - 1 : 0;
}

int64_t aida_abi_signed_quad(int64_t value)
{
    return value == INT64_MIN ? INT64_MIN + 1 : 0;
}

uint64_t aida_abi_unsigned_quad(uint64_t value)
{
    return value == UINT64_MAX ? UINT64_MAX - 1 : 0;
}

uint32_t aida_abi_mixed_words(int32_t a, int32_t b, int32_t c, int32_t d,
                             int32_t e, int32_t f, int32_t g, int32_t h,
                             int8_t signedByte, uint8_t unsignedByte,
                             int16_t signedHalf, uint16_t unsignedHalf, uint32_t word)
{
    return a == 1 && b == 2 && c == 3 && d == 4 && e == 5 && f == 6 && g == 7 && h == 8
        && signedByte == INT8_MIN && unsignedByte == UINT8_MAX && signedHalf == INT16_MIN
        && unsignedHalf == UINT16_MAX && word == UINT32_MAX ? UINT32_C(0x80000000) : 0;
}

int32_t aida_abi_update_layout(uint32_t* words, unsigned char* narrow, uint16_t* wide, uint32_t* widest)
{
    if (words[0] != UINT32_MAX || words[1] != UINT32_C(0x80000000) || words[2] != 17
        || narrow[0] != 0xFF || narrow[1] != 0x98 || wide[0] != 0xFEFF || wide[1] != 0x1234
        || widest[0] != UINT32_C(0x10FFFE) || widest[1] != UINT32_C(0x10000)) {
        return 0;
    }
    words[1] = UINT32_C(0xFEDCBA98);
    narrow[1] = 0xAB;
    wide[1] = 0xFFFE;
    widest[1] = UINT32_C(0x10FFFF);
    return 1;
}
