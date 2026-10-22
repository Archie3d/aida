/* Ada real 'Value: exact rational parsing followed by one binary rounding.
   No locale, C floating grammar, or intermediate host precision is involved. */
#include "adanumerictext.h"
#include <math.h>

static int shift(BigNatural* number, long long bits)
{
    while (bits >= 32) {
        if (!multiplyAdd(number, UINT64_C(1) << 32, 0)) { return 0; }
        bits -= 32;
    }
    return multiplyAdd(number, UINT64_C(1) << bits, 0);
}

static long long bitLength(const BigNatural* number)
{
    uint32_t top = number->words[number->size - 1];
    long long bits = (long long)(number->size - 1) * 32;
    while (top != 0) { ++bits; top >>= 1; }
    return bits;
}

static int roundRealRatio(BigNatural* numerator, BigNatural* denominator, int negative, int precision, double* result)
{
    BigNatural product = {0};
    int ok = 0;
    int minStep = precision == 24 ? -149 : -1074;
    int maxExponent = precision == 24 ? 127 : 1023;
    long long binaryExponent = bitLength(numerator) - bitLength(denominator);
    /* Bit lengths give floor(log2(N/D)) or one above it. */
    if (binaryExponent > maxExponent + 1) { goto done; }
    if (binaryExponent < minStep - 1) { *result = negative ? -0.0 : 0.0; ok = 1; goto done; }
    if (binaryExponent >= 0) {
        if (!times(&product, denominator, 1) || !shift(&product, binaryExponent)) { goto done; }
        if (compare(numerator, &product) < 0) { --binaryExponent; }
    } else {
        if (!times(&product, numerator, 1) || !shift(&product, -binaryExponent)) { goto done; }
        if (compare(&product, denominator) < 0) { --binaryExponent; }
    }
    if (binaryExponent > maxExponent) { goto done; }
    long long step = binaryExponent - (precision - 1);
    if (step < minStep) { step = minStep; }
    if (!shift(step < 0 ? numerator : denominator, step < 0 ? -step : step)) { goto done; }
    uint64_t lower = 0, upper = UINT64_C(1) << precision;
    while (lower < upper) {
        uint64_t middle = lower + (upper - lower + 1) / 2;
        if (!times(&product, denominator, middle)) { goto done; }
        if (compare(&product, numerator) <= 0) { lower = middle; }
        else { upper = middle - 1; }
    }
    if (!multiplyAdd(numerator, 2, 0) || !times(&product, denominator, 2 * lower + 1)) { goto done; }
    int halfway = compare(numerator, &product);
    if (halfway > 0 || (halfway == 0 && (lower & 1))) { ++lower; }
    if (binaryExponent == maxExponent && lower == (UINT64_C(1) << precision)) { goto done; }
    *result = ldexp((double)lower, (int)step);
    if (negative) { *result = -*result; }
    ok = 1;
done:
    free(product.words);
    return ok;
}

static int parseReal(const char* text, int length, int precision, double* result)
{
    BigNatural numerator = {0}, denominator = {0};
    int ok = 0, cursor = 0, negative = 0, base = 10, before = 0, after = 0;
    long long exponent = 0;
    int exponentNegative = 0;
    /* Only Ada spaces are ignored; tabs and other format effectors are not
       spaces in the attribute's grammar (RM 3.5). */
    while (cursor < length && text[cursor] == ' ') { ++cursor; }
    if (cursor < length && (text[cursor] == '+' || text[cursor] == '-')) {
        negative = text[cursor++] == '-';
    }
    if (!numeral(text, length, &cursor, 10, &numerator, &before)) { goto done; }
    int based = cursor < length && (text[cursor] == '#' || text[cursor] == ':');
    char delimiter = based ? text[cursor] : 0;
    if (based) {
        if (before == 0 || numerator.size != 1 || numerator.words[0] < 2 || numerator.words[0] > 16) { goto done; }
        base = (int)numerator.words[0];
        numerator.size = 0;
        ++cursor;
        if (!numeral(text, length, &cursor, base, &numerator, &before)) { goto done; }
    }
    if (cursor < length && text[cursor] == '.') {
        ++cursor;
        if (!numeral(text, length, &cursor, base, &numerator, &after)) { goto done; }
    }
    if (before == 0 && after == 0) { goto done; }
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
                /* Saturate only beyond all possible mantissa cancellation
                   plus the binary64 range, and still validate every digit. */
                if (exponent <= (long long)length * 4 + 2048) { exponent = exponent * 10 + c - '0'; }
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
    while (cursor < length && text[cursor] == ' ') { ++cursor; }
    if (cursor != length) { goto done; }
    if (numerator.size == 0) { *result = negative ? -0.0 : 0.0; ok = 1; goto done; }
    if (exponent > (long long)length * 4 + 2048) {
        if (exponentNegative) { *result = negative ? -0.0 : 0.0; ok = 1; }
        goto done;
    }
    long long power = (exponentNegative ? -exponent : exponent) - after;
    if (!multiplyAdd(&denominator, 1, 1)) { goto done; }
    for (long long i = 0; i < (power < 0 ? -power : power); ++i) {
        if (!multiplyAdd(power < 0 ? &denominator : &numerator, (uint64_t)base, 0)) { goto done; }
    }
    ok = roundRealRatio(&numerator, &denominator, negative, precision, result);
done:
    free(numerator.words);
    free(denominator.words);
    return ok;
}

float __ada_value_real_f32(const char* text, int length)
{
    double result = 0.0;
    if (!parseReal(text, length, 24, &result) && __ada_exception == NULL) { __ada_raise(ADA_CONSTRAINT_ERROR); }
    return (float)result;
}

double __ada_value_real(const char* text, int length)
{
    double result = 0.0;
    if (!parseReal(text, length, 53, &result) && __ada_exception == NULL) { __ada_raise(ADA_CONSTRAINT_ERROR); }
    return result;
}

/* The input counts and binary scales describe an exact rational, independent
   of the destination floating format. Round it directly to that format. */
double __ada_fixed_real_operation(int operation, long long left, int leftBits,
                                  long long right, int rightBits, int precision)
{
    if ((operation != 2 && operation != 3) || (precision != 24 && precision != 53)
        || leftBits < ADA_FIXED_MIN_BITS || leftBits > ADA_FIXED_MAX_BITS
        || rightBits < ADA_FIXED_MIN_BITS || rightBits > ADA_FIXED_MAX_BITS) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    __int128 n = operation == 2 ? (__int128)left * right : left;
    __int128 d = operation == 2 ? 1 : right;
    int power = operation == 2 ? -leftBits - rightBits : rightBits - leftBits;
    int negative = (n < 0) != (d < 0);
    unsigned __int128 a = n < 0 ? 0 - (unsigned __int128)n : (unsigned __int128)n;
    unsigned __int128 b = d < 0 ? 0 - (unsigned __int128)d : (unsigned __int128)d;
    BigNatural numerator = {0}, denominator = {0};
    double result = 0;
    int ok = d != 0;
    for (int limb = 3; limb >= 0 && ok; --limb) {
        ok = multiplyAdd(&numerator, UINT64_C(1) << 32, (uint32_t)(a >> (limb * 32)))
            && multiplyAdd(&denominator, UINT64_C(1) << 32, (uint32_t)(b >> (limb * 32)));
    }
    if (ok && numerator.size == 0) {
        result = negative ? -0.0 : 0.0;
    } else if (ok) {
        ok = shift(power >= 0 ? &numerator : &denominator, power >= 0 ? power : -power)
            && roundRealRatio(&numerator, &denominator, negative, precision, &result);
    }
    free(numerator.words);
    free(denominator.words);
    if (!ok && __ada_exception == NULL) { __ada_raise(ADA_CONSTRAINT_ERROR); }
    return result;
}
