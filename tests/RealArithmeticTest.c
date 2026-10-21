#include "../runtime/adart.h"
#include "../runtime/adathread.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "real arithmetic check failed at line %d\n", __LINE__); abort(); \
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
    for (int i = 0; i < 100; ++i) {
        CHECK(__ada_real_operation_f32(RealAdd, 16777216.0f, 1.0f) == 16777216.0f);
        CHECK(__ada_real_operation(RealAdd, 16777216.0, 1.0) == 16777217.0);
        CHECK(__ada_real_power_f32(-2.0f, -3) == -0.125f);
        CHECK(__ada_real_power(-2.0, -3) == -0.125);
        CHECK(__ada_real_power_f32(-1.0f, INT_MIN) == 1.0f);
        CHECK(__ada_real_power(-1.0, INT_MAX) == -1.0);
        CHECK(__ada_real_power_f32(0.0f, 0) == 1.0f);
        CHECK(__ada_real_power(0.0, 0) == 1.0);
        CHECK(__ada_real_power_f32(FLT_MAX, 1) == FLT_MAX);
        CHECK(__ada_real_power(DBL_MAX, 1) == DBL_MAX);
        CHECK(__ada_real_power_f32(FLT_MAX, -1) > 0.0f);
        CHECK(__ada_real_power(DBL_MAX, -1) > 0.0);
        CHECK(__ada_real_power_f32(2.0f, INT_MIN) == 0.0f);
        CHECK(__ada_real_power(2.0, INT_MIN) == 0.0);
        CHECK(__ada_real_power_f32(2.0f, -149) == nextafterf(0.0f, 1.0f));
        CHECK(__ada_real_power(2.0, -1074) == nextafter(0.0, 1.0));
        CHECK(__ada_real_power_f32(2.0f, -150) == 0.0f);
        CHECK(__ada_real_power(2.0, -1075) == 0.0);
        CHECK(__ada_real_operation_f32(RealDivide, FLT_MIN, 2.0f) > 0.0f);
        CHECK(__ada_real_operation(RealDivide, DBL_MIN, 2.0) > 0.0);
        CHECK(context.exception == NULL);
        __ada_real_operation_f32(RealDivide, 1.0f, -0.0f);
        expectConstraint(&context);
        __ada_real_operation(RealDivide, 0.0, 0.0);
        expectConstraint(&context);
        __ada_real_operation_f32(RealAdd, FLT_MAX, FLT_MAX);
        expectConstraint(&context);
        __ada_real_operation(RealSubtract, -DBL_MAX, DBL_MAX);
        expectConstraint(&context);
        __ada_real_operation_f32(RealMultiply, FLT_MAX, 2.0f);
        expectConstraint(&context);
        __ada_real_operation(RealDivide, DBL_MAX, 0.5);
        expectConstraint(&context);
        __ada_real_power_f32(-0.0f, INT_MIN);
        expectConstraint(&context);
        __ada_real_power(0.0, -1);
        expectConstraint(&context);
        __ada_real_power_f32(0.5f, INT_MIN);
        expectConstraint(&context);
        __ada_real_power(DBL_MAX, 2);
        expectConstraint(&context);
        __ada_real_power_f32(nextafterf(0.0f, 1.0f), -1);
        expectConstraint(&context);
        __ada_real_power(nextafter(0.0, 1.0), -1);
        expectConstraint(&context);
        __ada_real_operation_f32(RealAdd, NAN, 1.0f);
        expectConstraint(&context);
        __ada_real_operation(RealSubtract, INFINITY, 1.0);
        expectConstraint(&context);
        __ada_real_power_f32(INFINITY, 0);
        expectConstraint(&context);
        __ada_real_power(NAN, 0);
        expectConstraint(&context);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaThread workers[4];
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_create(&workers[i], exercise, NULL) == 0);
    }
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_join(workers[i], NULL) == 0);
    }
    CHECK(context.exception == NULL && __ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
