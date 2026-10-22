#include "../runtime/adart.h"
#include "../runtime/adathread.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "fixed arithmetic check failed at line %d\n", __LINE__); abort(); \
} } while (0)

static void constraint(AdaTaskContext* context)
{
    CHECK(context->exception == ADA_CONSTRAINT_ERROR);
    context->exception = NULL;
}

static void* exercise(void* argument)
{
    (void)argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    const int scales[] = { -62, -31, -1, 0, 1, 30, 31, 62 };
    long long result;
    for (unsigned i = 0; i < sizeof(scales) / sizeof(scales[0]); ++i) {
        int bits = scales[i];
        CHECK(__ada_fixed_scale(ldexp(1.0, -bits)) == bits);
        CHECK(fixedRescale(LLONG_MIN, bits, bits, &result) && result == LLONG_MIN);
        CHECK(fixedRescale(LLONG_MAX, bits, bits, &result) && result == LLONG_MAX);
        for (unsigned j = 0; j < sizeof(scales) / sizeof(scales[0]); ++j) {
            int other = scales[j];
            CHECK(fixedOperation(FixedDivide, LLONG_MAX, bits, LLONG_MAX, bits, other, &result));
            CHECK(result == (other >= 0 ? 1LL << other : other == -1 ? 1 : 0));
        }
    }
    CHECK(fixedOperation(FixedMultiply, LLONG_MIN, 62, LLONG_MIN, 62, 62, &result) == 0);
    CHECK(fixedOperation(FixedMultiply, LLONG_MIN, 62, LLONG_MIN, 62, 0, &result) && result == 4);
    CHECK(fixedOperation(FixedDivide, LLONG_MAX, -62, LLONG_MAX, -62, 62, &result)
        && result == (1LL << 62));
    CHECK(fixedOperation(FixedMultiply, 1, 62, 1, 62, -62, &result) && result == 0);
    CHECK(fixedOperation(FixedDivide, 1, 62, 1, -62, -62, &result) && result == 0);
    CHECK(fixedRatio(1, (__int128)1 << 126, -2, &result) && result == 0);
    CHECK(fixedRatio((__int128)1 << 126, 1, -127, &result) && result == 1);
    CHECK(fixedRatio(-((__int128)1 << 126), 1, -127, &result) && result == -1);
    CHECK(fixedRatio(1, 3, 64, &result) && result == 6148914691236517205LL);
    CHECK(__ada_modular_to_fixed(UINT64_MAX, -62) == 4);
    CHECK(__ada_fixed_numeric_operation(FixedMultiply, 3, 1, 1, 0, 0) == 2);
    CHECK(__ada_fixed_numeric_operation(FixedMultiply, -3, 1, 1, 0, 0) == -2);
    CHECK((uint64_t)__ada_fixed_numeric_operation(FixedMultiply, 1, -32, 1, -31, 1) == (UINT64_C(1) << 63));
    CHECK(__ada_fixed_real_operation(FixedMultiply, LLONG_MAX, 62, LLONG_MAX, 62, 53) == 4.0);
    CHECK(__ada_fixed_real_operation(FixedDivide, 1, 0, 3, 0, 24) == (double)(1.0f / 3.0f));
    CHECK(__ada_fixed_real_operation(FixedDivide, 1, -62, 1, 62, 53) == ldexp(1.0, 124));
    CHECK(context.exception == NULL);
    __ada_fixed_numeric_operation(FixedMultiply, -1, 0, 1, 0, 1);
    constraint(&context);
    __ada_fixed_real_operation(FixedMultiply, LLONG_MAX, -62, LLONG_MAX, -62, 24);
    constraint(&context);
    __ada_fixed_operation(FixedDivide, 1, 0, 0, 0, 0);
    constraint(&context);
    __ada_fixed_real_operation(FixedDivide, 1, 0, 0, 0, 53);
    constraint(&context);
    char text[160];
    CHECK(__ada_image_fixed(text, sizeof(text), 1, -62, 1) == 22);
    CHECK(strcmp(text, " 4611686018427387904.0") == 0);
    CHECK(__ada_value_fixed("4611686018427387904.0", 21, -62, LLONG_MIN, LLONG_MAX) == 1);
    CHECK(__ada_value_fixed("0.00000000000000000021684043449710088680149056017398834228515625", 64, 62, LLONG_MIN, LLONG_MAX) == 1);
    CHECK(context.exception == NULL && __ada_task_context_dispose(&context));
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
