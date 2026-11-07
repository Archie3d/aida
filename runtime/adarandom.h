#pragma once

#include <stdint.h>

#define ADA_RANDOM_IMAGE_PREFIX "AIDA-RNG1:"
enum
{
    kAdaRandomWordCount = 4,
    kAdaRandomImageWidth = sizeof(ADA_RANDOM_IMAGE_PREFIX) - 1 + kAdaRandomWordCount * 16
};

typedef struct AdaRandomState
{
    uint64_t words[kAdaRandomWordCount];
} AdaRandomState;

uint64_t __ada_random_next(void* data);
float __ada_random_float(void* data);
uint64_t __ada_random_discrete(void* data, uint64_t first, uint64_t last, int unsignedRange);
void __ada_random_seed(void* data, int32_t initiator);
void __ada_random_time(void* data);
void __ada_random_restore(void* data, const void* from);
void __ada_random_image(const void* data, char* text, int length);
void __ada_random_value(const char* text, int length, void* data);
