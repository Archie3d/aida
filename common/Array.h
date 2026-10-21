/* Internal array ABI v2. Compiler and C runtime must change together.
   Rank and element representation belong to the resolved array type. */
#ifndef ADA_ARRAY_H
#define ADA_ARRAY_H
#include <stdint.h>
#include <stddef.h>

typedef struct AdaArrayDimension
{
    int64_t first;
    int64_t last;
    int64_t length;
    int64_t stride; /* bytes to the next element/row */
} AdaArrayDimension;

typedef struct AdaArrayResult
{
    void* data;
    int64_t size; /* allocated transfer bytes, at least one for null arrays */
    int64_t first;
    int64_t last;
    int64_t length;
    int64_t stride;
    /* Further AdaArrayDimension records follow for rank > 1. */
} AdaArrayResult;

/* Saved local/captured views start with data, then dimension records.
   Owned results insert size between data and dimension records. Expanded Ada
   parameters pass data followed by first/last/length/stride per dimension. */
static inline long long adaArrayLocalSize(int rank)
{
    return 8 + 32LL * rank;
}
static inline long long adaArrayResultSize(int rank)
{
    return 16 + 32LL * rank;
}
static inline int adaArrayLocalFirst(int dimension)
{
    return 8 + dimension * 32;
}
static inline int adaArrayResultFirst(int dimension)
{
    return 16 + dimension * 32;
}
enum { AdaArrayBoundBytes = 8, AdaArrayTransferOffset = 8 };
#endif
