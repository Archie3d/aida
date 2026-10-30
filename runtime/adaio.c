#include "adaiotoken.h"
/* File handling shared by Text_IO, Sequential_IO, Direct_IO and Stream_IO.

   An Ada File_Type is a one component record holding a pointer into the pool
   below, so the generated code only ever passes the address of that record and
   never needs to know what a file looks like. */

#include "adaio.h"
#include "adart.h"
#include "adalock.h"

#include <ctype.h>
#include <errno.h>
#ifdef _WIN32
#include <conio.h>
#include <windows.h>
#include <io.h>
#else
#include <poll.h>
#include <termios.h>
#include <unistd.h>
#endif
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define ADA_MAX_FILES 32

/* Public operations pin a file and hold its mutex for validation and use.
   Only table/selection mutations retain the table lock; their blocking system
   calls drop it. Impl calls preserve the enclosing operation's ownership. */
#define ADA_IO_VOID(name, parameters, arguments, handle, shared) static void name##Impl parameters;
#define ADA_IO_VALUE(type, name, parameters, arguments, handle, shared) static type name##Impl parameters;
#include "adaio_entries.h"
#undef ADA_IO_VOID
#undef ADA_IO_VALUE

static AdaFile filePool[ADA_MAX_FILES];
static AdaFile standardInput;
static AdaFile standardOutput;
static AdaFile standardError;
static int poolPrepared = 0;
static AdaMutex ioMutex = ADA_MUTEX_INITIALIZER;
static _Thread_local AdaFile* activeFile;
static void prepare(void);

void __ada_io_lock(void)
{
    __ada_mutex_lock(&ioMutex);
}

void __ada_io_unlock(void)
{
    __ada_mutex_unlock(&ioMutex);
}

/* Begin returns with the table held; End requires it held. Never wait for
   a file mutex while holding it. Pins keep a closed pool slot unavailable until every waiter has left. */
void __ada_io_begin(AdaFileRef handle)
{
    __ada_io_lock();
    prepare();
    for (;;) {
        AdaFile* file = handle != NULL ? *handle : NULL;
        activeFile = file;
        if (file == NULL) { return; }
        if (!file->mutexReady) {
            if (pthread_mutex_init(&file->mutex, NULL) != 0) { abort(); }
            file->mutexReady = 1;
        }
        ++file->users;
        __ada_io_unlock();
        __ada_mutex_lock(&file->mutex);
        __ada_io_lock();
        if (*handle == file) { return; }
        --file->users;
        __ada_mutex_unlock(&file->mutex);
    }
}

void __ada_io_end(void)
{
    if (activeFile != NULL) {
        --activeFile->users;
        __ada_mutex_unlock(&activeFile->mutex);
        activeFile = NULL;
    }
    __ada_io_unlock();
}

/* Table-changing operations use these adapters for potentially blocking
   system calls. The file mutex and pin remain held. Ordinary reads/writes
   release the table lock for their entire implementation. */
#define IO_CALL(type, name, parameters, arguments) \
static type io##name parameters \
{ \
    __ada_io_unlock(); \
    type result = name arguments; \
    __ada_io_lock(); \
    return result; \
}
IO_CALL(int, fclose, (FILE* file), (file))
IO_CALL(int, fflush, (FILE* file), (file))
IO_CALL(FILE*, tmpfile, (void), ())
IO_CALL(FILE*, fopen, (const char* path, const char* mode), (path, mode))
IO_CALL(FILE*, freopen, (const char* path, const char* mode, FILE* file), (path, mode, file))
IO_CALL(int, remove, (const char* path), (path))
#undef IO_CALL

static void iorewind(FILE* file)
{
    __ada_io_unlock();
    rewind(file);
    __ada_io_lock();
}

/* Each of these is the single component of a File_Type record, which is why
   their addresses can be handed straight to the generated code. */
static AdaFile* standardInputHandle;
static AdaFile* standardOutputHandle;
static AdaFile* standardErrorHandle;
static AdaFile* currentInputHandle;
static AdaFile* currentOutputHandle;
#define CURRENT_ERROR_HANDLE (__ada_main_context.currentErrorFile)
static void textNewLine(AdaFile* file);
static void textNewPage(AdaFile* file);

/* Logical text lookahead has its own queue: ISO C promises only one byte of
   ungetc, while the Ada end-of-page/file predicates need several characters. */
int __ada_text_peek(AdaFile* file, int offset)
{
    while (file->pendingCount <= offset) {
        int c = fgetc(file->stream);
        if (file->text && c == '\r') {
            int next = fgetc(file->stream);
            if (next != '\n' && next != EOF) { ungetc(next, file->stream); }
            c = '\n';
        }
        file->pending[file->pendingCount++] = c;
        if (c == EOF) { break; }
    }
    return offset < file->pendingCount ? file->pending[offset] : EOF;
}

int __ada_text_take(AdaFile* file)
{
    int c = __ada_text_peek(file, 0);
    if (c == EOF) {
        if (ferror(file->stream)) { __ada_raise(ADA_DEVICE_ERROR); }
        return EOF;
    }
    --file->pendingCount;
    memmove(file->pending, file->pending + 1, (size_t)file->pendingCount * sizeof(int));
    if (c == '\n') { file->column = 1; ++file->line; }
    else if (c == '\f') { file->column = file->line = 1; ++file->page; }
    else { ++file->column; }
    return c;
}

int __ada_io_token_length(const char* text, int length, int kind)
{
    int state = 0, i = 0;
    while (i < length) {
        int next = ioTokenState(state, (unsigned char)text[i], kind);
        if (next < 0) { break; }
        state = next;
        ++i;
    }
    return i;
}

char* __ada_io_read_token(AdaFile* file, int width, int kind, int* length)
{
    *length = 0;
    if (width < 0) { __ada_raise(ADA_CONSTRAINT_ERROR); return NULL; }
    int c = __ada_text_peek(file, 0);
    if (width == 0) {
        while (c == ' ' || c == '\t' || c == '\n' || c == '\f') {
            __ada_text_take(file);
            c = __ada_text_peek(file, 0);
        }
    }
    if (c == EOF) {
        __ada_raise(ferror(file->stream) ? ADA_DEVICE_ERROR : ADA_END_ERROR);
        return NULL;
    }
    int capacity = 64, state = 0;
    char* text = malloc((size_t)capacity);
    if (text == NULL) { __ada_raise(ADA_STORAGE_ERROR); return NULL; }
    while (c != EOF && c != '\n' && c != '\f') {
        if (width != 0) {
            if (*length == width) { break; }
        } else {
            state = ioTokenState(state, c, kind);
            if (state < 0) { break; }
        }
        if (*length == capacity) {
            if (capacity > INT_MAX / 2) { __ada_raise(ADA_STORAGE_ERROR); break; }
            capacity *= 2;
            char* grown = realloc(text, (size_t)capacity);
            if (grown == NULL) { __ada_raise(ADA_STORAGE_ERROR); break; }
            text = grown;
        }
        text[(*length)++] = (char)__ada_text_take(file);
        c = __ada_text_peek(file, 0);
    }
    if (ferror(file->stream)) { __ada_raise(ADA_DEVICE_ERROR); }
    return text;
}

static void __ada_text_scanImpl(void* descriptor, AdaFileRef handle, int width, int kind)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    if (file == NULL) { return; }
    int length;
    char* text = __ada_io_read_token(file, width, kind, &length);
    if (__ada_exception == NULL) { __ada_array_result(descriptor, text, 1, length, 1); }
    free(text);
}

static void __ada_scanImpl(void* descriptor, int width, int kind)
{
    __ada_text_scanImpl(descriptor, &activeFile, width, kind);
}

static void closeEverything(void)
{
    int i;

    __ada_io_lock();
    iofflush(stdout);
    for (i = 0; i < ADA_MAX_FILES; ++i) {
        if (filePool[i].isOpen && filePool[i].stream != 0) {
            iofclose(filePool[i].stream);
            filePool[i].isOpen = 0;
            filePool[i].stream = 0;
        }
    }
    __ada_io_unlock();
}

static void openStandard(AdaFile* file, FILE* stream, int mode)
{
    file->self = file;
    file->stream = stream;
    if (mode == ADA_MODE_IN) { setvbuf(stream, NULL, _IONBF, 0); }
    file->mode = mode;
    file->isOpen = 1;
    file->isStandard = 1;
    file->text = 1;
    file->column = file->line = file->page = 1;
    file->pageTerminated = 0;
    file->name[0] = '\0';
}

static void prepare(void)
{
    if (poolPrepared) {
        return;
    }
    poolPrepared = 1;
    for (int i = 0; i < ADA_MAX_FILES; ++i) {
        filePool[i].self = &filePool[i];
    }

    openStandard(&standardInput, stdin, ADA_MODE_IN);
    openStandard(&standardOutput, stdout, ADA_MODE_OUT);
    openStandard(&standardError, stderr, ADA_MODE_OUT);

    standardInputHandle = &standardInput;
    standardOutputHandle = &standardOutput;
    standardErrorHandle = &standardError;
    currentInputHandle = &standardInput;
    currentOutputHandle = &standardOutput;
    CURRENT_ERROR_HANDLE = &standardError;

    atexit(closeEverything);
}

static AdaFileRef __ada_standard_inputImpl(void)
{
    prepare();
    return &standardInputHandle;
}

static AdaFileRef __ada_standard_outputImpl(void)
{
    prepare();
    return &standardOutputHandle;
}

static AdaFileRef __ada_standard_errorImpl(void)
{
    prepare();
    return &standardErrorHandle;
}

static AdaFileRef __ada_current_inputImpl(void)
{
    if (activeFile == NULL) { prepare(); }
    return activeFile != NULL ? &activeFile->self : &currentInputHandle->self;
}

static AdaFileRef __ada_current_outputImpl(void)
{
    if (activeFile == NULL) { prepare(); }
    return activeFile != NULL ? &activeFile->self : &currentOutputHandle->self;
}

static void __ada_set_inputImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);

    if (file != 0) {
        currentInputHandle = file;
    }
}

static void __ada_set_outputImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);

    if (file != 0) {
        currentOutputHandle = file;
    }
}

/* An unopened File_Type is a null handle, which every operation but Is_Open
   rejects with Status_Error. */
AdaFile* __ada_file_checked(AdaFileRef handle, int requiredMode)
{
    AdaFile* file = handle != 0 ? *handle : 0;

    if (file == 0 || !file->isOpen) {
        __ada_raise(ADA_STATUS_ERROR);
        return 0;
    }
    if (requiredMode == ADA_MODE_ANY) {
        return file;
    }
    if (requiredMode == ADA_MODE_IN) {
        if (file->mode != ADA_MODE_IN && file->mode != ADA_MODE_INOUT) {
            __ada_raise(ADA_MODE_ERROR);
            return 0;
        }
        return file;
    }
    if (file->mode == ADA_MODE_IN) {
        __ada_raise(ADA_MODE_ERROR);
        return 0;
    }
    return file;
}

static AdaFile* takeFreeEntry(void)
{
    int i;

    for (i = 0; i < ADA_MAX_FILES; ++i) {
        if (!filePool[i].isOpen && filePool[i].users == 0) {
            AdaFile* file = &filePool[i];
            if (!file->mutexReady) {
                if (pthread_mutex_init(&file->mutex, NULL) != 0) { abort(); }
                file->mutexReady = 1;
            }
            /* No pins and a closed slot: acquiring this mutex cannot wait. */
            __ada_mutex_lock(&file->mutex);
            if (activeFile != NULL) {
                --activeFile->users;
                __ada_mutex_unlock(&activeFile->mutex);
            }
            activeFile = file;
            ++file->users;
            return file;
        }
    }
    __ada_raise(ADA_STORAGE_ERROR);
    return 0;
}

static const char* modeString(int mode, int create)
{
    /* Create always starts from an empty file, even for reading, so it asks
       for a stream that may be written whatever the Ada mode says.  The mode
       recorded against the file is what later rejects the wrong operation. */
    if (create) {
        return mode == ADA_MODE_APPEND ? "a+b" : "w+b";
    }
    switch (mode) {
    case ADA_MODE_IN:
        return "rb";
    case ADA_MODE_OUT:
        return "wb";
    case ADA_MODE_APPEND:
        return "ab";
    case ADA_MODE_INOUT:
        return "r+b";
    default:
        return "rb";
    }
}

/* Create truncates or makes the file, Open insists that it is already there.
   An empty name asks for a scratch file that lives only as long as it is
   open, which is what Ada does for an anonymous file. */
static void __ada_file_openImpl(AdaFileRef handle, int mode, const char* name, int length, int create)
{
    AdaFile* file;

    prepare();
    if (handle == 0) {
        __ada_raise(ADA_USE_ERROR);
        return;
    }
    if (*handle != 0 && (*handle)->isOpen) {
        __ada_raise(ADA_STATUS_ERROR);
        return;
    }
    if (length < 0) {
        length = 0;
    }
    if (length >= (int)sizeof filePool[0].name || (length > 0 && memchr(name, 0, (size_t)length) != NULL)) {
        __ada_raise(ADA_NAME_ERROR);
        return;
    }

    file = takeFreeEntry();
    if (file == 0) {
        return;
    }

    *handle = file;
    if (length > 0) {
        memcpy(file->name, name, (size_t)length);
    }
    file->name[length] = '\0';
    file->mode = mode;
    file->owner = handle;
    file->text = 0;
    file->pendingCount = 0;
    file->column = file->line = file->page = 1;
    file->pageTerminated = 0;
    file->lineLength = file->pageLength = 0;
    file->isStandard = 0;

    if (length == 0) {
        file->stream = iotmpfile();
    } else {
        file->stream = iofopen(file->name, modeString(mode, create));
    }
    if (file->stream == 0) {
        /* Opening something that is not there is Name_Error; failing to make
           it is the environment refusing the request. */
        *handle = NULL;
        __ada_raise(create ? ADA_USE_ERROR : ADA_NAME_ERROR);
        return;
    }

    setvbuf(file->stream, NULL, _IONBF, 0);
    file->isOpen = 1;
    *handle = file;
}

static void __ada_file_closeImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);

    if (file == 0) {
        return;
    }
    if (file->isStandard) {
        __ada_raise(ADA_MODE_ERROR);
        return;
    }
    if (currentInputHandle == file) { currentInputHandle = &standardInput; }
    if (currentOutputHandle == file) { currentOutputHandle = &standardOutput; }
    if (CURRENT_ERROR_HANDLE == file) { CURRENT_ERROR_HANDLE = &standardError; }
    if (file->text && file->mode != ADA_MODE_IN && !file->pageTerminated) {
        __ada_io_unlock();
        textNewPage(file);
        __ada_io_lock();
    }
    int closeStatus = iofclose(file->stream);
    file->stream = 0;
    file->isOpen = 0;
    if (file->owner != NULL) { *file->owner = NULL; }
    *handle = 0;
    if (closeStatus != 0) { __ada_raise(ADA_DEVICE_ERROR); }
}

/* The cleanup chain calls this for unopened, closed and borrowed handles too.
   Close exactly the resource associated with the declaring object's address. */
static void __ada_file_finalizeImpl(AdaFileRef handle)
{
    AdaFile* file = handle != NULL ? *handle : NULL;
    if (file != NULL && file->isOpen && !file->isStandard && file->owner == handle) {
        __ada_file_closeImpl(handle);
    }
}

static void __ada_file_deleteImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);
    char name[sizeof filePool[0].name];

    if (file == 0) {
        return;
    }
    if (file->isStandard) {
        __ada_raise(ADA_USE_ERROR);
        return;
    }
    strcpy(name, file->name);
    if (currentInputHandle == file) { currentInputHandle = &standardInput; }
    if (currentOutputHandle == file) { currentOutputHandle = &standardOutput; }
    if (CURRENT_ERROR_HANDLE == file) { CURRENT_ERROR_HANDLE = &standardError; }
    if (file->text && file->mode != ADA_MODE_IN && !file->pageTerminated) {
        __ada_io_unlock();
        textNewPage(file);
        __ada_io_lock();
    }
    int closeStatus = iofclose(file->stream);
    file->stream = 0;
    file->isOpen = 0;
    if (file->owner != NULL) { *file->owner = NULL; }
    *handle = 0;
    if (closeStatus != 0) { __ada_raise(ADA_DEVICE_ERROR); }
    if (name[0] != '\0' && ioremove(name) != 0) { __ada_raise(ADA_USE_ERROR); }
}

/* Reset rewinds the file, optionally in a new mode.  A mode of ADA_MODE_ANY
   keeps the one the file already has. */
static void __ada_file_resetImpl(AdaFileRef handle, int mode)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);

    if (file == 0) {
        return;
    }
    if (file->isStandard) {
        __ada_raise(ADA_MODE_ERROR);
        return;
    }
    if (mode == ADA_MODE_ANY) {
        mode = file->mode;
    }
    if (file->text && mode != file->mode
        && (file == currentInputHandle || file == currentOutputHandle || file == CURRENT_ERROR_HANDLE)) {
        __ada_raise(ADA_MODE_ERROR);
        return;
    }
    if (file->text && file->mode != ADA_MODE_IN && !file->pageTerminated) {
        __ada_io_unlock();
        textNewPage(file);
        __ada_io_lock();
    }
    file->pendingCount = 0;
    file->column = file->line = file->page = 1;
    file->pageTerminated = 0;
    file->lineLength = file->pageLength = 0;
    iofflush(file->stream);
    if (file->name[0] == '\0') {
        /* A scratch file has no name to reopen, so it is only rewound. */
        iorewind(file->stream);
        file->mode = mode;
        return;
    }
    file->stream = iofreopen(file->name, modeString(mode, 0), file->stream);
    if (file->stream == 0) {
        if (currentInputHandle == file) { currentInputHandle = &standardInput; }
        if (currentOutputHandle == file) { currentOutputHandle = &standardOutput; }
    if (CURRENT_ERROR_HANDLE == file) { CURRENT_ERROR_HANDLE = &standardError; }
        file->isOpen = 0;
        *handle = 0;
        __ada_raise(ADA_USE_ERROR);
        return;
    }
    file->mode = mode;
}

/* Reset without a mode, which Ada declares as a profile of its own rather than
   as a default, keeps whatever mode the file was opened with. */
static void __ada_file_reset_sameImpl(AdaFileRef handle)
{
    __ada_file_resetImpl(handle, ADA_MODE_ANY);
}

static int __ada_file_is_openImpl(AdaFileRef handle)
{
    AdaFile* file = handle != 0 ? *handle : 0;

    return file != 0 && file->isOpen ? 1 : 0;
}

static int __ada_file_modeImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);

    return file != 0 ? file->mode : ADA_MODE_IN;
}

static void __ada_file_nameImpl(void* descriptor, AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);

    if (file != NULL) {
        __ada_array_result(descriptor, file->name, 1, (int)strlen(file->name), 1);
    }
}

/* Nothing in this run time reads the form string, so a file always reports
   the default one. */
static const char* __ada_file_formImpl(AdaFileRef handle)
{
    __ada_file_checked(handle, ADA_MODE_ANY);
    return "";
}

static int __ada_file_end_of_fileImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    int c;

    if (file == 0) {
        return 1;
    }
    if (file->text) {
        c = __ada_text_peek(file, 0);
        if (c == '\n') {
            c = __ada_text_peek(file, 1);
            if (c == '\f') { c = __ada_text_peek(file, 2); }
        }
    } else {
        c = fgetc(file->stream);
        if (c != EOF) { ungetc(c, file->stream); }
    }
    if (ferror(file->stream)) { __ada_raise(ADA_DEVICE_ERROR); }
    return c == EOF;
}

/* ------------------------------------------------------------------------ */
/* Text_IO                                                                   */
/* ------------------------------------------------------------------------ */

static void __ada_createImpl(AdaFileRef handle, int mode, const char* name, int length, const char* form, int formLength)
{
    (void)form;
    if (formLength != 0) { __ada_raise(ADA_USE_ERROR); return; }
    __ada_file_openImpl(handle, mode, name, length, 1);
    if (__ada_exception == NULL) { (*handle)->text = 1; }
}

static void __ada_openImpl(AdaFileRef handle, int mode, const char* name, int length, const char* form, int formLength)
{
    (void)form;
    if (formLength != 0) { __ada_raise(ADA_USE_ERROR); return; }
    __ada_file_openImpl(handle, mode, name, length, 0);
    if (__ada_exception == NULL) { (*handle)->text = 1; }
}

static void writeInteger(FILE* stream, int value, int width, int base)
{
    static const char* const kDigits = "0123456789ABCDEF";
    char body[80];
    char text[96];
    int at = (int)sizeof body - 1;
    unsigned int magnitude;

    if (base < 2 || base > 16) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return;
    }
    if (base == 10) {
        fprintf(stream, "%*d", width, value);
        return;
    }

    magnitude = value < 0 ? (unsigned int)(-(long)value) : (unsigned int)value;
    body[at] = '\0';
    do {
        body[--at] = kDigits[magnitude % (unsigned int)base];
        magnitude /= (unsigned int)base;
    } while (magnitude != 0);

    snprintf(text, sizeof text, "%s%d#%s#", value < 0 ? "-" : "", base, body + at);
    fprintf(stream, "%*s", width, text);
}

static void textWriteByte(AdaFile* file, int c)
{
    if (fputc(c, file->stream) == EOF) { __ada_raise(ADA_DEVICE_ERROR); }
}

static void textNewPage(AdaFile* file)
{
    if (file->column != 1 || file->line == 1) { textWriteByte(file, '\n'); }
    textWriteByte(file, '\f');
    file->column = file->line = 1;
    ++file->page;
    file->pageTerminated = 1;
}

static void textNewLine(AdaFile* file)
{
    textWriteByte(file, '\n');
    file->column = 1;
    file->pageTerminated = 0;
    if (file->pageLength > 0 && file->line >= (uint64_t)file->pageLength) {
        textWriteByte(file, '\f');
        file->line = 1;
        ++file->page;
        file->pageTerminated = 1;
    } else { ++file->line; }
}

void __ada_text_write_chars(AdaFile* file, const char* text, int length)
{
    for (int i = 0; i < length && __ada_exception == NULL; ++i) {
        if (file->lineLength > 0 && file->column > (uint64_t)file->lineLength) { textNewLine(file); }
        textWriteByte(file, (unsigned char)text[i]);
        ++file->column;
        file->pageTerminated = 0;
    }
}

void __ada_text_write_field(AdaFile* file, const char* text, int length)
{
    if (file->lineLength != 0) {
        if (length > file->lineLength) { __ada_raise(ADA_LAYOUT_ERROR); return; }
        if (file->column + (uint64_t)length - 1 > (uint64_t)file->lineLength) { textNewLine(file); }
    }
    __ada_text_write_chars(file, text, length);
}

static void __ada_text_put_fieldImpl(AdaFileRef handle, const char* text, int length)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file != NULL) { __ada_text_write_field(file, text, length); }
}

static void __ada_put_fieldImpl(const char* text, int length)
{
    __ada_text_put_fieldImpl(&activeFile, text, length);
}

static void __ada_text_putImpl(AdaFileRef handle, const char* item, int length)
{
    if (length == 0) { return; }
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file != NULL) { __ada_text_write_chars(file, item, length); }
}

static void __ada_text_put_lineImpl(AdaFileRef handle, const char* item, int length)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file != NULL) { __ada_text_write_chars(file, item, length); textNewLine(file); }
}

static void __ada_text_put_characterImpl(AdaFileRef handle, int item)
{
    char c = (char)item;
    __ada_text_putImpl(handle, &c, 1);
}

static void __ada_text_put_integerImpl(AdaFileRef handle, int item, int width, int base)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file != NULL) { writeInteger(file->stream, item, width, base); }
}

static void __ada_text_put_floatImpl(AdaFileRef handle, double item, int fore, int aft, int exponent)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file == NULL) { return; }
    if (aft > INT_MAX - 400 || fore > INT_MAX - aft - 400 || exponent > INT_MAX - aft - fore - 400) {
        __ada_raise(ADA_LAYOUT_ERROR); return;
    }
    int capacity = aft + fore + exponent + 400;
    char* text = malloc((size_t)capacity);
    if (text == NULL) { __ada_raise(ADA_STORAGE_ERROR); return; }
    __ada_format_float(text, capacity, item, fore, aft, exponent);
    __ada_text_write_field(file, text, (int)strlen(text));
    free(text);
}

static void __ada_text_new_lineImpl(AdaFileRef handle, int spacing)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file != NULL) { for (int i = 0; i < spacing && __ada_exception == NULL; ++i) { textNewLine(file); } }
}

static void skipLines(AdaFile* file, int spacing)
{
    for (int i = 0; i < spacing; ++i) {
        int c;
        do { c = __ada_text_take(file); } while (c != '\n' && c != EOF);
        if (c == EOF) { if (__ada_exception == NULL) { __ada_raise(ADA_END_ERROR); } return; }
        if (__ada_text_peek(file, 0) == '\f') { __ada_text_take(file); }
    }
}

static void __ada_text_skip_lineImpl(AdaFileRef handle, int spacing)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    if (file != NULL) { skipLines(file, spacing); }
}

static int atEndOfLine(AdaFile* file)
{
    int c = __ada_text_peek(file, 0);
    if (ferror(file->stream)) { __ada_raise(ADA_DEVICE_ERROR); }
    return c == EOF || c == '\n' || c == '\f';
}

static int __ada_text_end_of_lineImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    return file != NULL ? atEndOfLine(file) : 1;
}

static void readCharacter(AdaFile* file, char* item)
{
    int c;
    do { c = __ada_text_take(file); } while (c == '\n' || c == '\f');
    if (c == EOF) { if (__ada_exception == NULL) { __ada_raise(ADA_END_ERROR); } return; }
    *item = (char)c;
}

static void __ada_text_getImpl(AdaFileRef handle, char* item)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    if (file != NULL) { readCharacter(file, item); }
}

static void __ada_text_get_stringImpl(AdaFileRef handle, char* item, int length)
{
    if (length == 0) { return; }
    if (length < 0) { __ada_raise(ADA_CONSTRAINT_ERROR); return; }
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    if (file == NULL) { return; }
    for (int i = 0; i < length && __ada_exception == NULL; ++i) { readCharacter(file, item + i); }
}

static void readLine(AdaFile* file, char* item, int length, int* last)
{
    int count = 0;
    while (count < length) {
        int c = __ada_text_peek(file, 0);
        if (c == EOF) {
            if (ferror(file->stream)) { __ada_raise(ADA_DEVICE_ERROR); }
            else if (count == 0) { __ada_raise(ADA_END_ERROR); }
            break;
        }
        if (c == '\n' || c == '\f') { skipLines(file, 1); break; }
        item[count++] = (char)__ada_text_take(file);
    }
    *last = count;
}

static void __ada_text_get_lineImpl(AdaFileRef handle, char* item, int length, int* last)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    *last = 0;
    if (file != NULL) { readLine(file, item, length, last); }
}

/* The width of the target is not visible from the profile, so it travels with
   the address. */
static void storeInteger(void* item, int size, long value)
{
    if (size == 1) {
        *(signed char*)item = (signed char)value;
    } else if (size == 2) {
        *(short*)item = (short)value;
    } else if (size == 8) {
        *(long long*)item = (long long)value;
    } else {
        *(int*)item = (int)value;
    }
}

static void readInteger(AdaFile* file, void* item, int size, int low, int high)
{
    long value = 0;

    if (fscanf(file->stream, "%ld", &value) != 1) {
        __ada_raise(feof(file->stream) ? ADA_END_ERROR : ADA_DATA_ERROR);
        storeInteger(item, size, 0);
        return;
    }
    /* Ada makes a number the instantiated type cannot hold a Data_Error rather
       than a Constraint_Error. */
    if (value < (long)low || value > (long)high) {
        __ada_raise(ADA_DATA_ERROR);
        storeInteger(item, size, 0);
        return;
    }
    storeInteger(item, size, value);
}

static void __ada_text_get_integerImpl(AdaFileRef handle, void* item, int size, int low, int high)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);

    if (file != 0) {
        readInteger(file, item, size, low, high);
    }
}

/* The word the input is looking at, read up to the first character that cannot
   be part of one.  Recognising what the word means is the business of the Ada
   generic that asked for it, which is the only side that knows the type. */
static void __ada_text_get_wordImpl(AdaFileRef handle, char* item, int length, int* last)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    int written = 0;
    int c;

    *last = 0;
    if (file == 0) {
        return;
    }

    do {
        c = fgetc(file->stream);
    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');

    if (c == EOF) {
        __ada_raise(ADA_END_ERROR);
        return;
    }
    while (c != EOF && !isspace(c)) {
        if (written < length) {
            item[written++] = (char)c;
        }
        c = fgetc(file->stream);
    }
    if (c != EOF) {
        ungetc(c, file->stream);
    }
    *last = written;
}

static long long __ada_text_get_modularImpl(AdaFileRef handle, long long last, int width)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    if (file == NULL) { return 0; }
    int length;
    char* text = __ada_io_read_token(file, width, 0, &length);
    long long value = 0;
    if (__ada_exception == NULL) {
        for (int i = 0; i < length; ++i) { if (text[i] == '\t') { text[i] = ' '; } }
        value = __ada_value_modular(text, length, (long long)((uint64_t)last + 1));
    }
    free(text);
    if (__ada_exception == ADA_CONSTRAINT_ERROR) { __ada_raise(ADA_DATA_ERROR); }
    return value;
}

static long long __ada_get_modularImpl(long long last, int width)
{
    return __ada_text_get_modularImpl(__ada_current_inputImpl(), last, width);
}

static void __ada_get_wordImpl(char* item, int length, int* last)
{
    __ada_text_get_wordImpl(__ada_current_inputImpl(), item, length, last);
}

/* Float_IO.Get reports input that is not a number as Data_Error, so that is
   what a word which spells no real number raises here. */
double __ada_real_value(const char* text, int length)
{
    double result = __ada_value_real(text, length);
    if (__ada_exception == ADA_CONSTRAINT_ERROR) { __ada_raise(ADA_DATA_ERROR); }
    return result;
}

static void storeReal(void* item, int size, double value)
{
    if (size == 4) {
        *(float*)item = (float)value;
    } else {
        *(double*)item = value;
    }
}

static void readReal(AdaFile* file, void* item, int size)
{
    double value = 0.0;

    if (fscanf(file->stream, "%lf", &value) != 1) {
        __ada_raise(feof(file->stream) ? ADA_END_ERROR : ADA_DATA_ERROR);
        storeReal(item, size, 0.0);
        return;
    }
    storeReal(item, size, value);
}

static void __ada_text_get_floatImpl(AdaFileRef handle, void* item, int size)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);

    if (file != 0) {
        readReal(file, item, size);
    }
}

/* The compiler records the literals of an enumeration type in lower case, which
   is the spelling Lower_Case asks for; Upper_Case is the Ada default. */
static void __ada_text_put_enumImpl(AdaFileRef handle, int item, int width, int set, const char** names, int count)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    const char* name;
    int length;
    int i;

    if (file == 0) {
        return;
    }
    if (item < 0 || item >= count) {
        __ada_raise(ADA_DATA_ERROR);
        return;
    }

    name = names[item];
    for (i = 0; name[i] != '\0'; ++i) {
        fputc(set == 0 ? name[i] : toupper((unsigned char)name[i]), file->stream);
    }
    /* A field wider than the literal is filled out on the right. */
    for (length = i; length < width; ++length) {
        fputc(' ', file->stream);
    }
}

static void readEnum(AdaFile* file, void* item, int size, const char** names, int count)
{
    char word[128];
    int length = 0;
    int c;
    int i;

    do {
        c = fgetc(file->stream);
    } while (c == ' ' || c == '\t' || c == '\n' || c == '\r');

    if (c == EOF) {
        __ada_raise(ADA_END_ERROR);
        storeInteger(item, size, 0);
        return;
    }
    while (c != EOF && (isalnum(c) || c == '_')) {
        if (length < (int)sizeof word - 1) {
            word[length++] = (char)tolower(c);
        }
        c = fgetc(file->stream);
    }
    if (c != EOF) {
        ungetc(c, file->stream);
    }
    word[length] = '\0';

    for (i = 0; i < count; ++i) {
        if (strcmp(word, names[i]) == 0) {
            storeInteger(item, size, i);
            return;
        }
    }
    __ada_raise(ADA_DATA_ERROR);
    storeInteger(item, size, 0);
}

static void __ada_text_get_enumImpl(AdaFileRef handle, void* item, int size, const char** names, int count)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);

    if (file != 0) {
        readEnum(file, item, size, names, count);
    }
}

/* ------------------------------------------------------------------------ */
/* Text_IO on the current input and output                                   */
/* ------------------------------------------------------------------------ */

static void __ada_putImpl(const char* item, int length)
{
    __ada_text_putImpl(__ada_current_outputImpl(), item, length);
}

static void __ada_put_lineImpl(const char* item, int length)
{
    __ada_text_put_lineImpl(__ada_current_outputImpl(), item, length);
}

static void __ada_put_characterImpl(int item)
{
    __ada_text_put_characterImpl(__ada_current_outputImpl(), item);
}

static void __ada_put_integerImpl(int item, int width, int base)
{
    __ada_text_put_integerImpl(__ada_current_outputImpl(), item, width, base);
}

static void __ada_new_lineImpl(int spacing)
{
    __ada_text_new_lineImpl(__ada_current_outputImpl(), spacing);
}

static void __ada_skip_lineImpl(int spacing)
{
    __ada_text_skip_lineImpl(__ada_current_inputImpl(), spacing);
}

static int __ada_end_of_fileImpl(void)
{
    return __ada_file_end_of_fileImpl(__ada_current_inputImpl());
}

static int __ada_end_of_lineImpl(void)
{
    return __ada_text_end_of_lineImpl(__ada_current_inputImpl());
}

static void __ada_getImpl(char* item)
{
    __ada_text_getImpl(__ada_current_inputImpl(), item);
}

static void __ada_get_stringImpl(char* item, int length)
{
    __ada_text_get_stringImpl(__ada_current_inputImpl(), item, length);
}

static void __ada_get_lineImpl(char* item, int length, int* last)
{
    __ada_text_get_lineImpl(__ada_current_inputImpl(), item, length, last);
}

static void __ada_get_integerImpl(void* item, int size, int low, int high)
{
    __ada_text_get_integerImpl(__ada_current_inputImpl(), item, size, low, high);
}

static void __ada_get_floatImpl(void* item, int size)
{
    __ada_text_get_floatImpl(__ada_current_inputImpl(), item, size);
}

static void __ada_put_enumImpl(int item, int width, int set, const char** names, int count)
{
    __ada_text_put_enumImpl(__ada_current_outputImpl(), item, width, set, names, count);
}

static void __ada_get_enumImpl(void* item, int size, const char** names, int count)
{
    __ada_text_get_enumImpl(__ada_current_inputImpl(), item, size, names, count);
}

/* ------------------------------------------------------------------------ */
/* Sequential_IO                                                             */
/* ------------------------------------------------------------------------ */

static void __ada_read_elementImpl(AdaFileRef handle, void* item, int size)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);

    if (file == 0) {
        return;
    }
    if (fread(item, 1, (size_t)size, file->stream) != (size_t)size) {
        __ada_raise(ADA_END_ERROR);
    }
}

static void __ada_write_elementImpl(AdaFileRef handle, const void* item, int size)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);

    if (file == 0) {
        return;
    }
    if (fwrite(item, 1, (size_t)size, file->stream) != (size_t)size) {
        __ada_raise(ADA_DEVICE_ERROR);
    }
}

/* ------------------------------------------------------------------------ */
/* Direct_IO                                                                 */
/* ------------------------------------------------------------------------ */

/* Direct_IO writes its modes as In_File, Inout_File, Out_File. */
static int directMode(int mode)
{
    switch (mode) {
    case 0:
        return ADA_MODE_IN;
    case 1:
        return ADA_MODE_INOUT;
    case 2:
        return ADA_MODE_OUT;
    default:
        return ADA_MODE_ANY;
    }
}

static int adaDirectMode(int mode)
{
    switch (mode) {
    case ADA_MODE_IN:
        return 0;
    case ADA_MODE_INOUT:
        return 1;
    default:
        return 2;
    }
}

static void __ada_direct_createImpl(AdaFileRef handle, int mode, const char* name, int length, const char* form,
                         int formLength)
{
    (void)form;
    if (formLength != 0) { __ada_raise(ADA_USE_ERROR); return; }
    __ada_file_openImpl(handle, directMode(mode), name, length, 1);
}

static void __ada_direct_openImpl(AdaFileRef handle, int mode, const char* name, int length, const char* form, int formLength)
{
    (void)form;
    if (formLength != 0) { __ada_raise(ADA_USE_ERROR); return; }
    __ada_file_openImpl(handle, directMode(mode), name, length, 0);
}

static void __ada_direct_resetImpl(AdaFileRef handle, int mode)
{
    __ada_file_resetImpl(handle, mode < 0 ? ADA_MODE_ANY : directMode(mode));
}

static void __ada_direct_reset_sameImpl(AdaFileRef handle)
{
    __ada_file_reset_sameImpl(handle);
}

static int __ada_direct_modeImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);

    return file != 0 ? adaDirectMode(file->mode) : 0;
}

/* Ada counts elements from one, so an index becomes a byte offset here. */
static long offsetOf(int index, int size)
{
    return (long)(index - 1) * (long)size;
}

static void __ada_direct_read_atImpl(AdaFileRef handle, void* item, int size, int index)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);

    if (file == 0) {
        return;
    }
    if (index > 0 && fseek(file->stream, offsetOf(index, size), SEEK_SET) != 0) {
        __ada_raise(ADA_END_ERROR);
        return;
    }
    if (fread(item, 1, (size_t)size, file->stream) != (size_t)size) {
        __ada_raise(ADA_END_ERROR);
    }
}

static void __ada_direct_write_atImpl(AdaFileRef handle, const void* item, int size, int index)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);

    if (file == 0) {
        return;
    }
    if (index > 0 && fseek(file->stream, offsetOf(index, size), SEEK_SET) != 0) {
        __ada_raise(ADA_USE_ERROR);
        return;
    }
    if (fwrite(item, 1, (size_t)size, file->stream) != (size_t)size) {
        __ada_raise(ADA_DEVICE_ERROR);
    }
}

static void __ada_direct_set_indexImpl(AdaFileRef handle, int index, int size)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);

    if (file == 0) {
        return;
    }
    if (fseek(file->stream, offsetOf(index, size), SEEK_SET) != 0) {
        __ada_raise(ADA_USE_ERROR);
    }
}

static int __ada_direct_indexImpl(AdaFileRef handle, int size)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);
    long position;

    if (file == 0) {
        return 1;
    }
    position = ftell(file->stream);
    if (position < 0) {
        __ada_raise(ADA_USE_ERROR);
        return 1;
    }
    return (int)(position / size) + 1;
}

static int __ada_direct_sizeImpl(AdaFileRef handle, int size)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);
    long here;
    long end;

    if (file == 0) {
        return 0;
    }
    here = ftell(file->stream);
    if (here < 0 || fseek(file->stream, 0, SEEK_END) != 0) {
        __ada_raise(ADA_USE_ERROR);
        return 0;
    }
    end = ftell(file->stream);
    fseek(file->stream, here, SEEK_SET);
    return end < 0 ? 0 : (int)(end / size);
}

/* ------------------------------------------------------------------------ */
/* Stream_IO                                                                 */
/* ------------------------------------------------------------------------ */

static AdaFileStream* __ada_file_stream_viewImpl(AdaFileRef handle, struct AdaTag* tag)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);
    if (file == NULL) {
        return NULL;
    }
    if (file->streamView.tag == NULL) {
        file->streamView.file = file;
        file->streamView.tag = tag;
    } else if (file->streamView.tag != tag) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return NULL;
    }
    return &file->streamView;
}

static void __ada_file_stream_elements_readImpl(AdaFile* stream, void* item, int length, int first, int* last)
{
    __ada_stream_elements_readImpl(&stream, item, length, first, last);
}

static void __ada_file_stream_elements_writeImpl(AdaFile* stream, const void* item, int length)
{
    __ada_stream_elements_writeImpl(&stream, item, length);
}

static AdaFile* __ada_stream_ofImpl(AdaFileRef handle)
{
    return __ada_file_checked(handle, ADA_MODE_ANY);
}

static void __ada_stream_readImpl(AdaFile* stream, void* item, int64_t size)
{
    if (stream == 0 || !stream->isOpen) {
        __ada_raise(ADA_STATUS_ERROR);
        return;
    }
    if (stream->mode == ADA_MODE_OUT || stream->mode == ADA_MODE_APPEND) {
        __ada_raise(ADA_MODE_ERROR);
        return;
    }
    if (size < 0 || (uint64_t)size > SIZE_MAX) { __ada_raise(ADA_STORAGE_ERROR); return; }
    if (fread(item, 1, (size_t)size, stream->stream) != (size_t)size) {
        __ada_raise(ADA_END_ERROR);
    }
}

static void __ada_stream_writeImpl(AdaFile* stream, const void* item, int64_t size)
{
    if (stream == 0 || !stream->isOpen) {
        __ada_raise(ADA_STATUS_ERROR);
        return;
    }
    if (stream->mode == ADA_MODE_IN) {
        __ada_raise(ADA_MODE_ERROR);
        return;
    }
    if (size < 0 || (uint64_t)size > SIZE_MAX) { __ada_raise(ADA_STORAGE_ERROR); return; }
    if (fwrite(item, 1, (size_t)size, stream->stream) != (size_t)size) {
        __ada_raise(ADA_DEVICE_ERROR);
    }
}

static void __ada_stream_write_boundsImpl(AdaFile* stream, int64_t first, int64_t last)
{
    __ada_stream_writeImpl(stream, &first, (int)sizeof first);
    __ada_stream_writeImpl(stream, &last, (int)sizeof last);
}

/* Transfer a successful read using the ordinary unconstrained-result ABI.
   Until publication, this helper owns the buffer and frees it on failure. */
static void __ada_stream_read_arrayImpl(void* descriptor, AdaFile* stream, int64_t elementSize)
{
    AdaArrayResult* result = descriptor;
    *result = (AdaArrayResult) { NULL, 1, 0, 0 };
    int64_t first = 1, last = 0;
    __ada_stream_readImpl(stream, &first, sizeof first);
    if (__ada_exception != NULL) {
        return;
    }
    __ada_stream_readImpl(stream, &last, sizeof last);
    if (__ada_exception != NULL) {
        return;
    }
    int64_t size = __ada_array_size(first, last, elementSize);
    if (__ada_exception != NULL) {
        return;
    }
    void* buffer = __ada_allocate(size == 0 ? 1 : (long)size);
    if (buffer == NULL) {
        return;
    }
    if (size != 0) {
        __ada_stream_readImpl(stream, buffer, size);
        if (__ada_exception != NULL) {
            free(buffer);
            return;
        }
    }
    *result = (AdaArrayResult) { buffer, size == 0 ? 1 : size, first, last, __ada_array_length(first, last), elementSize };
}

/* Read and Write over a Stream_Element_Array work in bytes, since a stream
   element is one byte wide. */
static void __ada_stream_elements_readImpl(AdaFileRef handle, void* item, int length, int first, int* last)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    size_t got;

    if (file == 0) {
        *last = 0;
        return;
    }
    got = fread(item, 1, (size_t)length, file->stream);
    *last = (int)((long long)first + (long long)got - 1);
    if (ferror(file->stream)) {
        __ada_raise(ADA_DEVICE_ERROR);
    }
}

static void __ada_stream_elements_writeImpl(AdaFileRef handle, const void* item, int length)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);

    if (file == 0) {
        return;
    }
    if (fwrite(item, 1, (size_t)length, file->stream) != (size_t)length) {
        __ada_raise(ADA_DEVICE_ERROR);
    }
}

static void __ada_stream_set_indexImpl(AdaFileRef handle, int index)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);

    if (file != 0 && fseek(file->stream, (long)index - 1, SEEK_SET) != 0) {
        __ada_raise(ADA_USE_ERROR);
    }
}

static int __ada_stream_indexImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);
    long position;

    if (file == 0) {
        return 1;
    }
    position = ftell(file->stream);
    return position < 0 ? 1 : (int)position + 1;
}

static int __ada_stream_sizeImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);
    long here;
    long end;

    if (file == 0) {
        return 0;
    }
    here = ftell(file->stream);
    if (here < 0 || fseek(file->stream, 0, SEEK_END) != 0) {
        __ada_raise(ADA_USE_ERROR);
        return 0;
    }
    end = ftell(file->stream);
    fseek(file->stream, here, SEEK_SET);
    return end < 0 ? 0 : (int)end;
}

static void __ada_text_set_line_lengthImpl(AdaFileRef handle, int to)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file != NULL) { file->lineLength = to; }
}

static int __ada_text_line_lengthImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    return file != NULL ? file->lineLength : 0;
}

static void __ada_text_set_page_lengthImpl(AdaFileRef handle, int to)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file != NULL) { file->pageLength = to; }
}

static int __ada_text_page_lengthImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    return file != NULL ? file->pageLength : 0;
}

/* Remaining Text_IO operations, under the caller's per-file pin and lock. */
static AdaFileRef __ada_current_errorImpl(void)
{
    return activeFile != NULL ? &activeFile->self : &CURRENT_ERROR_HANDLE->self;
}

static void __ada_set_errorImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file != NULL) { CURRENT_ERROR_HANDLE = file; }
}

static void __ada_text_flushImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file != NULL && fflush(file->stream) != 0) { __ada_raise(ADA_DEVICE_ERROR); }
}

static void __ada_text_new_pageImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    if (file != NULL) { textNewPage(file); }
}

static void __ada_text_skip_pageImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    if (file == NULL) { return; }
    int c;
    do { c = __ada_text_take(file); } while (c != '\f' && c != EOF);
    if (c == EOF && __ada_exception == NULL) { __ada_raise(ADA_END_ERROR); }
}

static int __ada_text_end_of_pageImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    if (file == NULL) { return 1; }
    int c = __ada_text_peek(file, 0);
    if (c == '\n') { c = __ada_text_peek(file, 1); }
    if (ferror(file->stream)) { __ada_raise(ADA_DEVICE_ERROR); }
    return c == EOF || c == '\f';
}

static void __ada_text_set_colImpl(AdaFileRef handle, int to)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);
    if (file == NULL) { return; }
    if (file->mode == ADA_MODE_IN) {
        while (file->column != (uint64_t)to) {
            if (__ada_text_take(file) == EOF) {
                if (__ada_exception == NULL) { __ada_raise(ADA_END_ERROR); }
                return;
            }
        }
    } else {
        if (file->lineLength != 0 && to > file->lineLength) { __ada_raise(ADA_LAYOUT_ERROR); return; }
        if ((uint64_t)to < file->column) { textNewLine(file); }
        while (file->column < (uint64_t)to && __ada_exception == NULL) { __ada_text_write_chars(file, " ", 1); }
    }
}

static void __ada_text_set_lineImpl(AdaFileRef handle, int to)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);
    if (file == NULL) { return; }
    if (file->mode == ADA_MODE_IN) {
        while (file->line != (uint64_t)to && __ada_exception == NULL) { skipLines(file, 1); }
    } else {
        if (file->pageLength != 0 && to > file->pageLength) { __ada_raise(ADA_LAYOUT_ERROR); return; }
        if ((uint64_t)to < file->line) { textNewPage(file); }
        while (file->line < (uint64_t)to && __ada_exception == NULL) { textNewLine(file); }
    }
}

static int textPosition(AdaFileRef handle, int part)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_ANY);
    if (file == NULL) { return 1; }
    uint64_t value = part == 0 ? file->column : part == 1 ? file->line : file->page;
    if (value > INT_MAX) { __ada_raise(ADA_LAYOUT_ERROR); return 1; }
    return (int)value;
}

static void __ada_text_look_aheadImpl(AdaFileRef handle, char* item, int* end)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    if (file == NULL) { return; }
    *end = atEndOfLine(file);
    if (!*end) { *item = (char)__ada_text_peek(file, 0); }
}

static void textImmediate(AdaFile* file, char* item, int* available, int wait)
{
    *available = 0;
#ifdef _WIN32
    if (file->pendingCount == 0 && _isatty(_fileno(file->stream))) {
        if (!wait && !_kbhit()) { return; }
        *item = (char)_getch();
        *available = 1;
        return;
    }
    if (!wait && file->pendingCount == 0) {
        HANDLE handle = (HANDLE)_get_osfhandle(_fileno(file->stream));
        if (GetFileType(handle) == FILE_TYPE_PIPE) {
            DWORD ready = 0;
            if (!PeekNamedPipe(handle, NULL, 0, NULL, &ready, NULL)) {
                if (GetLastError() != ERROR_BROKEN_PIPE) { __ada_raise(ADA_DEVICE_ERROR); return; }
            } else if (ready == 0) { return; }
        }
    }
#else
    struct termios saved;
    int terminal = file->pendingCount == 0 && isatty(fileno(file->stream));
    if (terminal) {
        if (tcgetattr(fileno(file->stream), &saved) != 0) { __ada_raise(ADA_DEVICE_ERROR); return; }
        struct termios raw = saved;
        raw.c_lflag &= ~(ICANON | ECHO);
        raw.c_cc[VMIN] = 1; raw.c_cc[VTIME] = 0;
        if (tcsetattr(fileno(file->stream), TCSANOW, &raw) != 0) { __ada_raise(ADA_DEVICE_ERROR); return; }
    }
    if (!wait && file->pendingCount == 0) {
        struct pollfd descriptor = {fileno(file->stream), POLLIN, 0};
        int status;
        do { status = poll(&descriptor, 1, 0); } while (status < 0 && errno == EINTR);
        if (status <= 0) {
            if (status < 0) { __ada_raise(ADA_DEVICE_ERROR); }
            if (terminal && tcsetattr(fileno(file->stream), TCSANOW, &saved) != 0) { __ada_raise(ADA_DEVICE_ERROR); }
            return;
        }
    }
#endif
    /* Immediate input returns controls directly and never changes logical
       position. It must not wait for a byte after CR to normalize CRLF. */
    int c;
    if (file->pendingCount != 0) {
        c = file->pending[0];
        if (c != EOF) {
            --file->pendingCount;
            memmove(file->pending, file->pending + 1, (size_t)file->pendingCount * sizeof(int));
        }
    } else { c = fgetc(file->stream); }
#ifndef _WIN32
    if (terminal && tcsetattr(fileno(file->stream), TCSANOW, &saved) != 0) { __ada_raise(ADA_DEVICE_ERROR); }
#endif
    if (c == EOF) {
        __ada_raise(ferror(file->stream) ? ADA_DEVICE_ERROR : ADA_END_ERROR);
        return;
    }
    *item = (char)c;
    *available = 1;
}

static void __ada_text_get_immediateImpl(AdaFileRef handle, char* item)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    int available;
    if (file != NULL) { textImmediate(file, item, &available, 1); }
}

static void __ada_text_get_availableImpl(AdaFileRef handle, char* item, int* available)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    if (file != NULL) { textImmediate(file, item, available, 0); }
}

static void __ada_text_lineImpl(void* descriptor, AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);
    if (file == NULL) { return; }
    int capacity = 128, length = 0;
    char* text = malloc((size_t)capacity);
    if (text == NULL) { __ada_raise(ADA_STORAGE_ERROR); return; }
    int c;
    while ((c = __ada_text_peek(file, 0)) != EOF && c != '\n' && c != '\f') {
        if (length == capacity) {
            if (capacity > INT_MAX / 2) { __ada_raise(ADA_CONSTRAINT_ERROR); break; }
            capacity *= 2;
            char* grown = realloc(text, (size_t)capacity);
            if (grown == NULL) { __ada_raise(ADA_STORAGE_ERROR); break; }
            text = grown;
        }
        text[length++] = (char)__ada_text_take(file);
    }
    if (__ada_exception == NULL) {
        if (c == EOF) {
            if (ferror(file->stream)) { __ada_raise(ADA_DEVICE_ERROR); }
            else if (length == 0) { __ada_raise(ADA_END_ERROR); }
        } else { skipLines(file, 1); }
    }
    if (__ada_exception == NULL) { __ada_array_result(descriptor, text, 1, length, 1); }
    free(text);
}

static void __ada_binary_createImpl(AdaFileRef handle, int mode, const char* name, int length, const char* form, int formLength)
{
    (void)form;
    if (formLength != 0) { __ada_raise(ADA_USE_ERROR); return; }
    __ada_file_openImpl(handle, mode, name, length, 1);
}

static void __ada_binary_openImpl(AdaFileRef handle, int mode, const char* name, int length, const char* form, int formLength)
{
    (void)form;
    if (formLength != 0) { __ada_raise(ADA_USE_ERROR); return; }
    __ada_file_openImpl(handle, mode, name, length, 0);
}

void __ada_text_float_string(char* to, int length, double item, int aft, int exponent)
{
    if (aft > INT_MAX - 400 || exponent > INT_MAX - aft - 400) { __ada_raise(ADA_LAYOUT_ERROR); return; }
    int capacity = aft + exponent + 400;
    char* text = malloc((size_t)capacity);
    if (text == NULL) { __ada_raise(ADA_STORAGE_ERROR); return; }
    __ada_format_float(text, capacity, item, 1, aft, exponent);
    int size = (int)strlen(text);
    if (size > length) { __ada_raise(ADA_LAYOUT_ERROR); }
    else { memset(to, ' ', (size_t)(length - size)); memcpy(to + length - size, text, (size_t)size); }
    free(text);
}

static int __ada_text_colImpl(AdaFileRef handle)
{
    return textPosition(handle, 0);
}

static int __ada_text_line_numberImpl(AdaFileRef handle)
{
    return textPosition(handle, 1);
}

static int __ada_text_pageImpl(AdaFileRef handle)
{
    return textPosition(handle, 2);
}


static void __ada_flushImpl(void)
{
    __ada_text_flushImpl(&activeFile);
}


static void __ada_set_line_lengthImpl(int to)
{
    __ada_text_set_line_lengthImpl(&activeFile, to);
}


static void __ada_set_page_lengthImpl(int to)
{
    __ada_text_set_page_lengthImpl(&activeFile, to);
}


static int __ada_line_lengthImpl(void)
{
    return __ada_text_line_lengthImpl(&activeFile);
}


static int __ada_page_lengthImpl(void)
{
    return __ada_text_page_lengthImpl(&activeFile);
}


static void __ada_new_pageImpl(void)
{
    __ada_text_new_pageImpl(&activeFile);
}


static void __ada_skip_pageImpl(void)
{
    __ada_text_skip_pageImpl(&activeFile);
}


static int __ada_end_of_pageImpl(void)
{
    return __ada_text_end_of_pageImpl(&activeFile);
}


static void __ada_set_colImpl(int to)
{
    __ada_text_set_colImpl(&activeFile, to);
}


static void __ada_set_lineImpl(int to)
{
    __ada_text_set_lineImpl(&activeFile, to);
}


static int __ada_colImpl(void)
{
    return __ada_text_colImpl(&activeFile);
}


static int __ada_line_numberImpl(void)
{
    return __ada_text_line_numberImpl(&activeFile);
}


static int __ada_pageImpl(void)
{
    return __ada_text_pageImpl(&activeFile);
}


static void __ada_look_aheadImpl(char* item, int* end)
{
    __ada_text_look_aheadImpl(&activeFile, item, end);
}


static void __ada_get_immediateImpl(char* item)
{
    __ada_text_get_immediateImpl(&activeFile, item);
}


static void __ada_get_availableImpl(char* item, int* available)
{
    __ada_text_get_availableImpl(&activeFile, item, available);
}

static void __ada_lineImpl(void* descriptor)
{
    __ada_text_lineImpl(descriptor, &activeFile);
}

/* Public operations pin and lock one file; Impl calls retain that ownership. */
#define ADA_IO_VOID(name, parameters, arguments, handle, shared) \
void name parameters \
{ \
    __ada_io_begin(handle); \
    if (!shared) { __ada_io_unlock(); } \
    name##Impl arguments; \
    if (!shared) { __ada_io_lock(); } \
    __ada_io_end(); \
}
#define ADA_IO_VALUE(type, name, parameters, arguments, handle, shared) \
type name parameters \
{ \
    __ada_io_begin(handle); \
    if (!shared) { __ada_io_unlock(); } \
    type result = name##Impl arguments; \
    if (!shared) { __ada_io_lock(); } \
    __ada_io_end(); \
    return result; \
}
#include "adaio_entries.h"
#undef ADA_IO_VOID
#undef ADA_IO_VALUE
