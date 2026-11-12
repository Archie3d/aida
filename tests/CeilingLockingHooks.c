#include "../runtime/adart.h"

int __ada_test_active_priority(void)
{
    int base = __ada_get_priority(__ada_current_task());
    int inherited = atomic_load(&__ada_task_context()->inheritedPriority) - 1;
    return base > inherited ? base : inherited;
}
