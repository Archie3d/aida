#include <limits.h>
#include <stddef.h>
#include <stdint.h>

int aida_interfaces_widths(int longBits, int wcharBits, int sizeBits, int ptrdiffBits,
                           int char16Bits, int char32Bits, int singleBits, int doubleBits, int plainCharSigned)
{
    return longBits == sizeof(long) * CHAR_BIT && wcharBits == sizeof(wchar_t) * CHAR_BIT
        && sizeBits == sizeof(size_t) * CHAR_BIT && ptrdiffBits == sizeof(ptrdiff_t) * CHAR_BIT
        && char16Bits == sizeof(uint_least16_t) * CHAR_BIT && char32Bits == sizeof(uint_least32_t) * CHAR_BIT
        && singleBits == sizeof(float) * CHAR_BIT && doubleBits == sizeof(double) * CHAR_BIT
        && plainCharSigned == (CHAR_MIN < 0);
}

int aida_interfaces_integers(short narrow, int word, long wide, unsigned long unsignedWide,
                             size_t size, ptrdiff_t difference, signed char byte, unsigned char unsignedByte)
{
    return narrow == SHRT_MIN && word == INT_MIN && wide == LONG_MIN && unsignedWide == ULONG_MAX
        && size == SIZE_MAX && difference == PTRDIFF_MIN && byte == SCHAR_MIN && unsignedByte == UCHAR_MAX;
}

char aida_interfaces_char(char value)
{
    return value == (char)0xFF ? (char)0x9A : 0;
}

wchar_t aida_interfaces_wchar(wchar_t value)
{
    return value == (wchar_t)0xFEDC ? (wchar_t)0xFEFE : 0;
}

uint_least16_t aida_interfaces_char16(uint_least16_t value)
{
    return value == 0xFEFF ? 0xFDFF : 0;
}

uint_least32_t aida_interfaces_char32(uint_least32_t value)
{
    return value == UINT32_C(0x10FFFE) ? UINT32_C(0x10FFFF) : UINT32_MAX;
}

double aida_interfaces_float(float single, double wide)
{
    return single == 1.25f && wide == -2.5 ? -1.25 : 0.0;
}

long double aida_interfaces_long_double(int tag, long double a, long double b, long double c,
                                        long double d, long double e, long double f, long double g,
                                        long double h, long double i, long double j, long tail)
{
    if (tag != -17 || a != 1.25L || b != -2.5L || c != 3.75L || d != -4.0L
        || e != 5.5L || f != -6.25L || g != 7.75L || h != -8.5L
        || i != 9.5L || j != -10.5L || tail != 123456789) {
        return 0.0L;
    }
    return i + j;
}
