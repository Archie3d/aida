#include "adart.h"
#include "adaio.h"
#include <limits.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(AdaFileStream) == 16 && offsetof(AdaFileStream, file) == 8,
    "Ada.Streams.Stream_IO.File_Stream ABI");

static AdaDispatchEntry* streamOperation(void* stream, int slot)
{
    if (stream == NULL) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return NULL;
    }
    AdaTag* tag = *(AdaTag**)stream;
    if (tag == NULL || slot < 0 || slot >= tag->slotCount || tag->slots[slot].code == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return NULL;
    }
    return &tag->slots[slot];
}

/* Ada unconstrained arrays pass data plus four signed 64-bit descriptor words.
   A nested primitive additionally receives its static link before the stream.
   These calls hold no I/O/table lock: user streams may recurse, allocate,
   finalize, block, or raise through the ordinary per-task exception context. */
void __ada_stream_value_read(void* stream, int slot, void* item, int64_t size)
{
    AdaDispatchEntry* operation = streamOperation(stream, slot);
    if (operation == NULL) {
        return;
    }
    if (size < 0 || (uint64_t)size > SIZE_MAX) {
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    for (int64_t offset = 0; offset < size;) {
        int64_t count = size - offset > INT_MAX ? INT_MAX : size - offset;
        int last = 0;
        if (operation->link == NULL) {
            typedef void (*Read)(void*, void*, int64_t, int64_t, int64_t, int64_t, int*);
            ((Read)operation->code)(stream, (char*)item + offset, 1, count, count, 1, &last);
        } else {
            typedef void (*Read)(void*, void*, void*, int64_t, int64_t, int64_t, int64_t, int*);
            ((Read)operation->code)(operation->link, stream, (char*)item + offset, 1, count, count, 1, &last);
        }
        if (__ada_task_context()->exception != NULL) {
            return;
        }
        if (last < 0 || last > count) {
            __ada_raise(ADA_PROGRAM_ERROR);
            return;
        }
        if (last != count) {
            __ada_raise(ADA_END_ERROR);
            return;
        }
        offset += count;
    }
}

void __ada_stream_value_write(void* stream, int slot, const void* item, int64_t size)
{
    AdaDispatchEntry* operation = streamOperation(stream, slot);
    if (operation == NULL) {
        return;
    }
    if (size < 0 || (uint64_t)size > SIZE_MAX) {
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    for (int64_t offset = 0; offset < size;) {
        int64_t count = size - offset > INT_MAX ? INT_MAX : size - offset;
        if (operation->link == NULL) {
            typedef void (*Write)(void*, const void*, int64_t, int64_t, int64_t, int64_t);
            ((Write)operation->code)(stream, (const char*)item + offset, 1, count, count, 1);
        } else {
            typedef void (*Write)(void*, void*, const void*, int64_t, int64_t, int64_t, int64_t);
            ((Write)operation->code)(operation->link, stream, (const char*)item + offset, 1, count, count, 1);
        }
        if (__ada_task_context()->exception != NULL) {
            return;
        }
        offset += count;
    }
}

/* Class-wide payload hooks use the ordinary Ada tagged-result ABI. A tag
   created in a nested scope carries the same defining link as its equality
   and primitive dispatch entries. */
void* __ada_stream_tag_call(void* stream, int level, AdaTag* tag, int attribute,
    void* item, void* owner, void* arena)
{
    if (tag == NULL || attribute < 0 || attribute > 3 || tag->streams[attribute].code == NULL) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return NULL;
    }
    AdaDispatchEntry operation = tag->streams[attribute];
    if (attribute == 2) {
        void* result = NULL;
        if (operation.link == NULL) {
            typedef void (*Input)(void*, void*, void*, int, void*, int);
            ((Input)operation.code)(&result, owner, arena, 0, stream, level);
        } else {
            typedef void (*Input)(void*, void*, void*, int, void*, void*, int);
            ((Input)operation.code)(&result, owner, arena, 0, operation.link, stream, level);
        }
        return result;
    }
    if (operation.link == NULL) {
        typedef void (*Transfer)(void*, int, void*, int);
        ((Transfer)operation.code)(stream, level, item, 1);
    } else {
        typedef void (*Transfer)(void*, void*, int, void*, int);
        ((Transfer)operation.code)(operation.link, stream, level, item, 1);
    }
    return item;
}

void __ada_stream_external_tag(void* stream, int slot, const AdaTag* tag, const AdaTag* root)
{
    if (tag->level > root->level) {
        __ada_raise(&__ada_exc_tag_error);
        return;
    }
    size_t length = strlen(tag->name);
    if (length > INT_MAX) {
        __ada_raise(ADA_STORAGE_ERROR);
        return;
    }
    int bounds[2] = {1, (int)length};
    __ada_stream_value_write(stream, slot, bounds, sizeof bounds);
    if (__ada_task_context()->exception == NULL) {
        __ada_stream_value_write(stream, slot, tag->name, (int64_t)length);
    }
}

AdaTag* __ada_stream_input_tag(void* stream, int slot, AdaTag* root)
{
    int bounds[2];
    __ada_stream_value_read(stream, slot, bounds, sizeof bounds);
    if (__ada_task_context()->exception != NULL) { return NULL; }
    int64_t length = (int64_t)bounds[1] - bounds[0] + 1;
    if (length < 0) { length = 0; }
    if (length > INT_MAX || (length != 0 && bounds[0] < 1)) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return NULL;
    }
    char* name = malloc(length == 0 ? 1 : (size_t)length);
    if (name == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    __ada_stream_value_read(stream, slot, name, length);
    AdaTag* result = NULL;
    if (__ada_task_context()->exception == NULL) {
        result = __ada_tag_descendant(name, (int)length, root);
    }
    free(name);
    if (result != NULL && result->isAbstract) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return NULL;
    }
    return result;
}
