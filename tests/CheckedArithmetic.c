#include <limits.h>
#include <stdint.h>
#include <string.h>

/* Independent mathematical oracle: no generated-code overflow formulas and
   no changes to the runtime exception state. */
int checkedArithmetic(int operation, int bits, long long left, long long right,
                      long long* result)
{
    __int128 low = bits == 32 ? INT32_MIN : INT64_MIN;
    __int128 high = bits == 32 ? INT32_MAX : INT64_MAX;
    __int128 a = left;
    __int128 b = right;
    __int128 value = 0;
    switch (operation) {
    case 0: value = a + b; break;
    case 1: value = a - b; break;
    case 2: value = a * b; break;
    case 3:
    case 4:
    case 5:
        if (b == 0) {
            return 0;
        }
        value = operation == 3 ? a / b : a % b;
        if (operation == 5 && value != 0 && (value < 0) != (b < 0)) {
            value += b;
        }
        break;
    case 6:
        if (b < 0) {
            return 0;
        }
        value = 1;
        while (b != 0) {
            if (b & 1) {
                value *= a;
                if (value < low || value > high) {
                    return 0;
                }
            }
            b >>= 1;
            if (b != 0) {
                a *= a;
                if (a < low || a > high) {
                    return 0;
                }
            }
        }
        break;
    case 7: value = -a; break;
    case 8: value = a < 0 ? -a : a; break;
    default: return 0;
    }
    if (value < low || value > high) {
        return 0;
    }
    *result = (long long)value;
    return 1;
}

long long arithmeticSample(int index, int bits)
{
    static const int64_t edges[] = {
        INT64_MIN, INT64_MIN + 1, -4611686018427387904LL,
        -4294967296LL, -3037000500LL, -3037000499LL,
        INT32_MIN, INT32_MIN + 1, -65536, -46341, -46340,
        -3, -2, -1, 0, 1, 2, 3, 46340, 46341, 65536,
        INT32_MAX - 1, INT32_MAX, 3037000499LL, 3037000500LL,
        4294967296LL, 4611686018427387904LL, INT64_MAX - 1, INT64_MAX
    };
    uint64_t sample;
    if (index < (int)(sizeof(edges) / sizeof(edges[0]))) {
        sample = (uint64_t)edges[index];
    } else {
        sample = (uint64_t)index + UINT64_C(0x9e3779b97f4a7c15);
        sample = (sample ^ (sample >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        sample = (sample ^ (sample >> 27)) * UINT64_C(0x94d049bb133111eb);
        sample ^= sample >> 31;
    }
    if (bits == 32) {
        uint32_t word = (uint32_t)sample;
        int32_t signedWord;
        memcpy(&signedWord, &word, sizeof(word));
        return signedWord;
    }
    int64_t signedSample;
    memcpy(&signedSample, &sample, sizeof(sample));
    return signedSample;
}
