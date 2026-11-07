#include "../runtime/adart.h"
#include "../runtime/adaio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#endif

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "text streams check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kWorkers = 4, kIterations = 30 };
static AdaTag streamTag;

static void error(const AdaException* expected)
{
    CHECK(__ada_exception == expected);
    __ada_exception = NULL;
}

static int readCode(AdaFile** file, int width, int operation)
{
    uint32_t value = 0;
    int available = 0;
    if (width == 1) {
        char byte = 0;
        if (operation == 0) { __ada_text_get(file, &byte); }
        else { __ada_text_get_immediate(file, &byte); }
        return (unsigned char)byte;
    }
    __ada_wtext_read_character(file, &value, width, operation, &available);
    CHECK(available || __ada_exception != NULL);
    uint16_t narrow;
    memcpy(&narrow, &value, sizeof narrow);
    return width == 2 ? narrow : (int)value;
}

static void replay(int width)
{
    AdaFile* file = NULL;
    __ada_wtext_create(&file, ADA_MODE_OUT, "", 0, "", 0, width);
    CHECK(file != NULL && __ada_exception == NULL);
    AdaFileStream* view = __ada_file_stream_view(&file, &streamTag);
    const unsigned char encoded[] = {0xF0, 0x9F, 0x99, 0x82};
    const unsigned char small[] = {0xC3, 0xA9};
    const unsigned char narrow[] = {233};
    const unsigned char* character = width == 1 ? narrow : width == 2 ? small : encoded;
    int count = width == 1 ? 1 : width;
    __ada_file_stream_elements_write(file, "\r\n\f", 3);
    __ada_file_stream_elements_write(file, character, count);
    __ada_file_stream_elements_write(file, "B\rC\n", 4);
    CHECK(__ada_text_col(&file) == 1 && __ada_text_line_number(&file) == 1 && __ada_text_page(&file) == 1);
    __ada_file_reset(&file, ADA_MODE_IN);
    CHECK(__ada_file_stream_view(&file, &streamTag) == view);
    CHECK(!__ada_file_end_of_file(&file));
    CHECK(file->pendingCount == 3);
    int last = 99;
    unsigned char bytes[8] = {0};
    __ada_file_stream_elements_read(file, NULL, 0, 5, &last);
    CHECK(last == 4 && file->pendingCount == 3);
    __ada_file_stream_elements_read(file, bytes, 1, -2, &last);
    CHECK(last == -2 && bytes[0] == '\r');
    // A raw CR leaves the LF for normal text input; only the latter counts.
    CHECK(__ada_text_end_of_line(&file));
    __ada_io_begin(&file);
    __ada_io_unlock();
    CHECK(__ada_text_take(file) == '\n');
    __ada_io_lock();
    __ada_io_end();
    CHECK(__ada_text_line_number(&file) == 2 && __ada_text_page(&file) == 1);
    __ada_file_stream_elements_read(file, bytes, 1, 1, &last);
    CHECK(bytes[0] == '\f');
    __ada_file_stream_elements_read(file, bytes, count, 1, &last);
    CHECK(last == count && memcmp(bytes, character, (size_t)count) == 0);
    CHECK(__ada_text_col(&file) == 1 && __ada_text_line_number(&file) == 2 && __ada_text_page(&file) == 1);
    CHECK(readCode(&file, width, 0) == 'B');
    CHECK(__ada_text_end_of_line(&file));
    __ada_file_stream_elements_read(file, bytes, 1, 1, &last);
    CHECK(bytes[0] == '\r');
    CHECK(readCode(&file, width, 2) == 'C');
    CHECK(__ada_text_col(&file) == 2 && __ada_text_line_number(&file) == 2);
    __ada_file_stream_elements_write(file, NULL, 0);
    error(ADA_MODE_ERROR);
    AdaFile* borrowed = file;
    __ada_file_finalize(&borrowed);
    CHECK(__ada_file_is_open(&file));
    __ada_file_close(&file);
    CHECK(__ada_exception == NULL);
}

static void closedView(void)
{
    AdaFile* file = NULL;
    __ada_create(&file, ADA_MODE_OUT, "", 0, "", 0);
    AdaFileStream* view = __ada_file_stream_view(&file, &streamTag);
    __ada_file_close(&file);
    char byte;
    int last;
    // No other worker can reopen this slot during the stronger Status_Error check.
    __ada_file_stream_elements_read(view->file, &byte, 1, 1, &last);
    error(ADA_STATUS_ERROR);
}

static void termination(void)
{
    AdaFile* file = NULL;
    __ada_create(&file, ADA_MODE_OUT, "", 0, "", 0);
    __ada_text_new_page(&file);
    CHECK(__ada_text_page(&file) == 2);
    __ada_file_stream_elements_write(file, "x", 1);
    CHECK(__ada_text_page(&file) == 2 && __ada_text_col(&file) == 1);
    __ada_file_reset(&file, ADA_MODE_IN);
    char bytes[5];
    int last;
    __ada_file_stream_elements_read(file, bytes, sizeof bytes, 1, &last);
    CHECK(last == 5 && memcmp(bytes, "\n\fx\n\f", 5) == 0);
    __ada_file_close(&file);
    CHECK(__ada_exception == NULL);
}

static void invalidUnicode(void)
{
    AdaFile* file = NULL;
    const unsigned char bytes[] = {0xF0, 0x9F, 0x99, 0x82};
    __ada_wtext_create(&file, ADA_MODE_OUT, "", 0, "", 0, 2);
    __ada_file_stream_elements_write(file, bytes, sizeof bytes);
    __ada_file_reset(&file, ADA_MODE_IN);
    uint16_t code = 0;
    int available;
    __ada_wtext_read_character(&file, &code, 2, 1, &available);
    error(ADA_DATA_ERROR);
    unsigned char recovered[4];
    int last;
    __ada_file_stream_elements_read(file, recovered, sizeof recovered, 1, &last);
    CHECK(last == 4 && memcmp(bytes, recovered, sizeof bytes) == 0);
    CHECK(__ada_text_col(&file) == 1 && __ada_text_line_number(&file) == 1);
    __ada_file_close(&file);
    CHECK(__ada_exception == NULL);
}

static void* worker(void* argument)
{
    (void)argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int i = 0; i < kIterations; ++i) {
        replay(1);
        replay(2);
        replay(4);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

typedef struct Shared
{
    AdaFile* file;
    int width;
} Shared;

static void* sharedWriter(void* argument)
{
    Shared* shared = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    int code = shared->width == 1 ? 'X' : shared->width == 2 ? 233 : 0x1F642;
    for (int i = 0; i < kIterations; ++i) {
        __ada_file_stream_elements_write(shared->file, "raw!", 4);
        __ada_wtext_put_character(&shared->file, code, shared->width);
        CHECK(__ada_exception == NULL);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

static void sharedFile(int width)
{
    Shared shared = {.width = width};
    __ada_wtext_create(&shared.file, ADA_MODE_OUT, "", 0, "", 0, width);
    AdaThread threads[kWorkers];
    for (int i = 0; i < kWorkers; ++i) { CHECK(__ada_thread_create(&threads[i], sharedWriter, &shared) == 0); }
    for (int i = 0; i < kWorkers; ++i) { CHECK(__ada_thread_join(threads[i], NULL) == 0); }
    CHECK(__ada_text_col(&shared.file) == 1 + kWorkers * kIterations);
    CHECK(__ada_text_line_number(&shared.file) == 1 && __ada_text_page(&shared.file) == 1);
    __ada_file_reset(&shared.file, ADA_MODE_IN);
    unsigned char text[4] = {'X', 0, 0, 0};
    int count = width == 1 ? 1 : width;
    if (width == 2) { memcpy(text, "\xC3\xA9", 2); }
    if (width == 4) { memcpy(text, "\xF0\x9F\x99\x82", 4); }
    unsigned char bytes[kWorkers * kIterations * 8];
    int length = kWorkers * kIterations * (4 + count), last;
    __ada_file_stream_elements_read(shared.file, bytes, length, 1, &last);
    CHECK(last == length);
    int raw = 0, characters = 0;
    for (int i = 0; i < length;) {
        if (bytes[i] == 'r') {
            CHECK(i + 4 <= length && memcmp(bytes + i, "raw!", 4) == 0);
            i += 4;
            ++raw;
        } else {
            CHECK(i + count <= length && memcmp(bytes + i, text, (size_t)count) == 0);
            i += count;
            ++characters;
        }
    }
    CHECK(raw == kWorkers * kIterations && characters == raw);
    CHECK(__ada_text_col(&shared.file) == 1 && __ada_text_line_number(&shared.file) == 1);
    __ada_file_close(&shared.file);
    CHECK(__ada_exception == NULL);
}

#ifndef _WIN32
static void partialPrefix(void)
{
    for (int width = 2; width <= 4; width += 2) {
        int descriptors[2];
        CHECK(pipe(descriptors) == 0);
        AdaFile source = {.stream = fdopen(descriptors[0], "rb"), .isOpen = 1,
            .mode = ADA_MODE_IN, .text = 1, .textWidth = width, .column = 1, .line = 1, .page = 1};
        AdaFile* file = &source;
        CHECK(write(descriptors[1], "\xC3", 1) == 1);
        uint32_t value = 0;
        int available = 1;
        __ada_wtext_read_character(&file, &value, width, 3, &available);
        CHECK(!available && source.utf8Count == 1 && __ada_exception == NULL);
        int last = 0;
        __ada_file_stream_elements_read(file, NULL, 0, 1, &last);
        CHECK(last == 0 && source.utf8Count == 1);
        CHECK(write(descriptors[1], "\xA9R", 2) == 2);
        unsigned char bytes[2];
        __ada_file_stream_elements_read(file, bytes, 2, 1, &last);
        CHECK(last == 2 && bytes[0] == 0xC3 && bytes[1] == 0xA9 && source.utf8Count == 0);
        CHECK(readCode(&file, width, 0) == 'R');
        CHECK(close(descriptors[1]) == 0);
        __ada_file_stream_elements_read(file, bytes, 2, 1, &last);
        CHECK(last == 0 && __ada_exception == NULL);
        __ada_file_close(&file);
        CHECK(__ada_mutex_destroy(&source.mutex) == 0);
    }
}
#endif

int main(void)
{
    AdaThread threads[kWorkers];
    for (int i = 0; i < kWorkers; ++i) { CHECK(__ada_thread_create(&threads[i], worker, NULL) == 0); }
    for (int i = 0; i < kWorkers; ++i) { CHECK(__ada_thread_join(threads[i], NULL) == 0); }
#ifndef _WIN32
    partialPrefix();
#endif
    sharedFile(1);
    sharedFile(2);
    sharedFile(4);
    closedView();
    termination();
    invalidUnicode();
    CHECK(__ada_exception == NULL);
    return 0;
}
