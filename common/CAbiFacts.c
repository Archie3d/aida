#include "CAbiFacts.h"

#include <float.h>
#include <limits.h>
#include <stdint.h>

#if defined(__APPLE__) && defined(__aarch64__)
const char kCAbiTargetName[] = "Darwin arm64";
#elif defined(__APPLE__) && defined(__x86_64__)
const char kCAbiTargetName[] = "Darwin amd64";
#elif defined(_WIN32) && defined(__x86_64__)
const char kCAbiTargetName[] = "Windows amd64";
#elif defined(__linux__) && defined(__aarch64__)
const char kCAbiTargetName[] = "Linux arm64";
#elif defined(__linux__) && defined(__x86_64__)
const char kCAbiTargetName[] = "Linux amd64";
#else
const char kCAbiTargetName[] = "the current target";
#endif

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

const CAbiInteger kCAbiIntegers[kCAbiIntegerCount] = {
    [kCAbiChar] = C_INTEGER("char", char),
    [kCAbiSignedChar] = C_INTEGER("signed char", signed char),
    [kCAbiUnsignedChar] = C_INTEGER("unsigned char", unsigned char),
    [kCAbiShort] = C_INTEGER("short", short),
    [kCAbiUnsignedShort] = C_INTEGER("unsigned short", unsigned short),
    [kCAbiInt] = C_INTEGER("int", int),
    [kCAbiUnsignedInt] = C_INTEGER("unsigned int", unsigned int),
    [kCAbiLong] = C_INTEGER("long", long),
    [kCAbiUnsignedLong] = C_INTEGER("unsigned long", unsigned long),
    [kCAbiLongLong] = C_INTEGER("long long", long long),
    [kCAbiUnsignedLongLong] = C_INTEGER("unsigned long long", unsigned long long),
    [kCAbiSize] = C_INTEGER("size_t", size_t),
    [kCAbiPtrdiff] = C_INTEGER("ptrdiff_t", ptrdiff_t),
    [kCAbiWchar] = C_INTEGER("wchar_t", wchar_t),
    [kCAbiChar16] = C_INTEGER("char16_t", uint_least16_t),
    [kCAbiChar32] = C_INTEGER("char32_t", uint_least32_t),
};

#define C_FLOAT(Type, Prefix) \
    {#Type, sizeof(Type), _Alignof(Type), Prefix##_MANT_DIG, Prefix##_MIN_EXP, Prefix##_MAX_EXP}

const CAbiFloat kCAbiFloats[kCAbiFloatCount] = {
    [kCAbiFloat] = C_FLOAT(float, FLT),
    [kCAbiDouble] = C_FLOAT(double, DBL),
    [kCAbiLongDouble] = C_FLOAT(long double, LDBL),
};

static bool isBinary64(const CAbiFloat* type)
{
    return kCAbiTarget.floatRadix == 2 && type->bytes * kCAbiTarget.charBits == 64
        && type->mantissaDigits == 53 && type->minExponent == -1021 && type->maxExponent == 1024;
}

const char* cAbiFloatingFormat(const CAbiFloat* type)
{
    unsigned int bits = type->bytes * kCAbiTarget.charBits;
    if (isBinary64(type)) { return "binary64"; }
    if (kCAbiTarget.floatRadix == 2) {
        if (bits == 32 && type->mantissaDigits == 24
            && type->minExponent == -125 && type->maxExponent == 128) {
            return "binary32";
        }
        if (type->minExponent == -16381 && type->maxExponent == 16384) {
            if (bits >= 80 && type->mantissaDigits == 64) { return "x87 extended"; }
            if (bits == 128 && type->mantissaDigits == 113) { return "binary128"; }
        }
    }
    return "unknown";
}

bool cAbiHasNativeLongDouble(void)
{
#if defined(__APPLE__) && defined(__aarch64__)
    const CAbiFloat* native = &kCAbiFloats[kCAbiLongDouble];
    const CAbiFloat* ordinary = &kCAbiFloats[kCAbiDouble];
    return isBinary64(native) && isBinary64(ordinary) && native->alignment == ordinary->alignment;
#else
    return false;
#endif
}
