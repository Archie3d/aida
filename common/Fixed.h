#ifndef ADA_FIXED_H
#define ADA_FIXED_H

#include <limits.h>

/* Values are part of the compiler/runtime ABI and match the arithmetic
   portion of ModularOperation. */
typedef enum FixedOperation
{
    FixedInvalid = -1,
    FixedAdd = 0,
    FixedSubtract = 1,
    FixedMultiply = 2,
    FixedDivide = 3
} FixedOperation;

/* Binary Small = 2**(-bits), with -62..62 fractional bits and signed
   64-bit counts. Quotient/remainder scaling avoids overflowing a temporary
   when the rounded result fits. Round nearest, ties away from zero. */
#define ADA_FIXED_MIN_BITS (-62)
#define ADA_FIXED_MAX_BITS 62

static inline int fixedRatioMode(__int128 numerator, __int128 denominator, int shift, int isUnsigned, long long* result)
{
    if (denominator == 0) { return 0; }
    int negative = (numerator < 0) != (denominator < 0);
    unsigned __int128 n = numerator < 0 ? 0 - (unsigned __int128)numerator : (unsigned __int128)numerator;
    unsigned __int128 d = denominator < 0 ? 0 - (unsigned __int128)denominator : (unsigned __int128)denominator;
    unsigned __int128 limit = isUnsigned ? (unsigned long long)-1 : (unsigned __int128)LLONG_MAX + negative;
    if (shift < 0) {
        unsigned distance = (unsigned)-shift;
        unsigned __int128 maximum = ~(unsigned __int128)0;
        if (distance >= 128 || d > (maximum >> distance)) {
            unsigned __int128 rounded = 0;
            if (distance <= 128 && d <= (maximum >> (distance - 1))) {
                rounded = n >= (d << (distance - 1));
            }
            if (isUnsigned && negative && rounded != 0) { return 0; }
            *result = negative ? -(long long)rounded : (long long)rounded;
            return 1;
        }
        d <<= distance;
        shift = 0;
    }
    unsigned __int128 quotient = n / d;
    unsigned __int128 remainder = n % d;
    if (quotient > limit) { return 0; }
    while (shift-- > 0) {
        remainder *= 2;
        quotient = quotient * 2 + (remainder >= d);
        if (remainder >= d) { remainder -= d; }
        if (quotient > limit) { return 0; }
    }
    if (remainder >= d / 2 + d % 2) { ++quotient; }
    if (quotient > limit || (isUnsigned && negative && quotient != 0)) { return 0; }
    *result = negative ? (quotient == (unsigned __int128)LLONG_MAX + 1 ? LLONG_MIN : -(long long)quotient)
                       : (long long)quotient;
    return 1;
}

static inline int fixedRatio(__int128 numerator, __int128 denominator, int shift, long long* result)
{
    return fixedRatioMode(numerator, denominator, shift, 0, result);
}

static inline int fixedRound(__int128 numerator, __int128 denominator, long long* result)
{
    return fixedRatio(numerator, denominator, 0, result);
}

static inline int fixedRescale(long long value, int from, int to, long long* result)
{
    if (from < ADA_FIXED_MIN_BITS || from > ADA_FIXED_MAX_BITS
        || to < ADA_FIXED_MIN_BITS || to > ADA_FIXED_MAX_BITS) { return 0; }
    return fixedRatio(value, 1, to - from, result);
}

static inline int fixedOperation(FixedOperation operation, long long left, int leftBits,
                                long long right, int rightBits, int resultBits, long long* result)
{
    if (leftBits < ADA_FIXED_MIN_BITS || leftBits > ADA_FIXED_MAX_BITS
        || rightBits < ADA_FIXED_MIN_BITS || rightBits > ADA_FIXED_MAX_BITS
        || resultBits < ADA_FIXED_MIN_BITS || resultBits > ADA_FIXED_MAX_BITS) { return 0; }
    switch (operation) {
    case FixedAdd: return fixedRound((__int128)left + right, 1, result);
    case FixedSubtract: return fixedRound((__int128)left - right, 1, result);
    case FixedMultiply:
        return fixedRatio((__int128)left * right, 1, resultBits - leftBits - rightBits, result);
    case FixedDivide:
        return fixedRatio(left, right, resultBits + rightBits - leftBits, result);
    default: return 0;
    }
}

#endif
