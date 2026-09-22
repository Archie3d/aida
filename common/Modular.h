/*
    This file is shared between the compiler and the runtime.
*/

#ifndef ADA_MODULAR_H
#define ADA_MODULAR_H

#include <stdint.h>

/* Values are part of the compiler/runtime ABI. The arithmetic values also
   match the checked signed-integer runtime operations. */
typedef enum ModularOperation
{
    ModularInvalid = -1,
    ModularAdd = 0,
    ModularSubtract = 1,
    ModularMultiply = 2,
    ModularDivide = 3,
    ModularRemainder = 4,
    ModularModulo = 5,
    ModularPower = 6,
    ModularAnd = 7,
    ModularOr = 8,
    ModularXor = 9,
    ModularNot = 10
} ModularOperation;

/* Shared by static evaluation and the runtime. Operands are canonical modular
   values, except the signed exponent. Moduli are limited to 2**32, so the
   product of two reduced operands fits in uint64_t. */
static inline int modularOperation(ModularOperation operation, long long modulus, long long left,
                                  long long right, long long* result)
{
    uint64_t m = (uint64_t)modulus;
    uint64_t a = (uint64_t)left;
    uint64_t b = (uint64_t)right;
    uint64_t value = 0;
    if (modulus < 1 || modulus > 4294967296LL || left < 0 || a >= m
        || (operation != ModularPower && (right < 0 || b >= m))) {
        return 0;
    }
    switch (operation) {
    case ModularAdd: value = (a + b) % m; break;
    case ModularSubtract: value = (a + m - b) % m; break;
    case ModularMultiply: value = (a * b) % m; break;
    case ModularDivide:
    case ModularRemainder:
    case ModularModulo:
        if (b == 0) {
            return 0;
        }
        value = operation == ModularDivide ? a / b : a % b;
        break;
    case ModularPower:
        if (right < 0) {
            return 0;
        }
        value = 1 % m;
        while (b != 0) {
            if (b & 1) {
                value = (value * a) % m;
            }
            b >>= 1;
            a = (a * a) % m;
        }
        break;
    case ModularAnd: value = (a & b) % m; break;
    case ModularOr: value = (a | b) % m; break;
    case ModularXor: value = (a ^ b) % m; break;
    case ModularNot: value = m - 1 - a; break;
    default: return 0;
    }
    *result = (long long)value;
    return 1;
}

#endif /* ADA_MODULAR_H */
