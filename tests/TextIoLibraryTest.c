#define _POSIX_C_SOURCE 200809L
#include "../runtime/adart.h"
#include "../runtime/adaio.h"
#include <limits.h>
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
    fprintf(stderr, "Text_IO check failed at line %d\n", __LINE__); exit(1); \
} } while (0)

static void initialize(AdaFile* file, FILE* stream)
{
    memset(file, 0, sizeof *file);
    file->stream = stream;
    file->isOpen = 1;
    file->text = 1;
    file->column = file->line = file->page = 1;
    setvbuf(stream, NULL, _IONBF, 0);
}

static void immediateInput(void)
{
    int descriptors[2];
#ifdef _WIN32
    CHECK(_pipe(descriptors, 4096, _O_BINARY) == 0);
#else
    CHECK(pipe(descriptors) == 0);
#endif
    AdaFile file;
    initialize(&file, IO_FDOPEN(descriptors[0], "rb"));
    CHECK(file.stream != NULL);
    AdaFile* handle = &file;
    char item = '?';
    int available = -1;
    __ada_text_get_available(&handle, &item, &available);
    CHECK(__ada_exception == NULL && available == 0);
    CHECK(IO_WRITE(descriptors[1], "\r", 1) == 1);
    __ada_text_get_available(&handle, &item, &available);
    CHECK(__ada_exception == NULL && available == 1 && item == '\r');
    CHECK(file.column == 1 && file.line == 1 && file.page == 1);
    IO_CLOSE(descriptors[1]);
    __ada_text_get_available(&handle, &item, &available);
    CHECK(__ada_exception == ADA_END_ERROR);
    __ada_exception = NULL;
    fclose(file.stream);
    __ada_mutex_destroy(&file.mutex);
}

static void lineAndPosition(void)
{
    FILE* stream = tmpfile();
    CHECK(stream != NULL);
    AdaFile file;
    initialize(&file, stream);
    CHECK(fwrite("abc", 1, 3, stream) == 3);
    rewind(stream);
    AdaFile* handle = &file;
    char item[4] = {0};
    int last = -1;
    __ada_text_get_line(&handle, item, 3, &last);
    CHECK(__ada_exception == NULL && last == 3 && memcmp(item, "abc", 3) == 0);
    __ada_text_get_line(&handle, item, 3, &last);
    CHECK(__ada_exception == ADA_END_ERROR);
    __ada_exception = NULL;
    file.column = (uint64_t)INT_MAX + 1;
    __ada_text_col(&handle);
    CHECK(__ada_exception == ADA_LAYOUT_ERROR);
    __ada_exception = NULL;
    fclose(stream);
    __ada_mutex_destroy(&file.mutex);
}

static void ownerAndBorrow(void)
{
    AdaFile* owner = NULL;
    __ada_create(&owner, ADA_MODE_OUT, "", 0, "ignored=yes", 11);
    CHECK(__ada_exception == ADA_USE_ERROR && owner == NULL);
    __ada_exception = NULL;
    __ada_open(&owner, ADA_MODE_IN, "bad\0name", 8, "", 0);
    CHECK(__ada_exception == ADA_NAME_ERROR && owner == NULL);
    __ada_exception = NULL;
    __ada_create(&owner, ADA_MODE_OUT, "", 0, "", 0);
    CHECK(__ada_exception == NULL && owner != NULL);
    AdaFile* borrowed = owner;
    __ada_file_finalize(&borrowed);
    CHECK(__ada_exception == NULL && __ada_file_is_open(&owner));
    __ada_file_finalize(&owner);
    CHECK(__ada_exception == NULL && owner == NULL && !__ada_file_is_open(&borrowed));
    __ada_file_finalize(&owner);
    CHECK(__ada_exception == NULL);
}

int main(void)
{
    immediateInput();
    lineAndPosition();
    ownerAndBorrow();
    return 0;
}
