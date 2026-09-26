#include "../runtime/adart.h"
#include <limits.h>
#include <stdio.h>
#include <string.h>

static int failed;

static void check(int condition, const char* message)
{
    if (!condition) {
        fprintf(stderr, "%s\n", message);
        failed = 1;
    }
}

static void checkValue(const char* text, int bits, long long expected)
{
    __ada_exception = NULL;
    long long actual = __ada_value_fixed(text, (int)strlen(text), bits, LLONG_MIN, LLONG_MAX);
    check(__ada_exception == NULL && actual == expected, text);
}

static void checkInvalid(const char* text)
{
    __ada_exception = NULL;
    __ada_value_fixed(text, (int)strlen(text), 3, LLONG_MIN, LLONG_MAX);
    check(__ada_exception == ADA_CONSTRAINT_ERROR, text);
}

static void checkFormat(long long value, int bits, int aft, int exponent, const char* expected)
{
    char text[256];
    int length = (int)strlen(expected);
    __ada_exception = NULL;
    __ada_fixed_put_string(text, length, value, bits, aft, exponent);
    check(__ada_exception == NULL && memcmp(text, expected, (size_t)length) == 0, expected);
}

int main(void)
{
    checkValue("0", 0, 0);
    checkValue("-0.0625", 3, -1);
    checkValue("0.06249999999999999999999999999999999999999999999999999", 3, 0);
    checkValue("0.06250000000000000000000000000000000000000000000000001", 3, 1);
    checkValue("16#F.F#e1", 4, 4080);
    checkValue("2#1_0.0_1#e-1", 3, 9);
    checkValue("16:.8:", 3, 4);
    checkValue("9223372036854775807.499999999999999999999999999999999", 0, LLONG_MAX);
    checkValue("-9223372036854775808.499999999999999999999999999999999", 0, LLONG_MIN);
    checkValue("1e-999999999999999999999999999999999999999", 30, 0);
    checkValue("0e999999999999999999999999999999999999999", 0, 0);
    const char* invalid[] = {
        "", " ", "+", ".", "1__0", "1_", "_1", "1._0", "1e", "1e+", "1e_1",
        "1e1_", "1.0x", "16#1", "16#1:#", "17#1#", "2#2#", "16#_F#", "NaN", "Inf",
        "1152921504606846976", "-1152921504606846976.125", "1e9999999999999999999"
    };
    for (size_t i = 0; i < sizeof invalid / sizeof invalid[0]; ++i) { checkInvalid(invalid[i]); }
    checkFormat(10, 3, 1, 0, "1.3");
    checkFormat(-10, 3, 1, 0, "-1.3");
    checkFormat(1, 3, 0, 0, "0.1");
    checkFormat(1, 3, 3, 3, "1.250E-01");
    checkFormat(1, 30, 10, 0, "0.0000000009");
    checkFormat(0, 30, 3, 2, "0.000E+0");
    checkFormat(799, 3, 1, 0, "99.9");
    checkFormat(799, 3, 1, 2, "1.0E+2");
    checkFormat(LLONG_MIN, 0, 1, 0, "-9223372036854775808.0");
    checkFormat(LLONG_MAX, 0, 30, 0, "9223372036854775807.000000000000000000000000000000");
    checkFormat(1, 0, 70, 0, "1.0000000000000000000000000000000000000000000000000000000000000000000000");

    /* Exact 30-place output must recover every stored count at every scale.
       The deterministic sequence covers positive/negative full-width values. */
    uint64_t state = 123456789;
    for (int bits = 0; bits <= 30; ++bits) {
        for (int i = 0; i < 100; ++i) {
            state = state * UINT64_C(6364136223846793005) + 1;
            long long value;
            memcpy(&value, &state, sizeof value);
            __ada_exception = NULL;
            const char* image = __ada_image_fixed(value, bits, 30);
            long long restored = __ada_value_fixed(image, (int)strlen(image), bits, LLONG_MIN, LLONG_MAX);
            check(__ada_exception == NULL && restored == value, "exact fixed-point round trip");
        }
    }
    char text[] = "abc";
    __ada_exception = NULL;
    __ada_fixed_put_string(text, 2, 1, 0, 1, 0);
    check(__ada_exception == ADA_LAYOUT_ERROR && memcmp(text, "abc", 3) == 0, "short output preserves target");
    int last = 0;
    __ada_exception = NULL;
    long long value = __ada_fixed_get_string(" 1.25,tail", 10, 3, -100, 100, &last);
    check(__ada_exception == NULL && value == 10 && last == 5, "string input stopping position");
    return failed;
}
