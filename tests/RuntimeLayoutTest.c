#include "../runtime/adart.h"
#include "../common/Array.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "runtime layout check failed at line %d\n", __LINE__); abort(); \
} } while (0)

static void checkLayout(void)
{
    AdaTaskContext* context = __ada_task_context();
    CHECK(__ada_layout_append(3, 5, 8) == 13);
    CHECK(__ada_layout_append(0, INT64_MAX, 1) == INT64_MAX);
    CHECK(__ada_layout_append(INT64_MAX - 7, 0, 8) == INT64_MAX - 7);
    CHECK(context->exception == NULL);
    CHECK(__ada_layout_append(INT64_MAX, 0, 8) == 0);
    CHECK(context->exception == ADA_STORAGE_ERROR);
    context->exception = NULL;
    CHECK(__ada_layout_append(INT64_MAX - 3, 4, 1) == 0);
    CHECK(context->exception == ADA_STORAGE_ERROR);
    context->exception = NULL;
    CHECK(__ada_layout_append(-1, 0, 1) == 0);
    CHECK(context->exception == ADA_STORAGE_ERROR);
    context->exception = NULL;
    CHECK(__ada_layout_append(0, 0, 0) == 0);
    CHECK(context->exception == ADA_STORAGE_ERROR);
    context->exception = NULL;
}

int main(void)
{
    AdaTaskContext* context = __ada_task_context();
    checkLayout();
    void* collection = __ada_collection_create(NULL, NULL);
    CHECK(collection != NULL && context->exception == NULL);
    struct
    {
        void* data;
        AdaArrayDimension axes[2];
    } descriptor = {NULL, {{-5, -3, 3, 8}, {7, 8, 2, 4}}};
    void* object = __ada_collection_allocate(collection, 24);
    CHECK(object != NULL);
    __ada_allocation_set_array_bounds(object, &descriptor, 2, "matrix");
    CHECK(context->exception == NULL);
    void* saved = __ada_allocation_array_bounds(object, 2);
    CHECK(saved != NULL && saved != &descriptor);
    CHECK(memcmp(saved, &descriptor, sizeof descriptor) == 0);
    descriptor.axes[0].first = 100;
    CHECK(((AdaArrayDimension*)((char*)saved + 8))[0].first == -5);
    CHECK(__ada_allocation_array_bounds(object, 1) == NULL);
    CHECK(context->exception == ADA_PROGRAM_ERROR);
    context->exception = NULL;
    /* An aliased subobject may share an allocation's base address. Its view
       must not replace the owning array's descriptor, even at the same rank. */
    descriptor.axes[0].first = -100;
    __ada_array_borrow(collection, object, &descriptor, 2, "subobject");
    CHECK(((AdaArrayDimension*)((char*)__ada_access_array_bounds(object, 2, "matrix") + 8))[0].first == -5);
    CHECK(((AdaArrayDimension*)((char*)__ada_access_array_bounds(object, 2, "subobject") + 8))[0].first == -100);
    __ada_deallocate(object);
    CHECK(context->exception == NULL);
    __ada_library_finalize();
    CHECK(context->exception == NULL);
    return 0;
}
