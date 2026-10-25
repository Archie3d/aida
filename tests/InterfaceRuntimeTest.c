#include "../runtime/adart.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "interface runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

static void implementation(void)
{
}

int main(void)
{
    AdaTaskContext* context = __ada_task_context();
    int master = 0;
    AdaTag reader = { .slotCount = 2, .name = "INTERFACE.READER", .interfaceKind = 1 };
    AdaTag meter = { .slotCount = 1, .name = "INTERFACE.METER", .interfaceKind = 1 };
    const int64_t kReaderMap[] = { 2, 0 };
    const int64_t kMeterMap[] = { 1 };
    AdaInterfaceBinding bindings[] = { { &reader, kReaderMap }, { &meter, kMeterMap } };
    AdaDispatchEntry slots[] = {
        { (void*)implementation, &master },
        { (void*)implementation, &reader },
        { (void*)implementation, &meter }
    };
    AdaTag prototype = { .name = "INTERFACE.CONCRETE", .slotCount = 3, .slots = slots, .size = 32, .alignment = 8 };
    AdaTag* tag = __ada_tag_create_full(&prototype, NULL, &master, 32, NULL, 2, bindings);
    CHECK(tag != NULL && context->exception == NULL);
    CHECK(tag->interfaces != bindings && tag->slots != slots);
    CHECK(__ada_tag_has_ancestor(tag, &reader) && __ada_tag_has_ancestor(tag, &meter));
    CHECK(__ada_tag_has_ancestor(tag, tag) && !__ada_tag_has_ancestor(tag, &prototype));
    CHECK(__ada_tag_dispatch(tag, &reader, 0) == &tag->slots[2]);
    CHECK(__ada_tag_dispatch(tag, &reader, 1) == &tag->slots[0]);
    CHECK(__ada_tag_dispatch(tag, &meter, 0) == &tag->slots[1]);
    CHECK(((AdaDispatchEntry*)__ada_tag_dispatch(tag, &reader, 0))->link == &meter);
    bindings[0].tag = NULL;
    CHECK(__ada_tag_interface_count(tag) == 2);
    CHECK(__ada_tag_interface_at(tag, 1) == &reader);
    CHECK(__ada_tag_dispatch(tag, &reader, 0) == &tag->slots[2]);
    CHECK(__ada_tag_dispatch(tag, &reader, 2) == NULL && context->exception == ADA_PROGRAM_ERROR);
    context->exception = NULL;
    CHECK(__ada_tag_dispatch(tag, &prototype, 0) == NULL && context->exception == ADA_PROGRAM_ERROR);
    context->exception = NULL;
    CHECK(__ada_tag_interface_at(tag, 0) == NULL && context->exception == ADA_CONSTRAINT_ERROR);
    context->exception = NULL;
    CHECK(__ada_tag_interface_count(NULL) == 0 && context->exception != NULL);
    CHECK(strcmp(context->exception->name, "ADA.TAGS.TAG_ERROR") == 0);
    context->exception = NULL;
    CHECK(__ada_tag_interface_count(&prototype) == 0);
    return 0;
}
