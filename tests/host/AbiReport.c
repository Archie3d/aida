#include "../../runtime/adart.h"
#include "../../common/Array.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    uint16_t word = 1;
    unsigned char bytes[2];
    memcpy(bytes, &word, sizeof(word));
    if (CHAR_BIT != 8 || sizeof(void*) != 8 || sizeof(int) != 4
        || sizeof(int64_t) != 8 || sizeof(float) != 4 || sizeof(double) != 8
        || sizeof(size_t) != 8 || sizeof(ptrdiff_t) != 8 || bytes[0] != 1
        || sizeof(AdaTraceFrame) != 24 || offsetof(AdaTraceFrame, location) != 16
        || offsetof(AdaTaskContext, exception) != 0 || sizeof(AdaArrayDimension) != 32
        || sizeof(AdaArrayResult) != 48 || offsetof(AdaArrayResult, stride) != 40) {
        fprintf(stderr, "unsupported host/runtime data layout\n");
        return 1;
    }
    printf("{\"pointer_bits\":%zu,\"int_bits\":%zu,\"long_bits\":%zu,"
           "\"size_bits\":%zu,\"ptrdiff_bits\":%zu,\"little_endian\":true,"
           "\"trace_bytes\":%zu,\"array_result_bytes\":%zu}\n",
           sizeof(void*) * CHAR_BIT, sizeof(int) * CHAR_BIT, sizeof(long) * CHAR_BIT,
           sizeof(size_t) * CHAR_BIT, sizeof(ptrdiff_t) * CHAR_BIT,
           sizeof(AdaTraceFrame), sizeof(AdaArrayResult));
    return 0;
}
