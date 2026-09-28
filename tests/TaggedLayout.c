#include <stdint.h>
#include <string.h>

struct TypeDescriptor
{
    const struct TypeDescriptor* parent;
    int64_t size;
    int64_t alignment;
};

const void* tagOf(const void* object)
{
    const void* tag;
    memcpy(&tag, object, sizeof(tag));
    return tag;
}

int validDescriptor(const void* object, int64_t size, const void* parent)
{
    const struct TypeDescriptor* tag = tagOf(object);
    return tag != NULL && tag->parent == parent && tag->size == size && tag->alignment == 8;
}
