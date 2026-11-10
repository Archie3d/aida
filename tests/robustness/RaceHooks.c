/* The independent barriers keep both tasks alive across the Ada write.
   Relaxed turn flags select a physical order without a happens-before edge
   between the conflicting writes. Neither worker may publish runtime task
   completion until both writes have happened. No conflicting access is in C. */
#include "../../runtime/adathread.h"
#include <stdint.h>
#include <stdatomic.h>
#include <stdlib.h>

struct Barrier
{
    AdaMutex mutex;
    AdaCondition changed;
    int arrived;
};

static struct Barrier start = {ADA_MUTEX_INITIALIZER, ADA_CONDITION_INITIALIZER, 0};
static struct Barrier finish = {ADA_MUTEX_INITIALIZER, ADA_CONDITION_INITIALIZER, 0};
static atomic_uint tickets;
static atomic_uint turn;
static _Thread_local unsigned int lane;

static void awaitTurn(unsigned int expected)
{
    int64_t start, now;
    if (__ada_monotonic_clock(&start) != 0) { abort(); }
    while (atomic_load_explicit(&turn, memory_order_relaxed) != expected) {
        if (__ada_monotonic_clock(&now) != 0 || now - start > INT64_C(5000000000)) { abort(); }
    }
}

static void meet(struct Barrier* barrier)
{
    int64_t now;
    if (__ada_monotonic_clock(&now) != 0) { abort(); }
    __ada_mutex_lock(&barrier->mutex);
    ++barrier->arrived;
    if (__ada_condition_broadcast(&barrier->changed) != 0) { abort(); }
    while (barrier->arrived != 2) {
        if (__ada_condition_wait_until(&barrier->changed, &barrier->mutex,
            now + INT64_C(5000000000)) != 0) { abort(); }
    }
    __ada_mutex_unlock(&barrier->mutex);
}

void raceStart(void)
{
    lane = atomic_fetch_add_explicit(&tickets, 1, memory_order_relaxed);
    if (lane >= 2) { abort(); }
    meet(&start);
}

void raceWriteTurn(void)
{
    awaitTurn(lane);
}

void raceFinish(void)
{
    atomic_store_explicit(&turn, lane + 1, memory_order_relaxed);
    awaitTurn(2);
    meet(&finish);
}
