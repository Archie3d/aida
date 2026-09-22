/* Shared declarations for the run time library linked into every compiled
   program. */

#ifndef ADART_H
#define ADART_H

#include <stdint.h>
#include "../common/Modular.h"

/* An exception is identified by the address of its object, so that units
   compiled apart still agree on what a handler catches.  The run time owns the
   predefined ones; any other exception has an object emitted by the unit that
   declares it. */
typedef struct AdaException
{
    const char* name;
} AdaException;

extern const AdaException __ada_exc_constraint_error;
extern const AdaException __ada_exc_program_error;
extern const AdaException __ada_exc_storage_error;
extern const AdaException __ada_exc_numeric_error;
extern const AdaException __ada_exc_tasking_error;
extern const AdaException __ada_exc_status_error;
extern const AdaException __ada_exc_mode_error;
extern const AdaException __ada_exc_name_error;
extern const AdaException __ada_exc_use_error;
extern const AdaException __ada_exc_device_error;
extern const AdaException __ada_exc_end_error;
extern const AdaException __ada_exc_data_error;
extern const AdaException __ada_exc_layout_error;

#define ADA_CONSTRAINT_ERROR (&__ada_exc_constraint_error)
#define ADA_PROGRAM_ERROR (&__ada_exc_program_error)
#define ADA_STORAGE_ERROR (&__ada_exc_storage_error)
#define ADA_NUMERIC_ERROR ADA_CONSTRAINT_ERROR
#define ADA_TASKING_ERROR (&__ada_exc_tasking_error)
#define ADA_STATUS_ERROR (&__ada_exc_status_error)
#define ADA_MODE_ERROR (&__ada_exc_mode_error)
#define ADA_NAME_ERROR (&__ada_exc_name_error)
#define ADA_USE_ERROR (&__ada_exc_use_error)
#define ADA_DEVICE_ERROR (&__ada_exc_device_error)
#define ADA_END_ERROR (&__ada_exc_end_error)
#define ADA_DATA_ERROR (&__ada_exc_data_error)
#define ADA_LAYOUT_ERROR (&__ada_exc_layout_error)

/* The pending exception, null when there is none.  Generated code reads it
   after every call, and writes it to raise one of its own. */
extern const AdaException* __ada_exception;

/* Marks an exception as pending.  The generated code inspects the global after
   every call and jumps to the applicable handler. */
void __ada_raise(const AdaException* exception);

/* Ada.Exceptions' private layout. Captured messages belong to the enclosing
   activation's allocation list. Procedure saves use inline storage; function
   saves put the full message after the occurrence in one allocation. */
#define ADA_SAVED_MESSAGE_CAPACITY 200
#define ADA_TRACE_CAPACITY 32
typedef struct AdaTraceEntry
{
    const char* routine;
    const char* location;
} AdaTraceEntry;

typedef struct AdaTraceFrame
{
    struct AdaTraceFrame* previous;
    const char* routine;
    const char* location;
} AdaTraceFrame;

void __ada_trace_enter(AdaTraceFrame* frame, const char* routine, const char* location);
void __ada_trace_location(const char* location);
void __ada_trace_leave(AdaTraceFrame* frame);

typedef struct AdaExceptionOccurrence
{
    const AdaException* identity;
    const char* message;
    int32_t length;
    char savedMessage[ADA_SAVED_MESSAGE_CAPACITY];
    const char* origin;
    int32_t traceCount;
    AdaTraceEntry trace[ADA_TRACE_CAPACITY];
} AdaExceptionOccurrence;

void __ada_raise_message(const AdaException* exception, const char* message, int length);
void __ada_exception_capture(AdaExceptionOccurrence* target, void** owner);
void __ada_reraise(const AdaExceptionOccurrence* occurrence);
const AdaException* __ada_exception_identity(const AdaExceptionOccurrence* occurrence);
const char* __ada_exception_name(const AdaException* exception);
int __ada_exception_message_length(const AdaExceptionOccurrence* occurrence);
void __ada_exception_message_copy(const AdaExceptionOccurrence* occurrence, char* target, int length);
void __ada_save_occurrence(AdaExceptionOccurrence* target, const AdaExceptionOccurrence* source);
AdaExceptionOccurrence* __ada_save_occurrence_new(const AdaExceptionOccurrence* source);
int __ada_exception_information_length(const AdaExceptionOccurrence* occurrence);
void __ada_exception_information_copy(const AdaExceptionOccurrence* occurrence, char* target, int length);

/* The storage an allocator takes from and Ada.Unchecked_Deallocation gives
   back.  Every allocation comes out cleared, so that an access component of
   the new object starts as null.  Storage_Error is raised when the request
   cannot be met, and freeing null does nothing. */
void* __ada_allocate(long size);
void __ada_deallocate(void* address);
void* __ada_array_local(void** owner, int first, int last, int64_t elementSize);
void __ada_array_release(void** owner);
void __ada_array_rewind(void** owner, void* checkpoint);
void __ada_array_adopt(void** owner, void* data);

/* Internal unconstrained-array return descriptor: pointer, two 32-bit bounds,
   and a 64-bit transfer size. The caller owns and releases the buffer. */
void __ada_array_result(void* descriptor, const void* source, int first, int last, int64_t elementSize);

/* Renders a real value the way Ada.Text_IO does.  Fore is the least number of
   characters before the point including the sign, Aft the number after it, and
   Exp the width of the exponent field counting its sign.  An Exp of zero asks
   for plain decimal notation. */
void __ada_format_float(char* buffer, int size, double value, int fore, int aft, int exponent);

/* 'Image, whose enumeration form needs the literal names the emitter records
   for the type. */
const char* __ada_image_integer(int value);
const char* __ada_image_long_integer(long long value);
const char* __ada_image_enum(int value, const char** names, int count);
const char* __ada_image_character(int value);

/* 'Value, which reads back what 'Image wrote.  Surrounding blanks are ignored,
   and anything else raises Constraint_Error. */
int __ada_value_integer(const char* text, int length, int low, int high);
long long __ada_value_long_integer(const char* text, int length, long long low, long long high);

/* Checked signed arithmetic: add, subtract, multiply, divide, rem, mod, power. */
long long __ada_modular_operation(ModularOperation operation, long long modulus, long long left, long long right);
long long __ada_integer_operation(int operation, int bits, long long left, long long right);
int __ada_value_enum(const char* text, int length, const char** names, int count);
int __ada_value_character(const char* text, int length);

#endif
