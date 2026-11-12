#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include <pthread.h>
#include <sched.h>
#include <time.h>
#include <errno.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/resource.h>
#include <sys/syscall.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Linux priority contract failed at line %d\n", __LINE__); abort(); \
} } while (0)

static int nativePriority, nativePolicy, failure, updates;
static long nativeThread(long operation, ...)
{
    (void)operation;
    return 123;
}
static int readPriority(int which, id_t who)
{
    (void)which;
    (void)who;
    return 0;
}
static int writePriority(int which, id_t who, int value)
{
    CHECK(which == PRIO_PROCESS && who == 123);
    ++updates;
    if (failure != 0) { errno = failure; return -1; }
    nativePriority = value;
    return 0;
}
static int schedulerMinimum(int policy)
{
    CHECK(policy == SCHED_FIFO || policy == SCHED_RR);
    return 1;
}
static int schedulerMaximum(int policy)
{
    CHECK(policy == SCHED_FIFO || policy == SCHED_RR);
    return 99;
}
static int writeScheduler(pthread_t thread, int policy, const struct sched_param* parameter)
{
    (void)thread;
    ++updates;
    if (failure != 0) { return failure; }
    nativePolicy = policy;
    nativePriority = parameter->sched_priority;
    return 0;
}

static int conditionClock(pthread_condattr_t* attributes, clockid_t clock)
{
    (void)attributes;
    CHECK(clock == CLOCK_MONOTONIC);
    return 0;
}

#undef __APPLE__
#ifndef __linux__
#define __linux__ 1
#endif
#ifndef SYS_gettid
#define SYS_gettid 0
#endif
#define pthread_condattr_setclock conditionClock
#define syscall nativeThread
#define getpriority readPriority
#define setpriority writePriority
#define sched_get_priority_min schedulerMinimum
#define sched_get_priority_max schedulerMaximum
#define pthread_setschedparam writeScheduler
#include "../runtime/adathread.c"

int main(int argc, char** argv)
{
    CHECK(argc == 2 && setenv("AIDA_TASK_SCHEDULER", argv[1], 1) == 0);
    AdaPriorityThread self = __ada_priority_thread_self();
    if (strcmp(argv[1], "invalid") == 0) {
        CHECK(__ada_thread_set_priority(self, kAdaDefaultPriority) == EINVAL && updates == 0);
        CHECK(setenv("AIDA_TASK_SCHEDULER", "host", 1) == 0);
        CHECK(__ada_thread_set_priority(self, kAdaDefaultPriority) == EINVAL && updates == 0);
        return 0;
    }
    int previous = -1;
    for (int priority = 0; priority <= kAdaAnyPriorityLast; ++priority) {
        CHECK(__ada_thread_set_priority(self, priority) == 0);
        if (strcmp(argv[1], "host") == 0) {
            CHECK(nativePriority == kAdaDefaultPriority - priority);
        } else {
            CHECK(nativePriority > previous);
            CHECK(nativePolicy == (strcmp(argv[1], "fifo") == 0 ? SCHED_FIFO : SCHED_RR));
            previous = nativePriority;
        }
    }
    int saved = nativePriority;
    CHECK(setenv("AIDA_TASK_SCHEDULER", "invalid", 1) == 0);
    failure = EPERM;
    CHECK(__ada_thread_set_priority(self, kAdaDefaultPriority) == EPERM);
    CHECK(nativePriority == saved);
    failure = 0;
    CHECK(__ada_thread_set_priority(self, kAdaDefaultPriority) == 0);
    CHECK(__ada_thread_set_priority(self, -1) == EINVAL);
    CHECK(__ada_thread_set_priority(self, kAdaAnyPriorityLast + 1) == EINVAL);
    return 0;
}
