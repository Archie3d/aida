#include "adatime.h"
#include "adart.h"
#include "adathread.h"
#include "../common/Fixed.h"

#define NANOSECONDS INT64_C(1000000000)

int64_t __ada_real_clock(void)
{
    int64_t value = 0;
    if (__ada_monotonic_clock(&value) != 0) {
        __ada_raise(ADA_PROGRAM_ERROR);
    }
    return value;
}

int64_t __ada_real_tick(void)
{
    int64_t value = 0;
    if (__ada_monotonic_resolution(&value) != 0) {
        __ada_raise(ADA_PROGRAM_ERROR);
    }
    return value;
}

int64_t __ada_real_to_duration(int64_t span)
{
    long long value = 0;
    if (!fixedRound((__int128)span * ADA_DURATION_SCALE, NANOSECONDS, &value)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    }
    return value;
}

int64_t __ada_real_to_span(int64_t duration)
{
    long long value = 0;
    if (!fixedRound((__int128)duration * NANOSECONDS, ADA_DURATION_SCALE, &value)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    }
    return value;
}

void __ada_real_split(int64_t value, int64_t* seconds, int64_t* span)
{
    int64_t quotient = value / NANOSECONDS;
    int64_t remainder = value % NANOSECONDS;
    if (remainder < 0) {
        --quotient;
        remainder += NANOSECONDS;
    }
    *seconds = quotient;
    *span = remainder;
}

int64_t __ada_real_time_of(int64_t seconds, int64_t span)
{
    __int128 value = (__int128)seconds * NANOSECONDS + span;
    if (value < INT64_MIN || value > INT64_MAX) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return 0;
    }
    return (int64_t)value;
}

void __ada_delay_until(AdaTaskContext* context, int64_t deadline)
{
    // Even a zero/expired delay is a potentially blocking operation.
    if (context->protectedActions != NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    if (__ada_monotonic_delay_until(deadline) != 0) {
        __ada_raise(ADA_PROGRAM_ERROR);
    }
}

void __ada_delay_relative(AdaTaskContext* context, int64_t duration)
{
    if (context->protectedActions != NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    if (duration <= 0) {
        __ada_delay_until(context, 0);
        return;
    }
    int64_t now = __ada_real_clock();
    if (context->exception != NULL) {
        return;
    }
    // Round positive intervals upward so conversion cannot shorten a delay.
    __int128 nanoseconds = ((__int128)duration * NANOSECONDS + ADA_DURATION_SCALE - 1) / ADA_DURATION_SCALE;
    __int128 deadline = now + nanoseconds;
    if (deadline > INT64_MAX) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return;
    }
    __ada_delay_until(context, (int64_t)deadline);
}
