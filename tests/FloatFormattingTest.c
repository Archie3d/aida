#include "../runtime/adart.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

int main(void)
{
    const struct
    {
        double value;
        int exponent;
        const char* expected;
    } cases[] = {
        { 1.25, 1, " 1.25E+0" },
        { 1.25, 3, " 1.25E+00" },
        { -1.25e-12, 6, "-1.25E-00012" },
        { 1.25e100, 2, " 1.25E+100" },
        { 1.25e12, INT_MAX,
            " 1.25E+0000000000000000000000000000000000000000000000000000000000000000" }
    };

    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        /* Check every truncation point, including a zero-sized destination,
           and ensure that neither side of the destination is overwritten. */
        for (int size = 0; size <= 64; ++size) {
            char storage[66];
            memset(storage, '#', sizeof storage);
            __ada_format_float(storage + 1, size, cases[i].value, 2, 2, cases[i].exponent);
            size_t length = strlen(cases[i].expected);
            if (size > 0 && length >= (size_t)size) {
                length = (size_t)size - 1;
            }
            if (storage[0] != '#' || storage[size + 1] != '#'
                || (size > 0 && (memcmp(storage + 1, cases[i].expected, length) != 0
                    || storage[length + 1] != '\0'))) {
                fprintf(stderr, "Float formatting failed for case %zu, size %d\n", i, size);
                return 1;
            }
        }
    }
    return 0;
}
