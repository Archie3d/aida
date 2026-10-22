#include "../runtime/adart.h"
#include "../runtime/adathread.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "decimal runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

static void* exercise(void* unused)
{
    (void)unused;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    long long result;
    for (int scale = -18; scale <= 18; ++scale) {
        int tag = ADA_FIXED_DECIMAL_TAG + scale;
        CHECK(fixedRescale(LLONG_MIN, tag, tag, &result) && result == LLONG_MIN);
        CHECK(fixedRescale(LLONG_MAX, tag, tag, &result) && result == LLONG_MAX);
        CHECK(fixedOperation(FixedDivide, LLONG_MAX, tag, LLONG_MAX, tag, 118, &result)
            && result == 1000000000000000000LL);
        CHECK(fixedOperation(FixedMultiply, 1, 118, 1, 118, tag, &result) && result == 0);
    }
    /* A small independent integer oracle checks decimal truncation of both
       signs and explicit rounding at every half-count boundary. */
    for (long long a = -39; a <= 39; a += 3) {
        for (long long b = -19; b <= 19; b += 2) {
            CHECK(__ada_fixed_operation(FixedMultiply, a, 102, b, 101, 102) == a * b / 10);
            CHECK(__ada_fixed_operation(FixedDivide, a, 102, b, 101, 102) == a * 10 / b);
            long long product = a * b;
            long long rounded = product < 0 ? -((-product + 5) / 10) : (product + 5) / 10;
            CHECK(__ada_fixed_operation(FixedMultiplyRounded, a, 102, b, 101, 102) == rounded);
        }
    }
    CHECK(__ada_fixed_rescale(12350, 104, 102) == 124);
    CHECK(__ada_fixed_rescale(-12350, 104, 102) == -124);
    CHECK(__ada_fixed_operation(FixedMultiply, -125, 102, 50, 102, 102) == -62);
    CHECK(__ada_fixed_operation(FixedMultiplyRounded, -125, 102, 50, 102, 102) == -63);
    CHECK(__ada_fixed_rescale(3, 1, 102) == 150);
    CHECK(__ada_fixed_rescale(125, 102, 3) == 10);
    CHECK(__ada_fixed_rescale(LLONG_MAX, -59, 82) == 5316911983139663491LL);
    CHECK(__ada_float_to_fixed(1.235, 102) == 124);
    CHECK(__ada_float_to_fixed(0.145, 102) == 14);
    CHECK(__ada_float_to_fixed(-0.145, 102) == -14);
    CHECK(__ada_float_to_fixed(0.125, 102) == 13);
    CHECK(__ada_float_to_fixed(0.9999999999999999, 118) == 999999999999999889LL);
    CHECK(__ada_fixed_real_operation(FixedMultiply, 125, 102, 1, 0, 53) == 1.25);
    CHECK(__ada_fixed_real_operation(FixedDivide, 1, 102, 3, 102, 24) == (double)(1.0f / 3.0f));
    CHECK(__ada_fixed_fore(LLONG_MIN, LLONG_MAX, 82) == 38);
    CHECK(__ada_fixed_fore(-999, 999, 102) == 2);
    CHECK(__ada_fixed_fore(-999, 999, 100) == 4);
    CHECK(__ada_modular_to_fixed(UINT64_MAX, 99) == 1844674407370955162LL);
    CHECK((uint64_t)__ada_fixed_numeric_operation(FixedMultiply, LLONG_MAX, 100, 20, 101, 1)
        == UINT64_MAX - 1);
    char text[160];
    CHECK(__ada_image_fixed(text, sizeof(text), 999999999999999999LL, 118, 18) == 21);
    CHECK(strcmp(text, " 0.999999999999999999") == 0);
    CHECK(__ada_value_fixed("-0.0000000000000000005", 22, 118, LLONG_MIN, LLONG_MAX) == -1);
    CHECK(__ada_value_fixed("16#1.4#", 7, 102, LLONG_MIN, LLONG_MAX) == 125);
    CHECK(__ada_image_fixed(text, sizeof(text), 999999999999999999LL, 82, 1) == 39);
    CHECK(strcmp(text, " 999999999999999999000000000000000000.0") == 0);
    CHECK(context.exception == NULL);
    __ada_fixed_operation(FixedDivide, 1, 102, 0, 102, 102);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR);
    context.exception = NULL;
    __ada_fixed_rescale(LLONG_MAX, 82, 118);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR);
    context.exception = NULL;
    __ada_float_to_fixed(INFINITY, 102);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR);
    context.exception = NULL;
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaThread workers[4];
    for (int i = 0; i < 4; ++i) { CHECK(__ada_thread_create(&workers[i], exercise, NULL) == 0); }
    for (int i = 0; i < 4; ++i) { CHECK(__ada_thread_join(workers[i], NULL) == 0); }
    return 0;
}
