#ifndef C_ABI_FACTS_H
#define C_ABI_FACTS_H

#include <stdbool.h>
#include <stddef.h>

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
extern const CAbiInteger kCAbiIntegers[];
extern const size_t kCAbiIntegerCount;
extern const CAbiFloat kCAbiFloats[];
extern const size_t kCAbiFloatCount;

#endif
