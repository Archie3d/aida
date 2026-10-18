#include "../runtime/adathread.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "threading check failed at line %d\n", __LINE__); exit(1); \
} } while (0)

enum { workerCount = 4, rounds = 10000 };

typedef struct Gate
{
    AdaMutex mutex;
    AdaMutex probe;
    AdaCondition ready;
    AdaCondition release;
    int waiting;
    int open;
} Gate;

static int64_t now(void)
{
    int64_t value = -1;
    CHECK(__ada_monotonic_clock(&value) == 0 && value >= 0);
    return value;
}

static void* waitForGate(void* argument)
{
    Gate* gate = argument;
    CHECK(__ada_mutex_trylock(&gate->probe) == EBUSY);
    __ada_mutex_lock(&gate->mutex);
    ++gate->waiting;
    CHECK(__ada_condition_signal(&gate->ready) == 0);
    while (!gate->open) {
        CHECK(__ada_condition_wait(&gate->release, &gate->mutex) == 0);
    }
    __ada_mutex_unlock(&gate->mutex);
    return argument;
}

/* No mutex participates in publication: the atomic flag must order ordinary
   payload accesses as well as preserve all scalar widths/bit patterns. */
typedef struct Mailbox
{
    uint32_t full;
    uint8_t byte;
    uint16_t half;
    uint32_t word;
    uint64_t dword;
    int payload;
} Mailbox;

static void* publish(void* argument)
{
    Mailbox* box = argument;
    for (int i = 1; i <= rounds; ++i) {
        while (__ada_atomic_load_32(&box->full)) {
        }
        box->payload = i;
        __ada_atomic_store_8(&box->byte, (uint8_t)i);
        __ada_atomic_store_16(&box->half, (uint16_t)~i);
        __ada_atomic_store_32(&box->word, UINT32_MAX - i);
        __ada_atomic_store_64(&box->dword, UINT64_MAX - i);
        __ada_atomic_store_32(&box->full, 1);
    }
    return argument;
}

int main(void)
{
    AdaThread workers[workerCount];
    CHECK(__ada_thread_create(NULL, publish, NULL) == EINVAL);
    CHECK(__ada_thread_create(&workers[0], NULL, NULL) == EINVAL);
    CHECK(__ada_monotonic_clock(NULL) == EINVAL);
    int64_t first = now();
    for (int i = 0; i < 1000; ++i) {
        int64_t next = now();
        CHECK(next >= first);
        first = next;
    }

    Gate gate = { 0 };
    CHECK(__ada_mutex_init(&gate.mutex) == 0);
    CHECK(__ada_mutex_init(&gate.probe) == 0);
    __ada_mutex_lock(&gate.probe);
    CHECK(__ada_condition_init(&gate.ready) == 0);
    CHECK(__ada_condition_init(&gate.release) == 0);
    CHECK(__ada_mutex_trylock(&gate.mutex) == 0);
    /* The probe mutex remains locked until all workers have joined. */
    for (int i = 0; i < workerCount; ++i) {
        CHECK(__ada_thread_create(&workers[i], waitForGate, &gate) == 0);
    }
    int64_t deadline = now() + INT64_C(5000000000);
    while (gate.waiting != workerCount) {
        CHECK(__ada_condition_wait_until(&gate.ready, &gate.mutex, deadline) == 0);
    }
    gate.open = 1;
    CHECK(__ada_condition_broadcast(&gate.release) == 0);
    __ada_mutex_unlock(&gate.mutex);
    for (int i = 0; i < workerCount; ++i) {
        void* result = NULL;
        CHECK(__ada_thread_join(workers[i], &result) == 0 && result == &gate);
    }

    __ada_mutex_lock(&gate.mutex);
    int64_t durations[] = { 1, 999999, 1000001, 20000000 };
    int status;
    for (size_t index = 0; index < sizeof durations / sizeof durations[0]; ++index) {
        deadline = now() + durations[index];
        do {
            status = __ada_condition_wait_until(&gate.ready, &gate.mutex, deadline);
        } while (status == 0);
        CHECK(status == ETIMEDOUT && now() >= deadline);
    }
    CHECK(__ada_condition_wait_until(&gate.ready, &gate.mutex, -1) == EINVAL);
    do {
        status = __ada_condition_wait_until(&gate.ready, &gate.mutex, 0);
    } while (status == 0);
    CHECK(status == ETIMEDOUT);
    CHECK(__ada_mutex_trylock(&gate.mutex) == EBUSY);
    __ada_mutex_unlock(&gate.mutex);
    CHECK(__ada_condition_destroy(&gate.release) == 0);
    CHECK(__ada_condition_destroy(&gate.ready) == 0);
    CHECK(__ada_mutex_destroy(&gate.mutex) == 0);
    __ada_mutex_unlock(&gate.probe);
    CHECK(__ada_mutex_destroy(&gate.probe) == 0);

    Mailbox box = { 0 };
    CHECK(__ada_thread_create(&workers[0], publish, &box) == 0);
    for (int i = 1; i <= rounds; ++i) {
        while (!__ada_atomic_load_32(&box.full)) {
        }
        CHECK(box.payload == i);
        CHECK(__ada_atomic_load_8(&box.byte) == (uint8_t)i);
        CHECK(__ada_atomic_load_16(&box.half) == (uint16_t)~i);
        CHECK(__ada_atomic_load_32(&box.word) == UINT32_MAX - i);
        CHECK(__ada_atomic_load_64(&box.dword) == UINT64_MAX - i);
        __ada_atomic_store_32(&box.full, 0);
    }
    CHECK(__ada_thread_join(workers[0], NULL) == 0);
    return 0;
}
