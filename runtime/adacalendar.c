#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#include "adart.h"
#include "adacalendar.h"
#include "adatime.h"
#include <time.h>

#define MICROSECONDS INT64_C(1000000)
#define DAY_SECONDS INT64_C(86400)
#define DURATION_SCALE (INT64_C(1) << 30)

/* Proleptic Gregorian days relative to 1970-01-01. */
static int64_t daysFromCivil(int year, int month, int day)
{
    year -= month <= 2;
    int era = (year >= 0 ? year : year - 399) / 400;
    unsigned yoe = (unsigned)(year - era * 400);
    unsigned doy = (153 * (month + (month > 2 ? -3 : 9)) + 2) / 5 + day - 1;
    return (int64_t)era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468;
}

static int validTime(__int128 value)
{
    return value >= (__int128)daysFromCivil(1901, 1, 1) * DAY_SECONDS * MICROSECONDS
        && value < (__int128)daysFromCivil(2400, 1, 1) * DAY_SECONDS * MICROSECONDS;
}

int64_t __ada_calendar_clock(void)
{
    struct timespec now;
    if (clock_gettime(CLOCK_REALTIME, &now) != 0) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return INT64_MIN;
    }
    __int128 value = (__int128)now.tv_sec * MICROSECONDS + now.tv_nsec / 1000;
    return validTime(value) ? (int64_t)value : INT64_MIN;
}

int64_t __ada_calendar_compose(int year, int month, int day, int64_t seconds)
{
    static const int kMonthDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (year < 1901 || year > 2399 || month < 1 || month > 12 || day < 1
        || seconds < 0 || seconds > DAY_SECONDS * DURATION_SCALE) {
        return INT64_MIN;
    }
    int leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    if (day > kMonthDays[month - 1] + (month == 2 && leap)) {
        return INT64_MIN;
    }
    long long micros;
    fixedRound((__int128)seconds * MICROSECONDS, DURATION_SCALE, &micros);
    __int128 value = (__int128)daysFromCivil(year, month, day) * DAY_SECONDS * MICROSECONDS + micros;
    return validTime(value) ? (int64_t)value : INT64_MIN;
}

void __ada_calendar_split(int64_t date, int* year, int* month, int* day, int64_t* seconds, int* valid)
{
    *valid = 0;
    if (!validTime(date)) {
        return;
    }
    int64_t days = date / (DAY_SECONDS * MICROSECONDS);
    int64_t rest = date % (DAY_SECONDS * MICROSECONDS);
    if (rest < 0) {
        --days;
        rest += DAY_SECONDS * MICROSECONDS;
    }
    int64_t z = days + 719468;
    int64_t era = (z >= 0 ? z : z - 146096) / 146097;
    unsigned doe = (unsigned)(z - era * 146097);
    unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int y = (int)yoe + (int)era * 400;
    unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    unsigned mp = (5 * doy + 2) / 153;
    *day = (int)(doy - (153 * mp + 2) / 5 + 1);
    *month = (int)mp + (mp < 10 ? 3 : -9);
    *year = y + (*month <= 2);
    long long ticks;
    fixedRound((__int128)rest * DURATION_SCALE, MICROSECONDS, &ticks);
    *seconds = ticks;
    *valid = 1;
}

int64_t __ada_calendar_add(int64_t date, int64_t span, int subtract)
{
    long long micros;
    fixedRound((__int128)span * MICROSECONDS, DURATION_SCALE, &micros);
    __int128 value = (__int128)date + (subtract ? -(__int128)micros : micros);
    return validTime(value) ? (int64_t)value : INT64_MIN;
}

void __ada_calendar_difference(int64_t left, int64_t right, int64_t* span, int* valid)
{
    long long ticks;
    *valid = fixedRound(((__int128)left - right) * DURATION_SCALE, MICROSECONDS, &ticks);
    if (*valid) {
        *span = ticks;
    }
}

void __ada_calendar_delay_until(AdaTaskContext* context, int64_t deadline)
{
    if (context->protectedActions != NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    /* Recheck realtime at least every 100 ms so wall-clock adjustments are
       observed. Relative sleeps retain the monotonic backend. */
    for (;;) {
        int64_t now = __ada_calendar_clock();
        if (context->exception != NULL || now == INT64_MIN || deadline <= now) {
            return;
        }
        int64_t micros = deadline - now;
        if (micros > 100000) {
            micros = 100000;
        }
        int64_t ticks = (int64_t)(((__int128)micros * DURATION_SCALE + MICROSECONDS - 1) / MICROSECONDS);
        __ada_delay_relative(context, ticks);
        if (context->exception != NULL) {
            return;
        }
    }
}
