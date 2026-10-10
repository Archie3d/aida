#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "adathread.h"
#include <errno.h>
#include <time.h>

int __ada_thread_create(AdaThread* thread, AdaThreadEntry entry, void* argument)
{
    if (!thread || !entry) {
        return EINVAL;
    }
    return pthread_create(thread, NULL, entry, argument);
}

int __ada_thread_join(AdaThread thread, void** result)
{
    return pthread_join(thread, result);
}

int __ada_condition_init(AdaCondition* condition)
{
#if defined(__APPLE__)
    return pthread_cond_init(condition, NULL);
#else
    pthread_condattr_t attributes;
    int status = pthread_condattr_init(&attributes);
    if (status != 0) {
        return status;
    }
    status = pthread_condattr_setclock(&attributes, CLOCK_MONOTONIC);
    if (status == 0) {
        status = pthread_cond_init(condition, &attributes);
    }
    pthread_condattr_destroy(&attributes);
    return status;
#endif
}

int __ada_condition_destroy(AdaCondition* condition)
{
    return pthread_cond_destroy(condition);
}

int __ada_condition_wait(AdaCondition* condition, AdaMutex* mutex)
{
    return pthread_cond_wait(condition, mutex);
}

int __ada_condition_wait_until(AdaCondition* condition, AdaMutex* mutex,
                               int64_t deadline)
{
    if (deadline < 0) {
        return EINVAL;
    }
#if defined(__APPLE__)
    int64_t now;
    int status = __ada_monotonic_clock(&now);
    if (status != 0) {
        return status;
    }
    /* Even an expired deadline must release/reacquire the mutex. */
    deadline = deadline > now ? deadline - now : 0;
#endif
    struct timespec timeout;
    int64_t seconds = deadline / 1000000000;
    timeout.tv_sec = (time_t)seconds;
    if ((int64_t)timeout.tv_sec != seconds) {
        return EOVERFLOW;
    }
    timeout.tv_nsec = (long)(deadline % 1000000000);
#if defined(__APPLE__)
    return pthread_cond_timedwait_relative_np(condition, mutex, &timeout);
#else
    return pthread_cond_timedwait(condition, mutex, &timeout);
#endif
}

int __ada_condition_signal(AdaCondition* condition)
{
    return pthread_cond_signal(condition);
}

int __ada_condition_broadcast(AdaCondition* condition)
{
    return pthread_cond_broadcast(condition);
}

int __ada_monotonic_clock(int64_t* nanoseconds)
{
    if (!nanoseconds) {
        return EINVAL;
    }
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        return errno;
    }
    if (now.tv_sec < 0 || (uint64_t)now.tv_sec >
            (uint64_t)(INT64_MAX - now.tv_nsec) / 1000000000) {
        return EOVERFLOW;
    }
    *nanoseconds = (int64_t)now.tv_sec * 1000000000 + now.tv_nsec;
    return 0;
}

/* GCC/Clang builtins operate on ordinary generated storage, unlike C11
   _Atomic types whose representation would become part of the QBE ABI.
   Require native support so static runtime consumers never need libatomic. */
#define ADA_DEFINE_ATOMIC(bits) \
    _Static_assert(__atomic_always_lock_free(sizeof(uint##bits##_t), 0), \
                   "runtime requires lock-free " #bits "-bit scalar atomics"); \
    uint##bits##_t __ada_atomic_load_##bits(const volatile void* address) \
    { \
        return __atomic_load_n((const volatile uint##bits##_t*)address, __ATOMIC_SEQ_CST); \
    } \
    void __ada_atomic_store_##bits(volatile void* address, uint##bits##_t value) \
    { \
        __atomic_store_n((volatile uint##bits##_t*)address, value, __ATOMIC_SEQ_CST); \
    }
ADA_DEFINE_ATOMIC(8)
ADA_DEFINE_ATOMIC(16)
ADA_DEFINE_ATOMIC(32)
ADA_DEFINE_ATOMIC(64)
#undef ADA_DEFINE_ATOMIC
