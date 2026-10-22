#include "../runtime/adart.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "scalar bounds check failed at line %d\n", __LINE__); abort(); \
} } while (0)

int main(void)
{
    CHECK(__ada_width_integer(0, 0, 0) == 2);
    CHECK(__ada_width_integer((uint64_t)INT64_MIN, (uint64_t)INT64_MAX, 0) == 20);
    CHECK(__ada_width_integer((uint64_t)-1000, (uint64_t)-9, 0) == 5);
    CHECK(__ada_width_integer(0, UINT64_MAX, 1) == 21);
    CHECK(__ada_width_integer(UINT64_MAX, 0, 1) == 0);
    CHECK(__ada_width_integer(1, 0, 0) == 0);
    const char* names[] = {"A", "LONGEST_NAME", "B"};
    CHECK(__ada_width_enum(0, 2, names, 3) == 12);
    CHECK(__ada_width_enum(2, 2, names, 3) == 1);
    CHECK(__ada_width_enum(2, 1, names, 3) == 0);
    CHECK(__ada_width_enum(-1, 2, names, 3) == 0);
    CHECK(__ada_exception == ADA_CONSTRAINT_ERROR);
    CHECK(__ada_task_context_dispose(&__ada_main_context));
    return 0;
}
