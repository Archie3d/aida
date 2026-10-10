#define _POSIX_C_SOURCE 200809L
#include "../../runtime/adart.h"
#include "../../runtime/adaio.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

static double seconds(void)
{
    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    return now.tv_sec + now.tv_nsec * 1e-9;
}

int main(void)
{
    AdaFile* file = NULL;
    __ada_file_open(&file, ADA_MODE_INOUT, "", 0, 1);
    if (__ada_exception != NULL) {
        return 1;
    }
    double start = seconds();
    for (int i = 0; i < 20000; ++i) {
        __ada_text_put_line(&file, "benchmark", 9);
        __ada_file_reset_same(&file);
        char text[32];
        int last;
        __ada_text_get_line(&file, text, sizeof text, &last);
        if (last != 9 || memcmp(text, "benchmark", 9) != 0) {
            return 1;
        }
        __ada_file_reset_same(&file);
    }
    double ioTime = seconds() - start;
    __ada_file_close(&file);
    static AdaTag tag = { .name = "BENCHMARK" };
    __ada_tag_register(&tag);
    start = seconds();
    for (int i = 0; i < 1000000; ++i) {
        __ada_tag_register(&tag);
        if (__ada_tag_internal("BENCHMARK", 9) != &tag) {
            return 1;
        }
    }
    printf("%.6f %.6f\n", ioTime, seconds() - start);
    return __ada_exception != NULL;
}
