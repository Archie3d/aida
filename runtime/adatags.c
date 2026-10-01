#include "adart.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* The first three words are the published object layout descriptor prefix.
   Entries pair code with its enclosing activation, independently of callers. */
typedef struct AdaDispatchEntry
{
    void* code;
    void* link;
} AdaDispatchEntry;

typedef struct AdaTag
{
    struct AdaTag* parent;
    int64_t size;
    int64_t alignment;
    AdaDispatchEntry* slots;
    void* equality;
    void* equalityLink;
    const char* name;
    int64_t level;
    void* master;
    int64_t slotCount;
    int64_t m_isAbstract;
    int64_t m_needsFinalization;
} AdaTag;

_Static_assert(sizeof(AdaTag) == 96, "tag descriptor ABI");
_Static_assert(offsetof(AdaTag, slots) == 24, "dispatch table offset");
_Static_assert(offsetof(AdaTag, equality) == 32, "equality entry offset");
_Static_assert(offsetof(AdaTag, level) == 56, "accessibility level offset");

typedef struct TagRegistration
{
    struct TagRegistration* next;
    AdaTag* tag;
    int owned;
} TagRegistration;

static TagRegistration* registrations;
static int cleanupRegistered;
const AdaException __ada_exc_tag_error = { "ADA.TAGS.TAG_ERROR" };

static void releaseTags(void)
{
    while (registrations != NULL) {
        TagRegistration* registration = registrations;
        registrations = registration->next;
        if (registration->owned) {
            free(registration->tag->slots);
            free(registration->tag);
        }
        free(registration);
    }
}

static int registerTag(AdaTag* tag, int owned)
{
    TagRegistration* registration;
    for (registration = registrations; registration != NULL; registration = registration->next) {
        if (registration->tag == tag) {
            return 1;
        }
    }
    registration = malloc(sizeof *registration);
    if (registration == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return 0;
    }
    registration->next = registrations;
    registration->tag = tag;
    registration->owned = owned;
    registrations = registration;
    if (!cleanupRegistered) {
        atexit(releaseTags);
        cleanupRegistered = 1;
    }
    return 1;
}

void __ada_tag_register(void* value)
{
    registerTag(value, 0);
}

void* __ada_tag_create(const void* templateTag, void* parent, void* master)
{
    const AdaTag* source = templateTag;
    AdaTag* tag = malloc(sizeof *tag);
    if (tag == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    *tag = *source;
    tag->slots = malloc((source->slotCount == 0 ? 1 : source->slotCount) * sizeof *tag->slots);
    if (tag->slots == NULL) {
        free(tag);
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    memcpy(tag->slots, source->slots, source->slotCount * sizeof *tag->slots);
    tag->parent = parent;
    tag->master = tag->equalityLink = master;
    if (!registerTag(tag, 1)) {
        free(tag->slots);
        free(tag);
        return NULL;
    }
    return tag;
}

void __ada_tag_check_level(const void* object, int level)
{
    const AdaTag* tag = *(const AdaTag* const*)object;
    if (tag->m_needsFinalization || tag->level > level) {
        __ada_raise(ADA_PROGRAM_ERROR);
    }
}

void __ada_tagged_result(void** result, const void* object)
{
    const AdaTag* tag = *(const AdaTag* const*)object;
    if (tag->m_needsFinalization) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return;
    }
    void* copy = __ada_allocate(tag->size);
    if (copy != NULL) {
        memcpy(copy, object, tag->size);
        *result = copy;
    }
}

static int requireTag(const AdaTag* tag)
{
    if (tag != NULL) {
        return 1;
    }
    __ada_raise(&__ada_exc_tag_error);
    return 0;
}

const char* __ada_tag_name(const void* value)
{
    const AdaTag* tag = value;
    return requireTag(tag) ? tag->name : "";
}

void* __ada_tag_parent(void* value)
{
    AdaTag* tag = value;
    return requireTag(tag) ? tag->parent : NULL;
}

int __ada_tag_is_descendant(void* value, void* ancestorValue)
{
    AdaTag* tag = value;
    AdaTag* ancestor = ancestorValue;
    if (!requireTag(tag) || !requireTag(ancestor) || tag->level != ancestor->level
        || tag->master != ancestor->master) {
        return 0;
    }
    for (; tag != NULL; tag = tag->parent) {
        if (tag == ancestor) {
            return 1;
        }
    }
    return 0;
}

void* __ada_tag_internal(const char* name, int length)
{
    TagRegistration* entry;
    for (entry = registrations; entry != NULL; entry = entry->next) {
        if (strlen(entry->tag->name) == (size_t)length && memcmp(name, entry->tag->name, length) == 0) {
            return entry->tag;
        }
    }
    __ada_raise(&__ada_exc_tag_error);
    return NULL;
}

void* __ada_tag_descendant(const char* name, int length, void* ancestor)
{
    TagRegistration* entry;
    void* result = NULL;
    if (!requireTag(ancestor)) {
        return NULL;
    }
    for (entry = registrations; entry != NULL; entry = entry->next) {
        if (strlen(entry->tag->name) == (size_t)length && memcmp(name, entry->tag->name, length) == 0
            && __ada_tag_is_descendant(entry->tag, ancestor)) {
            if (result != NULL) {
                __ada_raise(&__ada_exc_tag_error);
                return NULL;
            }
            result = entry->tag;
        }
    }
    if (result == NULL) {
        __ada_raise(&__ada_exc_tag_error);
    }
    return result;
}

int __ada_tag_is_abstract(void* tag)
{
    return requireTag(tag) ? ((AdaTag*)tag)->m_isAbstract != 0 : 0;
}

int __ada_tagged_equal(const void* left, const void* right)
{
    const AdaTag* tag = *(const AdaTag* const*)left;
    const AdaTag* other = *(const AdaTag* const*)right;
    typedef int (*Equality)(void*, const void*, const void*);
    if (tag != other) {
        return 0;
    }
    return ((Equality)tag->equality)(tag->equalityLink, left, right);
}

void __ada_tag_check_copy(const void* object)
{
    const AdaTag* tag = *(const AdaTag* const*)object;
    if (tag->m_needsFinalization) {
        __ada_raise(ADA_PROGRAM_ERROR);
    }
}
