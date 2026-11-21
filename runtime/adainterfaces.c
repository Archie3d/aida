#include <stdint.h>

static uint64_t wordMask(unsigned int bits)
{
    return UINT64_MAX >> (64 - bits);
}

static uint64_t shiftRightArithmetic(uint64_t value, uint32_t amount, unsigned int bits)
{
    uint64_t fill = (value >> (bits - 1)) != 0 ? wordMask(bits) : 0;
    return amount < bits ? ((value ^ fill) >> amount) ^ fill : fill;
}

static uint64_t rotateLeft(uint64_t value, uint32_t amount, unsigned int bits)
{
    unsigned int shift = amount % bits;
    unsigned int complement = (bits - shift) % bits;
    return ((value << shift) | (value >> complement)) & wordMask(bits);
}

// The narrow overloads use the emitter's word class. Unsigned_32 and
// Unsigned_64 use its long class, including across these intrinsic calls.
#define DEFINE_SHIFTS(bits, Carrier) \
Carrier __ada_interfaces_shift_left_##bits(Carrier value, uint32_t amount) \
{ \
    return amount < bits ? ((uint64_t)value << amount) & wordMask(bits) : 0; \
} \
Carrier __ada_interfaces_shift_right_##bits(Carrier value, uint32_t amount) \
{ \
    return amount < bits ? (uint64_t)value >> amount : 0; \
} \
Carrier __ada_interfaces_shift_right_arithmetic_##bits(Carrier value, uint32_t amount) \
{ \
    return shiftRightArithmetic(value, amount, bits); \
} \
Carrier __ada_interfaces_rotate_left_##bits(Carrier value, uint32_t amount) \
{ \
    return rotateLeft(value, amount, bits); \
} \
Carrier __ada_interfaces_rotate_right_##bits(Carrier value, uint32_t amount) \
{ \
    return rotateLeft(value, bits - amount % bits, bits); \
}

DEFINE_SHIFTS(8, uint32_t)
DEFINE_SHIFTS(16, uint32_t)
DEFINE_SHIFTS(32, uint64_t)
DEFINE_SHIFTS(64, uint64_t)
