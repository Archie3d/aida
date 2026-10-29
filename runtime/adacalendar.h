#ifndef ADACALENDAR_H
#define ADACALENDAR_H
#include <stdint.h>
struct AdaTaskContext;
int64_t __ada_calendar_clock(void);
int64_t __ada_calendar_compose(int year, int month, int day, int64_t seconds);
void __ada_calendar_split(int64_t date, int* year, int* month, int* day, int64_t* seconds, int* valid);
int64_t __ada_calendar_add(int64_t date, int64_t span, int subtract);
void __ada_calendar_difference(int64_t left, int64_t right, int64_t* span, int* valid);
void __ada_calendar_delay_until(struct AdaTaskContext* context, int64_t deadline);
#endif
