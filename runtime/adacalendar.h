#ifndef ADACALENDAR_H
#define ADACALENDAR_H
#include <stdint.h>
struct AdaTaskContext;
int64_t __ada_calendar_clock(void);
int64_t __ada_calendar_compose(int year, int month, int day, int64_t seconds);
void __ada_calendar_split(int64_t date, int* year, int* month, int* day, int64_t* seconds, int* valid);
int64_t __ada_calendar_add(int64_t date, int64_t span, int subtract);
void __ada_calendar_difference(int64_t left, int64_t right, int64_t* span, int* valid);
int64_t __ada_calendar_compose_zone(int year, int month, int day, int64_t seconds, int offset);
void __ada_calendar_split_zone(int64_t date, int offset, int* year, int* month, int* day, int64_t* seconds, int* valid);
int64_t __ada_calendar_add_days(int64_t date, int64_t days, int subtract);
void __ada_calendar_day_difference(int64_t left, int64_t right, int* days, int64_t* seconds, int* valid);
int __ada_calendar_weekday(int64_t date);
int __ada_calendar_centiseconds(int64_t date);
void __ada_calendar_elapsed_parts(int64_t span, int* hours, int* minutes, int* seconds, int* hundredths, int* negative);
void __ada_calendar_delay_until(struct AdaTaskContext* context, int64_t deadline);
#endif
