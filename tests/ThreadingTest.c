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
    AdaMutex m_mutex;
    AdaMutex m_probe;
    AdaCondition m_ready;
    AdaCondition m_release;
    int m_waiting;
    int m_open;
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
    CHECK(__ada_mutex_trylock(&gate->m_probe) == EBUSY);
    __ada_mutex_lock(&gate->m_mutex);
    ++gate->m_waiting;
    CHECK(__ada_condition_signal(&gate->m_ready) == 0);
    while (!gate->m_open) {
        CHECK(__ada_condition_wait(&gate->m_release, &gate->m_mutex) == 0);
    }
    __ada_mutex_unlock(&gate->m_mutex);
    return argument;
}

/* No mutex participates in publication: the atomic flag must order ordinary
   payload accesses as well as preserve all scalar widths/bit patterns. */
typedef struct Mailbox
{
    uint32_t m_full;
    uint8_t m_byte;
    uint16_t m_half;
    uint32_t m_word;
    uint64_t m_long;
    int m_payload;
} Mailbox;

static void* publish(void* argument)
{
    Mailbox* box = argument;
    for (int i = 1; i <= rounds; ++i) {
        while (__ada_atomic_load_32(&box->m_full)) {
        }
        box->m_payload = i;
        __ada_atomic_store_8(&box->m_byte, (uint8_t)i);
        __ada_atomic_store_16(&box->m_half, (uint16_t)~i);
        __ada_atomic_store_32(&box->m_word, UINT32_MAX - i);
        __ada_atomic_store_64(&box->m_long, UINT64_MAX - i);
        __ada_atomic_store_32(&box->m_full, 1);
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
    CHECK(__ada_mutex_init(&gate.m_mutex) == 0);
    CHECK(__ada_mutex_init(&gate.m_probe) == 0);
    __ada_mutex_lock(&gate.m_probe);
    CHECK(__ada_condition_init(&gate.m_ready) == 0);
    CHECK(__ada_condition_init(&gate.m_release) == 0);
    CHECK(__ada_mutex_trylock(&gate.m_mutex) == 0);
    /* The probe mutex remains locked until all workers have joined. */
    for (int i = 0; i < workerCount; ++i) {
        CHECK(__ada_thread_create(&workers[i], waitForGate, &gate) == 0);
    }
    int64_t deadline = now() + INT64_C(5000000000);
    while (gate.m_waiting != workerCount) {
        CHECK(__ada_condition_wait_until(&gate.m_ready, &gate.m_mutex, deadline) == 0);
    }
    gate.m_open = 1;
    CHECK(__ada_condition_broadcast(&gate.m_release) == 0);
    __ada_mutex_unlock(&gate.m_mutex);
    for (int i = 0; i < workerCount; ++i) {
        void* result = NULL;
        CHECK(__ada_thread_join(workers[i], &result) == 0 && result == &gate);
    }

    __ada_mutex_lock(&gate.m_mutex);
    deadline = now() + INT64_C(20000000);
    int status;
    do {
        status = __ada_condition_wait_until(&gate.m_ready, &gate.m_mutex, deadline);
    } while (status == 0);
    CHECK(status == ETIMEDOUT && now() >= deadline);
    CHECK(__ada_condition_wait_until(&gate.m_ready, &gate.m_mutex, -1) == EINVAL);
    do {
        status = __ada_condition_wait_until(&gate.m_ready, &gate.m_mutex, 0);
    } while (status == 0);
    CHECK(status == ETIMEDOUT);
    CHECK(__ada_mutex_trylock(&gate.m_mutex) == EBUSY);
    __ada_mutex_unlock(&gate.m_mutex);
    CHECK(__ada_condition_destroy(&gate.m_release) == 0);
    CHECK(__ada_condition_destroy(&gate.m_ready) == 0);
    CHECK(__ada_mutex_destroy(&gate.m_mutex) == 0);
    __ada_mutex_unlock(&gate.m_probe);
    CHECK(__ada_mutex_destroy(&gate.m_probe) == 0);

    Mailbox box = { 0 };
    CHECK(__ada_thread_create(&workers[0], publish, &box) == 0);
    for (int i = 1; i <= rounds; ++i) {
        while (!__ada_atomic_load_32(&box.m_full)) {
        }
        CHECK(box.m_payload == i);
        CHECK(__ada_atomic_load_8(&box.m_byte) == (uint8_t)i);
        CHECK(__ada_atomic_load_16(&box.m_half) == (uint16_t)~i);
        CHECK(__ada_atomic_load_32(&box.m_word) == UINT32_MAX - i);
        CHECK(__ada_atomic_load_64(&box.m_long) == UINT64_MAX - i);
        __ada_atomic_store_32(&box.m_full, 0);
    }
    CHECK(__ada_thread_join(workers[0], NULL) == 0);
    return 0;
}
