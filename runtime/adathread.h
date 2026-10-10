#ifndef ADATHREAD_H
#define ADATHREAD_H

#include "adalock.h"
#include <stdint.h>

/* Internal C ABI for POSIX hosts, including MSYS2 winpthreads. These objects
   are caller-owned, noncopyable once initialized, and must outlive all users.
   Status functions return zero or a POSIX error number, never raise Ada
   exceptions, and do not modify the current task context.

   Threads are joinable; exactly one owner must join each successful creation.
   Entry arguments must remain alive until consumed. Entries invoking Ada must
   bind and dispose their own AdaTaskContext (see adart.h). No context is
   inherited, allocated or disposed by this low-level layer. Cancellation and
   detached threads are deliberately outside this API. */
typedef pthread_t AdaThread;
typedef pthread_cond_t AdaCondition;
typedef void* (*AdaThreadEntry)(void*);

int __ada_thread_create(AdaThread* thread, AdaThreadEntry entry, void* argument);
int __ada_thread_join(AdaThread thread, void** result);

/* Conditions require explicit initialization to select the monotonic clock.
   Wait with the mutex locked, using the same mutex for concurrent waiters.
   Waits release it while blocked and reacquire it before returning. Always
   recheck the predicate in a loop, including on ETIMEDOUT. Signal/broadcast
   do not store notifications. Destroy only after all waiters have left. */
int __ada_condition_init(AdaCondition* condition);
int __ada_condition_destroy(AdaCondition* condition);
int __ada_condition_wait(AdaCondition* condition, AdaMutex* mutex);
int __ada_condition_wait_until(AdaCondition* condition, AdaMutex* mutex,
                               int64_t deadline);
int __ada_condition_signal(AdaCondition* condition);
int __ada_condition_broadcast(AdaCondition* condition);

/* Nanoseconds from an unspecified monotonic epoch. Deadlines use this same
   clock; negative deadlines are invalid. Output is unchanged on failure.
   macOS implements timed waits using its relative monotonic pthread API. */
int __ada_monotonic_clock(int64_t* nanoseconds);

/* Generated-code ABI: naturally aligned scalar storage, widths in bits.
   All concurrent accesses to an object must use these helpers (no mixing
   with plain C/QBE accesses). Sequential consistency also provides the
   ordering needed for future Volatile lowering, with stronger atomicity.
   Signed integers, pointers and floats use their same-width bit patterns.
   Composite/unaligned objects and device memory are not supported. The
   compiler still rejects Atomic/Volatile pragmas until Phase 4. */
#define ADA_DECLARE_ATOMIC(bits) \
    uint##bits##_t __ada_atomic_load_##bits(const volatile void* address); \
    void __ada_atomic_store_##bits(volatile void* address, uint##bits##_t value);
ADA_DECLARE_ATOMIC(8)
ADA_DECLARE_ATOMIC(16)
ADA_DECLARE_ATOMIC(32)
ADA_DECLARE_ATOMIC(64)
#undef ADA_DECLARE_ATOMIC

#endif
