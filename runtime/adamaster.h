#ifndef ADAMASTER_H
#define ADAMASTER_H

#include "../common/Concurrent.h"

#include <stdatomic.h>
#include <stdint.h>
#include "../common/Master.h"
#include "../common/Select.h"

typedef struct AdaTaskContext AdaTaskContext;
struct AdaException;
struct AdaMaster;
extern const struct AdaException __ada_exc_async_transfer;
void* __ada_async_start_delay(AdaTaskContext* context, int64_t deadline, enum AdaSelectClock clock);
void* __ada_async_start_task(AdaTaskContext* context, void* identity, int entry, int64_t index, void* arguments);
void* __ada_async_start_task_packet(AdaTaskContext* context, void* identity, int entry, int64_t index,
    void* arguments, int64_t bytes);
void* __ada_async_start_protected_packet(AdaTaskContext* context, AdaConcurrentObject* object,
    int (*barrier)(void*, void*), void* link, void (*body)(void*), void* arguments, int64_t bytes);
void* __ada_async_start_protected(AdaTaskContext* context, AdaConcurrentObject* object,
    int (*barrier)(void*, void*), void* link, void (*body)(void*), void* arguments);
int __ada_async_run(void* operation);
void __ada_async_poll(AdaTaskContext* context);
int __ada_async_check(AdaTaskContext* context);
int __ada_async_ready(AdaTaskContext* context);
int __ada_async_leave(AdaTaskContext* context, void* operation);
int __ada_async_matches(AdaTaskContext* context, void* operation);
void __ada_abort_tasks(AdaTaskContext* context, void** identities, int count);
void __ada_abort_task(void* identity);
void __ada_task_block_begin(AdaTaskContext* context);
void __ada_task_block_end(AdaTaskContext* context);
void __ada_master_abort(AdaTaskContext* context, struct AdaMaster* master);

/* Caller-owned identity, live until its dependents have terminated and local
   finalization has finished. Empty lexical masters need no allocation;
   collections register ownership before foreign tasks can use them. */
typedef struct AdaMaster
{
    void* state;
    /* Monotonic while live. Foreign task creation marks every ancestor before
       publishing the task, so empty cleanup can avoid the shared registry. */
    atomic_uint activities;
    unsigned int deferralDepth;
} AdaMaster;

enum AdaMasterKind
{
    ADA_MASTER_SUBPROGRAM,
    ADA_MASTER_BLOCK,
    ADA_MASTER_LIBRARY,
    ADA_MASTER_COLLECTION
};

/* Enter before declarations, activate the group after successful elaboration,
   await before finalization/storage release, and leave afterwards. Collection
   masters are independent of the lexical stack and may activate repeatedly.
   Await/leave preserve the activator's pending exception. */
void __ada_master_enter(AdaTaskContext* context, AdaMaster* master, int kind);
void __ada_master_activate(AdaTaskContext* context, AdaMaster* master);
void __ada_master_await(AdaTaskContext* context, AdaMaster* master);
void __ada_master_leave(AdaTaskContext* context, AdaMaster* master);

typedef struct AdaSelectChoice
{
    enum AdaSelectKind kind;
    int entry;
    int64_t index;
    int64_t deadline;
    enum AdaSelectClock clock;
    int open;
} AdaSelectChoice;

int64_t __ada_select_deadline(int64_t value, enum AdaSelectClock clock);
int64_t __ada_select_wait_deadline(int64_t value, enum AdaSelectClock clock);
int __ada_task_select(AdaTaskContext* context, const AdaSelectChoice* choices,
    int count, int hasElse, void** request);
int __ada_task_entry_call_until(AdaTaskContext* context, void* identity, int entry,
    int64_t index, void* arguments, int64_t deadline, enum AdaSelectClock clock,
    enum AdaEntryCallMode mode);

/* Generated task body ABI, including a lexical link for library bodies. */
void __ada_task_create(AdaTaskContext* context, AdaConcurrentObject* object, void (*body)(void*),
                      void* link, const int* elaborated, void* collection, void* group);
/* Build-in-place results retain the caller's owner and activation group. */
AdaMaster* __ada_task_owner(AdaTaskContext* context, void* collection);
void __ada_task_create_configured(AdaTaskContext* context, AdaConcurrentObject* object, void (*body)(void*),
    void* link, const int* elaborated, void* collection, void* group, AdaMaster* owner,
    int priority, int specified, int64_t storageSize);
void __ada_task_create_owned(AdaTaskContext* context, AdaConcurrentObject* object, void (*body)(void*),
                            void* link, const int* elaborated, void* collection, void* group,
                            AdaMaster* owner);
void __ada_task_result_await(void* master);
void* __ada_task_result_mark(AdaMaster* master);
void __ada_task_result_discard(AdaMaster* master, void* mark, void* group);
void __ada_task_activate_group(AdaTaskContext* context, void* collection, void* group);
void __ada_task_entry_call(AdaTaskContext* context, void* identity, int entry, int64_t index, void* arguments);
void* __ada_task_entry_accept(AdaTaskContext* context, int entry, int64_t index);
void* __ada_task_entry_arguments(void* request);
void __ada_task_entry_complete(AdaTaskContext* context, void* request);
void* __ada_task_entry_caller(AdaTaskContext* context);
int __ada_task_entry_count(void* identity, int entry, int64_t index);
void __ada_task_activated(AdaTaskContext* context);
void __ada_task_activation_failed(AdaTaskContext* context);
void __ada_task_discard(AdaTaskContext* context, AdaMaster* master, void* object, long size);

/* Task_Id values remain valid while the corresponding task object exists.
   Queries synchronize with completion and termination without joining tasks. */
void* __ada_current_task(void);
void __ada_set_dependents_handler(const void* handler);
const void* __ada_current_fallback_handler(void);
void __ada_set_specific_handler(void* identity, const void* handler);
const void* __ada_specific_handler(void* identity);
int __ada_get_priority(void* identity);
void __ada_set_priority(int priority, void* identity);
void __ada_main_priority(int priority);
int __ada_priority_enter(int ceiling);
int __ada_priority_inherit(int priority);
void __ada_priority_leave(int saved);
int64_t __ada_task_storage_size(void* identity);
void* __ada_current_task_object(void);
void* __ada_task_identity(AdaConcurrentObject* object);
int __ada_task_id_equal(void* left, void* right);
const char* __ada_task_image(void* identity);
int __ada_task_is_callable(void* identity);
int __ada_task_is_terminated(void* identity);
void __ada_task_complete(AdaTaskContext* context);
void __ada_environment_task_termination(void);
void __ada_environment_task_state(int completed, int terminated);

#endif
