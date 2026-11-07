#include "../runtime/adart.h"
#include "../runtime/adaio.h"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/stat.h>
#include <unistd.h>
#endif

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "stream operations check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kWorkers = 4, kBlockSize = 64, kIterations = 200 };
static AdaFile* shared;
static AdaTag streamTag;

typedef struct Worker
{
    int number;
    int reading;
} Worker;

static void expectError(const AdaException* expected, int line)
{
    if (__ada_exception != expected) {
        fprintf(stderr, "stream operations check failed at line %d: expected %s, got %s\n",
            line, expected->name, __ada_exception != NULL ? __ada_exception->name : "no exception");
        abort();
    }
    __ada_exception = NULL;
}

#define EXPECT_ERROR(expected) expectError((expected), __LINE__)

static void* exercise(void* argument)
{
    Worker* worker = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    unsigned char bytes[kBlockSize];
    memset(bytes, worker->number + 1, sizeof bytes);
    int index = worker->number * kBlockSize + 1;
    for (int i = 0; i < kIterations; ++i) {
        if (worker->reading) {
            int last = -99;
            __ada_stream_reset(&shared, ADA_MODE_IN);
            __ada_stream_elements_read_at(&shared, bytes, sizeof bytes, -7, &last, index);
            CHECK(last == -7 + kBlockSize - 1);
            for (int j = 0; j < kBlockSize; ++j) { CHECK(bytes[j] == worker->number + 1); }
            CHECK(__ada_stream_size(&shared) == kWorkers * kBlockSize);
        } else {
            __ada_stream_set_mode(&shared, i % 2 ? ADA_MODE_APPEND : ADA_MODE_OUT);
            __ada_stream_elements_write_at(&shared, bytes, sizeof bytes, index);
            __ada_text_flush(&shared);
        }
        CHECK(__ada_file_stream_view(&shared, &streamTag)->file == shared);
        CHECK(__ada_exception == NULL);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

static void concurrency(void)
{
    __ada_stream_create(&shared, ADA_MODE_OUT, "", 0, "", 0);
    CHECK(shared != NULL && __ada_exception == NULL);
    AdaFileStream* view = __ada_file_stream_view(&shared, &streamTag);
    Worker workers[kWorkers];
    AdaThread threads[kWorkers];
    for (int phase = 0; phase < 2; ++phase) {
        __ada_stream_set_mode(&shared, phase ? ADA_MODE_IN : ADA_MODE_OUT);
        for (int i = 0; i < kWorkers; ++i) {
            workers[i] = (Worker){i, phase};
            CHECK(__ada_thread_create(&threads[i], exercise, &workers[i]) == 0);
        }
        for (int i = 0; i < kWorkers; ++i) { CHECK(__ada_thread_join(threads[i], NULL) == 0); }
    }
    CHECK(__ada_file_stream_view(&shared, &streamTag) == view);
    AdaFile* borrowed = shared;
    __ada_file_finalize(&borrowed);
    CHECK(__ada_file_is_open(&shared));
    __ada_stream_reset(&borrowed, ADA_MODE_OUT);
    CHECK(__ada_stream_index(&shared) == 1);
    __ada_file_finalize(&shared);
    CHECK(shared == NULL);
    unsigned char byte;
    int last;
    __ada_file_stream_elements_read(view->file, &byte, 1, 1, &last);
    EXPECT_ERROR(ADA_STATUS_ERROR);
    __ada_file_finalize(&borrowed);
    CHECK(__ada_exception == NULL);
}

static void identity(void)
{
    const char* name = "stream-operations-native.tmp";
#ifdef _WIN32
    const char* renamed = name;
#else
    const char* renamed = "stream-operations-renamed.tmp";
#endif
    AdaFile* file = NULL;
    __ada_stream_create(&file, ADA_MODE_OUT, name, (int)strlen(name), "", 0);
    __ada_stream_elements_write(&file, "abc", 3);
    CHECK(__ada_exception == NULL);
#ifndef _WIN32
    CHECK(rename(name, renamed) == 0);
#endif
    AdaFileStream* view = __ada_file_stream_view(&file, &streamTag);
    __ada_stream_set_mode(&file, ADA_MODE_IN);
    CHECK(__ada_stream_index(&file) == 4);
    __ada_stream_reset_same(&file);
    char bytes[3];
    int last;
    __ada_file_stream_elements_read(view->file, bytes, 3, 1, &last);
    CHECK(last == 3 && memcmp(bytes, "abc", 3) == 0);
    __ada_stream_set_mode(&file, ADA_MODE_OUT);
    __ada_file_stream_elements_write(view->file, "d", 1);
    __ada_stream_reset_same(&file);
    CHECK(__ada_stream_size(&file) == 4 && __ada_stream_index(&file) == 1);
    __ada_file_close(&file);
    CHECK(file == NULL && __ada_exception == NULL && remove(renamed) == 0);
}

static void boundaries(void)
{
    AdaFile* file = NULL;
    unsigned char bytes[2] = {71, 72};
    __ada_stream_create(&file, ADA_MODE_OUT, "", 0, "", 0);
    __ada_stream_elements_write_at(&file, bytes, 2, INT_MAX);
    EXPECT_ERROR(ADA_CONSTRAINT_ERROR);
    CHECK(__ada_stream_index(&file) == INT_MAX && __ada_stream_size(&file) == 0);
    __ada_stream_write(file, bytes, 2);
    EXPECT_ERROR(ADA_CONSTRAINT_ERROR);
    __ada_stream_elements_write_at(&file, bytes, 1, INT_MAX);
    CHECK(__ada_exception == NULL && __ada_stream_size(&file) == INT_MAX);
    __ada_stream_index(&file);
    EXPECT_ERROR(ADA_CONSTRAINT_ERROR);
    __ada_stream_reset(&file, ADA_MODE_IN);
    int last = 123;
    __ada_stream_elements_read_at(&file, bytes, 1, -2, &last, INT_MAX);
    CHECK(last == -2 && bytes[0] == 71 && __ada_exception == NULL);
    __ada_stream_set_index(&file, 0);
    EXPECT_ERROR(ADA_CONSTRAINT_ERROR);
    __ada_stream_set_index(&file, 1);
    __ada_stream_elements_read(&file, NULL, 0, INT_MIN, &last);
    EXPECT_ERROR(ADA_CONSTRAINT_ERROR);
    CHECK(last == -2 && __ada_stream_index(&file) == 1);
    __ada_stream_elements_read(&file, bytes, 2, INT_MAX, &last);
    EXPECT_ERROR(ADA_CONSTRAINT_ERROR);
    CHECK(last == -2);
    __ada_stream_elements_read_at(&file, NULL, 0, -3, &last, 5);
    CHECK(last == -4 && __ada_stream_index(&file) == 5);
    __ada_stream_set_mode(&file, 3);
    EXPECT_ERROR(ADA_CONSTRAINT_ERROR);
    CHECK(__ada_file_mode(&file) == ADA_MODE_IN);
#ifndef _WIN32
    CHECK(ftruncate(fileno(file->stream), (off_t)INT_MAX + 1) == 0);
    __ada_stream_size(&file);
    EXPECT_ERROR(ADA_CONSTRAINT_ERROR);
    CHECK(__ada_stream_index(&file) == 5);
#endif
    __ada_file_close(&file);
    CHECK(file == NULL && __ada_exception == NULL);
    __ada_stream_elements_read_at(&file, bytes, 1, 1, &last, 1);
    EXPECT_ERROR(ADA_STATUS_ERROR);
    __ada_stream_elements_write_at(&file, bytes, 1, 1);
    EXPECT_ERROR(ADA_STATUS_ERROR);
    __ada_stream_reset_same(&file);
    EXPECT_ERROR(ADA_STATUS_ERROR);
}

#ifndef _WIN32
static void nonpositionable(void)
{
    int descriptors[2];
    CHECK(pipe(descriptors) == 0);
    CHECK(write(descriptors[1], "abc", 3) == 3);
    CHECK(close(descriptors[1]) == 0);
    AdaFile source = {.stream = fdopen(descriptors[0], "rb"), .isOpen = 1,
        .mode = ADA_MODE_IN, .streamIo = 1, .canRead = 1};
    CHECK(source.stream != NULL);
    AdaFile* file = &source;
    unsigned char bytes[4];
    int last = 99;
    __ada_stream_index(&file);
    EXPECT_ERROR(ADA_USE_ERROR);
    __ada_stream_size(&file);
    EXPECT_ERROR(ADA_USE_ERROR);
    __ada_stream_elements_read_at(&file, bytes, 3, 1, &last, 1);
    EXPECT_ERROR(ADA_USE_ERROR);
    CHECK(last == 99);
    __ada_stream_elements_read_at(&file, NULL, 0, 1, &last, 1);
    EXPECT_ERROR(ADA_USE_ERROR);
    __ada_stream_set_index(&file, 1);
    EXPECT_ERROR(ADA_USE_ERROR);
    __ada_stream_elements_read(&file, bytes, 4, 1, &last);
    CHECK(last == 3 && memcmp(bytes, "abc", 3) == 0 && __ada_exception == NULL);
    __ada_stream_set_mode(&file, ADA_MODE_OUT);
    EXPECT_ERROR(ADA_USE_ERROR);
    CHECK(__ada_file_mode(&file) == ADA_MODE_IN);
    __ada_stream_reset_same(&file);
    EXPECT_ERROR(ADA_USE_ERROR);
    __ada_file_close(&file);
    CHECK(__ada_mutex_destroy(&source.mutex) == 0);

    CHECK(pipe(descriptors) == 0);
    AdaFile target = {.stream = fdopen(descriptors[1], "wb"), .isOpen = 1,
        .mode = ADA_MODE_OUT, .streamIo = 1, .canWrite = 1};
    file = &target;
    __ada_stream_elements_write_at(&file, "z", 1, 1);
    EXPECT_ERROR(ADA_USE_ERROR);
    __ada_stream_elements_write_at(&file, NULL, 0, 1);
    EXPECT_ERROR(ADA_USE_ERROR);
    __ada_stream_set_mode(&file, ADA_MODE_APPEND);
    __ada_stream_elements_write(&file, "xyz", 3);
    __ada_text_flush(&file);
    CHECK(__ada_exception == NULL);
    CHECK(read(descriptors[0], bytes, 3) == 3 && memcmp(bytes, "xyz", 3) == 0);
    close(descriptors[0]);
    __ada_file_close(&file);
    CHECK(__ada_mutex_destroy(&target.mutex) == 0);
}

static void deviceErrors(void)
{
    AdaFile* file = NULL;
    char buffer[BUFSIZ];
    __ada_stream_create(&file, ADA_MODE_OUT, "", 0, "", 0);
    // A NULL buffer can retain glibc's one-byte unbuffered storage. Supply
    // storage explicitly so the pending write makes Flush exercise the bad fd.
    CHECK(setvbuf(file->stream, buffer, _IOFBF, sizeof buffer) == 0);
    __ada_stream_elements_write(&file, "x", 1);
    CHECK(__ada_exception == NULL);
    struct stat status;
    CHECK(fstat(fileno(file->stream), &status) == 0 && status.st_size == 0);
    CHECK(close(fileno(file->stream)) == 0);
    __ada_text_flush(&file);
    EXPECT_ERROR(ADA_DEVICE_ERROR);
    __ada_file_close(&file);
    EXPECT_ERROR(ADA_DEVICE_ERROR);
    CHECK(file == NULL);
}
#endif

int main(void)
{
    concurrency();
    identity();
    boundaries();
#ifndef _WIN32
    nonpositionable();
    deviceErrors();
#endif
    CHECK(__ada_exception == NULL);
    return 0;
}
