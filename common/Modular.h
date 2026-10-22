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

/* Zero encodes modulus 2**64 at the runtime ABI. Unsigned 128-bit
   intermediates also handle nonbinary moduli without overflow. */
static inline int modularOperation(ModularOperation operation, long long modulus, long long left,
                                  long long right, long long* result)
{
    unsigned __int128 m = (uint64_t)modulus;
    if (m == 0) {
        m = (unsigned __int128)1 << 64;
    }
    unsigned __int128 a = (uint64_t)left;
    unsigned __int128 b = (uint64_t)right;
    unsigned __int128 value = 0;
    if (a >= m || (operation != ModularPower && b >= m)) {
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
