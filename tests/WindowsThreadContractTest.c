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
#include "platform/windows/windows.h"

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "Windows thread contract failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kMiB = 1024 * 1024, kGranularity = 64 * 1024 };
static struct Image
{
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS nt;
} image;
static size_t attributeSize, createdSize;
static int nativePriority;

void GetSystemInfo(SYSTEM_INFO* information)
{
    *information = (SYSTEM_INFO) {4096, kGranularity};
}

void* GetModuleHandleW(const wchar_t* name)
{
    CHECK(name == NULL);
    return &image;
}

static int attributeInit(pthread_attr_t* attributes)
{
    (void)attributes;
    attributeSize = 0; // winpthreads uses zero for the PE default.
    return 0;
}

static int attributeDestroy(pthread_attr_t* attributes)
{
    (void)attributes;
    return 0;
}

static int attributeGet(const pthread_attr_t* attributes, size_t* size)
{
    (void)attributes;
    *size = attributeSize;
    return 0;
}

static int attributeSet(pthread_attr_t* attributes, size_t size)
{
    (void)attributes;
    attributeSize = size;
    return 0;
}

static int createThread(pthread_t* thread, const pthread_attr_t* attributes,
    void* (*entry)(void*), void* argument)
{
    CHECK(attributes != NULL && entry != NULL && argument == NULL);
    *thread = pthread_self();
    createdSize = attributeSize;
    return 0;
}

static int setPriority(pthread_t thread, int policy, const struct sched_param* parameter)
{
    (void)thread;
    CHECK(policy == SCHED_OTHER);
    nativePriority = parameter->sched_priority;
    return 0;
}

static int relativeWait(pthread_cond_t* condition, pthread_mutex_t* mutex, const struct timespec* timeout)
{
    (void)condition;
    (void)mutex;
    (void)timeout;
    return ETIMEDOUT;
}

/* Compile the production Windows branches on POSIX as well. These API models
   deliberately expose zero stack defaults and signed native priorities. */
#undef __APPLE__
#undef __linux__
#define _WIN32 1
#define sysconf(...) WINDOWS_RUNTIME_MUST_NOT_CALL_SYSCONF
#define pthread_attr_init attributeInit
#define pthread_attr_destroy attributeDestroy
#define pthread_attr_getstacksize attributeGet
#define pthread_attr_setstacksize attributeSet
#define pthread_create createThread
#define pthread_setschedparam setPriority
#define pthread_cond_timedwait_relative_np relativeWait
#include "../runtime/adathread.c"

static void* entry(void* argument)
{
    return argument;
}

int main(void)
{
    CHECK(setenv("AIDA_TASK_SCHEDULER", "host", 1) == 0);
    image.dos.e_magic = IMAGE_DOS_SIGNATURE;
    image.dos.e_lfanew = offsetof(struct Image, nt);
    image.nt.Signature = IMAGE_NT_SIGNATURE;
    image.nt.OptionalHeader.SizeOfStackReserve = 2 * kMiB;
    int64_t size = -1;
    CHECK(__ada_thread_stack_size(0, &size) == 0 && size == 2 * kMiB);
    CHECK(__ada_thread_stack_size(1, &size) == 0 && size == 2 * kMiB);
    AdaThread thread;
    CHECK(__ada_thread_create_stack(&thread, entry, NULL, size) == 0 && createdSize == 0);
    CHECK(__ada_thread_stack_size(2 * kMiB + 1, &size) == 0 && size == 3 * kMiB);
    CHECK(__ada_thread_create_stack(&thread, entry, NULL, size) == 0 && createdSize == 3 * kMiB);
    CHECK(__ada_thread_stack_size(INT64_C(1) << 32, &size) == EOVERFLOW);
    CHECK(__ada_thread_create_stack(&thread, entry, NULL, INT64_C(1) << 32) == EOVERFLOW);
    CHECK(__ada_thread_stack_size(-1, &size) == EINVAL);
    // Nonstandard PE defaults are rounded by the OS allocation granularity.
    image.nt.OptionalHeader.SizeOfStackReserve = 2 * kMiB + 1;
    CHECK(__ada_thread_stack_size(0, &size) == 0 && size == 2 * kMiB + kGranularity);
    image.nt.Signature = 0;
    size = -1;
    CHECK(__ada_thread_stack_size(0, &size) == EIO && size == -1);
    AdaPriorityThread self = __ada_priority_thread_self();
    CHECK(__ada_thread_set_priority(self, 0) == 0 && nativePriority == -15);
    CHECK(__ada_thread_set_priority(self, 7) == 0 && nativePriority == -2);
    CHECK(__ada_thread_set_priority(self, 14) == 0 && nativePriority == -1);
    CHECK(__ada_thread_set_priority(self, kAdaDefaultPriority) == 0 && nativePriority == 0);
    CHECK(setenv("AIDA_TASK_SCHEDULER", "invalid", 1) == 0);
    CHECK(__ada_thread_set_priority(self, kAdaPriorityLast) == 0 && nativePriority == THREAD_PRIORITY_HIGHEST);
    CHECK(__ada_thread_set_priority(self, kAdaAnyPriorityLast) == 0 && nativePriority == 15);
    CHECK(__ada_thread_set_priority(self, -1) == EINVAL);
    return 0;
}
