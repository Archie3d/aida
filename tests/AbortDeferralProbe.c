#include "../runtime/adart.h"

int __test_abort_depth(void)
{
    return (int)__ada_task_context()->abortDeferralDepth;
}
