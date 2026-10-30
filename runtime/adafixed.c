/* Exact binary and decimal fixed-point text conversion. No binary floating conversion
   is used for values; the scale tag identifies the exact Small. */
#include "adart.h"
#include "../common/Fixed.h"
#include "adaio.h"
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "adanumerictext.h"

static int parseFixed(const char* text, int length, int bits, long long low, long long high,
                      int partial, int* last, long long* result)
{
    if (!fixedValidScale(bits)) { return 0; }
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
        if (numerator.size != 1 || numerator.words[0] < 2 || numerator.words[0] > 16) { goto done; }
        base = (int)numerator.words[0];
        numerator.size = 0;
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
    if (numerator.size == 0) {
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
    int twos = fixedTwos(bits), fives = fixedFives(bits);
    for (int i = 0; i < (twos < 0 ? -twos : twos); ++i) {
        if (!multiplyAdd(twos < 0 ? &denominator : &numerator, 2, 0)) { goto done; }
    }
    for (int i = 0; i < (fives < 0 ? -fives : fives); ++i) {
        if (!multiplyAdd(fives < 0 ? &denominator : &numerator, 5, 0)) { goto done; }
    }
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
    if (!reserve(&product, denominator.size + 4)) { goto done; }
    uint64_t carry = 0;
    size_t size = product.size > denominator.size ? product.size : denominator.size;
    if (!reserve(&product, size + 1)) { goto done; }
    for (size_t i = 0; i < size; ++i) {
        carry += (i < product.size ? product.words[i] : 0)
            + (uint64_t)(i < denominator.size ? denominator.words[i] : 0);
        product.words[i] = (uint32_t)carry;
        carry >>= 32;
    }
    product.size = size;
    if (carry != 0) { product.words[product.size++] = (uint32_t)carry; }
    if (compare(&numerator, &product) >= 0) { ++lower; }
    if (lower > (UINT64_C(1) << 63) || (!negative && lower > LLONG_MAX)) { goto done; }
    *result = negative ? (lower == (UINT64_C(1) << 63) ? LLONG_MIN : -(long long)lower) : (long long)lower;
    ok = *result >= low && *result <= high;
done:
    free(numerator.words);
    free(denominator.words);
    free(product.words);
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
    for (int bits = ADA_FIXED_MIN_BITS; bits <= ADA_FIXED_MAX_BITS; ++bits) {
        if (small == ldexp(1.0, -bits)) { return bits; }
    }
    __ada_raise(ADA_CONSTRAINT_ERROR);
    return 0;
}

/* Produce exact terminating decimal digits (at most 38+62), then round in
   decimal. Requested padding never changes which digits are significant. */
static char* formatFixed(long long value, int bits, int fore, int aft, int exponent, int image,
                         char* target, int capacity)
{
    if (!fixedValidScale(bits)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return NULL;
    }
    char digits[160], rounded[160], exponentDigits[24];
    unsigned __int128 magnitude = value < 0 ? (uint64_t)(-(value + 1)) + 1 : (uint64_t)value;
    int scale = fixedTwos(bits);
    unsigned __int128 factor = fixedIsDecimal(bits) ? (unsigned __int128)fixedPower10(scale < 0 ? -scale : scale)
        : (unsigned __int128)1 << (scale < 0 ? -scale : scale);
    unsigned __int128 divisor = scale >= 0 ? factor : 1;
    if (scale < 0) { magnitude *= factor; }
    unsigned __int128 whole = magnitude / divisor;
    char reverse[64];
    int integerDigits = 0;
    do {
        reverse[integerDigits++] = (char)('0' + whole % 10);
        whole /= 10;
    } while (whole != 0);
    for (int i = 0; i < integerDigits; ++i) { digits[i] = reverse[integerDigits - i - 1]; }
    int count = integerDigits;
    unsigned __int128 remainder = magnitude % divisor;
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
    if (target != NULL && length >= capacity) { __ada_raise(ADA_STORAGE_ERROR); return NULL; }
    char* result = target != NULL ? target : malloc((size_t)length + 1);
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

int __ada_image_fixed(char* buffer, int capacity, long long value, int bits, int aft)
{
    if (capacity <= 0) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
    char* text = formatFixed(value, bits, 0, aft, 0, 1, buffer, capacity);
    return text != NULL ? (int)strlen(text) : 0;
}

static void fixedPut(AdaFileRef handle, long long value, int bits, int fore, int aft, int exponent)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file == NULL) { return; }
    char* text = formatFixed(value, bits, fore, aft, exponent, 0, NULL, 0);
    if (text != NULL) {
        __ada_text_write_field(file, text, (int)strlen(text));
        free(text);
    }
}

void __ada_fixed_put_string(char* to, int length, long long value, int bits, int aft, int exponent)
{
    char* text = formatFixed(value, bits, 0, aft, exponent, 0, NULL, 0);
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

/* Fixed, decimal, integer and floating input share token boundaries. */
static long long fixedGet(AdaFileRef handle, int bits, long long low, long long high, int width)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    if (file == NULL) { return 0; }
    int length, last = 0;
    char* text = __ada_io_read_token(file, width, 1, &length);
    long long result = 0;
    if (__ada_exception == NULL && !parseFixed(text, length, bits, low, high, 0, &last, &result)
        && __ada_exception == NULL) {
        __ada_raise(ADA_DATA_ERROR);
    }
    free(text);
    return result;
}

void __ada_fixed_put(AdaFileRef handle, long long value, int bits, int fore, int aft, int exponent)
{
    __ada_io_begin(handle);
    __ada_io_unlock();
    fixedPut(handle, value, bits, fore, aft, exponent);
    __ada_io_lock();
    __ada_io_end();
}

long long __ada_fixed_get(AdaFileRef handle, int bits, long long low, long long high, int width)
{
    __ada_io_begin(handle);
    __ada_io_unlock();
    long long result = fixedGet(handle, bits, low, high, width);
    __ada_io_lock();
    __ada_io_end();
    return result;
}
