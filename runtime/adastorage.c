#include "adart.h"

#include <limits.h>

_Static_assert(sizeof(uintptr_t) == 8 && CHAR_BIT == 8, "64-bit byte addresses required");

void* __ada_storage_add(void* left, int64_t right)
{
    return (void*)((uintptr_t)left + (uint64_t)right);
}

void* __ada_storage_offset_add(int64_t left, void* right)
{
    return __ada_storage_add(right, left);
}

void* __ada_storage_subtract(void* left, int64_t right)
{
    return (void*)((uintptr_t)left - (uint64_t)right);
}

int64_t __ada_storage_difference(void* left, void* right)
{
    uint64_t bits = (uintptr_t)left - (uintptr_t)right;
    return bits <= INT64_MAX ? (int64_t)bits : -1 - (int64_t)(UINT64_MAX - bits);
}

int64_t __ada_storage_mod(void* left, int64_t right)
{
    if (right == 0) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return 0;
    }
    uint64_t magnitude = right < 0 ? 0 - (uint64_t)right : (uint64_t)right;
    uint64_t remainder = (uintptr_t)left % magnitude;
    return right >= 0 || remainder == 0 ? (int64_t)remainder : -(int64_t)(magnitude - remainder);
}

void* __ada_storage_to_address(uint64_t value)
{
    return (void*)(uintptr_t)value;
}

uint64_t __ada_storage_to_integer(void* value)
{
    return (uintptr_t)value;
}
