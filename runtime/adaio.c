/* File handling shared by Text_IO, Sequential_IO, Direct_IO and Stream_IO.

   An Ada File_Type is a one component record holding a pointer into the pool
   below, so the generated code only ever passes the address of that record and
   never needs to know what a file looks like. */

#include "adaio.h"
#include "adart.h"
#include "adalock.h"

#include <ctype.h>
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
        if (!file->m_mutexReady) {
            if (pthread_mutex_init(&file->m_mutex, NULL) != 0) { abort(); }
            file->m_mutexReady = 1;
        }
        ++file->m_users;
        __ada_io_unlock();
        __ada_mutex_lock(&file->m_mutex);
        __ada_io_lock();
        if (*handle == file) { return; }
        --file->m_users;
        __ada_mutex_unlock(&file->m_mutex);
    }
}

void __ada_io_end(void)
{
    if (activeFile != NULL) {
        --activeFile->m_users;
        __ada_mutex_unlock(&activeFile->m_mutex);
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
    file->m_self = file;
    file->stream = stream;
    file->mode = mode;
    file->isOpen = 1;
    file->isStandard = 1;
    file->name[0] = '\0';
}

static void prepare(void)
{
    if (poolPrepared) {
        return;
    }
    poolPrepared = 1;
    for (int i = 0; i < ADA_MAX_FILES; ++i) {
        filePool[i].m_self = &filePool[i];
    }

    openStandard(&standardInput, stdin, ADA_MODE_IN);
    openStandard(&standardOutput, stdout, ADA_MODE_OUT);
    openStandard(&standardError, stderr, ADA_MODE_OUT);

    standardInputHandle = &standardInput;
    standardOutputHandle = &standardOutput;
    standardErrorHandle = &standardError;
    currentInputHandle = &standardInput;
    currentOutputHandle = &standardOutput;

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
    return activeFile != NULL ? &activeFile->m_self : &currentInputHandle->m_self;
}

static AdaFileRef __ada_current_outputImpl(void)
{
    if (activeFile == NULL) { prepare(); }
    return activeFile != NULL ? &activeFile->m_self : &currentOutputHandle->m_self;
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
        if (!filePool[i].isOpen && filePool[i].m_users == 0) {
            AdaFile* file = &filePool[i];
            if (!file->m_mutexReady) {
                if (pthread_mutex_init(&file->m_mutex, NULL) != 0) { abort(); }
                file->m_mutexReady = 1;
            }
            /* No pins and a closed slot: acquiring this mutex cannot wait. */
            __ada_mutex_lock(&file->m_mutex);
            if (activeFile != NULL) {
                --activeFile->m_users;
                __ada_mutex_unlock(&activeFile->m_mutex);
            }
            activeFile = file;
            ++file->m_users;
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
    if (length >= (int)sizeof filePool[0].name) {
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
    iofclose(file->stream);
    file->stream = 0;
    file->isOpen = 0;
    *handle = 0;
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
    iofclose(file->stream);
    file->stream = 0;
    file->isOpen = 0;
    *handle = 0;
    if (name[0] != '\0') {
        ioremove(name);
    }
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
    c = fgetc(file->stream);
    if (c == EOF) {
        return 1;
    }
    ungetc(c, file->stream);
    return 0;
}

/* ------------------------------------------------------------------------ */
/* Text_IO                                                                   */
/* ------------------------------------------------------------------------ */

static void __ada_createImpl(AdaFileRef handle, int mode, const char* name, int length, const char* form, int formLength)
{
    (void)form;
    (void)formLength;
    __ada_file_openImpl(handle, mode, name, length, 1);
}

static void __ada_openImpl(AdaFileRef handle, int mode, const char* name, int length, const char* form, int formLength)
{
    (void)form;
    (void)formLength;
    __ada_file_openImpl(handle, mode, name, length, 0);
}

static void writeInteger(FILE* stream, int value, int width, int base)
{
    static const char* const digits = "0123456789ABCDEF";
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
        body[--at] = digits[magnitude % (unsigned int)base];
        magnitude /= (unsigned int)base;
    } while (magnitude != 0);

    snprintf(text, sizeof text, "%s%d#%s#", value < 0 ? "-" : "", base, body + at);
    fprintf(stream, "%*s", width, text);
}

static void __ada_text_putImpl(AdaFileRef handle, const char* item, int length)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);

    if (file != 0 && length > 0) {
        fwrite(item, 1, (size_t)length, file->stream);
    }
}

static void __ada_text_put_lineImpl(AdaFileRef handle, const char* item, int length)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);

    if (file == 0) {
        return;
    }
    if (length > 0) {
        fwrite(item, 1, (size_t)length, file->stream);
    }
    fputc('\n', file->stream);
}

static void __ada_text_put_characterImpl(AdaFileRef handle, int item)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);

    if (file != 0) {
        fputc(item, file->stream);
    }
}

static void __ada_text_put_integerImpl(AdaFileRef handle, int item, int width, int base)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);

    if (file != 0) {
        writeInteger(file->stream, item, width, base);
    }
}

static void __ada_text_put_floatImpl(AdaFileRef handle, double item, int fore, int aft, int exponent)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    char buffer[128];

    if (file != 0) {
        __ada_format_float(buffer, (int)sizeof buffer, item, fore, aft, exponent);
        fputs(buffer, file->stream);
    }
}

static void __ada_text_new_lineImpl(AdaFileRef handle, int spacing)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_OUT);
    int i;

    if (file == 0) {
        return;
    }
    for (i = 0; i < spacing; ++i) {
        fputc('\n', file->stream);
    }
}

static void skipLines(AdaFile* file, int spacing)
{
    int i;
    int c;

    for (i = 0; i < spacing; ++i) {
        do {
            c = fgetc(file->stream);
        } while (c != '\n' && c != EOF);
        if (c == EOF) {
            __ada_raise(ADA_END_ERROR);
            return;
        }
    }
}

static void __ada_text_skip_lineImpl(AdaFileRef handle, int spacing)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);

    if (file != 0) {
        skipLines(file, spacing);
    }
}

static int atEndOfLine(AdaFile* file)
{
    int c = fgetc(file->stream);

    if (c == EOF) {
        return 1;
    }
    ungetc(c, file->stream);
    return c == '\n' ? 1 : 0;
}

static int __ada_text_end_of_lineImpl(AdaFileRef handle)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);

    return file != 0 ? atEndOfLine(file) : 1;
}

static void readCharacter(AdaFile* file, char* item)
{
    int c;

    /* Get skips over line boundaries, which is what Ada asks of it. */
    do {
        c = fgetc(file->stream);
    } while (c == '\n');
    if (c == EOF) {
        __ada_raise(ADA_END_ERROR);
        *item = ' ';
        return;
    }
    *item = (char)c;
}

static void __ada_text_getImpl(AdaFileRef handle, char* item)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);

    if (file != 0) {
        readCharacter(file, item);
    }
}

/* Get_Line fills as much of the string as the line holds and reports how far
   it got, then steps past the line terminator. */
static void readLine(AdaFile* file, char* item, int length, int* last)
{
    int count = 0;
    int c;

    for (;;) {
        c = fgetc(file->stream);
        if (c == EOF) {
            if (count == 0) {
                __ada_raise(ADA_END_ERROR);
            }
            break;
        }
        if (c == '\n') {
            break;
        }
        if (count >= length) {
            ungetc(c, file->stream);
            break;
        }
        item[count++] = (char)c;
    }
    *last = count;
}

static void __ada_text_get_lineImpl(AdaFileRef handle, char* item, int length, int* last)
{
    AdaFile* file = __ada_file_checked(handle, ADA_MODE_IN);

    if (file == 0) {
        *last = 0;
        return;
    }
    readLine(file, item, length, last);
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

static void __ada_get_wordImpl(char* item, int length, int* last)
{
    __ada_text_get_wordImpl(__ada_current_inputImpl(), item, length, last);
}

/* Float_IO.Get reports input that is not a number as Data_Error, so that is
   what a word which spells no real number raises here. */
double __ada_real_value(const char* text, int length)
{
    char buffer[64];
    char* stop = 0;
    double value;

    if (length <= 0 || length >= (int)sizeof buffer) {
        __ada_raise(ADA_DATA_ERROR);
        return 0.0;
    }
    memcpy(buffer, text, (size_t)length);
    buffer[length] = '\0';

    value = strtod(buffer, &stop);
    if (stop == buffer || *stop != '\0') {
        __ada_raise(ADA_DATA_ERROR);
        return 0.0;
    }
    return value;
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
    (void)formLength;
    __ada_file_openImpl(handle, directMode(mode), name, length, 1);
}

static void __ada_direct_openImpl(AdaFileRef handle, int mode, const char* name, int length, const char* form, int formLength)
{
    (void)form;
    (void)formLength;
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

static AdaFile* __ada_stream_ofImpl(AdaFileRef handle)
{
    return __ada_file_checked(handle, ADA_MODE_ANY);
}

static void __ada_stream_readImpl(AdaFile* stream, void* item, int size)
{
    if (stream == 0 || !stream->isOpen) {
        __ada_raise(ADA_STATUS_ERROR);
        return;
    }
    if (stream->mode == ADA_MODE_OUT || stream->mode == ADA_MODE_APPEND) {
        __ada_raise(ADA_MODE_ERROR);
        return;
    }
    if (fread(item, 1, (size_t)size, stream->stream) != (size_t)size) {
        __ada_raise(ADA_END_ERROR);
    }
}

static void __ada_stream_writeImpl(AdaFile* stream, const void* item, int size)
{
    if (stream == 0 || !stream->isOpen) {
        __ada_raise(ADA_STATUS_ERROR);
        return;
    }
    if (stream->mode == ADA_MODE_IN) {
        __ada_raise(ADA_MODE_ERROR);
        return;
    }
    if (fwrite(item, 1, (size_t)size, stream->stream) != (size_t)size) {
        __ada_raise(ADA_DEVICE_ERROR);
    }
}

static void __ada_stream_write_boundsImpl(AdaFile* stream, int first, int last)
{
    __ada_stream_writeImpl(stream, &first, (int)sizeof first);
    __ada_stream_writeImpl(stream, &last, (int)sizeof last);
}

/* Transfer a successful read using the ordinary unconstrained-result ABI.
   Until publication, this helper owns the buffer and frees it on failure. */
static void __ada_stream_read_arrayImpl(void* descriptor, AdaFile* stream, int elementSize)
{
    AdaArrayResult* result = descriptor;
    *result = (AdaArrayResult) { NULL, 1, 0, 0 };
    int first = 1, last = 0;
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
    /* The stream helper's byte count is still a signed 32-bit integer. */
    if (size > INT_MAX) {
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    void* buffer = __ada_allocate(size == 0 ? 1 : (long)size);
    if (buffer == NULL) {
        return;
    }
    if (size != 0) {
        __ada_stream_readImpl(stream, buffer, (int)size);
        if (__ada_exception != NULL) {
            free(buffer);
            return;
        }
    }
    *result = (AdaArrayResult) { buffer, first, last, size == 0 ? 1 : size };
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
