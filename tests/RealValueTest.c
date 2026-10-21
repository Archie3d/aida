#include "../runtime/adart.h"
#include "../runtime/adathread.h"

#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PARSE32(text) __ada_value_real_f32(text, sizeof(text) - 1)
#define PARSE64(text) __ada_value_real(text, sizeof(text) - 1)

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "real value check failed at line %d\n", __LINE__); abort(); \
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
    for (int i = 0; i < 30; ++i) {
        CHECK(PARSE32("16#F.FFFFF#E31") == FLT_MAX);
        CHECK(PARSE64("16#F.FFFFFFFFFFFF8#E255") == DBL_MAX);
        CHECK(PARSE32("2#1#E-149") == nextafterf(0.0f, 1.0f));
        CHECK(PARSE64("2#1#E-1074") == nextafter(0.0, 1.0));
        CHECK(PARSE32("2#1#E-150") == 0.0f);
        CHECK(PARSE64("2#1#E-1075") == 0.0);
        CHECK(PARSE32("2#1.1#E-149") == 2 * nextafterf(0.0f, 1.0f));
        CHECK(PARSE64("2#1.1#E-1074") == 2 * nextafter(0.0, 1.0));
        CHECK(PARSE32("1.000000059604644775390625") == 1.0f);
        CHECK(PARSE32("1.0000000596046447753906250000000000000001") == nextafterf(1.0f, 2.0f));
        CHECK(PARSE64("1.00000000000000011102230246251565404236316680908203125") == 1.0);
        CHECK(PARSE64("1.000000000000000111022302462515654042363166809082031250001") == nextafter(1.0, 2.0));
        CHECK(PARSE32("0e99999999999999999999999") == 0.0f);
        CHECK(PARSE64("1e-99999999999999999999999") == 0.0);
        CHECK(signbit(PARSE64("-0")));
        CHECK(PARSE64("3#0.1#") == 1.0 / 3.0);
        CHECK(PARSE32("10#0.1#") == 0.1f);
        for (int base = 2; base <= 16; ++base) {
            char based[32];
            int length = snprintf(based, sizeof based, "%d#10#e-1", base);
            CHECK(__ada_value_real(based, length) == 1.0);
        }
        /* Independent host conversion oracle for decimal rounding, including
           both target formats' overflow and subnormal intervals. */
        for (int exponent = -340; exponent <= 320; exponent += 3) {
            char decimal[64];
            int length = snprintf(decimal, sizeof decimal, "12345678901234567890123456789e%d", exponent);
            double wide = strtod(decimal, NULL);
            double parsed = __ada_value_real(decimal, length);
            if (isfinite(wide)) { CHECK(parsed == wide); }
            else { expectConstraint(&context); }
            float narrow = strtof(decimal, NULL);
            float parsedNarrow = __ada_value_real_f32(decimal, length);
            if (isfinite(narrow)) { CHECK(parsedNarrow == narrow); }
            else { expectConstraint(&context); }
        }
        CHECK(context.exception == NULL);
        const char* invalid[] = {"", "_1", "1_", "1__0", "nan", "-inf", "0x1p0",
            "2#2#", "16#1:", "16#.#", "16#1.0", "1e+", "1e-99999999999999999999_",
            "1e9999999999999999999999", "1\t", "\n1", "1 2", "+ 1", "1e1__0"};
        for (size_t j = 0; j < sizeof invalid / sizeof *invalid; ++j) {
            __ada_value_real(invalid[j], (int)strlen(invalid[j]));
            expectConstraint(&context);
            __ada_value_real_f32(invalid[j], (int)strlen(invalid[j]));
            expectConstraint(&context);
        }
        PARSE32("2#1#E128");
        expectConstraint(&context);
        PARSE64("2#1#E1024");
        expectConstraint(&context);
        PARSE32("16#F.FFFFF8#E31");
        expectConstraint(&context);
        PARSE64("16#F.FFFFFFFFFFFFC#E255");
        expectConstraint(&context);
        CHECK(__ada_value_real("1.0\0bad", 7) == 0.0);
        expectConstraint(&context);
        // No fixed-length buffer: leading zeros and fractional cancellation.
        char text[5010];
        memset(text, '0', 5000);
        memcpy(text + 5000, "1.25", 4);
        CHECK(__ada_value_real(text, 5004) == 1.25);
        text[0] = '1';
        memset(text + 1, '0', 4999);
        memcpy(text + 5000, "e-4999", 6);
        CHECK(__ada_value_real(text, 5006) == 1.0);
        CHECK(context.exception == NULL);
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
