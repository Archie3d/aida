/* Separate barriers bracket the generated Ada writes. A worker cannot publish
   runtime task completion while its peer is still reaching/executing the loop.
   The finish barrier never acquires the start barrier's mutex, so it cannot
   order one loop before the other's start. The conflicting writes stay in Ada. */
#include "../../runtime/adathread.h"
#include <stdint.h>
#include <stdlib.h>

struct Barrier
{
    AdaMutex mutex;
    AdaCondition changed;
    int arrived;
};

static struct Barrier start = {ADA_MUTEX_INITIALIZER, ADA_CONDITION_INITIALIZER, 0};
static struct Barrier finish = {ADA_MUTEX_INITIALIZER, ADA_CONDITION_INITIALIZER, 0};

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
    meet(&start);
}

void raceFinish(void)
{
    meet(&finish);
}
