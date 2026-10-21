#include "adart.h"

#include <float.h>
#include <math.h>

#if defined(__FAST_MATH__) || (defined(__FINITE_MATH_ONLY__) && __FINITE_MATH_ONLY__ > 0)
#error "checked real arithmetic requires strict floating-point compilation"
#endif

_Static_assert(sizeof(float) == 4 && FLT_RADIX == 2 && FLT_MANT_DIG == 24
    && FLT_MIN_EXP == -125 && FLT_MAX_EXP == 128, "binary32 floating model required");
_Static_assert(sizeof(double) == 8 && DBL_MANT_DIG == 53
    && DBL_MIN_EXP == -1021 && DBL_MAX_EXP == 1024, "binary64 floating model required");
_Static_assert(FLT_EVAL_METHOD == 0, "arithmetic must use the declared floating precision");

float __ada_real_operation_f32(RealOperation operation, float left, float right)
{
    if (!isfinite(left) || !isfinite(right) || (operation == RealDivide && right == 0.0)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0.0;
    }
    float result;
    switch (operation) {
    case RealAdd: result = left + right; break;
    case RealSubtract: result = left - right; break;
    case RealMultiply: result = left * right; break;
    case RealDivide: result = left / right; break;
    default:
        __ada_raise(ADA_PROGRAM_ERROR);
        return 0.0;
    }
    if (!isfinite(result)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0.0;
    }
    return result;
}

float __ada_real_power_f32(float left, int exponent)
{
    if (!isfinite(left) || (left == 0.0 && exponent < 0)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0.0;
    }
    /* Widen before negation: Integer'First has no positive int counterpart.
       Reciprocate first to avoid overflow when the final result underflows. */
    uint32_t remaining = exponent < 0 ? (uint32_t)(-(int64_t)exponent) : (uint32_t)exponent;
    float factor = exponent < 0 ? 1.0f / left : left;
    if (!isfinite(factor)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0.0;
    }
    float result = 1.0f;
    while (remaining != 0) {
        if (remaining & 1) {
            result *= factor;
            if (!isfinite(result)) {
                __ada_raise(ADA_CONSTRAINT_ERROR);
                return 0.0;
            }
        }
        remaining >>= 1;
        if (remaining != 0) {
            factor *= factor;
            if (!isfinite(factor)) {
                __ada_raise(ADA_CONSTRAINT_ERROR);
                return 0.0;
            }
        }
    }
    return result;
}

double __ada_real_operation(RealOperation operation, double left, double right)
{
    if (!isfinite(left) || !isfinite(right) || (operation == RealDivide && right == 0.0)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0.0;
    }
    double result;
    switch (operation) {
    case RealAdd: result = left + right; break;
    case RealSubtract: result = left - right; break;
    case RealMultiply: result = left * right; break;
    case RealDivide: result = left / right; break;
    default:
        __ada_raise(ADA_PROGRAM_ERROR);
        return 0.0;
    }
    if (!isfinite(result)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0.0;
    }
    return result;
}

double __ada_real_power(double left, int exponent)
{
    if (!isfinite(left) || (left == 0.0 && exponent < 0)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0.0;
    }
    /* Widen before negation: Integer'First has no positive int counterpart.
       Reciprocate first to avoid overflow when the final result underflows. */
    uint32_t remaining = exponent < 0 ? (uint32_t)(-(int64_t)exponent) : (uint32_t)exponent;
    double factor = exponent < 0 ? 1.0 / left : left;
    if (!isfinite(factor)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0.0;
    }
    double result = 1.0;
    while (remaining != 0) {
        if (remaining & 1) {
            result *= factor;
            if (!isfinite(result)) {
                __ada_raise(ADA_CONSTRAINT_ERROR);
                return 0.0;
            }
        }
        remaining >>= 1;
        if (remaining != 0) {
            factor *= factor;
            if (!isfinite(factor)) {
                __ada_raise(ADA_CONSTRAINT_ERROR);
                return 0.0;
            }
        }
    }
    return result;
}

