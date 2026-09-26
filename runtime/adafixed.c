/* Exact ordinary fixed-point text conversion. No binary floating conversion
   is used for values; Small is an exact power of two in the generic ABI. */
#include "adart.h"
#include "adaio.h"
#include <ctype.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

/* Input can contain arbitrarily many significant digits. Little-endian limbs
   retain the exact rational until the single rounding to the target Small. */
typedef struct BigNatural
{
    uint32_t* m_words;
    size_t m_size;
    size_t m_capacity;
} BigNatural;

static int reserve(BigNatural* number, size_t size)
{
    if (size > number->m_capacity) {
        if (size > SIZE_MAX / sizeof(uint32_t) / 2) {
            __ada_raise(ADA_STORAGE_ERROR);
            return 0;
        }
        size_t capacity = size * 2;
        void* words = realloc(number->m_words, capacity * sizeof(uint32_t));
        if (words == NULL) {
            __ada_raise(ADA_STORAGE_ERROR);
            return 0;
        }
        number->m_words = words;
        number->m_capacity = capacity;
    }
    return 1;
}

static int multiplyAdd(BigNatural* number, uint64_t multiplier, uint32_t digit)
{
    if (!reserve(number, number->m_size + 3)) {
        return 0;
    }
    unsigned __int128 carry = digit;
    for (size_t i = 0; i < number->m_size; ++i) {
        carry += (unsigned __int128)number->m_words[i] * multiplier;
        number->m_words[i] = (uint32_t)carry;
        carry >>= 32;
    }
    while (carry != 0) {
        number->m_words[number->m_size++] = (uint32_t)carry;
        carry >>= 32;
    }
    while (number->m_size != 0 && number->m_words[number->m_size - 1] == 0) { --number->m_size; }
    return 1;
}

static int compare(const BigNatural* left, const BigNatural* right)
{
    if (left->m_size != right->m_size) {
        return left->m_size < right->m_size ? -1 : 1;
    }
    for (size_t i = left->m_size; i != 0; --i) {
        if (left->m_words[i - 1] != right->m_words[i - 1]) {
            return left->m_words[i - 1] < right->m_words[i - 1] ? -1 : 1;
        }
    }
    return 0;
}

static int times(BigNatural* result, const BigNatural* source, uint64_t factor)
{
    if (!reserve(result, source->m_size + 3)) {
        return 0;
    }
    memcpy(result->m_words, source->m_words, source->m_size * sizeof(uint32_t));
    result->m_size = source->m_size;
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

static int parseFixed(const char* text, int length, int bits, long long low, long long high,
                      int partial, int* last, long long* result)
{
    BigNatural numerator = {0}, denominator = {0}, product = {0};
    int ok = 0, cursor = 0, negative = 0, base = 10, before = 0, after = 0;
    int exponentNegative = 0;
    long long exponent = 0;
    while (cursor < length && (text[cursor] == ' ' || text[cursor] == '\t' || text[cursor] == '\r' || text[cursor] == '\n')) { ++cursor; }
    if (cursor < length && (text[cursor] == '+' || text[cursor] == '-')) {
        negative = text[cursor++] == '-';
    }
    if (!numeral(text, length, &cursor, 10, &numerator, &before)) { goto done; }
    int based = cursor < length && (text[cursor] == '#' || text[cursor] == ':');
    char delimiter = based ? text[cursor] : 0;
    if (based) {
        if (numerator.m_size != 1 || numerator.m_words[0] < 2 || numerator.m_words[0] > 16) { goto done; }
        base = (int)numerator.m_words[0];
        numerator.m_size = 0;
        ++cursor;
        if (!numeral(text, length, &cursor, base, &numerator, &before)) { goto done; }
    }
    if (cursor < length && text[cursor] == '.') {
        ++cursor;
        if (!numeral(text, length, &cursor, base, &numerator, &after)) { goto done; }
    } else if (before == 0) {
        goto done;
    }
    if (before + after == 0) { goto done; }
    if (based && (cursor >= length || text[cursor++] != delimiter)) { goto done; }
    if (cursor < length && (text[cursor] == 'e' || text[cursor] == 'E')) {
        ++cursor;
        if (cursor < length && (text[cursor] == '+' || text[cursor] == '-')) {
            exponentNegative = text[cursor++] == '-';
        }
        int digits = 0, previousDigit = 0;
        while (cursor < length) {
            int c = text[cursor];
            if (c >= '0' && c <= '9') {
                /* Beyond this bound any nonzero mantissa must underflow or
                   overflow. Continue scanning so malformed exponents fail. */
                if (exponent <= (long long)length * 4 + 128) { exponent = exponent * 10 + c - '0'; }
                ++digits;
                previousDigit = 1;
                ++cursor;
            } else if (c == '_') {
                if (!previousDigit) { goto done; }
                previousDigit = 0;
                ++cursor;
            } else { break; }
        }
        if (digits == 0 || !previousDigit) { goto done; }
    }
    *last = cursor;
    if (!partial) {
        while (cursor < length && isspace((unsigned char)text[cursor])) { ++cursor; }
        if (cursor != length) { goto done; }
    }
    if (numerator.m_size == 0) {
        *result = 0;
        ok = low <= 0 && high >= 0;
        goto done;
    }
    if (exponent > (long long)length * 4 + 128) {
        if (exponentNegative) {
            *result = 0;
            ok = low <= 0 && high >= 0;
        }
        goto done;
    }
    long long power = (exponentNegative ? -exponent : exponent) - after;
    if (!multiplyAdd(&denominator, 1, 1)) { goto done; }
    for (long long i = 0; i < (power < 0 ? -power : power); ++i) {
        if (!multiplyAdd(power < 0 ? &denominator : &numerator, (uint64_t)base, 0)) { goto done; }
    }
    if (!multiplyAdd(&numerator, UINT64_C(1) << bits, 0)) { goto done; }
    /* Find floor(numerator / denominator), with a sentinel just above the
       largest representable magnitude. */
    uint64_t lower = 0, upper = (UINT64_C(1) << 63) + 1;
    while (lower < upper) {
        uint64_t middle = lower + (upper - lower + 1) / 2;
        if (!times(&product, &denominator, middle)) { goto done; }
        if (compare(&product, &numerator) <= 0) { lower = middle; }
        else { upper = middle - 1; }
    }
    if (lower > (UINT64_C(1) << 63)) { goto done; }
    /* Compare 2*N with (2*floor + 1)*D without overflowing a 64-bit factor. */
    if (!times(&product, &denominator, lower) || !multiplyAdd(&product, 2, 0)
        || !multiplyAdd(&numerator, 2, 0)) { goto done; }
    /* Add D to the product using wide limb carries. */
    if (!reserve(&product, denominator.m_size + 4)) { goto done; }
    uint64_t carry = 0;
    size_t size = product.m_size > denominator.m_size ? product.m_size : denominator.m_size;
    if (!reserve(&product, size + 1)) { goto done; }
    for (size_t i = 0; i < size; ++i) {
        carry += (i < product.m_size ? product.m_words[i] : 0)
            + (uint64_t)(i < denominator.m_size ? denominator.m_words[i] : 0);
        product.m_words[i] = (uint32_t)carry;
        carry >>= 32;
    }
    product.m_size = size;
    if (carry != 0) { product.m_words[product.m_size++] = (uint32_t)carry; }
    if (compare(&numerator, &product) >= 0) { ++lower; }
    if (lower > (UINT64_C(1) << 63) || (!negative && lower > LLONG_MAX)) { goto done; }
    *result = negative ? (lower == (UINT64_C(1) << 63) ? LLONG_MIN : -(long long)lower) : (long long)lower;
    ok = *result >= low && *result <= high;
done:
    free(numerator.m_words);
    free(denominator.m_words);
    free(product.m_words);
    return ok;
}

long long __ada_value_fixed(const char* text, int length, int bits, long long low, long long high)
{
    int last = 0;
    long long result = 0;
    if (!parseFixed(text, length, bits, low, high, 0, &last, &result) && __ada_exception == NULL) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    }
    return result;
}

int __ada_fixed_scale(double small)
{
    int bits = 0;
    while (small < 1.0 && bits < 30) { small *= 2.0; ++bits; }
    return bits;
}

/* Produce exact terminating decimal digits (at most 19+30), then round in
   decimal. Requested padding never changes which digits are significant. */
static char* formatFixed(long long value, int bits, int fore, int aft, int exponent, int image)
{
    char digits[80], rounded[80], exponentDigits[24];
    uint64_t magnitude = value < 0 ? (uint64_t)(-(value + 1)) + 1 : (uint64_t)value;
    uint64_t divisor = UINT64_C(1) << bits;
    int integerDigits = snprintf(digits, sizeof digits, "%llu", (unsigned long long)(magnitude / divisor));
    int count = integerDigits;
    uint64_t remainder = magnitude % divisor;
    while (remainder != 0) {
        remainder *= 10;
        digits[count++] = (char)('0' + remainder / divisor);
        remainder %= divisor;
    }
    int start = 0, power = 0, before = integerDigits;
    if (aft < 1) { aft = 1; }
    if (exponent != 0 && magnitude != 0) {
        while (start < count - 1 && digits[start] == '0') { ++start; }
        power = integerDigits - start - 1;
        before = 1;
    }
    long long keep = (long long)before + aft;
    int available = count - start;
    int used = available < keep ? available : (int)keep;
    memcpy(rounded, digits + start, (size_t)used);
    if (keep < available && digits[start + (int)keep] >= '5') {
        int cursor = used - 1;
        while (cursor >= 0 && rounded[cursor] == '9') { rounded[cursor--] = '0'; }
        if (cursor >= 0) { ++rounded[cursor]; }
        else {
            memmove(rounded + 1, rounded, (size_t)used);
            rounded[0] = '1';
            ++used;
            if (exponent != 0) { ++power; --used; }
            else { ++before; ++keep; }
        }
    }
    int sign = value < 0 || image;
    long long padding = fore > before + sign ? (long long)fore - before - sign : 0;
    int exponentCount = 0;
    long long exponentPadding = 0;
    if (exponent != 0) {
        exponentCount = snprintf(exponentDigits, sizeof exponentDigits, "%d", power < 0 ? -power : power);
        exponentPadding = exponent > exponentCount + 1 ? (long long)exponent - exponentCount - 1 : 0;
    }
    long long length = padding + sign + before + 1LL + aft
        + (exponent != 0 ? 2 + exponentPadding + exponentCount : 0);
    if (length >= INT_MAX) { __ada_raise(ADA_LAYOUT_ERROR); return NULL; }
    char* result = malloc((size_t)length + 1);
    if (result == NULL) { __ada_raise(ADA_STORAGE_ERROR); return NULL; }
    size_t cursor = 0;
    memset(result, ' ', (size_t)padding);
    cursor += (size_t)padding;
    if (sign) { result[cursor++] = value < 0 ? '-' : ' '; }
    for (int i = 0; i < before; ++i) { result[cursor++] = i < used ? rounded[i] : '0'; }
    result[cursor++] = '.';
    for (int i = 0; i < aft; ++i) { result[cursor++] = before + (long long)i < used ? rounded[before + i] : '0'; }
    if (exponent != 0) {
        result[cursor++] = 'E';
        result[cursor++] = power < 0 ? '-' : '+';
        memset(result + cursor, '0', (size_t)exponentPadding);
        cursor += (size_t)exponentPadding;
        memcpy(result + cursor, exponentDigits, (size_t)exponentCount);
        cursor += (size_t)exponentCount;
    }
    result[cursor] = '\0';
    return result;
}

const char* __ada_image_fixed(long long value, int bits, int aft)
{
    static char* buffers[8];
    static unsigned int next;
    unsigned int slot = next++ % 8;
    free(buffers[slot]);
    buffers[slot] = formatFixed(value, bits, 0, aft, 0, 1);
    return buffers[slot] != NULL ? buffers[slot] : "";
}

void __ada_fixed_put(AdaFileRef handle, long long value, int bits, int fore, int aft, int exponent)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file == NULL) { return; }
    char* text = formatFixed(value, bits, fore, aft, exponent, 0);
    if (text != NULL) {
        if (fputs(text, file->stream) == EOF) { __ada_raise(ADA_DEVICE_ERROR); }
        free(text);
    }
}

void __ada_fixed_put_string(char* to, int length, long long value, int bits, int aft, int exponent)
{
    char* text = formatFixed(value, bits, 0, aft, exponent, 0);
    if (text == NULL) { return; }
    size_t size = strlen(text);
    if (size > (size_t)length) { __ada_raise(ADA_LAYOUT_ERROR); }
    else {
        memset(to, ' ', (size_t)length - size);
        memcpy(to + length - size, text, size);
    }
    free(text);
}

long long __ada_fixed_get_string(const char* from, int length, int bits, long long low, long long high, int* last)
{
    long long result = 0;
    if (!parseFixed(from, length, bits, low, high, 1, last, &result) && __ada_exception == NULL) {
        __ada_raise(ADA_DATA_ERROR);
    }
    return result;
}

/* A lexical scanner stops before a character that belongs to the following
   item. Unlike a whitespace-token reader it leaves punctuation and adjacent
   signed numbers in the input. Bad digit groups remain intact for validation. */
static int numericCharacter(int c, int position, int* hashes, int* exponent, int* point, int previous)
{
    if (c >= '0' && c <= '9') { return 1; }
    if (c == '+' || c == '-') { return position == 0 || (*exponent && (previous == 'E' || previous == 'e')); }
    if (c == '_') { return 1; }
    if (c == '.') {
        if (*exponent || *point || *hashes == 2) { return 0; }
        *point = 1;
        return 1;
    }
    if (c == '#' || c == ':') {
        if (*hashes >= 2 || *exponent || (*hashes == 0 && *point)) { return 0; }
        ++*hashes;
        return 1;
    }
    if (*hashes == 1 && digitValue(c) >= 10) { return 1; }
    if ((c == 'e' || c == 'E') && !*exponent) { *exponent = 1; return 1; }
    return 0;
}

long long __ada_fixed_get(AdaFileRef handle, int bits, long long low, long long high, int width)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    if (file == NULL) { return 0; }
    int c = fgetc(file->stream);
    if (width == 0) {
        while (c != EOF && isspace((unsigned char)c)) { c = fgetc(file->stream); }
    }
    if (c == EOF) { __ada_raise(ferror(file->stream) ? ADA_DEVICE_ERROR : ADA_END_ERROR); return 0; }
    char* text = NULL;
    int size = 0, capacity = 0, hashes = 0, exponent = 0, point = 0, previous = 0;
    while (c != EOF) {
        if (width != 0) {
            if (size == width || c == '\n' || c == '\r' || c == '\f') { break; }
        } else if (!numericCharacter(c, size, &hashes, &exponent, &point, previous)) { break; }
        if (size == capacity) {
            int newCapacity = capacity == 0 ? 64 : (capacity <= INT_MAX / 2 ? capacity * 2 : INT_MAX);
            if (newCapacity == capacity) { __ada_raise(ADA_STORAGE_ERROR); break; }
            char* grown = realloc(text, (size_t)newCapacity);
            if (grown == NULL) { __ada_raise(ADA_STORAGE_ERROR); break; }
            text = grown;
            capacity = newCapacity;
        }
        text[size++] = (char)c;
        previous = c;
        c = fgetc(file->stream);
    }
    if (c != EOF) { ungetc(c, file->stream); }
    long long result = 0;
    int last = 0;
    if (__ada_exception == NULL) {
        if (!parseFixed(text, size, bits, low, high, 1, &last, &result)) {
            if (__ada_exception == NULL) { __ada_raise(ADA_DATA_ERROR); }
        }
        else {
            while (last < size && (text[last] == ' ' || text[last] == '\t')) { ++last; }
            if (last != size) { __ada_raise(ADA_DATA_ERROR); }
        }
    }
    free(text);
    return result;
}
