#include "../runtime/adart.h"
#include "../runtime/adathread.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "modular runtime check failed at line %d\n", __LINE__); abort(); \
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
    const uint64_t moduli[] = { 1, 10, 256, UINT64_C(4294967297), UINT64_MAX - 58, UINT64_MAX, 0 };
    const uint64_t operands[] = { 0, 1, 2, INT64_MAX, UINT64_C(1) << 63, UINT64_MAX - 1, UINT64_MAX };
    for (unsigned i = 0; i < sizeof(moduli) / sizeof(moduli[0]); ++i) {
        unsigned __int128 modulus = moduli[i] == 0 ? (unsigned __int128)1 << 64 : moduli[i];
        for (unsigned j = 0; j < sizeof(operands) / sizeof(operands[0]); ++j) {
            uint64_t a = operands[j] % modulus;
            for (unsigned k = 0; k < sizeof(operands) / sizeof(operands[0]); ++k) {
                uint64_t b = operands[k] % modulus;
                uint64_t sum = __ada_modular_operation(ModularAdd, moduli[i], a, b);
                uint64_t difference = __ada_modular_operation(ModularSubtract, moduli[i], sum, b);
                uint64_t product = __ada_modular_operation(ModularMultiply, moduli[i], a, b);
                CHECK(difference == a);
                CHECK(product == (unsigned __int128)a * b % modulus);
                CHECK((uint64_t)__ada_modular_operation(ModularPower, moduli[i], a, 2)
                    == (unsigned __int128)a * a % modulus);
                if (b != 0) {
                    uint64_t quotient = __ada_modular_operation(ModularDivide, moduli[i], a, b);
                    uint64_t remainder = __ada_modular_operation(ModularRemainder, moduli[i], a, b);
                    CHECK((unsigned __int128)quotient * b + remainder == a);
                    CHECK(remainder < b);
                }
                CHECK(context.exception == NULL);
            }
        }
    }
    CHECK((uint64_t)__ada_modular_reduce(-1, 0, 0) == UINT64_MAX);
    CHECK(__ada_modular_reduce(LLONG_MIN, 0, 10) == 2);
    CHECK(__ada_modular_reduce(-1, 1, 10) == 5);
    CHECK(__ada_value_modular("18446744073709551615", 20, 0) == -1);
    CHECK(__ada_value_modular("16#FFFF_FFFF_FFFF_FFFF#", 23, 0) == -1);
    CHECK(__ada_value_modular("0e99999999999999999", 19, 0) == 0);
    char image[32];
    CHECK(__ada_image_modular(image, sizeof(image), UINT64_MAX) == 21);
    CHECK(strcmp(image, " 18446744073709551615") == 0);
    CHECK(__ada_round_to_modular(9223372036854775808.0) == LLONG_MIN);
    CHECK(__ada_round_to_modular(-0.4) == 0);
    CHECK(context.exception == NULL);
    __ada_modular_operation(ModularDivide, 0, -1, 0);
    expectConstraint(&context);
    __ada_modular_operation(ModularPower, 0, 2, -1);
    expectConstraint(&context);
    __ada_value_modular("18446744073709551616", 20, 0);
    expectConstraint(&context);
    __ada_value_modular("-0", 2, 0);
    expectConstraint(&context);
    __ada_value_modular("10", 2, 10);
    expectConstraint(&context);
    __ada_modular_convert(-1, 1, LLONG_MIN, LLONG_MAX, 0);
    expectConstraint(&context);
    __ada_modular_convert(-1, 0, 0, -1, 1);
    expectConstraint(&context);
    __ada_round_to_modular(-0.5);
    expectConstraint(&context);
    __ada_round_to_modular(18446744073709551616.0);
    expectConstraint(&context);
    __ada_round_to_modular(NAN);
    expectConstraint(&context);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaThread threads[4];
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_create(&threads[i], exercise, NULL) == 0);
    }
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_join(threads[i], NULL) == 0);
    }
    return 0;
}
