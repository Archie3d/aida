/* Minimal run time support for programs compiled by adac. */

#include "adart.h"

#include "adaio.h"

#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_BUFFERS 8
#define IMAGE_BUFFER_SIZE 64

const AdaException __ada_exc_constraint_error = { "CONSTRAINT_ERROR" };
const AdaException __ada_exc_program_error = { "PROGRAM_ERROR" };
const AdaException __ada_exc_storage_error = { "STORAGE_ERROR" };
const AdaException __ada_exc_numeric_error = { "NUMERIC_ERROR" };
const AdaException __ada_exc_tasking_error = { "TASKING_ERROR" };
const AdaException __ada_exc_status_error = { "STATUS_ERROR" };
const AdaException __ada_exc_mode_error = { "MODE_ERROR" };
const AdaException __ada_exc_name_error = { "NAME_ERROR" };
const AdaException __ada_exc_use_error = { "USE_ERROR" };
const AdaException __ada_exc_device_error = { "DEVICE_ERROR" };
const AdaException __ada_exc_end_error = { "END_ERROR" };
const AdaException __ada_exc_data_error = { "DATA_ERROR" };
const AdaException __ada_exc_layout_error = { "LAYOUT_ERROR" };

/* Every object file compiled from Ada refers to this, so the run time is the
   one place that defines it. */
const AdaException* __ada_exception = NULL;
static char* pendingMessage;
static int pendingMessageLength;
static AdaTraceFrame* currentTrace;
static const char* pendingOrigin;
static int pendingTraceCount;
static AdaTraceEntry pendingTrace[ADA_TRACE_CAPACITY];

void __ada_trace_enter(AdaTraceFrame* frame, const char* routine, const char* location)
{
    frame->previous = currentTrace;
    frame->routine = routine;
    frame->location = location;
    currentTrace = frame;
}

void __ada_trace_location(const char* location)
{
    if (currentTrace != NULL) {
        currentTrace->location = location;
    }
}

void __ada_trace_leave(AdaTraceFrame* frame)
{
    currentTrace = frame->previous;
}

static void copyTrace(AdaExceptionOccurrence* target, const AdaExceptionOccurrence* source)
{
    target->origin = source->identity == NULL ? NULL : source->origin;
    target->traceCount = source->identity == NULL ? 0 : source->traceCount;
    memcpy(target->trace, source->trace, (size_t)target->traceCount * sizeof *target->trace);
}

void __ada_raise(const AdaException* exception)
{
    free(pendingMessage);
    pendingMessage = NULL;
    pendingMessageLength = 0;
    __ada_exception = exception == NULL ? ADA_PROGRAM_ERROR : exception;
    pendingOrigin = currentTrace == NULL ? NULL : currentTrace->location;
    pendingTraceCount = 0;
    for (AdaTraceFrame* frame = currentTrace; frame != NULL && pendingTraceCount < ADA_TRACE_CAPACITY;
         frame = frame->previous) {
        pendingTrace[pendingTraceCount++] = (AdaTraceEntry) { frame->routine, frame->location };
    }
}

void __ada_raise_message(const AdaException* exception, const char* message, int length)
{
    char* copy = NULL;
    if (exception == NULL || length < 0) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return;
    }
    if (length != 0) {
        copy = malloc((size_t)length);
        if (copy == NULL) {
            __ada_raise(ADA_STORAGE_ERROR);
            return;
        }
        memcpy(copy, message, (size_t)length);
    }
    __ada_raise(exception);
    pendingMessage = copy;
    pendingMessageLength = length;
}

void __ada_exception_capture(AdaExceptionOccurrence* target, void** owner)
{
    const AdaException* identity = __ada_exception;
    int length = pendingMessageLength;
    char* copy = NULL;
    if (length != 0) {
        copy = __ada_array_local(owner, 1, length, 1);
        if (copy == NULL) {
            return; /* The allocator has replaced the pending exception. */
        }
        memcpy(copy, pendingMessage, (size_t)length);
    }
    target->identity = identity;
    target->message = copy;
    target->length = length;
    target->origin = pendingOrigin;
    target->traceCount = pendingTraceCount;
    memcpy(target->trace, pendingTrace, (size_t)pendingTraceCount * sizeof *pendingTrace);
    free(pendingMessage);
    pendingMessage = NULL;
    pendingMessageLength = 0;
    __ada_exception = NULL;
}

void __ada_reraise(const AdaExceptionOccurrence* occurrence)
{
    if (occurrence->identity != NULL) {
        __ada_raise_message(occurrence->identity, occurrence->message, occurrence->length);
        // A failure while copying the message is a fresh Storage_Error. Only
        // a successful re-raise restores the original diagnostic snapshot.
        if (__ada_exception == occurrence->identity && pendingMessageLength == occurrence->length) {
            pendingOrigin = occurrence->origin;
            pendingTraceCount = occurrence->traceCount;
            memcpy(pendingTrace, occurrence->trace, (size_t)pendingTraceCount * sizeof *pendingTrace);
        }
    }
}

const AdaException* __ada_exception_identity(const AdaExceptionOccurrence* occurrence)
{
    return occurrence->identity;
}

const char* __ada_exception_name(const AdaException* exception)
{
    if (exception == NULL) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return "";
    }
    return exception->name;
}

int __ada_exception_message_length(const AdaExceptionOccurrence* occurrence)
{
    if (occurrence->identity == NULL) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    return occurrence->length;
}

void __ada_exception_message_copy(const AdaExceptionOccurrence* occurrence, char* target, int length)
{
    if (occurrence->identity == NULL || length != occurrence->length) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return;
    }
    if (length != 0) {
        memcpy(target, occurrence->message, (size_t)length);
    }
}

void __ada_save_occurrence(AdaExceptionOccurrence* target, const AdaExceptionOccurrence* source)
{
    if (target == source) {
        return;
    }
    int length = source->identity == NULL ? 0 : source->length;
    if (length > ADA_SAVED_MESSAGE_CAPACITY) {
        length = ADA_SAVED_MESSAGE_CAPACITY;
    }
    if (length != 0) {
        memmove(target->savedMessage, source->message, (size_t)length);
    }
    target->identity = source->identity;
    target->length = length;
    target->message = length == 0 ? NULL : target->savedMessage;
    copyTrace(target, source);
}

AdaExceptionOccurrence* __ada_save_occurrence_new(const AdaExceptionOccurrence* source)
{
    int length = source->identity == NULL ? 0 : source->length;
    // A single allocation lets ordinary Unchecked_Deallocation release both
    // the occurrence and its message without adding general finalization.
    AdaExceptionOccurrence* result = __ada_allocate((long)sizeof *result + length);
    if (result == NULL) {
        return NULL;
    }
    result->identity = source->identity;
    result->length = length;
    copyTrace(result, source);
    if (length != 0) {
        char* message = (char*)(result + 1);
        memcpy(message, source->message, (size_t)length);
        result->message = message;
    }
    return result;
}

static void informationAppend(char* target, size_t* position, const char* source, size_t length)
{
    if (target != NULL && length != 0) {
        memcpy(target + *position, source, length);
    }
    *position += length;
}

static size_t renderInformation(const AdaExceptionOccurrence* occurrence, char* target)
{
    size_t position = 0;
    const char* name = occurrence->identity->name;
    informationAppend(target, &position, name, strlen(name));
    if (occurrence->length != 0) {
        informationAppend(target, &position, ": ", 2);
        informationAppend(target, &position, occurrence->message, (size_t)occurrence->length);
    }
    if (occurrence->origin != NULL) {
        informationAppend(target, &position, "\nraised at ", 11);
        informationAppend(target, &position, occurrence->origin, strlen(occurrence->origin));
    }
    if (occurrence->traceCount != 0) {
        informationAppend(target, &position, "\nAda traceback:", 15);
        for (int i = 0; i < occurrence->traceCount; ++i) {
            const AdaTraceEntry* frame = &occurrence->trace[i];
            informationAppend(target, &position, "\n  ", 3);
            informationAppend(target, &position, frame->routine, strlen(frame->routine));
            informationAppend(target, &position, " at ", 4);
            informationAppend(target, &position, frame->location, strlen(frame->location));
        }
        if (occurrence->traceCount == ADA_TRACE_CAPACITY) {
            const char* limit = "\n  (trace limited to 32 frames)";
            informationAppend(target, &position, limit, strlen(limit));
        }
    }
    return position;
}

int __ada_exception_information_length(const AdaExceptionOccurrence* occurrence)
{
    if (occurrence->identity == NULL) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    size_t length = renderInformation(occurrence, NULL);
    if (length > INT_MAX) {
        __ada_raise(ADA_STORAGE_ERROR);
        return 0;
    }
    return (int)length;
}

void __ada_exception_information_copy(const AdaExceptionOccurrence* occurrence, char* target, int length)
{
    if (occurrence->identity == NULL || length < 0 || renderInformation(occurrence, NULL) != (size_t)length) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return;
    }
    renderInformation(occurrence, target);
}

void* __ada_allocate(long size)
{
    void* address;

    if (size <= 0) {
        size = 1;
    }
    address = calloc(1, (size_t)size);
    if (address == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
    }
    return address;
}

void __ada_deallocate(void* address)
{
    free(address);
}

/* Integer'Image renders a leading space in front of non-negative values.  The
   buffers rotate so that a few images can be combined in one expression. */
const char* __ada_image_integer(int value)
{
    static char buffers[IMAGE_BUFFERS][IMAGE_BUFFER_SIZE];
    static int next = 0;

    char* buffer = buffers[next];
    next = (next + 1) % IMAGE_BUFFERS;

    if (value < 0) {
        snprintf(buffer, IMAGE_BUFFER_SIZE, "%d", value);
    } else {
        snprintf(buffer, IMAGE_BUFFER_SIZE, " %d", value);
    }
    return buffer;
}

/* The image of an enumeration value is its literal in upper case, and the image
   of a character is the literal between its quotes. */
const char* __ada_image_enum(int value, const char** names, int count)
{
    static char buffers[IMAGE_BUFFERS][IMAGE_BUFFER_SIZE];
    static int next = 0;

    char* buffer = buffers[next];
    const char* name;
    int i;

    next = (next + 1) % IMAGE_BUFFERS;

    if (value < 0 || value >= count) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        buffer[0] = '\0';
        return buffer;
    }

    name = names[value];
    for (i = 0; name[i] != '\0' && i < IMAGE_BUFFER_SIZE - 1; ++i) {
        buffer[i] = (char)toupper((unsigned char)name[i]);
    }
    buffer[i] = '\0';
    return buffer;
}

const char* __ada_image_character(int value)
{
    static char buffers[IMAGE_BUFFERS][IMAGE_BUFFER_SIZE];
    static int next = 0;

    char* buffer = buffers[next];
    next = (next + 1) % IMAGE_BUFFERS;

    snprintf(buffer, IMAGE_BUFFER_SIZE, "'%c'", value);
    return buffer;
}

/* 'Value ignores the blanks around what it is given, so both ends are trimmed
   before anything is made of the rest. */
static void trim(const char* text, int length, int* from, int* to)
{
    int start = 0;
    int stop = length;

    while (start < stop && isspace((unsigned char)text[start])) {
        ++start;
    }
    while (stop > start && isspace((unsigned char)text[stop - 1])) {
        --stop;
    }
    *from = start;
    *to = stop;
}

/* Read a numeral without losing overflow or separator errors. The caller
   handles delimiters and requires that the complete input is consumed. */
static int readValueNumeral(const char* text, int stop, int* position, unsigned base,
                            unsigned long long limit, unsigned long long* value)
{
    int hasDigit = 0;
    int separator = 0;

    *value = 0;
    while (*position < stop) {
        unsigned char c = (unsigned char)text[*position];
        unsigned digit;
        if (c == '_') {
            if (!hasDigit || separator) {
                return 0;
            }
            separator = 1;
            ++*position;
            continue;
        }
        digit = c >= '0' && c <= '9' ? (unsigned)(c - '0')
                : c >= 'a' && c <= 'f' ? (unsigned)(c - 'a' + 10)
                : c >= 'A' && c <= 'F' ? (unsigned)(c - 'A' + 10) : 16;
        if (digit >= base) {
            break;
        }
        if (digit > limit || *value > (limit - digit) / base) {
            return 0;
        }
        *value = *value * base + digit;
        hasDigit = 1;
        separator = 0;
        ++*position;
    }
    return hasDigit && !separator;
}

long long __ada_value_long_integer(const char* text, int length, long long low, long long high)
{
    int start;
    int stop;
    int negative = 0;
    unsigned long long value = 0;
    unsigned long long limit;
    long long result;
    int i;

    trim(text, length, &start, &stop);
    if (start < stop && (text[start] == '+' || text[start] == '-')) {
        negative = text[start] == '-';
        ++start;
    }
    limit = (unsigned long long)LLONG_MAX + negative;
    i = start;
    if (!readValueNumeral(text, stop, &i, 10, limit, &value)) {
        goto invalid;
    }
    unsigned base = 10;
    if (i < stop && text[i] == '#') {
        if (value < 2 || value > 16) {
            goto invalid;
        }
        base = (unsigned)value;
        ++i;
        if (!readValueNumeral(text, stop, &i, base, limit, &value)
            || i == stop || text[i] != '#') {
            goto invalid;
        }
        ++i;
    }
    if (i < stop && (text[i] == 'e' || text[i] == 'E')) {
        unsigned long long exponent = 0;
        ++i;
        if (i < stop && text[i] == '+') {
            ++i;
        }
        // Saturate the exponent: no nonzero 64-bit value can survive 64
        // multiplications, but zero remains valid even for huge exponents.
        int exponentStart = i;
        int separator = 0;
        for (; i < stop; ++i) {
            if (text[i] == '_' && i > exponentStart && !separator) {
                separator = 1;
                continue;
            }
            if (text[i] < '0' || text[i] > '9') {
                goto invalid;
            }
            separator = 0;
            if (exponent < 64) {
                exponent = exponent * 10 + (unsigned)(text[i] - '0');
            }
        }
        if (i == exponentStart || separator) {
            goto invalid;
        }
        while (value != 0 && exponent != 0) {
            if (value > limit / base) {
                goto invalid;
            }
            value *= base;
            --exponent;
        }
    }
    if (i != stop) {
        goto invalid;
    }
    result = negative ? (value == (unsigned long long)LLONG_MAX + 1 ? LLONG_MIN : -(long long)value)
                      : (long long)value;
    if (result < low || result > high) {
        goto invalid;
    }
    return result;

invalid:
    __ada_raise(ADA_CONSTRAINT_ERROR);
    return low;
}

int __ada_value_integer(const char* text, int length, int low, int high)
{
    return (int)__ada_value_long_integer(text, length, low, high);
}

const char* __ada_image_long_integer(long long value)
{
    static char buffers[IMAGE_BUFFERS][IMAGE_BUFFER_SIZE];
    static int next = 0;
    char* buffer = buffers[next];
    next = (next + 1) % IMAGE_BUFFERS;
    snprintf(buffer, IMAGE_BUFFER_SIZE, value < 0 ? "%lld" : " %lld", value);
    return buffer;
}

long long __ada_modular_operation(ModularOperation operation, long long modulus, long long left, long long right)
{
    long long result = 0;
    if (!modularOperation(operation, modulus, left, right, &result)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    }
    return result;
}

long long __ada_integer_operation(int operation, int bits, long long left, long long right)
{
    long long result = 0;
    long long low = bits == 32 ? INT_MIN : LLONG_MIN;
    long long high = bits == 32 ? INT_MAX : LLONG_MAX;
    int overflow = 0;

    switch (operation) {
    case 0:
        overflow = __builtin_add_overflow(left, right, &result);
        break;
    case 1:
        overflow = __builtin_sub_overflow(left, right, &result);
        break;
    case 2:
        overflow = __builtin_mul_overflow(left, right, &result);
        break;
    case 3:
    case 4:
    case 5:
        if (right == 0) {
            overflow = 1;
        } else if (left == LLONG_MIN && right == -1) {
            overflow = operation == 3;
        } else if (operation == 3) {
            result = left / right;
        } else {
            result = left % right;
            if (operation == 5 && result != 0 && (result < 0) != (right < 0)) {
                result += right;
            }
        }
        break;
    case 6:
        if (right < 0) {
            overflow = 1;
            break;
        }
        result = 1;
        while (right != 0) {
            if ((right & 1) && (__builtin_mul_overflow(result, left, &result)
                               || result < low || result > high)) {
                overflow = 1;
                break;
            }
            right >>= 1;
            if (right != 0 && (__builtin_mul_overflow(left, left, &left)
                              || left < low || left > high)) {
                overflow = 1;
                break;
            }
        }
        break;
    default:
        overflow = 1;
        break;
    }
    if (overflow || result < low || result > high) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    return result;
}

/* An enumeration literal is named without regard to case, which is why the
   comparison here folds both sides rather than the text alone. */
int __ada_value_enum(const char* text, int length, const char** names, int count)
{
    int start;
    int stop;
    int i;

    trim(text, length, &start, &stop);
    for (i = 0; i < count; ++i) {
        const char* name = names[i];
        int j;

        for (j = 0; j < stop - start && name[j] != '\0'; ++j) {
            if (tolower((unsigned char)text[start + j]) != tolower((unsigned char)name[j])) {
                break;
            }
        }
        if (j == stop - start && name[j] == '\0') {
            return i;
        }
    }

    __ada_raise(ADA_CONSTRAINT_ERROR);
    return 0;
}

/* A character is named by its spelling, so its value is written between quotes
   the way __ada_image_character writes it. */
int __ada_value_character(const char* text, int length)
{
    int start;
    int stop;

    trim(text, length, &start, &stop);
    if (stop - start != 3 || text[start] != '\'' || text[stop - 1] != '\'') {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    return (unsigned char)text[start + 1];
}

/* Arrays compare element by element; a prefix is smaller than what extends it. */
int __ada_string_compare(const char* left, int leftLength, const char* right, int rightLength)
{
    int shorter = leftLength < rightLength ? leftLength : rightLength;
    int i;

    for (i = 0; i < shorter; ++i) {
        unsigned char leftCharacter = (unsigned char)left[i];
        unsigned char rightCharacter = (unsigned char)right[i];
        if (leftCharacter != rightCharacter) {
            return leftCharacter < rightCharacter ? -1 : 1;
        }
    }
    if (leftLength == rightLength) {
        return 0;
    }
    return leftLength < rightLength ? -1 : 1;
}

/* Renders a real value the way Ada.Text_IO does.  Fore is the least number of
   characters before the point including the sign, Aft the number after it, and
   Exp the width of the exponent field counting its sign.  An Exp of zero asks
   for plain decimal notation. */
void __ada_format_float(char* buffer, int size, double value, int fore, int aft, int exponent)
{
    char digits[64];
    char sign;
    int power;
    int written;
    int i;

    if (fore < 1) {
        fore = 1;
    }
    if (aft < 1) {
        aft = 1;
    }

    if (exponent <= 0) {
        snprintf(buffer, size, "%*.*f", fore + 1 + aft, aft, value);
        return;
    }

    sign = value < 0.0 ? '-' : ' ';
    snprintf(digits, sizeof digits, "%.*e", aft, value < 0.0 ? -value : value);

    /* Split the mantissa from the exponent that printf produced. */
    {
        char* marker = strchr(digits, 'e');
        power = marker != 0 ? (int)strtol(marker + 1, 0, 10) : 0;
        if (marker != 0) {
            *marker = '\0';
        }
    }

    /* Ada counts the sign as part of the exponent field. */
    written = snprintf(buffer, size, "%*c%sE%c", fore - 1, sign, digits, power < 0 ? '-' : '+');
    if (written < 0 || written >= size) {
        return;
    }
    power = power < 0 ? -power : power;
    snprintf(buffer + written, size - written, "%0*d", exponent - 1, power);
}

void __ada_put_float(double value, int fore, int aft, int exponent)
{
    char buffer[128];

    __ada_format_float(buffer, (int)sizeof buffer, value, fore, aft, exponent);
    fputs(buffer, __ada_output_stream());
}

const char* __ada_image_float(double value, int aft, int exponent)
{
    static char buffers[IMAGE_BUFFERS][IMAGE_BUFFER_SIZE];
    static int next = 0;

    char* buffer = buffers[next];
    next = (next + 1) % IMAGE_BUFFERS;

    __ada_format_float(buffer, IMAGE_BUFFER_SIZE, value, 2, aft, exponent);
    return buffer;
}

/* Ada rounds away from zero when a real value becomes an integer, while a cast
   would drop the fraction. */
long long __ada_round_to_integer(double value)
{
    long long result;
    double fraction;
    /* The upper endpoint is exclusive: LLONG_MAX rounds to 2**63 as a double.
       This comparison also rejects NaNs before the C floating-to-integer cast. */
    if (!(value >= -9223372036854775808.0 && value < 9223372036854775808.0)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    result = (long long)value;
    fraction = value - (double)result;
    if (fraction >= 0.5) {
        ++result;
    } else if (fraction <= -0.5) {
        --result;
    }
    return result;
}

void __ada_unhandled(const AdaException* exception)
{
    fflush(stdout);
    fprintf(stderr, "\nraised %s", exception == NULL ? "EXCEPTION" : exception->name);
    if (pendingMessageLength != 0) {
        fputs(" : ", stderr);
        fwrite(pendingMessage, 1, (size_t)pendingMessageLength, stderr);
    }
    fputc('\n', stderr);
    free(pendingMessage);
    pendingMessage = NULL;
    pendingMessageLength = 0;
}

/* Internal Ada ABI: an unconstrained result carries a transfer buffer, its
   first dimension's bounds, and the buffer size. The emitter appends any
   further dimension bounds after this 24-byte header. The caller adopts the
   buffer into its temporary allocation list; no callee stack pointer escapes. */
void __ada_array_result(void* descriptor, const void* source, int first, int last, int64_t elementSize)
{
    int64_t length = last < first ? 0 : (int64_t)last - (int64_t)first + 1;
    int64_t size;
    void* buffer;

    if (elementSize < 0 || (elementSize != 0 && length > INT64_MAX / elementSize)) {
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    size = length * elementSize;
    if ((uint64_t)size > SIZE_MAX) {
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    buffer = calloc(1, size == 0 ? 1 : (size_t)size);
    if (buffer == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    if (size != 0) {
        memcpy(buffer, source, (size_t)size);
    }
    memcpy(descriptor, &buffer, sizeof buffer);
    memcpy((char*)descriptor + 8, &first, sizeof first);
    memcpy((char*)descriptor + 12, &last, sizeof last);
    /* alloc8 requires storage even for a null result. */
    if (size == 0) {
        size = 1;
    }
    memcpy((char*)descriptor + 16, &size, sizeof size);
}

/* Each activation has separate local-object and temporary allocation lists.
   Checkpoints release a suffix on scope/statement exit; returns release all. */
typedef struct AdaArrayAllocation
{
    struct AdaArrayAllocation* next;
    void* data;
} AdaArrayAllocation;

/* Checked row strides for multidimensional arrays. Descriptor lengths and
   array loops currently use signed 32-bit counts. */
int64_t __ada_array_size(int first, int last, int64_t elementSize)
{
    int64_t length = last < first ? 0 : (int64_t)last - first + 1;
    if (length > INT_MAX || elementSize < 0
        || (elementSize != 0 && length > INT64_MAX / elementSize)) {
        __ada_raise(ADA_STORAGE_ERROR);
        return 0;
    }
    return length * elementSize;
}

void* __ada_array_local(void** owner, int first, int last, int64_t elementSize)
{
    int64_t length = last < first ? 0 : (int64_t)last - first + 1;
    AdaArrayAllocation* allocation;
    size_t size;
    if (length > INT_MAX || elementSize < 0
        || (elementSize != 0 && (uint64_t)length > SIZE_MAX / (uint64_t)elementSize)) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    size = (size_t)length * (size_t)elementSize;
    allocation = malloc(sizeof *allocation);
    if (allocation == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    allocation->data = calloc(1, size == 0 ? 1 : size);
    if (allocation->data == NULL) {
        free(allocation);
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    allocation->next = *owner;
    *owner = allocation;
    return allocation->data;
}

void __ada_array_rewind(void** owner, void* checkpoint)
{
    AdaArrayAllocation* allocation = *owner;
    while (allocation != checkpoint) {
        AdaArrayAllocation* next = allocation->next;
        free(allocation->data);
        free(allocation);
        allocation = next;
    }
    *owner = checkpoint;
}

void __ada_array_release(void** owner)
{
    __ada_array_rewind(owner, NULL);
}

void __ada_array_adopt(void** owner, void* data)
{
    AdaArrayAllocation* allocation = malloc(sizeof *allocation);
    if (allocation == NULL) {
        free(data);
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    allocation->data = data;
    allocation->next = *owner;
    *owner = allocation;
}
