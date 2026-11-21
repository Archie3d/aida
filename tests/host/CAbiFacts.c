#include "CAbiFacts.h"

#include <float.h>
#include <limits.h>
#include <stdint.h>

const CAbiTarget kCAbiTarget = {
    .charBits = CHAR_BIT,
    .signedCharMin = SCHAR_MIN,
    .signedCharMax = SCHAR_MAX,
    .unsignedCharMax = UCHAR_MAX,
    .pointerBytes = sizeof(void*),
    .pointerAlignment = _Alignof(void*),
    .floatRadix = FLT_RADIX,
};

// Keep the data usable in a freestanding target compilation. C11 defines
// char16_t and char32_t in <uchar.h> as uint_least16_t and uint_least32_t.
#define C_INTEGER(Name, Type) {Name, sizeof(Type), _Alignof(Type), (Type)-1 < (Type)1}

const CAbiInteger kCAbiIntegers[] = {
    C_INTEGER("char", char),
    C_INTEGER("signed char", signed char),
    C_INTEGER("unsigned char", unsigned char),
    C_INTEGER("short", short),
    C_INTEGER("unsigned short", unsigned short),
    C_INTEGER("int", int),
    C_INTEGER("unsigned int", unsigned int),
    C_INTEGER("long", long),
    C_INTEGER("unsigned long", unsigned long),
    C_INTEGER("long long", long long),
    C_INTEGER("unsigned long long", unsigned long long),
    C_INTEGER("size_t", size_t),
    C_INTEGER("ptrdiff_t", ptrdiff_t),
    C_INTEGER("wchar_t", wchar_t),
    C_INTEGER("char16_t", uint_least16_t),
    C_INTEGER("char32_t", uint_least32_t),
};

const size_t kCAbiIntegerCount = sizeof(kCAbiIntegers) / sizeof(*kCAbiIntegers);

#define C_FLOAT(Type, Prefix) \
    {#Type, sizeof(Type), _Alignof(Type), Prefix##_MANT_DIG, Prefix##_MIN_EXP, Prefix##_MAX_EXP}

const CAbiFloat kCAbiFloats[] = {
    C_FLOAT(float, FLT),
    C_FLOAT(double, DBL),
    C_FLOAT(long double, LDBL),
};

const size_t kCAbiFloatCount = sizeof(kCAbiFloats) / sizeof(*kCAbiFloats);
