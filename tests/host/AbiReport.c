#include "CAbiFacts.h"
#include "../../runtime/adart.h"
#include "../../common/Array.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char* floatingFormat(const CAbiFloat* type)
{
    unsigned int bits = type->bytes * kCAbiTarget.charBits;
    if (kCAbiTarget.floatRadix == 2) {
        if (bits == 32 && type->mantissaDigits == 24
            && type->minExponent == -125 && type->maxExponent == 128) {
            return "binary32";
        }
        if (bits == 64 && type->mantissaDigits == 53
            && type->minExponent == -1021 && type->maxExponent == 1024) {
            return "binary64";
        }
        if (type->minExponent == -16381 && type->maxExponent == 16384) {
            if (bits >= 80 && type->mantissaDigits == 64) {
                return "x87 extended";
            }
            if (bits == 128 && type->mantissaDigits == 113) {
                return "binary128";
            }
        }
    }
    return "unknown";
}

static void reportCAbi(void)
{
    printf(",\"char_bit\":%u,\"schar_min\":%d,\"schar_max\":%d,\"uchar_max\":%u,"
           "\"pointer_alignment\":%u,\"float_radix\":%d,\"c_integers\":{",
           kCAbiTarget.charBits, kCAbiTarget.signedCharMin, kCAbiTarget.signedCharMax,
           kCAbiTarget.unsignedCharMax, kCAbiTarget.pointerAlignment, kCAbiTarget.floatRadix);
    for (size_t index = 0; index < kCAbiIntegerCount; ++index) {
        const CAbiInteger* type = &kCAbiIntegers[index];
        printf("%s\"%s\":{\"bytes\":%u,\"alignment\":%u,\"signed\":%s}",
               index == 0 ? "" : ",", type->name, type->bytes, type->alignment,
               type->isSigned ? "true" : "false");
    }

    printf("},\"c_floats\":{");
    for (size_t index = 0; index < kCAbiFloatCount; ++index) {
        const CAbiFloat* type = &kCAbiFloats[index];
        printf("%s\"%s\":{\"bytes\":%u,\"alignment\":%u,\"mantissa_digits\":%d,"
               "\"min_exponent\":%d,\"max_exponent\":%d,\"format\":\"%s\"}",
               index == 0 ? "" : ",", type->name, type->bytes, type->alignment,
               type->mantissaDigits, type->minExponent, type->maxExponent, floatingFormat(type));
    }
    printf("}}\n");
}

int main(void)
{
    uint16_t word = 1;
    unsigned char bytes[2];
    memcpy(bytes, &word, sizeof(word));
    if (kCAbiTarget.charBits != 8 || kCAbiTarget.pointerBytes != 8 || sizeof(int) != 4
        || sizeof(int64_t) != 8 || sizeof(float) != 4 || sizeof(double) != 8
        || sizeof(size_t) != 8 || sizeof(ptrdiff_t) != 8 || bytes[0] != 1
        || sizeof(AdaTraceFrame) != 24 || offsetof(AdaTraceFrame, location) != 16
        || sizeof(AdaConcurrentObject) != kAdaConcurrentHeaderBytes
        || offsetof(AdaConcurrentObject, resource) != 8
        || sizeof(AdaMaster) != kAdaMasterBytes || offsetof(AdaMaster, activities) != 8
        || offsetof(AdaTaskContext, exception) != 0 || sizeof(AdaArrayDimension) != 32
        || sizeof(AdaArrayResult) != 48 || offsetof(AdaArrayResult, stride) != 40) {
        fprintf(stderr, "unsupported host/runtime data layout\n");
        return 1;
    }
    printf("{\"pointer_bits\":%u,\"int_bits\":%zu,\"long_bits\":%zu,"
           "\"size_bits\":%zu,\"ptrdiff_bits\":%zu,\"little_endian\":true,"
           "\"trace_bytes\":%zu,\"array_result_bytes\":%zu,\"master_bytes\":%zu",
           kCAbiTarget.pointerBytes * kCAbiTarget.charBits,
           sizeof(int) * CHAR_BIT, sizeof(long) * CHAR_BIT,
           sizeof(size_t) * CHAR_BIT, sizeof(ptrdiff_t) * CHAR_BIT,
           sizeof(AdaTraceFrame), sizeof(AdaArrayResult), sizeof(AdaMaster));
    reportCAbi();
    return 0;
}
