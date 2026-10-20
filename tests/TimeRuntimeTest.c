#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#include "../runtime/adart.h"
#include "../runtime/adatime.h"
#include "../runtime/adathread.h"
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "time runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

#if !defined(_WIN32)
static volatile sig_atomic_t signalsReceived;

static void interruptDelay(int signalNumber)
{
    (void)signalNumber;
    ++signalsReceived;
}

static void* sendSignals(void* argument)
{
    pthread_t* target = argument;
    for (int i = 0; i < 5; ++i) {
        int64_t now;
        CHECK(__ada_monotonic_clock(&now) == 0);
        CHECK(__ada_monotonic_delay_until(now + INT64_C(3000000)) == 0);
        CHECK(pthread_kill(*target, SIGUSR1) == 0);
    }
    return NULL;
}

#endif

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    const int64_t scale = INT64_C(1) << 30;
    CHECK(__ada_real_to_duration(0) == 0);
    CHECK(__ada_real_to_duration(1000000000) == scale);
    CHECK(__ada_real_to_duration(-1000000000) == -scale);
    CHECK(__ada_real_to_duration(1953125) == 2097152);
    CHECK(__ada_real_to_span(scale) == 1000000000);
    CHECK(__ada_real_to_span(1048576) == 976563);
    CHECK(__ada_real_to_span(-1048576) == -976563);
    CHECK(__ada_real_to_span(INT64_MIN) == -INT64_C(8589934592000000000));
    __ada_real_to_duration(INT64_MAX);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR);
    context.exception = NULL;
    int64_t seconds, span;
    __ada_real_split(INT64_MIN, &seconds, &span);
    CHECK(seconds == -INT64_C(9223372037) && span == 145224192);
    CHECK(__ada_real_time_of(seconds, span) == INT64_MIN);
    __ada_real_split(INT64_MAX, &seconds, &span);
    CHECK(seconds == INT64_C(9223372036) && span == 854775807);
    CHECK(__ada_real_time_of(seconds, span) == INT64_MAX);
    CHECK(__ada_real_time_of(INT64_C(9223372037), -145224193) == INT64_MAX);
    __ada_real_time_of(INT64_MAX, INT64_MIN);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR);
    context.exception = NULL;
    CHECK(__ada_real_tick() >= 1);
    int64_t before = __ada_real_clock();
    __ada_delay_relative(&context, scale / 500);
    CHECK(context.exception == NULL && __ada_real_clock() - before >= 2000000);
    __ada_delay_relative(&context, -scale);
    __ada_delay_relative(&context, 0);
    __ada_delay_until(&context, INT64_MIN);
    CHECK(context.exception == NULL);

#if !defined(_WIN32)
    struct sigaction handler = {0};
    struct sigaction saved;
    handler.sa_handler = interruptDelay;
    sigemptyset(&handler.sa_mask);
    CHECK(sigaction(SIGUSR1, &handler, &saved) == 0);
    pthread_t target = pthread_self();
    AdaThread sender;
    CHECK(__ada_thread_create(&sender, sendSignals, &target) == 0);
    int64_t deadline = __ada_real_clock() + INT64_C(40000000);
    __ada_delay_until(&context, deadline);
    CHECK(context.exception == NULL && __ada_real_clock() >= deadline);
    CHECK(__ada_thread_join(sender, NULL) == 0 && signalsReceived > 0);
    CHECK(sigaction(SIGUSR1, &saved, NULL) == 0);
#else
    int64_t deadline = __ada_real_clock() + INT64_C(4000000);
    __ada_delay_until(&context, deadline);
    CHECK(context.exception == NULL && __ada_real_clock() >= deadline);
#endif

    void* object = NULL;
    __ada_protected_init(&object);
    AdaProtectedAction action;
    __ada_protected_enter(&context, &object, &action);
    __ada_delay_relative(&context, 0);
    CHECK(context.exception == ADA_PROGRAM_ERROR && context.protectedActions == &action);
    context.exception = NULL;
    __ada_delay_until(&context, INT64_MIN);
    CHECK(context.exception == ADA_PROGRAM_ERROR && context.protectedActions == &action);
    __ada_protected_leave(&context, &action);
    context.exception = NULL;
    __ada_protected_destroy(&object);
    CHECK(context.exception == NULL && __ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
