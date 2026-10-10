/* Track runtime allocation failures and leaks independently on each worker.
   libc FILE storage is deliberately outside the runtime allocation counter. */
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

static _Thread_local int liveAllocations;
static _Thread_local int failAfter = -1;

static void* trackedMalloc(size_t size)
{
    if (failAfter == 0) {
        failAfter = -1;
        return NULL;
    }
    if (failAfter > 0) {
        --failAfter;
    }
    void* result = malloc(size);
    liveAllocations += result != NULL;
    return result;
}

static void* trackedCalloc(size_t count, size_t size)
{
    void* result = trackedMalloc(count * size);
    if (result != NULL) {
        memset(result, 0, count * size);
    }
    return result;
}

static void trackedFree(void* pointer)
{
    liveAllocations -= pointer != NULL;
    free(pointer);
}

#define malloc trackedMalloc
#define calloc trackedCalloc
#define free trackedFree
#include "../runtime/adart.c"
#include "../runtime/adatags.c"
#include "../runtime/adafixed.c"
#include "../runtime/adaio.c"
#undef malloc
#undef calloc
#undef free

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "result buffer check failed at line %d\n", __LINE__); exit(1); \
} } while (0)

static void checkImages(void)
{
    char results[6][16][128];
    int lengths[6][16];
    const char* names[] = { "mixed_case_enumeration_literal_longer_than_sixty_four_characters_without_any_truncation" };
    for (int i = 0; i < 16; ++i) {
        lengths[0][i] = __ada_image_integer(results[0][i], 128, i);
        lengths[1][i] = __ada_image_long_integer(results[1][i], 128, LLONG_MAX - i);
        lengths[2][i] = __ada_image_float(results[2][i], 128, i, 2, 3);
        lengths[3][i] = __ada_image_fixed(results[3][i], 128, i * 8, 3, 3);
        lengths[4][i] = __ada_image_character(results[4][i], 128, i);
        lengths[5][i] = __ada_image_enum(results[5][i], 128, 0, names, 1);
    }
    CHECK(__ada_exception == NULL && liveAllocations == 0);
    for (int i = 0; i < 16; ++i) {
        char expected[128];
        snprintf(expected, sizeof expected, " %d", i);
        CHECK(strcmp(results[0][i], expected) == 0);
        snprintf(expected, sizeof expected, " %lld", LLONG_MAX - i);
        CHECK(strcmp(results[1][i], expected) == 0);
        __ada_format_float(expected, sizeof expected, i, 2, 2, 3);
        CHECK(strcmp(results[2][i], expected) == 0);
        snprintf(expected, sizeof expected, " %d.000", i);
        CHECK(strcmp(results[3][i], expected) == 0);
        CHECK(lengths[4][i] == 3 && results[4][i][1] == i && results[4][i][2] == '\'');
        CHECK(strcmp(results[5][i], "MIXED_CASE_ENUMERATION_LITERAL_LONGER_THAN_SIXTY_FOUR_CHARACTERS_WITHOUT_ANY_TRUNCATION") == 0);
        for (int kind = 0; kind < 6; ++kind) {
            CHECK(results[kind][i][lengths[kind][i]] == '\0');
        }
    }
    char small[4] = { '#', '#', '#', '#' };
    CHECK(__ada_image_integer(small + 1, 2, 123) == 0 && __ada_exception == ADA_STORAGE_ERROR);
    CHECK(small[0] == '#' && small[3] == '#');
    __ada_exception = NULL;
    CHECK(__ada_image_enum(small, sizeof small, 1, names, 1) == 0 && __ada_exception == ADA_CONSTRAINT_ERROR);
    __ada_exception = NULL;
    CHECK(__ada_image_fixed(small, sizeof small, 1, 0, 10) == 0 && __ada_exception == ADA_STORAGE_ERROR);
    __ada_exception = NULL;
    CHECK(liveAllocations == 0);
}

static void checkStreams(void)
{
    AdaFile file = { 0 };
    file.stream = tmpfile();
    CHECK(file.stream != NULL);
    file.mode = ADA_MODE_INOUT;
    file.isOpen = 1;
    char payload[8192];
    for (int i = 0; i < 12; ++i) {
        memset(payload, 'a' + i, sizeof payload);
        __ada_stream_write_bounds(&file, 5, 8196);
        __ada_stream_write(&file, payload, sizeof payload);
    }
    __ada_stream_write_bounds(&file, INT_MAX, INT_MIN);
    CHECK(__ada_exception == NULL);
    rewind(file.stream);
    AdaArrayResult results[13];
    void* owner = NULL;
    for (int i = 0; i < 13; ++i) {
        __ada_stream_read_array(&results[i], &file, 1);
        CHECK(__ada_exception == NULL && results[i].m_data != NULL);
        __ada_array_adopt(&owner, results[i].m_data);
    }
    for (int i = 0; i < 12; ++i) {
        CHECK(results[i].m_first == 5 && results[i].m_last == 8196 && results[i].m_size == sizeof payload);
        for (int j = 0; j < (int)sizeof payload; ++j) {
            CHECK(((char*)results[i].m_data)[j] == 'a' + i);
        }
    }
    CHECK(results[12].m_first == INT_MAX && results[12].m_last == INT_MIN);
    __ada_array_release(&owner);
    CHECK(liveAllocations == 0);

    /* Fail both allocation and caller adoption. Neither can leak a transfer. */
    for (int failure = 0; failure < 2; ++failure) {
        rewind(file.stream);
        failAfter = failure;
        AdaArrayResult result;
        __ada_stream_read_array(&result, &file, 1);
        if (__ada_exception == NULL) {
            __ada_array_adopt(&owner, result.m_data);
        } else {
            CHECK(result.m_data == NULL);
        }
        CHECK(__ada_exception == ADA_STORAGE_ERROR && owner == NULL && liveAllocations == 0);
        __ada_exception = NULL;
    }
    fclose(file.stream);

    /* Truncated payload, truncated bounds, and oversized byte counts must
       publish no result and leave no orphaned allocation. */
    for (int scenario = 0; scenario < 3; ++scenario) {
        file.stream = tmpfile();
        CHECK(file.stream != NULL);
        int first = 1;
        __ada_stream_write(&file, &first, sizeof first);
        if (scenario != 1) {
            int last = scenario == 0 ? 20 : INT_MAX;
            __ada_stream_write(&file, &last, sizeof last);
        }
        rewind(file.stream);
        AdaArrayResult result;
        __ada_stream_read_array(&result, &file, scenario == 2 ? 8 : 1);
        CHECK(result.m_data == NULL && liveAllocations == 0);
        CHECK(__ada_exception == (scenario == 2 ? ADA_STORAGE_ERROR : ADA_END_ERROR));
        __ada_exception = NULL;
        fclose(file.stream);
    }
    CHECK(pthread_mutex_destroy(&file.m_mutex) == 0);
}

static void* worker(void* argument)
{
    (void)argument;
    AdaTaskContext context = { 0 };
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int i = 0; i < 20; ++i) {
        checkImages();
        checkStreams();
    }
    CHECK(__ada_task_context_dispose(&context) && liveAllocations == 0);
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    pthread_t workers[4];
    for (int i = 0; i < 4; ++i) {
        CHECK(pthread_create(&workers[i], NULL, worker, NULL) == 0);
    }
    for (int i = 0; i < 4; ++i) {
        CHECK(pthread_join(workers[i], NULL) == 0);
    }
    return 0;
}
