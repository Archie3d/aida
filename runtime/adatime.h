#ifndef ADATIME_H
#define ADATIME_H

#include <stdint.h>

typedef struct AdaTaskContext AdaTaskContext;

/* Duration is a count of 2**(-30) seconds. Real_Time uses signed nanoseconds
   on the monotonic clock's unspecified epoch; no wall-clock synchronization. */
int64_t __ada_real_clock(void);
int64_t __ada_real_tick(void);
int64_t __ada_real_to_duration(int64_t span);
int64_t __ada_real_to_span(int64_t duration);
void __ada_real_split(int64_t value, int64_t* seconds, int64_t* span);
int64_t __ada_real_time_of(int64_t seconds, int64_t span);
void __ada_delay_relative(AdaTaskContext* context, int64_t duration);
void __ada_delay_until(AdaTaskContext* context, int64_t deadline);

#endif
