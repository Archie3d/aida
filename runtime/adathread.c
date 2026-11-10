#if defined(__APPLE__) && !defined(_DARWIN_C_SOURCE)
#define _DARWIN_C_SOURCE
#endif
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif

#include "adathread.h"
#include <errno.h>
#include <time.h>
#include <sched.h>
#include <limits.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <mach/mach.h>
#include <mach/thread_policy.h>
#elif defined(__linux__)
#include <sys/resource.h>
#include <sys/syscall.h>
extern long syscall(long, ...);
#endif

int __ada_thread_create(AdaThread* thread, AdaThreadEntry entry, void* argument)
{
    if (!thread || !entry) {
        return EINVAL;
    }
    return pthread_create(thread, NULL, entry, argument);
}

int __ada_thread_stack_size(int64_t requested, int64_t* actual)
{
    if (requested < 0 || actual == NULL || (uint64_t)requested > SIZE_MAX) { return EINVAL; }
    pthread_attr_t attributes;
    int status = pthread_attr_init(&attributes);
    if (status != 0) { return status; }
    size_t size;
    status = pthread_attr_getstacksize(&attributes, &size);
    if (status == 0 && (uint64_t)requested > size) {
        long page = sysconf(_SC_PAGESIZE);
        size_t alignment = page > 0 ? (size_t)page : 4096;
        size = (size_t)requested;
        size_t remainder = size % alignment;
        if (remainder != 0) {
            if (size > SIZE_MAX - (alignment - remainder)) { status = EOVERFLOW; }
            else { size += alignment - remainder; }
        }
    }
    if (status == 0) { status = pthread_attr_setstacksize(&attributes, size); }
    pthread_attr_destroy(&attributes);
    if (status == 0 && size <= INT64_MAX) { *actual = (int64_t)size; }
    else if (status == 0) { status = EOVERFLOW; }
    return status;
}

int __ada_thread_create_stack(AdaThread* thread, AdaThreadEntry entry, void* argument, int64_t bytes)
{
    if (thread == NULL || entry == NULL || bytes <= 0 || (uint64_t)bytes > SIZE_MAX) { return EINVAL; }
    pthread_attr_t attributes;
    int status = pthread_attr_init(&attributes);
    if (status != 0) { return status; }
    status = pthread_attr_setstacksize(&attributes, (size_t)bytes);
    if (status == 0) { status = pthread_create(thread, &attributes, entry, argument); }
    pthread_attr_destroy(&attributes);
    return status;
}

AdaPriorityThread __ada_priority_thread_self(void)
{
    AdaPriorityThread result = { .thread = pthread_self(), .nativeId = 0 };
#if defined(__linux__)
    result.nativeId = syscall(SYS_gettid);
#endif
    return result;
}

#if defined(__linux__)
static pthread_once_t priorityBaselineOnce = PTHREAD_ONCE_INIT;
static int priorityBaseline;
static void capturePriorityBaseline(void)
{
    priorityBaseline = getpriority(PRIO_PROCESS, 0);
}
#endif

int __ada_thread_set_priority(AdaPriorityThread thread, int priority)
{
    if (priority < kAdaPriorityFirst || priority > kAdaAnyPriorityLast) { return EINVAL; }
#if defined(__APPLE__)
    thread_precedence_policy_data_t policy = { .importance = priority - kAdaDefaultPriority };
    kern_return_t status = thread_policy_set(pthread_mach_thread_np(thread.thread),
        THREAD_PRECEDENCE_POLICY, (thread_policy_t)&policy, THREAD_PRECEDENCE_POLICY_COUNT);
    return status == KERN_SUCCESS ? 0 : ENOTSUP;
#elif defined(__linux__)
    pthread_once(&priorityBaselineOnce, capturePriorityBaseline);
    int nice = priorityBaseline + kAdaDefaultPriority - priority;
    if (nice < -20 || nice > 19) { return ENOTSUP; }
    return setpriority(PRIO_PROCESS, (id_t)thread.nativeId, nice) == 0 ? 0 : errno;
#else
    int policy;
    struct sched_param parameter;
    int status = pthread_getschedparam(thread.thread, &policy, &parameter);
    if (status != 0) { return status; }
    int first = sched_get_priority_min(policy), last = sched_get_priority_max(policy);
    if (first < 0 || last < 0) { return errno; }
    if (last - first < kAdaAnyPriorityLast) { return priority == kAdaDefaultPriority ? 0 : ENOTSUP; }
    parameter.sched_priority = first + (priority * (last - first)) / kAdaAnyPriorityLast;
    return pthread_setschedparam(thread.thread, policy, &parameter);
#endif
}

int __ada_thread_join(AdaThread thread, void** result)
{
    return pthread_join(thread, result);
}

int __ada_condition_init(AdaCondition* condition)
{
#if defined(__APPLE__) || defined(_WIN32)
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
#if defined(__APPLE__) || defined(_WIN32)
    int64_t now;
    int status = __ada_monotonic_clock(&now);
    if (status != 0) {
        return status;
    }
    /* Even an expired deadline must release/reacquire the mutex. */
    int64_t remaining = deadline > now ? deadline - now : 0;
#if defined(_WIN32)
    if (remaining > INT64_C(86400000000000)) {
        remaining = INT64_C(86400000000000);
    }
    remaining = ((remaining + 999999) / 1000000) * 1000000;
#endif
#else
    int64_t remaining = deadline;
#endif
    struct timespec timeout;
    int64_t seconds = remaining / 1000000000;
    timeout.tv_sec = (time_t)seconds;
    if ((int64_t)timeout.tv_sec != seconds) {
        return EOVERFLOW;
    }
    timeout.tv_nsec = (long)(remaining % 1000000000);
#if defined(_WIN32)
    status = pthread_cond_timedwait_relative_np(condition, mutex, &timeout);
    if (status == ETIMEDOUT) {
        status = __ada_monotonic_clock(&now);
        if (status != 0) {
            return status;
        }
        return now >= deadline ? ETIMEDOUT : 0;
    }
    return status;
#elif defined(__APPLE__)
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

int __ada_monotonic_resolution(int64_t* nanoseconds)
{
    if (nanoseconds == NULL) {
        return EINVAL;
    }
    struct timespec resolution;
    if (clock_getres(CLOCK_MONOTONIC, &resolution) != 0) {
        return errno;
    }
    if (resolution.tv_sec < 0 || (uint64_t)resolution.tv_sec >
            (uint64_t)(INT64_MAX - resolution.tv_nsec) / 1000000000) {
        return EOVERFLOW;
    }
    *nanoseconds = (int64_t)resolution.tv_sec * 1000000000 + resolution.tv_nsec;
    if (*nanoseconds == 0) {
        *nanoseconds = 1;
    }
    return 0;
}

int __ada_monotonic_delay_until(int64_t deadline)
{
    for (;;) {
        int64_t now;
        int status = __ada_monotonic_clock(&now);
        if (status != 0) {
            return status;
        }
        if (deadline <= now) {
            return sched_yield() == 0 ? 0 : errno;
        }
        int64_t remaining = deadline - now;
        // Bounded chunks fit time_t on POSIX hosts and winpthreads.
        if (remaining > INT64_C(86400000000000)) {
            remaining = INT64_C(86400000000000);
        }
        struct timespec request;
        request.tv_sec = (time_t)(remaining / 1000000000);
        request.tv_nsec = (long)(remaining % 1000000000);
        if (nanosleep(&request, NULL) != 0 && errno != EINTR) {
            return errno;
        }
    }
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
