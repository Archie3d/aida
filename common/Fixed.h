#ifndef ADA_FIXED_H
#define ADA_FIXED_H

#include <limits.h>

/* Values are part of the compiler/runtime ABI. The first four match the
   arithmetic portion of ModularOperation; the last two implement Round. */
typedef enum FixedOperation
{
    FixedInvalid = -1,
    FixedAdd = 0,
    FixedSubtract = 1,
    FixedMultiply = 2,
    FixedDivide = 3,
    FixedMultiplyRounded = 4,
    FixedDivideRounded = 5
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

/* Decimal scales use a tagged exponent in the same scalar ABI. Binary
   exponents retain their original values, including Duration's scale. */
#define ADA_FIXED_DECIMAL_TAG 100
#define ADA_FIXED_MIN_SCALE (-18)
#define ADA_FIXED_MAX_SCALE 18

static inline int fixedIsDecimal(int scale)
{
    return scale >= ADA_FIXED_DECIMAL_TAG + ADA_FIXED_MIN_SCALE
        && scale <= ADA_FIXED_DECIMAL_TAG + ADA_FIXED_MAX_SCALE;
}

static inline int fixedValidScale(int scale)
{
    return (scale >= ADA_FIXED_MIN_BITS && scale <= ADA_FIXED_MAX_BITS) || fixedIsDecimal(scale);
}

static inline int fixedTwos(int scale)
{
    return fixedIsDecimal(scale) ? scale - ADA_FIXED_DECIMAL_TAG : scale;
}

static inline int fixedFives(int scale)
{
    return fixedIsDecimal(scale) ? scale - ADA_FIXED_DECIMAL_TAG : 0;
}

static inline __int128 fixedPower10(int power)
{
    __int128 value = 1;
    while (power-- > 0) { value *= 10; }
    return value;
}

/* At most 128 input bits plus 186 binary scale bits and 54 decimal
   scale digits occur in a supported operation. Fixed-size scratch keeps
   exact arithmetic allocation-free and independent of task state. */
#define ADA_FIXED_LIMBS 20
typedef struct FixedMagnitude
{
    unsigned int words[ADA_FIXED_LIMBS];
} FixedMagnitude;

static inline FixedMagnitude fixedMagnitude(unsigned __int128 value)
{
    FixedMagnitude result = {{0}};
    for (int i = 0; i < 4; ++i) { result.words[i] = (unsigned int)value; value >>= 32; }
    return result;
}

static inline void fixedMagnitudeTimes(FixedMagnitude* value, unsigned long long factor)
{
    unsigned __int128 carry = 0;
    for (int i = 0; i < ADA_FIXED_LIMBS; ++i) {
        carry += (unsigned __int128)value->words[i] * factor;
        value->words[i] = (unsigned int)carry;
        carry >>= 32;
    }
}

static inline int fixedMagnitudeCompare(const FixedMagnitude* left, const FixedMagnitude* right)
{
    for (int i = ADA_FIXED_LIMBS - 1; i >= 0; --i) {
        if (left->words[i] != right->words[i]) { return left->words[i] > right->words[i] ? 1 : -1; }
    }
    return 0;
}

static inline void fixedMagnitudeSubtract(FixedMagnitude* left, const FixedMagnitude* right)
{
    unsigned long long borrow = 0;
    for (int i = 0; i < ADA_FIXED_LIMBS; ++i) {
        unsigned long long sub = (unsigned long long)right->words[i] + borrow;
        borrow = (unsigned long long)left->words[i] < sub;
        left->words[i] = (unsigned int)((unsigned long long)left->words[i] - sub);
    }
}

static inline int fixedScaleRatio(__int128 numerator, __int128 denominator, int twos, int fives,
                                 int isUnsigned, int truncate, long long* result)
{
    if (denominator == 0) { return 0; }
    if (fives == 0 && !truncate) { return fixedRatioMode(numerator, denominator, twos, isUnsigned, result); }
    int negative = (numerator < 0) != (denominator < 0);
    FixedMagnitude n = fixedMagnitude(numerator < 0 ? 0 - (unsigned __int128)numerator : (unsigned __int128)numerator);
    FixedMagnitude d = fixedMagnitude(denominator < 0 ? 0 - (unsigned __int128)denominator : (unsigned __int128)denominator);
    for (int i = 0; i < (twos < 0 ? -twos : twos); ++i) { fixedMagnitudeTimes(twos < 0 ? &d : &n, 2); }
    for (int i = 0; i < (fives < 0 ? -fives : fives); ++i) { fixedMagnitudeTimes(fives < 0 ? &d : &n, 5); }
    unsigned long long limit = isUnsigned ? ULLONG_MAX : (unsigned long long)LLONG_MAX + negative;
    unsigned long long lower = 0, upper = limit;
    while (lower < upper) {
        unsigned long long distance = upper - lower;
        unsigned long long middle = lower + distance / 2 + distance % 2;
        FixedMagnitude product = d;
        fixedMagnitudeTimes(&product, middle);
        if (fixedMagnitudeCompare(&product, &n) <= 0) { lower = middle; }
        else { upper = middle - 1; }
    }
    FixedMagnitude product = d;
    fixedMagnitudeTimes(&product, lower);
    fixedMagnitudeSubtract(&n, &product);
    if (fixedMagnitudeCompare(&n, &d) >= 0) { return 0; }
    fixedMagnitudeTimes(&n, 2);
    if (!truncate && fixedMagnitudeCompare(&n, &d) >= 0) {
        if (lower == limit) { return 0; }
        ++lower;
    }
    if (isUnsigned && negative && lower != 0) { return 0; }
    *result = negative ? (lower == (unsigned long long)LLONG_MAX + 1 ? LLONG_MIN : -(long long)lower) : (long long)lower;
    return 1;
}

static inline int fixedNumericOperation(FixedOperation operation, long long left, int leftScale,
                                       long long right, int rightScale, int isUnsigned, long long* result)
{
    if (!fixedValidScale(leftScale) || !fixedValidScale(rightScale)
        || (operation != FixedMultiply && operation != FixedDivide)) { return 0; }
    int multiply = operation == FixedMultiply;
    return fixedScaleRatio(multiply ? (__int128)left * right : left, multiply ? 1 : right,
        -fixedTwos(leftScale) + (multiply ? -fixedTwos(rightScale) : fixedTwos(rightScale)),
        -fixedFives(leftScale) + (multiply ? -fixedFives(rightScale) : fixedFives(rightScale)),
        isUnsigned, 0, result);
}

static inline int fixedRescale(long long value, int from, int to, long long* result)
{
    if (!fixedValidScale(from) || !fixedValidScale(to)) { return 0; }
    return fixedScaleRatio(value, 1, fixedTwos(to) - fixedTwos(from),
        fixedFives(to) - fixedFives(from), 0, 0, result);
}

static inline int fixedOperation(FixedOperation operation, long long left, int leftBits,
                                long long right, int rightBits, int resultBits, long long* result)
{
    if (!fixedValidScale(leftBits) || !fixedValidScale(rightBits) || !fixedValidScale(resultBits)) { return 0; }
    switch (operation) {
    case FixedAdd: return fixedRound((__int128)left + right, 1, result);
    case FixedSubtract: return fixedRound((__int128)left - right, 1, result);
    case FixedMultiply:
    case FixedDivide:
    case FixedMultiplyRounded:
    case FixedDivideRounded: {
        int multiply = operation == FixedMultiply || operation == FixedMultiplyRounded;
        return fixedScaleRatio(multiply ? (__int128)left * right : left, multiply ? 1 : right,
            fixedTwos(resultBits) - fixedTwos(leftBits) + (multiply ? -fixedTwos(rightBits) : fixedTwos(rightBits)),
            fixedFives(resultBits) - fixedFives(leftBits) + (multiply ? -fixedFives(rightBits) : fixedFives(rightBits)),
            0, fixedIsDecimal(resultBits) && (operation == FixedMultiply || operation == FixedDivide), result);
    }
    default: return 0;
    }
}

#endif
