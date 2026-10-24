#include "../runtime/adart.h"

#include <stdio.h>
#include <stdlib.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "access-level check failed at line %d\n", __LINE__); abort(); \
} } while (0)

int main(void)
{
    AdaTaskContext* context = __ada_task_context();
    AdaMaster outer = {0}, block = {0}, callee = {0};
    __ada_master_enter(context, &outer, ADA_MASTER_SUBPROGRAM);
    int outerLevel = __ada_current_access_level();
    __ada_master_enter(context, &block, ADA_MASTER_BLOCK);
    int blockLevel = __ada_current_access_level();
    __ada_master_enter(context, &callee, ADA_MASTER_SUBPROGRAM);
    int calleeLevel = __ada_current_access_level();
    CHECK(outerLevel != 0 && blockLevel != outerLevel && calleeLevel != blockLevel);
    __ada_accessibility_check(blockLevel, calleeLevel);
    CHECK(context->exception == NULL);
    __ada_accessibility_check(calleeLevel, blockLevel);
    CHECK(context->exception == ADA_PROGRAM_ERROR);
    context->exception = NULL;
    __ada_accessibility_check(0, calleeLevel);
    CHECK(context->exception == NULL);
    __ada_access_return_check(calleeLevel, outerLevel, &callee, &callee);
    CHECK(context->exception == NULL);
    __ada_access_return_check(calleeLevel, outerLevel, &callee, &outer);
    CHECK(context->exception == ADA_PROGRAM_ERROR);
    context->exception = NULL;
    __ada_master_leave(context, &callee);
    __ada_master_leave(context, &block);
    __ada_master_enter(context, &block, ADA_MASTER_BLOCK);
    int reusedLevel = __ada_current_access_level();
    CHECK(reusedLevel != blockLevel);
    __ada_accessibility_check(blockLevel, reusedLevel);
    CHECK(context->exception == ADA_PROGRAM_ERROR);
    context->exception = NULL;
    __ada_master_leave(context, &block);

    /* Sibling task activations are incomparable even when their depths match. */
    AdaTaskContext first = {0}, second = {0};
    AdaMaster firstMaster = {0}, secondMaster = {0};
    first.currentMaster = &outer;
    second.currentMaster = &outer;
    AdaTaskContext* previous = __ada_task_context_bind(&first);
    __ada_master_enter(&first, &firstMaster, ADA_MASTER_SUBPROGRAM);
    int firstLevel = __ada_current_access_level();
    __ada_task_context_bind(&second);
    __ada_master_enter(&second, &secondMaster, ADA_MASTER_SUBPROGRAM);
    int secondLevel = __ada_current_access_level();
    __ada_accessibility_check(firstLevel, secondLevel);
    CHECK(second.exception == ADA_PROGRAM_ERROR);
    second.exception = NULL;
    __ada_accessibility_check(outerLevel, secondLevel);
    CHECK(second.exception == NULL);
    __ada_master_leave(&second, &secondMaster);
    second.currentMaster = NULL;
    CHECK(__ada_task_context_dispose(&second));
    __ada_task_context_bind(&first);
    __ada_master_leave(&first, &firstMaster);
    first.currentMaster = NULL;
    CHECK(__ada_task_context_dispose(&first));
    __ada_task_context_bind(previous);
    __ada_master_leave(context, &outer);
    CHECK(context->currentMaster == NULL && context->sharedAccessLevels == NULL);
    return 0;
}
