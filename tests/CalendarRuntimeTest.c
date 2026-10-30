#include "../runtime/adart.h"
#include "../runtime/adacalendar.h"
#include "../runtime/adatime.h"
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "calendar check failed at line %d\n", __LINE__); abort(); \
} } while (0)

static void civilRange(void)
{
    int weekday = 1; /* 1901-01-01 was Tuesday. */
    int64_t previous = 0;
    int dayCount = 0;
    const int kMonthDays[] = {31,28,31,30,31,30,31,31,30,31,30,31};
    for (int year = 1901; year <= 2399; ++year) {
        int leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
        for (int month = 1; month <= 12; ++month) {
            int count = kMonthDays[month - 1] + (month == 2 && leap);
            for (int day = 1; day <= count; ++day) {
                int64_t date = __ada_calendar_compose(year, month, day, ADA_DURATION_SCALE / 4);
                CHECK(date != INT64_MIN);
                int y, m, d, valid;
                int64_t seconds;
                __ada_calendar_split(date, &y, &m, &d, &seconds, &valid);
                CHECK(valid && y == year && m == month && d == day && seconds == ADA_DURATION_SCALE / 4);
                CHECK(__ada_calendar_weekday(date) == weekday);
                if (dayCount != 0) { CHECK(__ada_calendar_add_days(previous, 1, 0) == date); }
                weekday = (weekday + 1) % 7;
                previous = date;
                ++dayCount;
            }
            CHECK(__ada_calendar_compose(year, month, count + 1, 0) == INT64_MIN);
        }
    }
    CHECK(dayCount == 182256);
}

static void boundsAndOffsets(void)
{
    int64_t first = __ada_calendar_compose(1901, 1, 1, 0);
    int64_t last = __ada_calendar_compose(2399, 12, 31, 86399 * ADA_DURATION_SCALE);
    int days, valid;
    int64_t seconds;
    __ada_calendar_day_difference(last, first, &days, &seconds, &valid);
    CHECK(valid && days == 182255 && seconds == 86399 * ADA_DURATION_SCALE);
    __ada_calendar_day_difference(first, last, &days, &seconds, &valid);
    CHECK(valid && days == -182255 && seconds == -86399 * ADA_DURATION_SCALE);
    CHECK(__ada_calendar_add_days(first, INT64_MAX, 0) == INT64_MIN);
    CHECK(__ada_calendar_add_days(last, INT64_MIN, 1) == INT64_MIN);
    CHECK(__ada_calendar_compose_zone(1901, 1, 1, 0, 1) == INT64_MIN);
    CHECK(__ada_calendar_compose_zone(2399, 12, 31, 86400 * ADA_DURATION_SCALE, 60) != INT64_MIN);
    for (int offset = -1680; offset <= 1680; ++offset) {
        int64_t date = __ada_calendar_compose_zone(2000, 2, 29, 45296 * ADA_DURATION_SCALE, offset);
        int y, m, d;
        __ada_calendar_split_zone(date, offset, &y, &m, &d, &seconds, &valid);
        CHECK(valid && y == 2000 && m == 2 && d == 29 && seconds == 45296 * ADA_DURATION_SCALE);
    }
    int y, m, d;
    __ada_calendar_split_zone(first, -1, &y, &m, &d, &seconds, &valid);
    CHECK(!valid);
    CHECK(__ada_calendar_compose_zone(2000, 1, 1, 0, 1681) == INT64_MIN);
    int hours, minutes, second, hundredths, negative;
    __ada_calendar_elapsed_parts(INT64_MIN, &hours, &minutes, &second, &hundredths, &negative);
    CHECK(hours == 2386092 && minutes == 56 && second == 32 && hundredths == 0 && negative);
    __ada_calendar_elapsed_parts(INT64_MAX, &hours, &minutes, &second, &hundredths, &negative);
    CHECK(hours == 2386092 && minutes == 56 && second == 31 && hundredths == 99 && !negative);
    CHECK(__ada_calendar_centiseconds(-1) == 99);
    CHECK(__ada_calendar_centiseconds(10000) == 1);
}

static void* worker(void* unused)
{
    (void)unused;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int i = 0; i < 1000; ++i) {
        boundsAndOffsets();
    }
    CHECK(context.exception == NULL && __ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    civilRange();
    boundsAndOffsets();
    pthread_t threads[4];
    for (int i = 0; i < 4; ++i) { CHECK(pthread_create(&threads[i], NULL, worker, NULL) == 0); }
    for (int i = 0; i < 4; ++i) { CHECK(pthread_join(threads[i], NULL) == 0); }
    return 0;
}
