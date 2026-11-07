#include "adarandom.h"
#include "adart.h"
#include "adacalendar.h"

#include <stdatomic.h>
#include <string.h>

static const uint64_t kSignBit = UINT64_C(1) << 63;
static const char kImagePrefix[] = ADA_RANDOM_IMAGE_PREFIX;

/* xoshiro256** 1.0 and SplitMix64 seeding, from the public-domain algorithms
   by David Blackman and Sebastiano Vigna: https://prng.di.unimi.it/ .
   The transition uses per-object state, never the C library's global rand(). */
static uint64_t rotateLeft(uint64_t value, int distance)
{
    return (value << distance) | (value >> (64 - distance));
}

static int validState(const AdaRandomState* state)
{
    uint64_t bits = 0;
    for (int i = 0; i < kAdaRandomWordCount; ++i) { bits |= state->words[i]; }
    return bits != 0;
}

static int loadState(const void* data, AdaRandomState* state)
{
    if (data != NULL) {
        memcpy(state, data, sizeof *state);
        if (validState(state)) { return 1; }
    }
    __ada_raise(ADA_CONSTRAINT_ERROR);
    return 0;
}

static uint64_t nextWord(AdaRandomState* state)
{
    uint64_t* words = state->words;
    uint64_t result = rotateLeft(words[1] * 5, 7) * 9;
    uint64_t shifted = words[1] << 17;
    words[2] ^= words[0];
    words[3] ^= words[1];
    words[1] ^= words[2];
    words[0] ^= words[3];
    words[2] ^= shifted;
    words[3] = rotateLeft(words[3], 45);
    return result;
}

uint64_t __ada_random_next(void* data)
{
    AdaRandomState state;
    if (!loadState(data, &state)) { return 0; }
    uint64_t result = nextWord(&state);
    memcpy(data, &state, sizeof state);
    return result;
}

float __ada_random_float(void* data)
{
    return (float)(__ada_random_next(data) >> 40) * 0x1p-24f;
}

uint64_t __ada_random_discrete(void* data, uint64_t first, uint64_t last, int unsignedRange)
{
    uint64_t orderedFirst = unsignedRange ? first : first ^ kSignBit;
    uint64_t orderedLast = unsignedRange ? last : last ^ kSignBit;
    if (orderedFirst > orderedLast) { __ada_raise(ADA_CONSTRAINT_ERROR); return 0; }
    AdaRandomState state;
    if (!loadState(data, &state)) { return 0; }
    uint64_t width = last - first + 1;
    uint64_t result;
    if (width == 0) {
        result = first + nextWord(&state);
    } else {
        // Discard the incomplete low interval so every residue has the same
        // number of preimages, including ranges wider than signed 64 bits.
        uint64_t threshold = -width % width;
        do { result = nextWord(&state); } while (result < threshold);
        result = first + result % width;
    }
    memcpy(data, &state, sizeof state);
    return result;
}

static void seedState(void* data, uint64_t seed)
{
    AdaRandomState state;
    for (int i = 0; i < kAdaRandomWordCount; ++i) {
        seed += UINT64_C(0x9e3779b97f4a7c15);
        uint64_t word = seed;
        word = (word ^ (word >> 30)) * UINT64_C(0xbf58476d1ce4e5b9);
        word = (word ^ (word >> 27)) * UINT64_C(0x94d049bb133111eb);
        state.words[i] = word ^ (word >> 31);
    }
    memcpy(data, &state, sizeof state);
}

void __ada_random_seed(void* data, int32_t initiator)
{
    if (data == NULL) { __ada_raise(ADA_CONSTRAINT_ERROR); return; }
    seedState(data, (uint64_t)(int64_t)initiator);
}

void __ada_random_time(void* data)
{
    static atomic_uint_fast64_t lastSeed;
    if (data == NULL) { __ada_raise(ADA_CONSTRAINT_ERROR); return; }
    int64_t clock = __ada_calendar_clock();
    if (__ada_exception != NULL) { return; }
    uint_fast64_t observed = atomic_load(&lastSeed);
    uint_fast64_t selected;
    do {
        if (observed == UINT64_MAX) { __ada_raise(ADA_STORAGE_ERROR); return; }
        uint64_t orderedClock = (uint64_t)clock ^ kSignBit;
        selected = orderedClock > observed ? orderedClock : observed + 1;
    } while (!atomic_compare_exchange_weak(&lastSeed, &observed, selected));
    seedState(data, selected);
}

void __ada_random_restore(void* data, const void* from)
{
    AdaRandomState state;
    if (data == NULL) { __ada_raise(ADA_CONSTRAINT_ERROR); return; }
    if (loadState(from, &state)) { memcpy(data, &state, sizeof state); }
}

void __ada_random_image(const void* data, char* text, int length)
{
    static const char kDigits[] = "0123456789ABCDEF";
    AdaRandomState state;
    if (text == NULL || length != kAdaRandomImageWidth) { __ada_raise(ADA_CONSTRAINT_ERROR); return; }
    if (!loadState(data, &state)) { return; }
    char encoded[kAdaRandomImageWidth];
    memcpy(encoded, kImagePrefix, sizeof kImagePrefix - 1);
    for (int i = 0; i < kAdaRandomWordCount; ++i) {
        for (int n = 0; n < 16; ++n) {
            encoded[sizeof kImagePrefix - 1 + i * 16 + n] = kDigits[(state.words[i] >> (60 - n * 4)) & 15];
        }
    }
    memcpy(text, encoded, sizeof encoded);
}

void __ada_random_value(const char* text, int length, void* data)
{
    if (data == NULL || text == NULL || length != kAdaRandomImageWidth
        || memcmp(text, kImagePrefix, sizeof kImagePrefix - 1) != 0) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return;
    }
    AdaRandomState state = {0};
    for (int i = 0; i < kAdaRandomWordCount; ++i) {
        for (int n = 0; n < 16; ++n) {
            unsigned char digit = (unsigned char)text[sizeof kImagePrefix - 1 + i * 16 + n];
            int value = digit >= '0' && digit <= '9' ? digit - '0'
                : digit >= 'A' && digit <= 'F' ? digit - 'A' + 10
                : digit >= 'a' && digit <= 'f' ? digit - 'a' + 10 : -1;
            if (value < 0) { __ada_raise(ADA_CONSTRAINT_ERROR); return; }
            state.words[i] = (state.words[i] << 4) | (unsigned)value;
        }
    }
    if (!validState(&state)) { __ada_raise(ADA_CONSTRAINT_ERROR); return; }
    memcpy(data, &state, sizeof state);
}
