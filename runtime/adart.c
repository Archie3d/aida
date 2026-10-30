/* Minimal run time support for programs compiled by adac. */

#include "adart.h"

#include "adaio.h"
#include "adamodulartext.h"

#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define IMAGE_BUFFER_SIZE 64

const AdaException __ada_exc_constraint_error = { "CONSTRAINT_ERROR" };
const AdaException __ada_exc_program_error = { "PROGRAM_ERROR" };
const AdaException __ada_exc_storage_error = { "STORAGE_ERROR" };
const AdaException __ada_exc_numeric_error = { "NUMERIC_ERROR" };
const AdaException __ada_exc_tasking_error = { "TASKING_ERROR" };
const AdaException __ada_exc_status_error = { "ADA.IO_EXCEPTIONS.STATUS_ERROR" };
const AdaException __ada_exc_mode_error = { "ADA.IO_EXCEPTIONS.MODE_ERROR" };
const AdaException __ada_exc_name_error = { "ADA.IO_EXCEPTIONS.NAME_ERROR" };
const AdaException __ada_exc_use_error = { "ADA.IO_EXCEPTIONS.USE_ERROR" };
const AdaException __ada_exc_device_error = { "ADA.IO_EXCEPTIONS.DEVICE_ERROR" };
const AdaException __ada_exc_end_error = { "ADA.IO_EXCEPTIONS.END_ERROR" };
const AdaException __ada_exc_data_error = { "ADA.IO_EXCEPTIONS.DATA_ERROR" };
const AdaException __ada_exc_layout_error = { "ADA.IO_EXCEPTIONS.LAYOUT_ERROR" };

/* C TLS stays inside the runtime: generated QBE never refers to a TLS symbol. */
AdaTaskContext __ada_main_context = {
    .sharedStorageMutex = ADA_MUTEX_INITIALIZER,
    .sharedMasterMutex = ADA_MUTEX_INITIALIZER,
    .sharedMasterChanged = ADA_CONDITION_INITIALIZER
};
static _Thread_local AdaTaskContext* currentTaskContext = &__ada_main_context;

_Static_assert(offsetof(AdaTaskContext, exception) == 0, "pending exception context ABI");
_Static_assert(sizeof(AdaTraceFrame) == 24, "trace frame ABI");
_Static_assert(offsetof(AdaTraceFrame, location) == 16, "trace location ABI");

AdaTaskContext* __ada_task_context(void)
{
    return currentTaskContext;
}

AdaTaskContext* __ada_task_context_bind(AdaTaskContext* context)
{
    AdaTaskContext* previous = currentTaskContext;
    currentTaskContext = context == NULL ? &__ada_main_context : context;
    return previous;
}

int __ada_task_context_dispose(AdaTaskContext* context)
{
    if (context == NULL) {
        return 0;
    }
    int environment = context == &__ada_main_context;
    if (environment) {
        __ada_mutex_lock(&context->sharedStorageMutex);
        __ada_mutex_lock(&context->sharedMasterMutex);
    }
    int live = context->currentTrace != NULL || context->registeredFinalizations != NULL
        || context->libraryFinalizations != NULL || context->libraryFinalizationArena != NULL
        || context->libraryFinalizing || context->currentMaster != NULL || context->taskMasters != NULL
        || context->protectedActions != NULL || context->abortDeferralDepth != 0;
    if (environment) {
        live |= context->registeredAllocations != NULL || context->sharedFinalizations != NULL
            || context->sharedMasters != NULL;
    }
    if (!live) {
        free(context->pendingMessage);
        size_t bytes = environment ? offsetof(AdaTaskContext, sharedStorageMutex) : sizeof *context;
        memset(context, 0, bytes);
    }
    if (environment) {
        __ada_mutex_unlock(&context->sharedMasterMutex);
        __ada_mutex_unlock(&context->sharedStorageMutex);
    }
    return !live;
}

void __ada_abort_defer(AdaTaskContext* context)
{
    if (context != __ada_task_context() || context->abortDeferralDepth == UINT_MAX) {
        abort();
    }
    ++context->abortDeferralDepth;
}

void __ada_abort_undefer(AdaTaskContext* context)
{
    if (context != __ada_task_context() || context->abortDeferralDepth == 0) {
        abort();
    }
    --context->abortDeferralDepth;
}

static int argumentCount;
static char** argumentValues;
/* Process-wide status; it does not publish or synchronize other task state. */
static atomic_int exitStatus;

void __ada_command_line_init(int argc, char** argv)
{
    argumentCount = argc > 0 && argv != NULL ? argc - 1 : 0;
    argumentValues = argc > 0 ? argv : NULL;
    atomic_store_explicit(&exitStatus, 0, memory_order_relaxed);
}

int __ada_argument_count(void)
{
    return argumentCount;
}

static const char* commandString(const char* value)
{
    if (value == NULL) {
        return "";
    }
    if (strlen(value) > INT32_MAX) {
        __ada_raise(ADA_STORAGE_ERROR);
        return "";
    }
    return value;
}

const char* __ada_argument(int number)
{
    if (number < 1 || number > argumentCount) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return "";
    }
    return commandString(argumentValues[number]);
}

const char* __ada_command_name(void)
{
    return commandString(argumentValues == NULL ? NULL : argumentValues[0]);
}

void __ada_set_exit_status(int code)
{
    atomic_store_explicit(&exitStatus, code, memory_order_relaxed);
}

int __ada_get_exit_status(void)
{
    return atomic_load_explicit(&exitStatus, memory_order_relaxed);
}

AdaTaskContext* __ada_trace_enter(AdaTraceFrame* frame, const char* routine, const char* location)
{
    AdaTaskContext* context = __ada_task_context();
    frame->previous = context->currentTrace;
    frame->routine = routine;
    frame->location = location;
    context->currentTrace = frame;
    return context;
}

void __ada_trace_location(const char* location)
{
    AdaTaskContext* context = __ada_task_context();
    if (context->currentTrace != NULL) {
        context->currentTrace->location = location;
    }
}

void __ada_trace_leave_context(AdaTaskContext* context, AdaTraceFrame* frame)
{
    context->currentTrace = frame->previous;
}

void __ada_trace_leave(AdaTraceFrame* frame)
{
    __ada_trace_leave_context(__ada_task_context(), frame);
}

static void copyTrace(AdaExceptionOccurrence* target, const AdaExceptionOccurrence* source)
{
    target->origin = source->identity == NULL ? NULL : source->origin;
    target->traceCount = source->identity == NULL ? 0 : source->traceCount;
    memcpy(target->trace, source->trace, (size_t)target->traceCount * sizeof *target->trace);
}

void __ada_raise(const AdaException* exception)
{
    AdaTaskContext* context = __ada_task_context();
    free(context->pendingMessage);
    context->pendingMessage = NULL;
    context->pendingMessageLength = 0;
    context->exception = exception == NULL ? ADA_PROGRAM_ERROR : exception;
    context->pendingOrigin = context->currentTrace == NULL ? NULL : context->currentTrace->location;
    context->pendingTraceCount = 0;
    for (AdaTraceFrame* frame = context->currentTrace;
         frame != NULL && context->pendingTraceCount < ADA_TRACE_CAPACITY; frame = frame->previous) {
        context->pendingTrace[context->pendingTraceCount++] = (AdaTraceEntry) { frame->routine, frame->location };
    }
}

void __ada_raise_message(const AdaException* exception, const char* message, int length)
{
    AdaTaskContext* context = __ada_task_context();
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
    context->pendingMessage = copy;
    context->pendingMessageLength = length;
}

void __ada_exception_capture(AdaExceptionOccurrence* target, void** owner)
{
    AdaTaskContext* context = __ada_task_context();
    const AdaException* identity = context->exception;
    int length = context->pendingMessageLength;
    char* copy = NULL;
    if (length != 0) {
        copy = __ada_array_local(owner, 1, length, 1);
        if (copy == NULL) {
            return; /* The allocator has replaced the pending exception. */
        }
        memcpy(copy, context->pendingMessage, (size_t)length);
    }
    target->ownedMessage = NULL;
    target->identity = identity;
    target->message = copy;
    target->length = length;
    target->origin = context->pendingOrigin;
    target->traceCount = context->pendingTraceCount;
    memcpy(target->trace, context->pendingTrace,
           (size_t)context->pendingTraceCount * sizeof *context->pendingTrace);
    free(context->pendingMessage);
    context->pendingMessage = NULL;
    context->pendingMessageLength = 0;
    context->exception = NULL;
}

void __ada_reraise(const AdaExceptionOccurrence* occurrence)
{
    AdaTaskContext* context = __ada_task_context();
    if (occurrence->identity != NULL) {
        __ada_raise_message(occurrence->identity, occurrence->message, occurrence->length);
        // A failure while copying the message is a fresh Storage_Error. Only
        // a successful re-raise restores the original diagnostic snapshot.
        if (context->exception == occurrence->identity && context->pendingMessageLength == occurrence->length) {
            context->pendingOrigin = occurrence->origin;
            context->pendingTraceCount = occurrence->traceCount;
            memcpy(context->pendingTrace, occurrence->trace,
                   (size_t)context->pendingTraceCount * sizeof *context->pendingTrace);
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
    free(target->ownedMessage);
    target->ownedMessage = NULL;
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
    result->ownedMessage = NULL;
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

static int releaseControlledAllocation(void* address);

void __ada_deallocate(void* address)
{
    if (address == NULL) {
        return;
    }
    if (!releaseControlledAllocation(address)) {
        free(address);
    }
}

/* Images belong to the caller. Return a length as well as a C terminator so
   Character'Image can contain a zero byte without losing its closing quote. */
static int imageLength(int length, int capacity)
{
    if (length < 0 || length >= capacity) {
        __ada_raise(ADA_STORAGE_ERROR);
        return 0;
    }
    return length;
}

/* Integer'Image renders a leading space in front of non-negative values. */
int __ada_image_integer(char* buffer, int capacity, int value)
{
    return imageLength(snprintf(buffer, capacity > 0 ? (size_t)capacity : 0,
                                value < 0 ? "%d" : " %d", value), capacity);
}

/* The image of an enumeration value is its literal in upper case, and the image
   of a character is the literal between its quotes. */
int __ada_image_enum(char* buffer, int capacity, int value, const char** names, int count)
{
    if (value < 0 || value >= count) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    const char* name = names[value];
    size_t length = strlen(name);
    if (capacity <= 0 || length >= (size_t)capacity) {
        __ada_raise(ADA_STORAGE_ERROR);
        return 0;
    }
    for (size_t i = 0; i < length; ++i) {
        buffer[i] = name[0] == '\'' ? name[i] : (char)toupper((unsigned char)name[i]);
    }
    buffer[length] = '\0';
    return (int)length;
}

int __ada_image_character(char* buffer, int capacity, int value)
{
    if (capacity < 4) {
        __ada_raise(ADA_STORAGE_ERROR);
        return 0;
    }
    buffer[0] = '\'';
    buffer[1] = (char)value;
    buffer[2] = '\'';
    buffer[3] = '\0';
    return 3;
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

static long long valueInteger(const char* text, int length, long long low, long long high, int isUnsigned)
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
    limit = isUnsigned ? ULLONG_MAX : (unsigned long long)LLONG_MAX + negative;
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
    if (isUnsigned && negative) {
        goto invalid;
    }
    result = negative ? (value == (unsigned long long)LLONG_MAX + 1 ? LLONG_MIN : -(long long)value)
                      : (long long)value;
    if (isUnsigned ? value < (unsigned long long)low || value > (unsigned long long)high
                   : result < low || result > high) {
        goto invalid;
    }
    return result;

invalid:
    __ada_raise(ADA_CONSTRAINT_ERROR);
    return low;
}

long long __ada_value_long_integer(const char* text, int length, long long low, long long high)
{
    return valueInteger(text, length, low, high, 0);
}

int __ada_modular_token(const char* text, int length)
{
    int state = 0;
    int position = 0;
    while (position < length) {
        state = modularTokenState(state, (unsigned char)text[position]);
        if (state < 0) { break; }
        ++position;
    }
    return position;
}

long long __ada_value_modular(const char* text, int length, long long modulus)
{
    return valueInteger(text, length, 0, (long long)((uint64_t)modulus - 1), 1);
}

int __ada_image_modular(char* buffer, int capacity, unsigned long long value)
{
    return imageLength(snprintf(buffer, capacity > 0 ? (size_t)capacity : 0, " %llu", value), capacity);
}

long long __ada_modular_minmax(unsigned long long left, unsigned long long right, int maximum)
{
    return (long long)((maximum ? left > right : left < right) ? left : right);
}

long long __ada_round_to_modular(double value)
{
    value = round(value);
    if (!(value >= 0 && value < 18446744073709551616.0)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    return (long long)(unsigned long long)value;
}

long long __ada_modular_reduce(long long value, int isUnsigned, long long modulus)
{
    __int128 m = (uint64_t)modulus;
    if (m == 0) { m = (__int128)1 << 64; }
    __int128 reduced = isUnsigned ? (__int128)(uint64_t)value : (__int128)value;
    reduced %= m;
    if (reduced < 0) { reduced += m; }
    return (long long)reduced;
}

long long __ada_modular_convert(long long value, int isUnsigned, long long low, long long high, int targetUnsigned)
{
    __int128 v = isUnsigned ? (__int128)(uint64_t)value : (__int128)value;
    __int128 first = targetUnsigned ? (__int128)(uint64_t)low : (__int128)low;
    __int128 last = targetUnsigned ? (__int128)(uint64_t)high : (__int128)high;
    if (v < first || v > last) { __ada_raise(ADA_CONSTRAINT_ERROR); }
    return value;
}

int __ada_value_integer(const char* text, int length, int low, int high)
{
    return (int)__ada_value_long_integer(text, length, low, high);
}

int __ada_image_long_integer(char* buffer, int capacity, long long value)
{
    return imageLength(snprintf(buffer, capacity > 0 ? (size_t)capacity : 0,
                                value < 0 ? "%lld" : " %lld", value), capacity);
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
            if (name[0] == '\'' ? text[start + j] != name[j]
                : tolower((unsigned char)text[start + j]) != tolower((unsigned char)name[j])) {
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
    int digitCount = snprintf(digits, sizeof digits, "%d", power);
    int padding = exponent - 1 - digitCount;
    /* Preserve the formatted prefix when the exponent field does not fit,
       without asking printf to produce an arbitrarily large padded field. */
    while (padding > 0 && written < size - 1) {
        buffer[written++] = '0';
        --padding;
    }
    for (i = 0; i < digitCount && written < size - 1; ++i) {
        buffer[written++] = digits[i];
    }
    buffer[written] = '\0';
}

void __ada_put_float(double value, int fore, int aft, int exponent)
{
    char buffer[128];

    __ada_format_float(buffer, (int)sizeof buffer, value, fore, aft, exponent);
    __ada_put(buffer, (int)strlen(buffer));
}

int __ada_image_float(char* buffer, int capacity, double value, int aft, int exponent)
{
    char text[IMAGE_BUFFER_SIZE];
    __ada_format_float(text, sizeof text, value, 2, aft, exponent);
    int length = (int)strlen(text);
    if (imageLength(length, capacity) == 0) {
        return 0;
    }
    memcpy(buffer, text, (size_t)length + 1);
    return length;
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
    AdaTaskContext* context = __ada_task_context();
    fflush(stdout);
    fprintf(stderr, "\nraised %s", exception == NULL ? "EXCEPTION" : exception->name);
    if (context->pendingMessageLength != 0) {
        fputs(" : ", stderr);
        fwrite(context->pendingMessage, 1, (size_t)context->pendingMessageLength, stderr);
    }
    fputc('\n', stderr);
    free(context->pendingMessage);
    context->pendingMessage = NULL;
    context->pendingMessageLength = 0;
}

static int arrayLengthChecked(int64_t first, int64_t last, int64_t* length);

_Static_assert(sizeof(void*) == 8 && sizeof(ptrdiff_t) == 8, "64-bit array ABI required");
_Static_assert(sizeof(AdaArrayDimension) == 32, "array dimension ABI");
_Static_assert(offsetof(AdaArrayResult, length) == 32, "array length ABI");
_Static_assert(offsetof(AdaArrayResult, stride) == 40, "array stride ABI");
_Static_assert(sizeof(AdaArrayResult) == 48, "array result ABI");
_Static_assert(offsetof(AdaArrayResult, first) == 16, "array result bounds ABI");
_Static_assert(offsetof(AdaArrayResult, last) == 24, "array result bounds ABI");
_Static_assert(offsetof(AdaArrayResult, size) == 8, "array result size ABI");

/* Internal Ada ABI: an unconstrained result carries a transfer buffer, its
   first dimension's bounds, and the buffer size. The emitter appends any
   further dimension records after this 48-byte header. The caller adopts the
   buffer into its temporary allocation list; no callee stack pointer escapes. */
void __ada_array_result(void* descriptor, const void* source, int64_t first, int64_t last, int64_t elementSize)
{
    int64_t length;
    if (!arrayLengthChecked(first, last, &length)) { return; }
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
    memcpy((char*)descriptor + adaArrayResultFirst(0), &first, sizeof first);
    memcpy((char*)descriptor + adaArrayResultFirst(0) + AdaArrayBoundBytes, &last, sizeof last);
    /* alloc8 requires storage even for a null result. */
    if (size == 0) {
        size = 1;
    }
    memcpy((char*)descriptor + AdaArrayTransferOffset, &size, sizeof size);
    memcpy((char*)descriptor + adaArrayResultFirst(0) + 16, &length, sizeof length);
    memcpy((char*)descriptor + adaArrayResultFirst(0) + 24, &elementSize, sizeof elementSize);
}

/* A return transfer is not adopted by the caller if a local finalizer or
   dependent task fails while the callee is leaving. Preserve that exception
   and discard only the untransferred buffer. The descriptor starts at null. */
void __ada_array_result_cleanup(AdaTaskContext* context, void* descriptor)
{
    if (context->exception != NULL) {
        void* buffer;
        memcpy(&buffer, descriptor, sizeof buffer);
        free(buffer);
        buffer = NULL;
        memcpy(descriptor, &buffer, sizeof buffer);
    }
}

/* Each activation has separate local-object and temporary allocation lists.
   Checkpoints release a suffix on scope/statement exit; returns release all. */
typedef struct AdaArrayAllocation
{
    struct AdaArrayAllocation* next;
    void* data;
} AdaArrayAllocation;

/* Counts and byte sizes are nonnegative signed 64-bit quantities. Return
   status directly: allocation is also used while capturing a pending exception. */
static int arrayLengthChecked(int64_t first, int64_t last, int64_t* length)
{
    if (last < first) { *length = 0; return 1; }
    uint64_t span = (uint64_t)last - (uint64_t)first;
    if (span >= INT64_MAX) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
    *length = (int64_t)(span + 1);
    return 1;
}

int64_t __ada_array_length(int64_t first, int64_t last)
{
    int64_t length = 0;
    arrayLengthChecked(first, last, &length);
    return length;
}

int __ada_array_narrow(int64_t length)
{
    if (length < 0 || length > INT_MAX) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    return (int)length;
}

int64_t __ada_array_offset(int64_t index, int64_t first, int64_t last, int64_t stride)
{
    if (index < first || index > last) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    uint64_t distance = (uint64_t)index - (uint64_t)first;
    if (stride < 0 || (stride != 0 && distance > INT64_MAX / (uint64_t)stride)) {
        __ada_raise(ADA_STORAGE_ERROR);
        return 0;
    }
    return (int64_t)(distance * (uint64_t)stride);
}

int64_t __ada_array_slice_offset(int64_t low, int64_t high, int64_t first, int64_t last, int64_t stride)
{
    /* Null slices preserve their bounds without forming an invalid pointer. */
    if (high < low) { return 0; }
    if (high > last) { __ada_raise(ADA_CONSTRAINT_ERROR); return 0; }
    return __ada_array_offset(low, first, last, stride);
}

static int arraySizeChecked(int64_t first, int64_t last, int64_t elementSize, int64_t* bytes)
{
    int64_t length;
    if (!arrayLengthChecked(first, last, &length)) { return 0; }
    if (elementSize < 0 || (elementSize != 0 && length > INT64_MAX / elementSize)) {
        __ada_raise(ADA_STORAGE_ERROR);
        return 0;
    }
    *bytes = length * elementSize;
    return 1;
}

int64_t __ada_array_size(int64_t first, int64_t last, int64_t elementSize)
{
    int64_t bytes = 0;
    arraySizeChecked(first, last, elementSize, &bytes);
    return bytes;
}

/* RM 4.5.3: a null left operand yields the complete right bounds;
   otherwise the ancestor definition determines the lower bound. */
int64_t __ada_array_concat_shape(AdaArrayDimension* shape, int64_t leftFirst, int64_t leftLength,
    int64_t rightFirst, int64_t rightLast, int64_t rightLength, int64_t indexFirst, int64_t indexLast,
    int constrainedAncestor, int64_t elementSize)
{
    if (leftLength < 0 || rightLength < 0 || leftLength > INT64_MAX - rightLength) {
        __ada_raise(ADA_STORAGE_ERROR);
        return 0;
    }
    int64_t length = leftLength + rightLength;
    int64_t first = rightFirst, last = rightLast;
    if (leftLength != 0) {
        first = constrainedAncestor ? indexFirst : leftFirst;
        __int128 high = (__int128)first + length - 1;
        if (high > INT64_MAX) { __ada_raise(ADA_CONSTRAINT_ERROR); return 0; }
        last = (int64_t)high;
    }
    if (length != 0 && (last < indexFirst || last > indexLast)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    int64_t bytes;
    if (!arraySizeChecked(first, last, elementSize, &bytes)) { return 0; }
    *shape = (AdaArrayDimension) { first, last, length, elementSize };
    return bytes;
}

/* Tagged dependent components own separate arenas so an ancestor-view
   assignment cannot release storage belonging to the extension. */
void __ada_component_array_copy(void* descriptor, int rank)
{
    void** data = descriptor;
    AdaArrayDimension* dimensions = (AdaArrayDimension*)((char*)descriptor + 8);
    void** arena = (void**)((char*)descriptor + adaArrayLocalSize(rank));
    void* source = *data;
    *arena = NULL;
    void* copy = __ada_array_local(arena, dimensions[0].first, dimensions[0].last, dimensions[0].stride);
    if (copy == NULL) { return; }
    int64_t bytes = __ada_array_size(dimensions[0].first, dimensions[0].last, dimensions[0].stride);
    if (__ada_task_context()->exception != NULL) { return; }
    if (bytes != 0) { memcpy(copy, source, (size_t)bytes); }
    *data = copy;
}

void* __ada_array_local(void** owner, int64_t first, int64_t last, int64_t elementSize)
{
    int64_t bytes;
    if (!arraySizeChecked(first, last, elementSize, &bytes)) { return NULL; }
    AdaArrayAllocation* allocation;
    size_t size;
    if ((uint64_t)bytes > SIZE_MAX) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    size = (size_t)bytes;
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


long long __ada_fixed_operation(FixedOperation operation, long long left, int leftBits,
                               long long right, int rightBits, int resultBits)
{
    long long result = 0;
    if (!fixedOperation(operation, left, leftBits, right, rightBits, resultBits, &result)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    }
    return result;
}

long long __ada_fixed_minmax(long long left, long long right, int maximum)
{
    return (maximum ? left > right : left < right) ? left : right;
}

long long __ada_fixed_fore(long long low, long long high, int bits)
{
    unsigned __int128 a = low < 0 ? 0 - (unsigned __int128)low : (unsigned __int128)low;
    unsigned __int128 b = high < 0 ? 0 - (unsigned __int128)high : (unsigned __int128)high;
    unsigned __int128 whole = a > b ? a : b;
    if (fixedIsDecimal(bits)) {
        int scale = fixedFives(bits);
        unsigned __int128 factor = (unsigned __int128)fixedPower10(scale < 0 ? -scale : scale);
        whole = scale >= 0 ? whole / factor : whole * factor;
    } else { whole = bits >= 0 ? whole >> bits : whole << -bits; }
    long long width = 2;
    while (whole >= 10) { whole /= 10; ++width; }
    return width;
}

long long __ada_fixed_numeric_operation(FixedOperation operation, long long left, int leftBits,
                                       long long right, int rightBits, int isUnsigned)
{
    long long result = 0;
    if (!fixedNumericOperation(operation, left, leftBits, right, rightBits, isUnsigned, &result)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    }
    return result;
}

long long __ada_modular_to_fixed(unsigned long long value, int bits)
{
    long long result = 0;
    if (!fixedValidScale(bits)
        || !fixedScaleRatio((__int128)value, 1, fixedTwos(bits), fixedFives(bits), 0, 0, &result)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    }
    return result;
}

long long __ada_fixed_rescale(long long value, int fromBits, int toBits)
{
    long long result = 0;
    if (!fixedRescale(value, fromBits, toBits, &result)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    }
    return result;
}

long long __ada_float_to_fixed(double value, int bits)
{
    if (!fixedValidScale(bits) || !isfinite(value)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    int exponent = 0;
    double fraction = frexp(value, &exponent);
    if (exponent < -256 || value == 0) { return 0; }
    long long result = 0;
    if (exponent > 256 || !fixedScaleRatio((long long)ldexp(fraction, 53), 1,
            exponent - 53 + fixedTwos(bits), fixedFives(bits), 0, 0, &result)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    }
    return result;
}

_Static_assert(sizeof(AdaFinalization) == 40, "finalization record ABI");

/* Library masters belong to the environment task. Reject foreign-context
   access before touching the chain; callbacks run without a global lock. */
static int requireLibraryOwner(void)
{
    if (__ada_task_context() != &__ada_main_context) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return 0;
    }
    return 1;
}

static int allocationOwnsChain(AdaFinalization** owner);

static AdaFinalization* findFinalization(void* object)
{
    AdaTaskContext* context = __ada_task_context();
    for (AdaFinalization* record = context->registeredFinalizations; record != NULL;
         record = record->registeredNext) {
        if (record->object == object) {
            return record;
        }
    }
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    AdaFinalization* shared = __ada_main_context.sharedFinalizations;
    while (shared != NULL && shared->object != object) {
        shared = shared->registeredNext;
    }
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    if (shared != NULL) {
        return shared;
    }
    __ada_raise(ADA_PROGRAM_ERROR);
    return NULL;
}

static void unlinkFinalization(AdaFinalization** link, AdaFinalization* record)
{
    while (*link != NULL && *link != record) {
        link = &(*link)->registeredNext;
    }
    if (*link != NULL) {
        *link = record->registeredNext;
    }
}

static void unregisterFinalization(AdaFinalization* record)
{
    unlinkFinalization(&__ada_task_context()->registeredFinalizations, record);
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    unlinkFinalization(&__ada_main_context.sharedFinalizations, record);
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
}

void __ada_finalization_push(AdaFinalization** owner, AdaFinalization* record,
                             void* object, void (*finalize)(void*))
{
    AdaTaskContext* context = __ada_task_context();
    record->next = *owner;
    record->object = object;
    record->finalize = finalize;
    record->active = 1;
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    AdaFinalization** registry = allocationOwnsChain(owner)
        ? &__ada_main_context.sharedFinalizations : &context->registeredFinalizations;
    record->registeredNext = *registry;
    *registry = record;
    *owner = record;
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
}

void __ada_finalize_to(AdaFinalization** owner, AdaFinalization* checkpoint)
{
    AdaTaskContext* context = __ada_task_context();
    if (*owner == checkpoint) {
        return;
    }
    __ada_abort_defer(context);
    /* Suspend the complete pending occurrence without allocating. Ada calls
       made by a finalizer must start with a clear pending exception status. */
    const AdaException* savedException = context->exception;
    char* savedMessage = context->pendingMessage;
    int savedLength = context->pendingMessageLength;
    const char* savedOrigin = context->pendingOrigin;
    int savedCount = context->pendingTraceCount;
    AdaTraceEntry savedTrace[ADA_TRACE_CAPACITY];
    memcpy(savedTrace, context->pendingTrace, (size_t)savedCount * sizeof *savedTrace);
    context->exception = NULL;
    context->pendingMessage = NULL;
    context->pendingMessageLength = 0;
    int failed = 0;
    while (*owner != checkpoint) {
        AdaFinalization* record = *owner;
        /* Pop before calling: nested cleanup cannot finalize this object twice. */
        *owner = record->next;
        if (record->active) {
            record->active = 0;
            record->finalize(record->object);
        }
        unregisterFinalization(record);
        if (context->exception != NULL) {
            failed = 1;
        }
        free(context->pendingMessage);
        context->pendingMessage = NULL;
        context->pendingMessageLength = 0;
        context->exception = NULL;
    }
    if (failed) {
        free(savedMessage);
        __ada_raise(ADA_PROGRAM_ERROR);
    } else {
        context->exception = savedException;
        context->pendingMessage = savedMessage;
        context->pendingMessageLength = savedLength;
        context->pendingOrigin = savedOrigin;
        context->pendingTraceCount = savedCount;
        memcpy(context->pendingTrace, savedTrace, (size_t)savedCount * sizeof *savedTrace);
    }
    __ada_abort_undefer(context);
}

void __ada_finalization_reserve(AdaFinalization** owner, void** arena, void* object, void (*finalize)(void*))
{
    AdaFinalization* record = __ada_array_local(arena, 1, 1, sizeof *record);
    if (record == NULL) {
        return;
    }
    __ada_finalization_push(owner, record, object, finalize);
    record->active = 0;
}

void __ada_controlled_activate(void* object, void (*finalize)(void*))
{
    AdaFinalization* record = findFinalization(object);
    if (record != NULL) {
        record->finalize = finalize;
        record->active = 1;
    }
}

/* Library objects share the environment task's master across separately
   compiled units. Keep registrations alive until the binder shuts it down,
   including the successfully constructed parts of a failed elaboration. */
void __ada_library_reserve(void* object, void (*finalize)(void*))
{
    if (!requireLibraryOwner()) {
        return;
    }
    __ada_finalization_reserve(&__ada_main_context.libraryFinalizations, &__ada_main_context.libraryFinalizationArena, object, finalize);
}

void __ada_library_finalize(void)
{
    if (!requireLibraryOwner()) {
        return;
    }
    if (__ada_main_context.libraryFinalizing) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    __ada_main_context.libraryFinalizing = 1;
    __ada_environment_task_state(1, 0);
    __ada_master_await(&__ada_main_context, &__ada_main_context.libraryMaster);
    __ada_finalize_to(&__ada_main_context.libraryFinalizations, NULL);
    __ada_array_release(&__ada_main_context.libraryFinalizationArena);
    __ada_master_leave(&__ada_main_context, &__ada_main_context.libraryMaster);
    __ada_main_context.libraryFinalizing = 0;
    __ada_environment_task_state(1, 1);
}

void __ada_library_enter(void)
{
    if (requireLibraryOwner()) {
        __ada_environment_task_state(0, 0);
        __ada_master_enter(&__ada_main_context, &__ada_main_context.libraryMaster, ADA_MASTER_LIBRARY);
    }
}

void __ada_library_activate(void)
{
    if (requireLibraryOwner()) {
        __ada_master_activate(&__ada_main_context, &__ada_main_context.libraryMaster);
    }
}

typedef struct AdaCollection AdaCollection;
typedef struct AdaAllocation
{
    struct AdaAllocation* next;
    struct AdaAllocation* registeredNext;
    AdaCollection* collection;
    void* object;
    AdaFinalization* finalizations;
    void* arena;
    int arenaOwnsObject;
    int borrowed;
    int constrained;
    const char* viewIdentity;
    long size;
    void* arrayBounds;
    int arrayRank;
} AdaAllocation;

struct AdaCollection
{
    AdaMaster master;
    int accessLevel;
    AdaTaskContext* context;
    AdaAllocation* allocations;
    int closing;
};


/* The storage mutex protects indexes and collection membership, not user
   objects. A program must synchronize publication and access to each object. */
static int allocationOwnsChain(AdaFinalization** owner)
{
    for (AdaAllocation* allocation = __ada_main_context.registeredAllocations; allocation != NULL;
         allocation = allocation->registeredNext) {
        if (&allocation->finalizations == owner) {
            return 1;
        }
    }
    return 0;
}

static AdaAllocation* findAllocation(void* address)
{
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    AdaAllocation* allocation = __ada_main_context.registeredAllocations;
    AdaAllocation* borrowed = NULL;
    while (allocation != NULL) {
        if (allocation->object == address) {
            if (!allocation->borrowed) { break; }
            if (borrowed == NULL) { borrowed = allocation; }
        }
        allocation = allocation->registeredNext;
    }
    if (allocation == NULL) { allocation = borrowed; }
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    return allocation;
}

void __ada_collection_activate(void* owner)
{
    AdaCollection* collection = owner;
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    int invalid = collection == NULL || collection->closing;
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    if (invalid) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    __ada_master_activate(__ada_task_context(), &collection->master);
}

static void releaseAllocation(AdaAllocation* allocation)
{
    AdaTaskContext* context = __ada_task_context();
    // Unlink before calling user code: a finalizer may deallocate another
    // member of this collection. Keep the object alive until all parts finish.
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    AdaAllocation** link = &allocation->collection->allocations;
    while (*link != allocation) {
        link = &(*link)->next;
    }
    *link = allocation->next;
    link = &__ada_main_context.registeredAllocations;
    while (*link != allocation) {
        link = &(*link)->registeredNext;
    }
    *link = allocation->registeredNext;
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    if (!allocation->borrowed) {
        __ada_task_discard(context, &allocation->collection->master, allocation->object, allocation->size);
    }
    __ada_finalize_to(&allocation->finalizations, NULL);
    __ada_array_release(&allocation->arena);
    if (!allocation->arenaOwnsObject && !allocation->borrowed) {
        free(allocation->object);
    }
    free(allocation);
}

static int releaseControlledAllocation(void* address)
{
    AdaAllocation* allocation = findAllocation(address);
    if (allocation == NULL) {
        return 0;
    }
    if (allocation->borrowed) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return 1;
    }
    releaseAllocation(allocation);
    return 1;
}

static void finalizeCollection(void* object)
{
    AdaCollection* collection = object;
    __ada_master_await(collection->context, &collection->master);
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    collection->closing = 1;
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    while (collection->allocations != NULL) {
        releaseAllocation(collection->allocations);
    }
    __ada_master_leave(collection->context, &collection->master);
}

void* __ada_collection_create(AdaFinalization** owner, void** arena)
{
    AdaTaskContext* context = __ada_task_context();
    if (owner == NULL) {
        if (!requireLibraryOwner()) {
            return NULL;
        }
        owner = &__ada_main_context.libraryFinalizations;
        arena = &__ada_main_context.libraryFinalizationArena;
    }
    AdaCollection* collection = __ada_array_local(arena, 1, 1, sizeof *collection);
    if (collection == NULL) {
        return NULL;
    }
    memset(collection, 0, sizeof *collection);
    collection->context = context;
    collection->accessLevel = owner == &__ada_main_context.libraryFinalizations ? 0 : __ada_current_access_level();
    if (context->exception != NULL) { return NULL; }
    __ada_finalization_reserve(owner, arena, collection, finalizeCollection);
    if (context->exception != NULL) {
        return NULL;
    }
    __ada_controlled_activate(collection, finalizeCollection);
    __ada_master_enter(context, &collection->master, ADA_MASTER_COLLECTION);
    return collection;
}

static int registerAllocation(AdaCollection* collection, AdaAllocation* allocation)
{
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    if (collection == NULL || collection->closing) {
        __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
        __ada_raise(ADA_PROGRAM_ERROR);
        return 0;
    }
    allocation->collection = collection;
    allocation->next = collection->allocations;
    collection->allocations = allocation;
    allocation->registeredNext = __ada_main_context.registeredAllocations;
    __ada_main_context.registeredAllocations = allocation;
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    return 1;
}

void* __ada_collection_allocate(void* owner, long size)
{
    AdaAllocation* allocation = calloc(1, sizeof *allocation);
    if (allocation == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    allocation->object = __ada_allocate(size);
    if (allocation->object == NULL || !registerAllocation(owner, allocation)) {
        free(allocation->object);
        free(allocation);
        return NULL;
    }
    allocation->size = size;
    return allocation->object;
}

void __ada_allocation_set_array_bounds(void* object, const void* descriptor, int rank, const char* identity)
{
    AdaAllocation* allocation = findAllocation(object);
    if (allocation == NULL || rank < 1 || rank > INT_MAX / (int)sizeof(AdaArrayDimension)) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    size_t size = (size_t)adaArrayLocalSize(rank);
    void* bounds = __ada_array_local(&allocation->arena, 1, 1, (int64_t)size);
    if (bounds == NULL) { return; }
    memcpy(bounds, descriptor, size);
    allocation->arrayBounds = bounds;
    allocation->arrayRank = rank;
    allocation->viewIdentity = identity;
}

void __ada_access_check(int sourceLevel, int targetLevel)
{
    if (sourceLevel > targetLevel) { __ada_raise(ADA_PROGRAM_ERROR); }
}

/* Borrowed array bounds use the existing shared allocation index. The access
   type's collection owns metadata only; the aliased object's master still owns
   its storage and any controlled elements. Never mutate a published view. */
void __ada_array_borrow(void* owner, void* object, const void* descriptor, int rank, const char* identity)
{
    AdaCollection* collection = owner;
    if (object == NULL || rank < 1 || rank > INT_MAX / (int)sizeof(AdaArrayDimension)) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    AdaAllocation* existing = __ada_main_context.registeredAllocations;
    while (existing != NULL) {
        if (existing->object == object && (!existing->borrowed || existing->collection == collection)
            && existing->arrayBounds != NULL && existing->arrayRank == rank
            && existing->viewIdentity != NULL && strcmp(existing->viewIdentity, identity) == 0) {
            break;
        }
        existing = existing->registeredNext;
    }
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    if (existing != NULL) { return; }
    AdaAllocation* allocation = calloc(1, sizeof *allocation);
    if (allocation == NULL) { __ada_raise(ADA_STORAGE_ERROR); return; }
    allocation->object = object;
    allocation->borrowed = 1;
    allocation->arrayRank = rank;
    allocation->viewIdentity = identity;
    int64_t size = adaArrayLocalSize(rank);
    allocation->arrayBounds = __ada_array_local(&allocation->arena, 1, 1, size);
    if (allocation->arrayBounds == NULL) { free(allocation); return; }
    memcpy(allocation->arrayBounds, descriptor, (size_t)size);
    if (!registerAllocation(collection, allocation)) {
        __ada_array_release(&allocation->arena);
        free(allocation);
    }
}

void* __ada_allocation_array_bounds(void* object, int rank)
{
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    AdaAllocation* allocation = __ada_main_context.registeredAllocations;
    while (allocation != NULL && (allocation->object != object || allocation->arrayBounds == NULL || allocation->arrayRank != rank)) {
        allocation = allocation->registeredNext;
    }
    void* bounds = allocation != NULL ? allocation->arrayBounds : NULL;
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    if (bounds == NULL) { __ada_raise(ADA_PROGRAM_ERROR); }
    return bounds;
}

void* __ada_access_array_bounds(void* object, int rank, const char* identity)
{
    void* bounds = NULL;
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    for (AdaAllocation* allocation = __ada_main_context.registeredAllocations; allocation != NULL;
         allocation = allocation->registeredNext) {
        if (allocation->object == object && allocation->arrayBounds != NULL && allocation->arrayRank == rank
            && allocation->viewIdentity != NULL && strcmp(allocation->viewIdentity, identity) == 0) {
            bounds = allocation->arrayBounds;
            break;
        }
    }
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    if (bounds == NULL) { __ada_raise(ADA_PROGRAM_ERROR); }
    return bounds;
}

void __ada_object_borrow(void* owner, void* object, const char* identity, int constrained)
{
    AdaCollection* collection = owner;
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    AdaAllocation* existing = __ada_main_context.registeredAllocations;
    while (existing != NULL) {
        if (existing->object == object && existing->borrowed && existing->collection == collection
            && existing->viewIdentity != NULL && strcmp(existing->viewIdentity, identity) == 0) { break; }
        existing = existing->registeredNext;
    }
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    if (existing != NULL) { return; }
    AdaAllocation* allocation = calloc(1, sizeof *allocation);
    if (allocation == NULL) { __ada_raise(ADA_STORAGE_ERROR); return; }
    allocation->object = object;
    allocation->borrowed = 1;
    allocation->constrained = constrained;
    allocation->viewIdentity = identity;
    if (!registerAllocation(collection, allocation)) { free(allocation); }
}

int __ada_access_constrained(void* object, const char* identity)
{
    int constrained = 1;
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    for (AdaAllocation* allocation = __ada_main_context.registeredAllocations; allocation != NULL;
         allocation = allocation->registeredNext) {
        if (allocation->object == object && allocation->viewIdentity != NULL
            && strcmp(allocation->viewIdentity, identity) == 0) {
            constrained = allocation->constrained;
            break;
        }
    }
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    return constrained;
}

int64_t __ada_layout_append(int64_t offset, int64_t size, int64_t alignment)
{
    if (offset < 0 || size < 0 || alignment < 1) {
        __ada_raise(ADA_STORAGE_ERROR);
        return 0;
    }
    uint64_t padding = (uint64_t)offset % (uint64_t)alignment;
    if (padding != 0) { padding = (uint64_t)alignment - padding; }
    if ((uint64_t)offset + padding > INT64_MAX
        || (uint64_t)size > INT64_MAX - (uint64_t)offset - padding) {
        __ada_raise(ADA_STORAGE_ERROR);
        return 0;
    }
    return offset + (int64_t)padding + size;
}

void __ada_allocation_reserve(void* object, void* part, void (*finalize)(void*))
{
    AdaAllocation* allocation = findAllocation(object);
    if (allocation == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    __ada_finalization_reserve(&allocation->finalizations, &allocation->arena, part, finalize);
}

int __ada_controlled_finalize(void* object)
{
    AdaTaskContext* context = __ada_task_context();
    AdaFinalization* record = findFinalization(object);
    if (record == NULL) {
        context->exception = NULL;
        return 1;
    }
    if (!record->active) {
        return 0;
    }
    record->active = 0;
    /* Finalize one object without removing its persistent registration. Reuse
       the exception suspension and Program_Error handling of scope cleanup. */
    AdaFinalization current = { NULL, object, record->finalize, NULL, 1 };
    AdaFinalization* owner = &current;
    __ada_finalize_to(&owner, NULL);
    int failed = context->exception != NULL;
    context->exception = NULL;
    return failed;
}

int __ada_controlled_adjust(void* object, void (*adjust)(void*))
{
    AdaTaskContext* context = __ada_task_context();
    AdaFinalization* record = findFinalization(object);
    if (record == NULL) {
        context->exception = NULL;
        return 1;
    }
    record->active = 0;
    adjust(object);
    if (context->exception != NULL) {
        /* The emitter accumulates failures while adjusting the other parts,
           then raises Program_Error. A failed adjustment is not finalized. */
        __ada_raise(ADA_PROGRAM_ERROR);
        context->exception = NULL;
        return 1;
    }
    record->active = 1;
    return 0;
}

/* Dynamic ownership uses the concrete tag's component walk. Records remain
   inactive until each adjustment succeeds, just as for statically typed copies. */
typedef struct TaggedCopyContext
{
    AdaFinalization** owner;
    void** arena;
    int failed;
} TaggedCopyContext;

static void reserveTaggedPart(void* part, void (*finalize)(void*), void (*adjust)(void*), void* context)
{
    (void)adjust;
    TaggedCopyContext* copy = context;
    __ada_finalization_reserve(copy->owner, copy->arena, part, finalize);
}

static void adjustTaggedPart(void* part, void (*finalize)(void*), void (*adjust)(void*), void* context)
{
    TaggedCopyContext* copy = context;
    __ada_controlled_activate(part, finalize);
    copy->failed |= __ada_controlled_adjust(part, adjust);
}

static void finalizeTaggedPart(void* part, void (*finalize)(void*), void (*adjust)(void*), void* context)
{
    (void)finalize;
    (void)adjust;
    TaggedCopyContext* copy = context;
    copy->failed |= __ada_controlled_finalize(part);
}

static void copyTaggedObject(void* target, const void* source, TaggedCopyContext* copy, int reserve)
{
    AdaTaskContext* context = __ada_task_context();
    const AdaTag* tag = *(const AdaTag* const*)source;
    if (tag->hasLimitedParts) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    if (reserve && tag->parts != NULL) {
        *(const AdaTag**)target = tag;
        tag->parts(target, reserveTaggedPart, copy, 2);
        if (context->exception != NULL) {
            return;
        }
    }
    memcpy(target, source, tag->size);
    if (tag->parts != NULL) {
        tag->parts(target, adjustTaggedPart, copy, 0);
        if (copy->failed) {
            __ada_raise(ADA_PROGRAM_ERROR);
        }
    }
}

void* __ada_tagged_owned_copy(AdaFinalization** owner, void** arena, const void* source)
{
    AdaTaskContext* context = __ada_task_context();
    if (owner == NULL) {
        if (!requireLibraryOwner()) {
            return NULL;
        }
        owner = &__ada_main_context.libraryFinalizations;
        arena = &__ada_main_context.libraryFinalizationArena;
    }
    const AdaTag* tag = *(const AdaTag* const*)source;
    AdaFinalization* mark = *owner;
    void* arenaMark = *arena;
    void* target = __ada_array_local(arena, 1, 1, tag->size);
    if (target != NULL) {
        TaggedCopyContext copy = { owner, arena, 0 };
        copyTaggedObject(target, source, &copy, 1);
    }
    if (context->exception != NULL) {
        __ada_finalize_to(owner, mark);
        __ada_array_rewind(arena, arenaMark);
        return NULL;
    }
    return target;
}

void* __ada_tagged_allocation(void* collection, const void* source)
{
    AdaTaskContext* context = __ada_task_context();
    const AdaTag* tag = *(const AdaTag* const*)source;
    void* target = __ada_collection_allocate(collection, tag->size);
    if (target == NULL) {
        return NULL;
    }
    AdaAllocation* allocation = findAllocation(target);
    TaggedCopyContext copy = { &allocation->finalizations, &allocation->arena, 0 };
    copyTaggedObject(target, source, &copy, 1);
    if (context->exception != NULL) {
        __ada_deallocate(target);
        return NULL;
    }
    return target;
}

void __ada_tagged_assign(void* target, const void* source, AdaFinalization** owner, void** arena)
{
    const AdaTag* tag = *(const AdaTag* const*)target;
    if (tag != *(const AdaTag* const*)source) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return;
    }
    if (target == source) {
        return;
    }
    /* RM 7.6(21) permits omitting the adjusted snapshot for disjoint
       objects. Keep it for overlapping views, which need source ownership
       preserved while the old target is finalized. */
    const void* assigned = source;
    uintptr_t targetAddress = (uintptr_t)target, sourceAddress = (uintptr_t)source;
    if (targetAddress - sourceAddress < (uint64_t)tag->size
        || sourceAddress - targetAddress < (uint64_t)tag->size) {
        assigned = __ada_tagged_owned_copy(owner, arena, source);
        if (assigned == NULL) {
            return;
        }
    }
    TaggedCopyContext copy = { owner, arena, 0 };
    if (tag->parts != NULL) {
        tag->parts(target, finalizeTaggedPart, &copy, 1);
        if (copy.failed) {
            __ada_raise(ADA_PROGRAM_ERROR);
            return;
        }
    }
    copyTaggedObject(target, assigned, &copy, 0);
}

void* __ada_construction_owner(void* allocation)
{
    if (allocation == NULL && !requireLibraryOwner()) {
        return NULL;
    }
    return allocation == NULL ? (void*)&__ada_main_context.libraryFinalizations
        : (void*)&findAllocation(allocation)->finalizations;
}

void* __ada_construction_arena(void* allocation)
{
    if (allocation == NULL && !requireLibraryOwner()) {
        return NULL;
    }
    return allocation == NULL ? (void*)&__ada_main_context.libraryFinalizationArena
        : (void*)&findAllocation(allocation)->arena;
}

int __ada_controlled_adjust_view(void* object, void (*adjust)(void*, void*), void* tag)
{
    AdaTaskContext* context = __ada_task_context();
    AdaFinalization* record = findFinalization(object);
    if (record == NULL) {
        return 1;
    }
    record->active = 0;
    adjust(object, tag);
    int failed = context->exception != NULL;
    context->exception = NULL;
    record->active = !failed;
    return failed;
}

int __ada_controlled_finalize_view(void* object, void (*finalize)(void*, void*), void* tag)
{
    AdaTaskContext* context = __ada_task_context();
    AdaFinalization* record = findFinalization(object);
    if (record == NULL) {
        return 1;
    }
    if (!record->active) {
        return 0;
    }
    record->active = 0;
    __ada_abort_defer(context);
    finalize(object, tag);
    int failed = context->exception != NULL;
    context->exception = NULL;
    __ada_abort_undefer(context);
    return failed;
}

// A limited class-wide constructor determines its concrete size. Give it an
// allocation-owned arena before calling it, then publish its resulting address.
void* __ada_collection_begin(void* owner)
{
    AdaAllocation* allocation = calloc(1, sizeof *allocation);
    if (allocation == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    allocation->arenaOwnsObject = 1;
    if (!registerAllocation(owner, allocation)) {
        free(allocation);
        return NULL;
    }
    return allocation;
}

void* __ada_collection_result_owner(void* pending)
{
    return &((AdaAllocation*)pending)->finalizations;
}

void* __ada_collection_result_arena(void* pending)
{
    return &((AdaAllocation*)pending)->arena;
}

void __ada_collection_finish(void* pending, void* object)
{
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    ((AdaAllocation*)pending)->object = object;
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    __ada_collection_activate(((AdaAllocation*)pending)->collection);
}

void __ada_collection_abort(void* pending)
{
    releaseAllocation(pending);
}

/* Enumeration objects contain codes; Ada scalar expressions use positions. */
int __ada_enum_code(int position, const int32_t* codes, int count)
{
    if (position < 0 || position >= count) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    return codes[position];
}

int __ada_enum_position(int code, const int32_t* codes, int count)
{
    int first = 0;
    int last = count;
    while (first < last) {
        int middle = first + (last - first) / 2;
        if (codes[middle] < code) {
            first = middle + 1;
        } else {
            last = middle;
        }
    }
    if (first < count && codes[first] == code) { return first; }
    __ada_raise(ADA_CONSTRAINT_ERROR);
    return 0;
}

/* Width uses positions, not enumeration representation codes. Integer images
   have one sign/blank column, including unsigned modular values. */
int __ada_width_integer(uint64_t low, uint64_t high, int isUnsigned)
{
    if (isUnsigned ? low > high : (int64_t)low > (int64_t)high) { return 0; }
    uint64_t left = !isUnsigned && (int64_t)low < 0 ? 0 - low : low;
    uint64_t right = !isUnsigned && (int64_t)high < 0 ? 0 - high : high;
    uint64_t magnitude = left > right ? left : right;
    int width = 1;
    do { ++width; magnitude /= 10; } while (magnitude != 0);
    return width;
}

int __ada_width_enum(int low, int high, const char** names, int count)
{
    if (low > high) { return 0; }
    if (low < 0 || high >= count) { __ada_raise(ADA_CONSTRAINT_ERROR); return 0; }
    int width = 0;
    for (int i = low; i <= high; ++i) {
        size_t length = strlen(names[i]);
        if (length > INT_MAX) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
        if ((int)length > width) { width = (int)length; }
    }
    return width;
}

void __ada_access_return_check(int sourceLevel, int targetLevel, void* owner, void* resultOwner)
{
    if (owner == NULL || owner != resultOwner) { __ada_accessibility_check(sourceLevel, targetLevel); }
}

void __ada_collection_finish_sized(void* pending, void* object, long size)
{
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    ((AdaAllocation*)pending)->size = size;
    ((AdaAllocation*)pending)->object = object;
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
}

int __ada_collection_access_level(void* owner)
{
    return ((AdaCollection*)owner)->accessLevel;
}

void __ada_access_array_copy_bounds(void* object, int rank, const char* identity, void* descriptor)
{
    int found = 0;
    __ada_mutex_lock(&__ada_main_context.sharedStorageMutex);
    for (AdaAllocation* allocation = __ada_main_context.registeredAllocations; allocation != NULL;
         allocation = allocation->registeredNext) {
        if (allocation->object == object && allocation->arrayBounds != NULL && allocation->arrayRank == rank
            && allocation->viewIdentity != NULL && strcmp(allocation->viewIdentity, identity) == 0) {
            memcpy(descriptor, allocation->arrayBounds, (size_t)adaArrayLocalSize(rank));
            found = 1;
            break;
        }
    }
    __ada_mutex_unlock(&__ada_main_context.sharedStorageMutex);
    if (!found) { __ada_raise(ADA_PROGRAM_ERROR); }
}

void __ada_access_parameter_adopt(AdaFinalization** owner, void** arena, void* object)
{
    __ada_finalization_reserve(owner, arena, object, __ada_deallocate);
    if (__ada_exception == NULL) { __ada_controlled_activate(object, __ada_deallocate); }
}

void __ada_collection_set_access_level(void* owner, int level)
{
    AdaCollection* collection = owner;
    __ada_access_collection_parent(&collection->master, level);
    if (__ada_exception == NULL) { collection->accessLevel = level; }
}
