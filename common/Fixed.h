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

/* Ordinary fixed point uses signed 64-bit counts and Small = 2**(-bits).
   Supported scales have 0..30 fractional bits. Round nearest, ties away
   from zero, using checked wide intermediates in both compiler and runtime. */
static inline int fixedRound(__int128 numerator, __int128 denominator, long long* result)
{
    if (denominator == 0) {
        return 0;
    }
    if (denominator < 0) {
        if (__builtin_sub_overflow((__int128)0, numerator, &numerator)
            || __builtin_sub_overflow((__int128)0, denominator, &denominator)) {
            return 0;
        }
    }
    __int128 quotient = numerator / denominator;
    __int128 remainder = numerator % denominator;
    if (remainder < 0) {
        remainder = -remainder;
    }
    if (remainder >= denominator / 2 + denominator % 2) {
        quotient += numerator < 0 ? -1 : 1;
    }
    if (quotient < LLONG_MIN || quotient > LLONG_MAX) {
        return 0;
    }
    *result = (long long)quotient;
    return 1;
}

static inline int fixedRescale(long long value, int from, int to, long long* result)
{
    __int128 numerator = value;
    __int128 denominator = 1;
    if (to >= from) {
        numerator *= (__int128)1 << (to - from);
    } else {
        denominator <<= from - to;
    }
    return fixedRound(numerator, denominator, result);
}

static inline int fixedOperation(FixedOperation operation, long long left, int leftBits,
                                long long right, int rightBits, int resultBits, long long* result)
{
    __int128 numerator;
    __int128 denominator = 1;
    int shift = 0;
    switch (operation) {
    case FixedAdd:
        numerator = (__int128)left + right;
        break;
    case FixedSubtract:
        numerator = (__int128)left - right;
        break;
    case FixedMultiply:
        numerator = (__int128)left * right;
        shift = resultBits - leftBits - rightBits;
        break;
    case FixedDivide:
        numerator = left;
        denominator = right;
        shift = resultBits + rightBits - leftBits;
        break;
    default:
        return 0;
    }
    if (shift >= 0) {
        if (__builtin_mul_overflow(numerator, (__int128)1 << shift, &numerator)) {
            return 0;
        }
    } else if (__builtin_mul_overflow(denominator, (__int128)1 << -shift, &denominator)) {
        return 0;
    }
    return fixedRound(numerator, denominator, result);
}

#endif
