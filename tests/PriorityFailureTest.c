#include "../runtime/adart.h"
#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "priority failure check failed at line %d\n", __LINE__); abort(); \
} } while (0)

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    __ada_environment_task_state(0, 0);
    __ada_set_priority(kAdaDefaultPriority + 1, __ada_current_task());
    CHECK(context.exception == ADA_TASKING_ERROR);
    CHECK(__ada_get_priority(__ada_current_task()) == kAdaDefaultPriority);
    context.exception = NULL;
    CHECK(__ada_priority_inherit(kAdaAnyPriorityLast) == -1);
    CHECK(context.exception == ADA_TASKING_ERROR && atomic_load(&context.inheritedPriority) == 0);
    context.exception = NULL;
    AdaConcurrentObject object = {0};
    __ada_protected_init(&object);
    __ada_protected_set_ceiling(&object, kAdaPriorityLast);
    AdaProtectedAction action;
    __ada_protected_enter(&context, &object, &action);
    CHECK(context.exception == ADA_TASKING_ERROR && context.protectedActions == NULL);
    CHECK(context.abortDeferralDepth == 0 && atomic_load(&context.inheritedPriority) == 0);
    context.exception = NULL;
    __ada_protected_destroy(&object);
    CHECK(context.exception == NULL && __ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
