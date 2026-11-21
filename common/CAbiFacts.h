#ifndef C_ABI_FACTS_H
#define C_ABI_FACTS_H

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum CAbiIntegerKind
{
    kCAbiChar,
    kCAbiSignedChar,
    kCAbiUnsignedChar,
    kCAbiShort,
    kCAbiUnsignedShort,
    kCAbiInt,
    kCAbiUnsignedInt,
    kCAbiLong,
    kCAbiUnsignedLong,
    kCAbiLongLong,
    kCAbiUnsignedLongLong,
    kCAbiSize,
    kCAbiPtrdiff,
    kCAbiWchar,
    kCAbiChar16,
    kCAbiChar32,
    kCAbiIntegerCount
} CAbiIntegerKind;

typedef enum CAbiFloatKind
{
    kCAbiFloat,
    kCAbiDouble,
    kCAbiLongDouble,
    kCAbiFloatCount
} CAbiFloatKind;

typedef struct CAbiTarget
{
    unsigned int charBits;
    int signedCharMin;
    int signedCharMax;
    unsigned int unsignedCharMax;
    unsigned int pointerBytes;
    unsigned int pointerAlignment;
    int floatRadix;
} CAbiTarget;

typedef struct CAbiInteger
{
    const char* name;
    unsigned int bytes;
    unsigned int alignment;
    bool isSigned;
} CAbiInteger;

typedef struct CAbiFloat
{
    const char* name;
    unsigned int bytes;
    unsigned int alignment;
    int mantissaDigits;
    int minExponent;
    int maxExponent;
} CAbiFloat;

extern const CAbiTarget kCAbiTarget;
extern const char kCAbiTargetName[];
extern const CAbiInteger kCAbiIntegers[kCAbiIntegerCount];
extern const CAbiFloat kCAbiFloats[kCAbiFloatCount];

const char* cAbiFloatingFormat(const CAbiFloat* type);
bool cAbiHasNativeLongDouble(void);

#ifdef __cplusplus
}
#endif

#endif
