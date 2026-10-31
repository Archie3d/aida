#ifndef ADA_FLOAT_ATTRIBUTES_H
#define ADA_FLOAT_ATTRIBUTES_H

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

/* Shared attribute profiles and binary32/binary64 policy. */
typedef enum FloatAttribute
{
    FloatExponent, FloatFraction, FloatCompose, FloatScaling,
    FloatFloor, FloatCeiling, FloatRounding, FloatUnbiasedRounding,
    FloatMachineRounding, FloatTruncation, FloatRemainder, FloatAdjacent,
    FloatCopySign, FloatLeadingPart, FloatMachine, FloatModel,
    FloatMachineRadix, FloatMachineMantissa, FloatMachineEmin, FloatMachineEmax,
    FloatDenorm, FloatSignedZeros, FloatModelMantissa, FloatModelEmin,
    FloatModelEpsilon, FloatModelSmall, FloatSafeFirst, FloatSafeLast
} FloatAttribute;

typedef enum FloatAttributeResult
{
    FloatResult, FloatIntegerResult, FloatRealResult, FloatBooleanResult
} FloatAttributeResult;

typedef struct FloatAttributeInfo
{
    const char* name;
    FloatAttribute operation;
    int arguments;
    int integerArgument;
    FloatAttributeResult result;
} FloatAttributeInfo;

static inline const FloatAttributeInfo* floatAttributeInfo(const char* name)
{
    static const FloatAttributeInfo kAttributes[] = {
        {"exponent", FloatExponent, 1, 0, FloatIntegerResult},
        {"fraction", FloatFraction, 1, 0, FloatResult},
        {"compose", FloatCompose, 2, 1, FloatResult},
        {"scaling", FloatScaling, 2, 1, FloatResult},
        {"floor", FloatFloor, 1, 0, FloatResult},
        {"ceiling", FloatCeiling, 1, 0, FloatResult},
        {"rounding", FloatRounding, 1, 0, FloatResult},
        {"unbiased_rounding", FloatUnbiasedRounding, 1, 0, FloatResult},
        {"machine_rounding", FloatMachineRounding, 1, 0, FloatResult},
        {"truncation", FloatTruncation, 1, 0, FloatResult},
        {"remainder", FloatRemainder, 2, 0, FloatResult},
        {"adjacent", FloatAdjacent, 2, 0, FloatResult},
        {"copy_sign", FloatCopySign, 2, 0, FloatResult},
        {"leading_part", FloatLeadingPart, 2, 1, FloatResult},
        {"machine", FloatMachine, 1, 0, FloatResult},
        {"model", FloatModel, 1, 0, FloatResult},
        {"machine_radix", FloatMachineRadix, 0, 0, FloatIntegerResult},
        {"machine_mantissa", FloatMachineMantissa, 0, 0, FloatIntegerResult},
        {"machine_emin", FloatMachineEmin, 0, 0, FloatIntegerResult},
        {"machine_emax", FloatMachineEmax, 0, 0, FloatIntegerResult},
        {"denorm", FloatDenorm, 0, 0, FloatBooleanResult},
        {"signed_zeros", FloatSignedZeros, 0, 0, FloatBooleanResult},
        {"model_mantissa", FloatModelMantissa, 0, 0, FloatIntegerResult},
        {"model_emin", FloatModelEmin, 0, 0, FloatIntegerResult},
        {"model_epsilon", FloatModelEpsilon, 0, 0, FloatRealResult},
        {"model_small", FloatModelSmall, 0, 0, FloatRealResult},
        {"safe_first", FloatSafeFirst, 0, 0, FloatRealResult},
        {"safe_last", FloatSafeLast, 0, 0, FloatRealResult}
    };
    for (unsigned i = 0; i < sizeof(kAttributes) / sizeof(kAttributes[0]); ++i) {
        if (strcmp(name, kAttributes[i].name) == 0) { return &kAttributes[i]; }
    }
    return NULL;
}

static inline int floatMantissa(int narrow)
{
    return narrow ? FLT_MANT_DIG : DBL_MANT_DIG;
}

static inline int floatMinimumExponent(int narrow)
{
    return narrow ? FLT_MIN_EXP : DBL_MIN_EXP;
}

static inline double floatAttributeProperty(FloatAttribute operation, int narrow)
{
    switch (operation) {
    case FloatMachineRadix: return FLT_RADIX;
    case FloatMachineMantissa:
    case FloatModelMantissa: return floatMantissa(narrow);
    case FloatMachineEmin:
    case FloatModelEmin: return floatMinimumExponent(narrow);
    case FloatMachineEmax: return narrow ? FLT_MAX_EXP : DBL_MAX_EXP;
    case FloatDenorm:
    case FloatSignedZeros: return 1;
    case FloatModelEpsilon: return ldexp(1.0, 1 - floatMantissa(narrow));
    case FloatModelSmall: return ldexp(1.0, floatMinimumExponent(narrow) - 1);
    case FloatSafeFirst: return narrow ? -FLT_MAX : -DBL_MAX;
    case FloatSafeLast: return narrow ? FLT_MAX : DBL_MAX;
    default: return NAN;
    }
}

/* Round independently of the caller's floating environment. */
static inline double floatUnbiasedRounding(double value)
{
    double whole = trunc(value);
    double fraction = fabs(value - whole);
    if (fraction > 0.5 || (fraction == 0.5 && fmod(fabs(whole), 2.0) != 0.0)) {
        whole += copysign(1.0, value);
    }
    return copysign(fabs(whole), value);
}

/* No Ada state here: the runtime wrapper raises for a nonfinite result.
   Keeping this evaluator shared also pins static machine-operation choices. */
static inline double floatAttributeValue(FloatAttribute operation, double x, double y, int64_t n, int narrow)
{
    int exponent = 0;
    switch (operation) {
    case FloatExponent:
        frexp(x, &exponent);
        return exponent;
    case FloatFraction: return frexp(x, &exponent);
    case FloatCompose: x = frexp(x, &exponent); /* fall through */
    case FloatScaling:
        if (n > INT_MAX) { n = INT_MAX; }
        if (n < INT_MIN) { n = INT_MIN; }
        return narrow ? (double)scalbnf((float)x, (int)n) : scalbn(x, (int)n);
    case FloatFloor: return floor(x);
    case FloatCeiling: return ceil(x);
    case FloatRounding: return round(x);
    case FloatUnbiasedRounding:
    case FloatMachineRounding: return floatUnbiasedRounding(x);
    case FloatTruncation: return trunc(x);
    case FloatRemainder: return narrow ? (double)remainderf((float)x, (float)y) : remainder(x, y);
    case FloatAdjacent:
        /* Ada keeps X (including its zero sign) when X = Towards. */
        if (x == y) { return x; }
        return narrow ? (double)nextafterf((float)x, (float)y) : nextafter(x, y);
    case FloatCopySign: return copysign(x, y);
    case FloatLeadingPart:
        if (n <= 0) { return NAN; }
        if (n >= floatMantissa(narrow) || x == 0.0) { return x; }
        x = frexp(x, &exponent);
        return ldexp(trunc(ldexp(x, (int)n)), exponent - (int)n);
    case FloatMachine:
    case FloatModel: return narrow ? (double)(float)x : x;
    default: return NAN;
    }
}

#endif
