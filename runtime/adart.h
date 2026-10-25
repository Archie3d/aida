/* Shared declarations for the run time library linked into every compiled
   program. */

#ifndef ADART_H
#define ADART_H

#include <stdint.h>
#include "adamaster.h"
#include "adathread.h"
#include "adaprotected.h"
#include "../common/Modular.h"
#include "../common/Fixed.h"
#include "../common/Real.h"
#include "../common/Array.h"

typedef struct AdaDispatchEntry
{
    void* code;
    void* link;
} AdaDispatchEntry;

struct AdaTag;
typedef struct AdaInterfaceBinding
{
    struct AdaTag* tag;
    const int64_t* slots;
} AdaInterfaceBinding;

typedef struct AdaTag
{
    struct AdaTag* parent;
    int64_t size;
    int64_t alignment;
    AdaDispatchEntry* slots;
    void* equality;
    void* equalityLink;
    const char* name;
    int64_t level;
    void* master;
    int64_t slotCount;
    int64_t isAbstract;
    int64_t needsFinalization;
    /* Walk mode: 0 active child-first, 1 active parent-first, 2 reserve all alternatives. */
    void (*parts)(void*, void (*)(void*, void (*)(void*), void (*)(void*), void*), void*, int);
    int64_t hasLimitedParts;
    const int64_t* layout;
    int64_t interfaceCount;
    AdaInterfaceBinding* interfaces;
    int64_t interfaceKind;
} AdaTag;


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

/* The binder captures the host arguments before library elaboration. */
void __ada_command_line_init(int argc, char** argv);
int __ada_argument_count(void);
const char* __ada_argument(int number);
const char* __ada_command_name(void);
void __ada_set_exit_status(int code);
int __ada_get_exit_status(void);

/* Marks an exception as pending.  The generated code inspects the context after
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

/* One context belongs to one task and may be used by only one thread at a
   time. Zero initialization creates an empty context. Chains and arenas still
   belong to generated stack frames; these registries only index their records.
   Stack finalization stays task-local. Collection allocations and their records
   use the environment context's locked indexes and outlive allocating tasks. */
typedef struct AdaTaskContext
{
    const AdaException* exception;
    char* pendingMessage;
    int pendingMessageLength;
    AdaTraceFrame* currentTrace;
    const char* pendingOrigin;
    int pendingTraceCount;
    AdaTraceEntry pendingTrace[ADA_TRACE_CAPACITY];
    struct AdaFinalization* registeredFinalizations;
    struct AdaAllocation* registeredAllocations; // Environment-owned shared index.
    /* Only the environment task may use library lifetime storage. */
    struct AdaFinalization* libraryFinalizations;
    void* libraryFinalizationArena;
    int libraryFinalizing;
    AdaMaster libraryMaster;
    AdaMaster* currentMaster;
    int accessibilityParentLevel;
    struct AdaMasterState* taskMasters;
    struct AdaTaskState* taskSelf;
    AdaProtectedAction* protectedActions;
    /* Task-owned nesting depth; cross-task abort requests belong to the
       synchronized task state when abort is implemented in Phase 9. */
    unsigned int abortDeferralDepth;
    /* Only the environment context uses these shared service fields. Keep
       their initialized synchronization objects alive across context disposal. */
    AdaMutex sharedStorageMutex;
    struct AdaFinalization* sharedFinalizations;
    AdaMutex sharedMasterMutex;
    AdaCondition sharedMasterChanged;
    struct AdaMasterState* sharedMasters;
    struct AdaAccessLevel* sharedAccessLevels;
    unsigned int nextAccessLevel;
} AdaTaskContext;

/* Internal balanced regions. These never deliver abort or replace a pending
   exception; the outermost leave only makes the next completion point eligible.
   Misbalanced regions are runtime invariant failures. See docs/abort-deferral.md. */
void __ada_abort_defer(AdaTaskContext* context);
void __ada_abort_undefer(AdaTaskContext* context);

/* Generated-code ABI: pending exception at offset zero. Subprograms cache the
   context returned by trace entry; binder entry uses __ada_task_context. */
extern AdaTaskContext __ada_main_context;
AdaTaskContext* __ada_task_context(void);

/* Bind before using runtime helpers on a worker thread. NULL selects the main
   context; the returned previous binding must be restored before it expires.
   Restore the binding before returning to an existing generated Ada activation.
   Unbound threads default to the main context for single-threaded entry points. */
AdaTaskContext* __ada_task_context_bind(AdaTaskContext* context);

/* After all masters and trace frames have exited, free a pending message and
   reset the context. Returns zero without changing it if records remain live.
   This does not unwind stacks or finalize objects on the caller's behalf. */
int __ada_task_context_dispose(AdaTaskContext* context);

/* C helpers use the bound context; there is no separate exception global. */
#define __ada_exception (__ada_task_context()->exception)

/* Entry returns the bound context for generated code to cache. Its binding
   must stay the same until return (a C callback may switch and restore it).
   Generated code updates its stack frame's location directly at offset 16. */
AdaTaskContext* __ada_trace_enter(AdaTraceFrame* frame, const char* routine, const char* location);
void __ada_trace_location(const char* location);
void __ada_trace_leave(AdaTraceFrame* frame);
void __ada_trace_leave_context(AdaTaskContext* context, AdaTraceFrame* frame);

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
   cannot be met, and freeing null does nothing. Deallocation of a collection
   member finalizes its active controlled parts before releasing storage. */
void* __ada_allocate(long size);
void __ada_deallocate(void* address);
int64_t __ada_array_length(int64_t first, int64_t last);
int __ada_array_narrow(int64_t length);
int64_t __ada_array_offset(int64_t index, int64_t first, int64_t last, int64_t stride);
int64_t __ada_array_slice_offset(int64_t low, int64_t high, int64_t first, int64_t last, int64_t stride);
int64_t __ada_array_concat_shape(AdaArrayDimension* shape, int64_t leftFirst, int64_t leftLength,
    int64_t rightFirst, int64_t rightLast, int64_t rightLength, int64_t indexFirst, int64_t indexLast,
    int constrainedAncestor, int64_t elementSize);
int64_t __ada_array_size(int64_t first, int64_t last, int64_t elementSize);
void __ada_component_array_copy(void* descriptor, int rank);
void* __ada_array_local(void** owner, int64_t first, int64_t last, int64_t elementSize);
void __ada_array_release(void** owner);
void __ada_array_rewind(void** owner, void* checkpoint);
void __ada_array_adopt(void** owner, void* data);

void __ada_array_result(void* descriptor, const void* source, int64_t first, int64_t last, int64_t elementSize);
void __ada_array_result_cleanup(AdaTaskContext* context, void* descriptor);

/* Tagged descriptors preserve their first three layout words. Local tag
   metadata survives its master for tag queries; object escape checks protect
   the captured frames. Tagged results transfer a heap copy to the caller. */
void __ada_tag_register(void* tag);
void* __ada_tag_create(const void* templateTag, void* parent, void* master);
void* __ada_tag_create_layout(const void* templateTag, void* parent, void* master, int64_t size, const int64_t* layout);
void* __ada_tag_create_full(const void* templateTag, void* parent, void* master, int64_t size,
    const int64_t* layout, int64_t interfaceCount, const AdaInterfaceBinding* interfaces);
int __ada_tag_has_ancestor(const void* tag, const void* ancestor);
void* __ada_tag_dispatch(void* tag, void* interfaceTag, int slot);
int __ada_tag_interface_count(void* value);
void* __ada_tag_interface_at(void* value, int position);
void __ada_tag_set_layout(void* value, int64_t size, const int64_t* layout);
void __ada_tag_check_level(const void* object, int level);
void __ada_tag_check_accessibility(const void* object, int level);
void* __ada_collection_begin(void* collection);
void* __ada_collection_result_owner(void* pending);
void* __ada_collection_result_arena(void* pending);
void __ada_collection_finish(void* pending, void* object);
void __ada_collection_abort(void* pending);
void* __ada_construction_owner(void* allocation);
void* __ada_construction_arena(void* allocation);
struct AdaFinalization;
void* __ada_tagged_owned_copy(struct AdaFinalization** owner, void** arena, const void* source);
void* __ada_tagged_allocation(void* collection, const void* source);
void __ada_tagged_assign(void* target, const void* source, struct AdaFinalization** owner, void** arena);
void __ada_tagged_result(void** result, const void* object);
int __ada_tagged_equal(const void* left, const void* right);
const char* __ada_tag_name(const void* tag);
void* __ada_tag_parent(void* tag);
int __ada_tag_is_descendant(void* descendant, void* ancestor);
void* __ada_tag_internal(const char* name, int length);
void* __ada_tag_descendant(const char* name, int length, void* ancestor);
int __ada_tag_is_abstract(void* tag);
void __ada_tag_check_copy(const void* object);

/* Fixed-point arithmetic uses signed count storage and binary scale exponents. */
long long __ada_fixed_operation(FixedOperation operation, long long left, int leftBits,
                               long long right, int rightBits, int resultBits);
long long __ada_modular_to_fixed(unsigned long long value, int bits);
long long __ada_fixed_rescale(long long value, int fromBits, int toBits);
long long __ada_float_to_fixed(double value, int bits);
long long __ada_fixed_minmax(long long left, long long right, int maximum);
long long __ada_fixed_fore(long long low, long long high, int bits);
double __ada_fixed_real_operation(int operation, long long left, int leftBits, long long right, int rightBits, int precision);
long long __ada_fixed_numeric_operation(FixedOperation operation, long long left, int leftBits,
                                       long long right, int rightBits, int isUnsigned);

/* Exact fixed-point text conversion; values use signed scaled counts. */
int __ada_image_fixed(char* buffer, int capacity, long long value, int bits, int aft);
long long __ada_value_fixed(const char* text, int length, int bits, long long low, long long high);
int __ada_fixed_scale(double small);
void __ada_fixed_put_string(char* to, int length, long long value, int bits, int aft, int exponent);
long long __ada_fixed_get_string(const char* from, int length, int bits, long long low, long long high, int* last);

/* Renders a real value the way Ada.Text_IO does.  Fore is the least number of
   characters before the point including the sign, Aft the number after it, and
   Exp the width of the exponent field counting its sign.  An Exp of zero asks
   for plain decimal notation. */
void __ada_format_float(char* buffer, int size, double value, int fore, int aft, int exponent);

/* Images write into caller-owned storage, including a trailing C zero, and
   return the Ada length (which may include an embedded zero). The emitter
   reserves 64 bytes for numeric images, at least max literal length + 1 for enums,
   and at least max(aft, 1) + 42 bytes for fixed-point images. A short buffer
   raises Storage_Error. No pointer to runtime scratch storage escapes. */
int __ada_image_float(char* buffer, int capacity, double value, int aft, int exponent);
int __ada_width_integer(uint64_t low, uint64_t high, int isUnsigned);
int __ada_width_enum(int low, int high, const char** names, int count);
int __ada_image_integer(char* buffer, int capacity, int value);
int __ada_image_long_integer(char* buffer, int capacity, long long value);
int __ada_enum_code(int position, const int32_t* codes, int count);
int __ada_enum_position(int code, const int32_t* codes, int count);
int __ada_image_enum(char* buffer, int capacity, int value, const char** names, int count);
int __ada_image_character(char* buffer, int capacity, int value);

/* 'Value, which reads back what 'Image wrote.  Surrounding blanks are ignored,
   and anything else raises Constraint_Error. */
int __ada_value_integer(const char* text, int length, int low, int high);
long long __ada_value_long_integer(const char* text, int length, long long low, long long high);

/* Checked signed arithmetic: add, subtract, multiply, divide, rem, mod, power. */
long long __ada_modular_minmax(unsigned long long left, unsigned long long right, int maximum);
long long __ada_round_to_modular(double value);
long long __ada_modular_reduce(long long value, int isUnsigned, long long modulus);
long long __ada_modular_convert(long long value, int isUnsigned, long long low, long long high, int targetUnsigned);
int __ada_image_modular(char* buffer, int capacity, unsigned long long value);
int __ada_modular_token(const char* text, int length);
long long __ada_value_modular(const char* text, int length, long long modulus);
long long __ada_modular_operation(ModularOperation operation, long long modulus, long long left, long long right);
long long __ada_integer_operation(int operation, int bits, long long left, long long right);
/* Core real arithmetic preserves the operand precision. Zero divisors and
   nonfinite inputs/results raise Constraint_Error in the bound task context. */
float __ada_real_operation_f32(RealOperation operation, float left, float right);
double __ada_real_operation(RealOperation operation, double left, double right);
float __ada_value_real_f32(const char* text, int length);
double __ada_value_real(const char* text, int length);
float __ada_real_power_f32(float left, int exponent);
double __ada_real_power(double left, int exponent);
int __ada_value_enum(const char* text, int length, const char** names, int count);
int __ada_value_character(const char* text, int length);

/* Compiler-owned activation records: no heap allocation during registration. */
typedef struct AdaFinalization
{
    struct AdaFinalization* next;
    void* object;
    void (*finalize)(void*);
    struct AdaFinalization* registeredNext;
    int active;
} AdaFinalization;

void __ada_finalization_push(AdaFinalization** owner, AdaFinalization* record,
                             void* object, void (*finalize)(void*));
void __ada_finalize_to(AdaFinalization** owner, AdaFinalization* checkpoint);

void __ada_finalization_reserve(AdaFinalization** owner, void** arena, void* object, void (*finalize)(void*));
void __ada_library_reserve(void* object, void (*finalize)(void*));
void __ada_library_finalize(void);
void __ada_library_enter(void);
void __ada_library_activate(void);
/* Collection identity belongs to the declaring master. Members and their
   heap finalization records may be used by any synchronized task. Callbacks
   run in the executing task; only stack chains require their original context. */
void* __ada_collection_create(AdaFinalization** owner, void** arena);
void* __ada_collection_allocate(void* collection, long size);
/* Called after successful allocator initialization, never on a partial object. */
void __ada_collection_activate(void* collection);
void __ada_allocation_reserve(void* allocation, void* part, void (*finalize)(void*));
void __ada_controlled_activate(void* object, void (*finalize)(void*));
int __ada_controlled_finalize(void* object);
int __ada_controlled_adjust(void* object, void (*adjust)(void*));
int __ada_controlled_adjust_view(void* object, void (*adjust)(void*, void*), void* tag);
int __ada_controlled_finalize_view(void* object, void (*finalize)(void*, void*), void* tag);

int64_t __ada_layout_append(int64_t offset, int64_t size, int64_t alignment);
void __ada_allocation_set_array_bounds(void* object, const void* descriptor, int rank, const char* identity);
void* __ada_allocation_array_bounds(void* object, int rank);
void __ada_access_check(int sourceLevel, int targetLevel);
int __ada_current_access_level(void);
void __ada_accessibility_check(int sourceLevel, int targetLevel);
int __ada_collection_access_level(void* collection);
void __ada_collection_set_access_level(void* collection, int level);
void __ada_access_collection_parent(AdaMaster* master, int level);
void __ada_access_parameter_adopt(AdaFinalization** owner, void** arena, void* object);
void __ada_access_array_copy_bounds(void* object, int rank, const char* identity, void* descriptor);
void __ada_access_return_check(int sourceLevel, int targetLevel, void* owner, void* resultOwner);
void __ada_collection_finish_sized(void* pending, void* object, long size);
void __ada_object_borrow(void* collection, void* object, const char* identity, int constrained);
int __ada_access_constrained(void* object, const char* identity);
void __ada_array_borrow(void* collection, void* object, const void* descriptor, int rank, const char* identity);
void* __ada_access_array_bounds(void* object, int rank, const char* identity);

#endif
