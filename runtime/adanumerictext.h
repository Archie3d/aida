/* Internal exact numeric text primitives shared by fixed and floating input. */
#ifndef ADA_NUMERIC_TEXT_H
#define ADA_NUMERIC_TEXT_H

#include "adart.h"
#include <stdlib.h>
#include <string.h>

/* Input can contain arbitrarily many significant digits. Little-endian limbs
   retain exact magnitudes until the caller rounds to its target format. */
typedef struct BigNatural
{
    uint32_t* words;
    size_t size;
    size_t capacity;
} BigNatural;

static int reserve(BigNatural* number, size_t size)
{
    if (size > number->capacity) {
        if (size > SIZE_MAX / sizeof(uint32_t) / 2) {
            __ada_raise(ADA_STORAGE_ERROR);
            return 0;
        }
        size_t capacity = size * 2;
        void* words = realloc(number->words, capacity * sizeof(uint32_t));
        if (words == NULL) {
            __ada_raise(ADA_STORAGE_ERROR);
            return 0;
        }
        number->words = words;
        number->capacity = capacity;
    }
    return 1;
}

static int multiplyAdd(BigNatural* number, uint64_t multiplier, uint32_t digit)
{
    if (!reserve(number, number->size + 3)) {
        return 0;
    }
    unsigned __int128 carry = digit;
    for (size_t i = 0; i < number->size; ++i) {
        carry += (unsigned __int128)number->words[i] * multiplier;
        number->words[i] = (uint32_t)carry;
        carry >>= 32;
    }
    while (carry != 0) {
        number->words[number->size++] = (uint32_t)carry;
        carry >>= 32;
    }
    while (number->size != 0 && number->words[number->size - 1] == 0) { --number->size; }
    return 1;
}

static int compare(const BigNatural* left, const BigNatural* right)
{
    if (left->size != right->size) {
        return left->size < right->size ? -1 : 1;
    }
    for (size_t i = left->size; i != 0; --i) {
        if (left->words[i - 1] != right->words[i - 1]) {
            return left->words[i - 1] < right->words[i - 1] ? -1 : 1;
        }
    }
    return 0;
}

static int times(BigNatural* result, const BigNatural* source, uint64_t factor)
{
    if (!reserve(result, source->size + 3)) {
        return 0;
    }
    memcpy(result->words, source->words, source->size * sizeof(uint32_t));
    result->size = source->size;
    return multiplyAdd(result, factor, 0);
}

static int digitValue(int c)
{
    if (c >= '0' && c <= '9') { return c - '0'; }
    if (c >= 'a' && c <= 'f') { return c - 'a' + 10; }
    if (c >= 'A' && c <= 'F') { return c - 'A' + 10; }
    return -1;
}

/* Digit groups require an underscore to be surrounded by digits. */
static int numeral(const char* text, int length, int* cursor, int base, BigNatural* number, int* count)
{
    int previousDigit = 0;
    *count = 0;
    while (*cursor < length) {
        int digit = digitValue((unsigned char)text[*cursor]);
        if (digit >= 0 && digit < base) {
            if (!multiplyAdd(number, (uint64_t)base, (uint32_t)digit)) { return 0; }
            ++*count;
            ++*cursor;
            previousDigit = 1;
        } else if (text[*cursor] == '_') {
            if (!previousDigit) { return 0; }
            previousDigit = 0;
            ++*cursor;
        } else {
            break;
        }
    }
    return *count == 0 || previousDigit;
}

#endif
