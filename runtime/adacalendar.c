#if !defined(_WIN32)
#define _POSIX_C_SOURCE 200809L
#endif
#include "adart.h"
#include "adacalendar.h"
#include "adatime.h"
#include <time.h>

#define MICROSECONDS INT64_C(1000000)
#define DAY_SECONDS INT64_C(86400)

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

int64_t __ada_calendar_compose_zone(int year, int month, int day, int64_t seconds, int offset)
{
    static const int kMonthDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (year < 1901 || year > 2399 || month < 1 || month > 12 || day < 1
        || seconds < 0 || seconds > DAY_SECONDS * ADA_DURATION_SCALE || offset < -1680 || offset > 1680) {
        return INT64_MIN;
    }
    int leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
    if (day > kMonthDays[month - 1] + (month == 2 && leap)) {
        return INT64_MIN;
    }
    long long micros;
    fixedRound((__int128)seconds * MICROSECONDS, ADA_DURATION_SCALE, &micros);
    __int128 value = (__int128)daysFromCivil(year, month, day) * DAY_SECONDS * MICROSECONDS + micros
        - (__int128)offset * 60 * MICROSECONDS;
    return validTime(value) ? (int64_t)value : INT64_MIN;
}

int64_t __ada_calendar_compose(int year, int month, int day, int64_t seconds)
{
    return __ada_calendar_compose_zone(year, month, day, seconds, 0);
}

void __ada_calendar_split_zone(int64_t date, int offset, int* year, int* month, int* day, int64_t* seconds, int* valid)
{
    *valid = 0;
    if (!validTime(date) || offset < -1680 || offset > 1680) { return; }
    date += (int64_t)offset * 60 * MICROSECONDS;
    if (!validTime(date)) { return; }
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
    fixedRound((__int128)rest * ADA_DURATION_SCALE, MICROSECONDS, &ticks);
    *seconds = ticks;
    *valid = 1;
}

void __ada_calendar_split(int64_t date, int* year, int* month, int* day, int64_t* seconds, int* valid)
{
    __ada_calendar_split_zone(date, 0, year, month, day, seconds, valid);
}

int64_t __ada_calendar_add(int64_t date, int64_t span, int subtract)
{
    if (!validTime(date)) { return INT64_MIN; }
    long long micros;
    fixedRound((__int128)span * MICROSECONDS, ADA_DURATION_SCALE, &micros);
    __int128 value = (__int128)date + (subtract ? -(__int128)micros : micros);
    return validTime(value) ? (int64_t)value : INT64_MIN;
}

void __ada_calendar_difference(int64_t left, int64_t right, int64_t* span, int* valid)
{
    long long ticks;
    *valid = validTime(left) && validTime(right)
        && fixedRound(((__int128)left - right) * ADA_DURATION_SCALE, MICROSECONDS, &ticks);
    if (*valid) {
        *span = ticks;
    }
}

/* Calendar day arithmetic does not pass the entire interval through Duration:
   the calendar's full range is wider than Duration can represent. */
int64_t __ada_calendar_add_days(int64_t date, int64_t days, int subtract)
{
    if (!validTime(date)) { return INT64_MIN; }
    __int128 delta = (__int128)days * DAY_SECONDS * MICROSECONDS;
    __int128 result = (__int128)date + (subtract ? -delta : delta);
    return validTime(result) ? (int64_t)result : INT64_MIN;
}

void __ada_calendar_day_difference(int64_t left, int64_t right, int* days, int64_t* seconds, int* valid)
{
    *valid = 0;
    if (!validTime(left) || !validTime(right)) { return; }
    int64_t delta = left - right;
    int64_t daySize = DAY_SECONDS * MICROSECONDS;
    *days = (int)(delta / daySize);
    long long ticks;
    *valid = fixedRound((__int128)(delta % daySize) * ADA_DURATION_SCALE, MICROSECONDS, &ticks);
    if (*valid) { *seconds = ticks; }
}

int __ada_calendar_weekday(int64_t date)
{
    if (!validTime(date)) { return -1; }
    int64_t daySize = DAY_SECONDS * MICROSECONDS;
    int64_t days = date / daySize - (date % daySize < 0);
    return (int)(((days + 3) % 7 + 7) % 7);
}

int __ada_calendar_centiseconds(int64_t date)
{
    int64_t fraction = date % MICROSECONDS;
    if (fraction < 0) { fraction += MICROSECONDS; }
    return (int)(fraction / (MICROSECONDS / 100));
}

/* A negative Duration's magnitude can exceed Duration'Last by one tick.
   Extract its printable parts without negating it in signed 64-bit storage. */
void __ada_calendar_elapsed_parts(int64_t span, int* hours, int* minutes, int* seconds, int* hundredths, int* negative)
{
    __int128 magnitude = span < 0 ? -(__int128)span : span;
    int64_t whole = (int64_t)(magnitude / ADA_DURATION_SCALE);
    *hours = (int)(whole / 3600);
    *minutes = (int)(whole / 60 % 60);
    *seconds = (int)(whole % 60);
    *hundredths = (int)(magnitude % ADA_DURATION_SCALE * 100 / ADA_DURATION_SCALE);
    *negative = span < 0;
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
        int64_t ticks = (int64_t)(((__int128)micros * ADA_DURATION_SCALE + MICROSECONDS - 1) / MICROSECONDS);
        __ada_delay_relative(context, ticks);
        if (context->exception != NULL) {
            return;
        }
    }
}
