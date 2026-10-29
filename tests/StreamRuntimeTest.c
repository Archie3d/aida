#include "../runtime/adart.h"
#include "../runtime/adaio.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "stream runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

static const AdaException kFailure = {"STREAM_TEST_FAILURE"};

typedef struct MemoryStream
{
    AdaTag* tag;
    unsigned char bytes[256];
    int used;
    int position;
    int invalidLast;
    int fail;
} MemoryStream;

static void readMemory(void* object, void* item, int64_t first, int64_t last,
    int64_t length, int64_t stride, int* final)
{
    MemoryStream* stream = object;
    CHECK(first == 1 && last == length && stride == 1);
    if (stream->fail) { __ada_raise(&kFailure); return; }
    int count = stream->used - stream->position;
    if (count > length) { count = (int)length; }
    memcpy(item, stream->bytes + stream->position, (size_t)count);
    stream->position += count;
    *final = stream->invalidLast ? (int)length + 1 : count;
}

static void writeMemory(void* object, const void* item, int64_t first, int64_t last,
    int64_t length, int64_t stride)
{
    MemoryStream* stream = object;
    CHECK(first == 1 && last == length && stride == 1 && length <= 256 - stream->used);
    if (stream->fail) { __ada_raise(&kFailure); return; }
    memcpy(stream->bytes + stream->used, item, (size_t)length);
    stream->used += (int)length;
}

static void readNested(void* link, void* object, void* item, int64_t first, int64_t last,
    int64_t length, int64_t stride, int* final)
{
    ++*(int*)link;
    readMemory(object, item, first, last, length, stride, final);
}

static void writeNested(void* link, void* object, const void* item, int64_t first, int64_t last,
    int64_t length, int64_t stride)
{
    ++*(int*)link;
    writeMemory(object, item, first, last, length, stride);
}

static void* exercise(void* argument)
{
    AdaTag* tag = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    MemoryStream stream = {.tag = tag};
    for (int i = 0; i < 100; ++i) {
        int value = -1234567, back = 0;
        stream.used = stream.position = 0;
        __ada_stream_value_write(&stream, 1, &value, sizeof value);
        __ada_stream_value_read(&stream, 0, &back, sizeof back);
        CHECK(context.exception == NULL && value == back && stream.position == 4);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaDispatchEntry slots[2] = {{readMemory, NULL}, {writeMemory, NULL}};
    AdaTag tag = {.slots = slots, .slotCount = 2, .name = "STREAM_TEST"};
    MemoryStream stream = {.tag = &tag};
    int value = 42, back = 0;
    __ada_stream_value_read(NULL, 0, &back, 4);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR);
    context.exception = NULL;
    __ada_stream_value_write(&stream, -1, &value, 4);
    CHECK(context.exception == ADA_PROGRAM_ERROR);
    context.exception = NULL;
    __ada_stream_value_read(&stream, 0, &back, -1);
    CHECK(context.exception == ADA_STORAGE_ERROR);
    context.exception = NULL;
    __ada_stream_value_read(&stream, 0, &back, 4);
    CHECK(context.exception == ADA_END_ERROR);
    context.exception = NULL;
    stream.invalidLast = 1;
    __ada_stream_value_read(&stream, 0, &back, 4);
    CHECK(context.exception == ADA_PROGRAM_ERROR);
    context.exception = NULL;
    stream.invalidLast = 0;
    stream.fail = 1;
    __ada_stream_value_read(&stream, 0, &back, 4);
    CHECK(context.exception == &kFailure);
    context.exception = NULL;
    __ada_stream_value_write(&stream, 1, &value, 4);
    CHECK(context.exception == &kFailure);
    context.exception = NULL;
    stream.fail = 0;
    int calls = 0;
    AdaDispatchEntry nested[2] = {{readNested, &calls}, {writeNested, &calls}};
    AdaTag nestedTag = {.slots = nested, .slotCount = 2};
    stream.tag = &nestedTag;
    __ada_stream_value_write(&stream, 1, &value, 4);
    __ada_stream_value_read(&stream, 0, &back, 4);
    CHECK(context.exception == NULL && back == value && calls == 2);
    stream.tag = &tag;
    stream.used = stream.position = 0;
    AdaTag child = {.parent = &tag, .name = "STREAM_TEST.CHILD"};
    __ada_tag_register(&tag);
    __ada_tag_register(&child);
    __ada_stream_external_tag(&stream, 1, &child, &tag);
    CHECK(context.exception == NULL && __ada_stream_input_tag(&stream, 0, &tag) == &child);
    child.level = 1;
    __ada_stream_external_tag(&stream, 1, &child, &tag);
    CHECK(context.exception == &__ada_exc_tag_error);
    context.exception = NULL;
    AdaThread threads[4];
    for (int i = 0; i < 4; ++i) { CHECK(__ada_thread_create(&threads[i], exercise, &tag) == 0); }
    for (int i = 0; i < 4; ++i) { CHECK(__ada_thread_join(threads[i], NULL) == 0); }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
