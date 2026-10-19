/* A bounded rendezvous proves that generated tasks execute concurrently.
   All communication uses runtime locks or C11 atomics, ahead of protected
   objects and Atomic/Volatile syntax in later Phase 4 items. */
#include "../runtime/adart.h"
#include "../runtime/adathread.h"
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

static AdaMutex mutex = ADA_MUTEX_INITIALIZER;
static AdaCondition condition;
static int arrived;
static int released;
static atomic_int activations;
static atomic_int completions;

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "tasking check failed at line %d\n", __LINE__); abort(); \
} } while (0)

void taskReset(void)
{
    CHECK(__ada_condition_init(&condition) == 0);
    arrived = 0;
    released = 0;
    atomic_store(&activations, 0);
    atomic_store(&completions, 0);
}

int taskActivate(void)
{
    CHECK(__ada_task_context() != &__ada_main_context);
    atomic_fetch_add(&activations, 1);
    return 0;
}

int taskActivationCount(void)
{
    return atomic_load(&activations);
}

void taskRelease(void)
{
    CHECK(atomic_load(&activations) == 2);
    __ada_mutex_lock(&mutex);
    released = 1;
    CHECK(__ada_condition_broadcast(&condition) == 0);
    __ada_mutex_unlock(&mutex);
}

void taskRendezvous(void)
{
    int64_t now;
    CHECK(__ada_monotonic_clock(&now) == 0);
    __ada_mutex_lock(&mutex);
    ++arrived;
    CHECK(__ada_condition_broadcast(&condition) == 0);
    while (arrived != 2 || !released) {
        CHECK(__ada_condition_wait_until(&condition, &mutex, now + INT64_C(5000000000)) == 0);
    }
    __ada_mutex_unlock(&mutex);
    atomic_fetch_add(&completions, 1);
}

void taskFinalized(void)
{
    CHECK(atomic_load(&completions) == 2);
    CHECK(__ada_condition_destroy(&condition) == 0);
}
