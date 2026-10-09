#define _POSIX_C_SOURCE 200809L
#include "../../runtime/adart.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

extern AdaTaskContext* hiddenContextEntry(AdaTaskContext*, AdaTraceFrame*, const char*, const char*);

static double seconds(void)
{
    struct timespec time;
    clock_gettime(CLOCK_MONOTONIC, &time);
    return time.tv_sec + time.tv_nsec * 1e-9;
}

int main(int argc, char** argv)
{
    int mode = argc == 2 ? atoi(argv[1]) : -1;
    AdaTaskContext* context = __ada_task_context();
    double start = seconds();
    for (int i = 0; i < 10000000; ++i) {
        AdaTraceFrame frame;
        AdaTaskContext* current;
        switch (mode) {
        case 0: /* Separate runtime accessor and frame-entry calls. */
            current = __ada_task_context();
            hiddenContextEntry(current, &frame, "routine", "location");
            break;
        case 1: /* Context acquisition folded into existing frame entry. */
            current = __ada_trace_enter(&frame, "routine", "location");
            break;
        case 2: /* Hidden context parameter already supplied by the caller. */
            current = hiddenContextEntry(context, &frame, "routine", "location");
            break;
        default:
            fprintf(stderr, "usage: context_access_benchmark 0|1|2\n");
            return 1;
        }
        frame.location = "next";
        __ada_trace_leave_context(current, &frame);
    }
    printf("%.6f\n", seconds() - start);
    return context->m_currentTrace != NULL;
}
