#define _POSIX_C_SOURCE 200809L
#include "../runtime/adart.h"
#include "../runtime/adaio.h"
#include <limits.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#define IO_CLOSE _close
#define IO_WRITE _write
#define IO_FDOPEN _fdopen
#else
#include <unistd.h>
#define IO_CLOSE close
#define IO_WRITE write
#define IO_FDOPEN fdopen
#endif

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "wide Text_IO check failed at %d\n", __LINE__); abort(); \
} } while (0)

static void initialize(AdaFile* file, FILE* stream, int width)
{
    memset(file, 0, sizeof *file);
    file->stream = stream;
    file->isOpen = file->text = 1;
    file->textWidth = width;
    file->column = file->line = file->page = 1;
    setvbuf(stream, NULL, _IONBF, 0);
}

static void malformed(const char* bytes, int length, int width, int immediate)
{
    FILE* stream = tmpfile();
    CHECK(stream != NULL);
    CHECK(fwrite(bytes, 1, (size_t)length, stream) == (size_t)length);
    rewind(stream);
    AdaFile file;
    initialize(&file, stream, width);
    AdaFile* handle = &file;
    uint32_t cell = 0;
    int ready;
    __ada_wtext_read_character(&handle, &cell, width, immediate ? 2 : 0, &ready);
    CHECK(__ada_exception == ADA_DATA_ERROR);
    __ada_exception = NULL;
    fclose(stream);
    __ada_mutex_destroy(&file.mutex);
}

static void roundTrip(int width)
{
    AdaFile* owner = NULL;
    __ada_wtext_create(&owner, ADA_MODE_OUT, "", 0, "", 0, width);
    CHECK(__ada_exception == NULL && owner != NULL && owner->textWidth == width);
    uint16_t wide[] = {0x3A9, 0xE9, 0, 0xFFFF};
    uint32_t words[] = {0x3A9, 0xE9, 0, 0x1F600};
    const void* cells = width == 2 ? (const void*)wide : (const void*)words;
    const char* bytes = width == 2 ? "\xCE\xA9\xC3\xA9\0\xEF\xBF\xBF" : "\xCE\xA9\xC3\xA9\0\xF0\x9F\x98\x80";
    int byteCount = width == 2 ? 8 : 9;
    __ada_wtext_write(&owner, cells, 4, width, 1);
    CHECK(__ada_exception == NULL && owner->column == 1 && owner->line == 2);
    CHECK(ftell(owner->stream) == byteCount + 1);
    rewind(owner->stream);
    char actual[16];
    CHECK(fread(actual, 1, (size_t)byteCount + 1, owner->stream) == (size_t)byteCount + 1);
    CHECK(memcmp(actual, bytes, (size_t)byteCount) == 0 && actual[byteCount] == '\n');
    __ada_file_reset(&owner, ADA_MODE_IN);
    uint32_t buffer[8] = {0};
    int count;
    __ada_wtext_read_line(&owner, buffer, 4, &count, width);
    CHECK(__ada_exception == NULL && count == 4 && memcmp(buffer, cells, (size_t)4 * width) == 0);
    CHECK(__ada_text_end_of_line(&owner));
    __ada_wtext_read_line(&owner, buffer, 4, &count, width);
    CHECK(__ada_exception == NULL && count == 0);
    AdaArrayResult result = {0};
    __ada_file_reset(&owner, ADA_MODE_IN);
    __ada_wtext_line(&result, &owner, width);
    CHECK(__ada_exception == NULL && result.first == 1 && result.length == 4);
    CHECK(memcmp(result.data, cells, (size_t)4 * width) == 0);
    free(result.data);
    AdaFile* borrowed = owner;
    __ada_file_finalize(&borrowed);
    CHECK(__ada_exception == NULL && __ada_file_is_open(&owner));
    __ada_file_finalize(&owner);
    CHECK(__ada_exception == NULL && owner == NULL);
}

static void immediate(void)
{
    int descriptors[2];
#ifdef _WIN32
    CHECK(_pipe(descriptors, 4096, _O_BINARY) == 0);
#else
    CHECK(pipe(descriptors) == 0);
#endif
    AdaFile file;
    initialize(&file, IO_FDOPEN(descriptors[0], "rb"), 4);
    CHECK(file.stream != NULL);
    AdaFile* handle = &file;
    uint32_t cell = 0;
    int ready;
    __ada_wtext_read_character(&handle, &cell, 4, 3, &ready);
    CHECK(__ada_exception == NULL && !ready);
    CHECK(IO_WRITE(descriptors[1], "\xF0\x9F", 2) == 2);
    __ada_wtext_read_character(&handle, &cell, 4, 3, &ready);
    CHECK(__ada_exception == NULL && !ready && file.utf8Count == 2);
    CHECK(IO_WRITE(descriptors[1], "\x98\x80\r", 3) == 3);
    __ada_wtext_read_character(&handle, &cell, 4, 3, &ready);
    CHECK(__ada_exception == NULL && ready && cell == 0x1F600 && file.column == 1);
    __ada_wtext_read_character(&handle, &cell, 4, 2, &ready);
    CHECK(__ada_exception == NULL && ready && cell == '\r' && file.column == 1);
    // A partial immediate read can also be completed by ordinary input.
    CHECK(IO_WRITE(descriptors[1], "\xCE", 1) == 1);
    __ada_wtext_read_character(&handle, &cell, 4, 3, &ready);
    CHECK(__ada_exception == NULL && !ready);
    CHECK(IO_WRITE(descriptors[1], "\xA9\n", 2) == 2);
    __ada_wtext_read_character(&handle, &cell, 4, 0, &ready);
    CHECK(__ada_exception == NULL && ready && cell == 0x3A9 && file.column == 2);
    IO_CLOSE(descriptors[1]);
    fclose(file.stream);
    __ada_mutex_destroy(&file.mutex);
}

static void errorsAndLayout(void)
{
    const char* kInvalid[] = {"\xC0\x80", "\xED\xA0\x80", "\xF4\x90\x80\x80", "\xF0\x9F"};
    const int kLengths[] = {2, 3, 4, 2};
    for (int width = 2; width <= 4; width += 2) {
        for (int i = 0; i < 4; ++i) {
            malformed(kInvalid[i], kLengths[i], width, 0);
            malformed(kInvalid[i], kLengths[i], width, 1);
        }
    }
    malformed("\xF0\x9F\x98\x80", 4, 2, 0);
    AdaFile* file = NULL;
    __ada_wtext_create(&file, ADA_MODE_OUT, "", 0, "", 0, 4);
    uint32_t text[] = {0x3A9, 0x1F600, 'a'};
    __ada_text_set_line_length(&file, 2);
    __ada_wtext_write(&file, text, 3, 4, 2);
    CHECK(__ada_exception == ADA_LAYOUT_ERROR && ftell(file->stream) == 0);
    __ada_exception = NULL;
    __ada_wtext_write(&file, text, 3, 4, 0);
    CHECK(__ada_exception == NULL && file->column == 2 && file->line == 2);
    uint32_t invalid[] = {'x', 0xD800};
    long position = ftell(file->stream);
    __ada_wtext_write(&file, invalid, 2, 4, 0);
    CHECK(__ada_exception == ADA_DATA_ERROR && ftell(file->stream) == position);
    __ada_exception = NULL;
    invalid[1] = INT_MAX;
    __ada_wtext_write(&file, invalid, 2, 4, 0);
    CHECK(__ada_exception == ADA_DATA_ERROR);
    __ada_exception = NULL;
    __ada_wtext_select(&file, 4, 1);
    CHECK(*__ada_wtext_selected(4, 1, 1) == file);
    __ada_file_close(&file);
    CHECK(__ada_exception == NULL && *__ada_wtext_selected(4, 1, 1) == *__ada_wtext_selected(4, 1, 0));
    CHECK(*__ada_wtext_selected(2, 1, 0) != *__ada_wtext_selected(4, 1, 0));
}

static void* worker(void* unused)
{
    (void)unused;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int i = 0; i < 20; ++i) {
        roundTrip(2);
        roundTrip(4);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    immediate();
    errorsAndLayout();
    pthread_t threads[4];
    for (int i = 0; i < 4; ++i) { CHECK(pthread_create(&threads[i], NULL, worker, NULL) == 0); }
    for (int i = 0; i < 4; ++i) { CHECK(pthread_join(threads[i], NULL) == 0); }
    return 0;
}
