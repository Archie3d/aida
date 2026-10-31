#include "../runtime/adart.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "floating attribute check failed at line %d\n", __LINE__); abort(); \
} } while (0)

static void expectConstraint(AdaTaskContext* context)
{
    CHECK(context->exception == ADA_CONSTRAINT_ERROR);
    context->exception = NULL;
}

static void* exercise(void* argument)
{
    (void)argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int narrow = 0; narrow < 2; ++narrow) {
        double maximum = narrow ? FLT_MAX : DBL_MAX;
        double minimum = narrow ? FLT_MIN : DBL_MIN;
        double tiny = narrow ? nextafterf(0.0f, 1.0f) : nextafter(0.0, 1.0);
        for (int operation = FloatFloor; operation <= FloatTruncation; ++operation) {
            double result = __ada_float_attribute(operation, -0.0, 0.0, 0);
            CHECK(result == 0.0 && signbit(result));
            float narrowResult = __ada_float_attribute_f32(operation, -0.0f, 0.0f, 0);
            CHECK(narrowResult == 0.0f && signbit(narrowResult));
        }
        CHECK(__ada_float_exponent(tiny) == (narrow ? -148 : -1073));
        CHECK(__ada_float_exponent(-maximum) == (narrow ? 128 : 1024));
        CHECK(__ada_float_exponent(-0.0) == 0);
        for (int i = -1025; i <= 1025; ++i) {
            double value = i / 16.0;
            CHECK(__ada_float_attribute(FloatFloor, value, 0.0, 0) == floor(value));
            CHECK(__ada_float_attribute_f32(FloatCeiling, value, 0.0, 0) == ceil(value));
            CHECK(__ada_float_attribute(FloatRounding, value, 0.0, 0) == round(value));
            CHECK(__ada_float_attribute_f32(FloatUnbiasedRounding, value, 0.0, 0) == nearbyint(value));
            CHECK(__ada_float_attribute(FloatMachineRounding, value, 0.0, 0) == nearbyint(value));
            CHECK(__ada_float_attribute_f32(FloatTruncation, value, 0.0, 0) == trunc(value));
            int exponent;
            double fraction = frexp(value, &exponent);
            CHECK(__ada_float_attribute(FloatFraction, value, 0.0, 0) == fraction);
            CHECK(__ada_float_attribute_f32(FloatCompose, fraction, 0.0, exponent) == value);
        }
        CHECK(context.exception == NULL);
        CHECK(__ada_float_attribute(FloatRemainder, 7.0, 2.0, 0) == -1.0);
        CHECK(__ada_float_attribute_f32(FloatRemainder, 5.0f, 2.0f, 0) == 1.0f);
        CHECK(__ada_float_attribute(FloatRemainder, -7.0, -2.0, 0) == 1.0);
        CHECK(__ada_float_attribute(FloatRemainder, maximum, tiny, 0) == 0.0);
        CHECK(__ada_float_attribute(FloatLeadingPart, -3.75, 0.0, 2) == -3.0);
        CHECK(__ada_float_attribute(FloatLeadingPart, tiny, 0.0, INT64_MAX) == tiny);
        CHECK(__ada_float_attribute(FloatLeadingPart, tiny, 0.0, 1) == tiny);
        CHECK(__ada_float_attribute(FloatScaling, minimum, 0.0, -1) == minimum / 2.0);
        CHECK(__ada_float_attribute(FloatScaling, -tiny, 0.0, INT64_MIN) == 0.0);
        CHECK(signbit(__ada_float_attribute(FloatScaling, -tiny, 0.0, INT64_MIN)));
        CHECK(__ada_float_attribute(FloatCompose, tiny, 0.0, 0) == 0.5);
        CHECK(__ada_float_attribute(FloatCopySign, 2.0, -0.0, 0) == -2.0);
        CHECK(signbit(__ada_float_attribute(FloatAdjacent, -0.0, 0.0, 0)));
        CHECK(signbit(__ada_float_attribute(FloatAdjacent, -tiny, 0.0, 0)));
        CHECK(__ada_float_attribute(FloatMachine, maximum, 0.0, 0) == maximum);
        CHECK(__ada_float_attribute(FloatModel, tiny, 0.0, 0) == tiny);
        CHECK(context.exception == NULL);
        __ada_float_attribute(FloatAdjacent, DBL_MAX, INFINITY, 0);
        expectConstraint(&context);
        __ada_float_attribute_f32(FloatAdjacent, FLT_MAX, INFINITY, 0);
        expectConstraint(&context);
        __ada_float_attribute(FloatScaling, maximum, 0.0, INT64_MAX);
        expectConstraint(&context);
        __ada_float_attribute_f32(FloatScaling, FLT_MAX, 0.0f, 1);
        expectConstraint(&context);
        __ada_float_attribute(FloatRemainder, 1.0, -0.0, 0);
        expectConstraint(&context);
        __ada_float_attribute_f32(FloatRemainder, 1.0f, 0.0f, 0);
        expectConstraint(&context);
        __ada_float_attribute(FloatLeadingPart, 1.0, 0.0, 0);
        expectConstraint(&context);
        __ada_float_attribute_f32(FloatLeadingPart, 0.0f, 0.0f, INT64_MIN);
        expectConstraint(&context);
        __ada_float_attribute(FloatFloor, NAN, 0.0, 0);
        expectConstraint(&context);
        __ada_float_exponent(INFINITY);
        expectConstraint(&context);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaThread workers[4];
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_create(&workers[i], exercise, NULL) == 0);
    }
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_join(workers[i], NULL) == 0);
    }
    return 0;
}
