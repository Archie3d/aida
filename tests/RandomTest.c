#include "../runtime/adart.h"
#include "../runtime/adarandom.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "random check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kWorkers = 4, kTimedResets = 100, kSamples = 100000 };

static void clearError(void)
{
    CHECK(__ada_exception == ADA_CONSTRAINT_ERROR);
    __ada_exception = NULL;
}

static void knownAnswers(void)
{
    static const uint64_t kExpected[] = {
        UINT64_C(0x99ec5f36cb75f2b4), UINT64_C(0xbf6e1f784956452a),
        UINT64_C(0x1a5f849d4933e6e0), UINT64_C(0x6aa594f1262d2d2c),
        UINT64_C(0xbba5ad4a1f842e59), UINT64_C(0xffef8375d9ebcaca),
        UINT64_C(0x6c160deed2f54c98), UINT64_C(0x8920ad648fc30a3f)
    };
    static const uint32_t kFloatBits[] = {
        0x3f19ec5f, 0x3f3f6e1f, 0x3dd2fc20, 0x3ed54b28,
        0x3f3ba5ad, 0x3f7fef83, 0x3ed82c1a, 0x3f0920ad
    };
    AdaRandomState raw, floats, bounded, unsignedFull, signedFull;
    __ada_random_seed(&raw, 0);
    __ada_random_seed(&floats, 0);
    __ada_random_seed(&bounded, 0);
    __ada_random_seed(&unsignedFull, 0);
    __ada_random_seed(&signedFull, 0);
    for (unsigned i = 0; i < sizeof kExpected / sizeof *kExpected; ++i) {
        CHECK(__ada_random_next(&raw) == kExpected[i]);
        CHECK(__ada_random_discrete(&unsignedFull, 0, UINT64_MAX, 1) == kExpected[i]);
        CHECK(__ada_random_discrete(&signedFull, UINT64_C(1) << 63, INT64_MAX, 0)
            == (kExpected[i] ^ (UINT64_C(1) << 63)));
        float value = __ada_random_float(&floats);
        uint32_t bits;
        memcpy(&bits, &value, sizeof bits);
        CHECK(bits == kFloatBits[i] && memcmp(&raw, &floats, sizeof raw) == 0);
    }
    // Width 2**63+1 forces rejection of the third/fourth reference words.
    CHECK(__ada_random_discrete(&bounded, 0, UINT64_C(0x8000000000000000), 1) == UINT64_C(0x19ec5f36cb75f2b3));
    CHECK(__ada_random_discrete(&bounded, 0, UINT64_C(0x8000000000000000), 1) == UINT64_C(0x3f6e1f7849564529));
    CHECK(__ada_random_discrete(&bounded, 0, UINT64_C(0x8000000000000000), 1) == UINT64_C(0x3ba5ad4a1f842e58));
    __ada_random_seed(&raw, 0);
    for (int i = 0; i < 5; ++i) { __ada_random_next(&raw); }
    CHECK(memcmp(&raw, &bounded, sizeof raw) == 0 && __ada_exception == NULL);
}

static void imageErrors(void)
{
    AdaRandomState state, saved, decoded;
    __ada_random_seed(&state, INT32_MIN);
    saved = state;
    struct
    {
        char first;
        char data[kAdaRandomImageWidth];
        char last;
    } text = {.first = 71, .last = 93};
    __ada_random_image(&state, text.data, sizeof text.data);
    CHECK(text.first == 71 && text.last == 93 && memcmp(&state, &saved, sizeof state) == 0);
    __ada_random_value(text.data, sizeof text.data, &decoded);
    CHECK(memcmp(&state, &decoded, sizeof state) == 0);
    for (unsigned i = sizeof ADA_RANDOM_IMAGE_PREFIX - 1; i < sizeof text.data; ++i) {
        if (text.data[i] >= 'A' && text.data[i] <= 'F') { text.data[i] += 'a' - 'A'; }
    }
    __ada_random_value(text.data, sizeof text.data, &decoded);
    CHECK(memcmp(&state, &decoded, sizeof state) == 0);
    for (int i = 0; i < kAdaRandomImageWidth; ++i) {
        char original = text.data[i];
        text.data[i] = '!';
        decoded = saved;
        __ada_random_value(text.data, sizeof text.data, &decoded);
        clearError();
        CHECK(memcmp(&decoded, &saved, sizeof saved) == 0);
        text.data[i] = original;
    }
    for (int length = -1; length <= kAdaRandomImageWidth + 1; ++length) {
        if (length == kAdaRandomImageWidth) { continue; }
        __ada_random_value(text.data, length, &decoded);
        clearError();
        CHECK(memcmp(&decoded, &saved, sizeof saved) == 0);
    }
    memset(text.data + sizeof ADA_RANDOM_IMAGE_PREFIX - 1, '0', kAdaRandomWordCount * 16);
    __ada_random_value(text.data, sizeof text.data, &decoded);
    clearError();
    CHECK(memcmp(&decoded, &saved, sizeof saved) == 0);
    AdaRandomState invalid = {0};
    __ada_random_restore(&state, &invalid);
    clearError();
    CHECK(memcmp(&state, &saved, sizeof state) == 0);
    __ada_random_next(&invalid);
    clearError();
    __ada_random_restore(NULL, &saved);
    clearError();
    __ada_random_discrete(&state, 3, 2, 1);
    clearError();
    __ada_random_discrete(&state, 1, (uint64_t)-1, 0);
    clearError();
    CHECK(memcmp(&state, &saved, sizeof state) == 0);
    unsigned char unaligned[sizeof state + 1];
    __ada_random_seed(unaligned + 1, 0);
    __ada_random_seed(&state, 0);
    CHECK(__ada_random_next(unaligned + 1) == __ada_random_next(&state));
}

static void coverage(void)
{
    static unsigned counts[32768];
    AdaRandomState state;
    __ada_random_seed(&state, 1297);
    for (int i = 0; i < 2000000; ++i) {
        uint64_t value = __ada_random_discrete(&state, 0, 32767, 1);
        CHECK(value < 32768);
        ++counts[value];
    }
    for (int i = 0; i < 32768; ++i) { CHECK(counts[i] > 10 && counts[i] < 130); }
}

typedef struct Worker
{
    int number;
    AdaRandomState timed[kTimedResets];
} Worker;

static void* exercise(void* argument)
{
    Worker* worker = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaRandomState state, reference;
    __ada_random_seed(&state, -worker->number);
    reference = state;
    unsigned counts[6] = {0};
    double sum = 0;
    for (int i = 0; i < kSamples; ++i) {
        float value = __ada_random_float(&state);
        CHECK(isfinite(value) && value >= 0 && value < 1);
        sum += value;
        uint64_t die = __ada_random_discrete(&state, 1, 6, 0);
        CHECK(die >= 1 && die <= 6);
        ++counts[die - 1];
        uint64_t high = __ada_random_discrete(&state, UINT64_MAX - 9, UINT64_MAX, 1);
        CHECK(high >= UINT64_MAX - 9);
        uint64_t negative = __ada_random_discrete(&state, (uint64_t)-19, (uint64_t)-7, 0);
        CHECK((int64_t)negative >= -19 && (int64_t)negative <= -7);
        CHECK(__ada_random_discrete(&state, 19, 19, 0) == 19);
    }
    CHECK(sum / kSamples > 0.49 && sum / kSamples < 0.51);
    for (int i = 0; i < 6; ++i) { CHECK(counts[i] > 15000 && counts[i] < 18500); }
    __ada_random_seed(&state, -worker->number);
    CHECK(memcmp(&state, &reference, sizeof state) == 0);
    for (int i = 0; i < kTimedResets; ++i) { __ada_random_time(&worker->timed[i]); }
    CHECK(context.exception == NULL && __ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    knownAnswers();
    imageErrors();
    coverage();
    Worker workers[kWorkers] = {0};
    AdaThread threads[kWorkers];
    for (int i = 0; i < kWorkers; ++i) {
        workers[i].number = i;
        CHECK(__ada_thread_create(&threads[i], exercise, &workers[i]) == 0);
    }
    for (int i = 0; i < kWorkers; ++i) { CHECK(__ada_thread_join(threads[i], NULL) == 0); }
    for (int i = 0; i < kWorkers * kTimedResets; ++i) {
        for (int j = 0; j < i; ++j) {
            CHECK(memcmp(&workers[i / kTimedResets].timed[i % kTimedResets],
                &workers[j / kTimedResets].timed[j % kTimedResets], sizeof(AdaRandomState)) != 0);
        }
    }
    CHECK(__ada_exception == NULL);
    return 0;
}
