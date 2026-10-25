#include "adart.h"
#include "adalock.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/* The first three words are the published object layout descriptor prefix.
   Entries pair code with its enclosing activation, independently of callers. */
_Static_assert(sizeof(AdaTag) == 144, "tag descriptor ABI");
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
static AdaMutex tagMutex = ADA_MUTEX_INITIALIZER;
static int cleanupRegistered;
const AdaException __ada_exc_tag_error = { "ADA.TAGS.TAG_ERROR" };

static void releaseTags(void)
{
    __ada_mutex_lock(&tagMutex);
    while (registrations != NULL) {
        TagRegistration* registration = registrations;
        registrations = registration->next;
        if (registration->owned) {
            free(registration->tag->slots);
            free(registration->tag->interfaces);
            free(registration->tag);
        }
        free(registration);
    }
    __ada_mutex_unlock(&tagMutex);
}

static int registerTagImpl(AdaTag* tag, int owned)
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
        if (atexit(releaseTags) != 0) {
            registrations = registration->next;
            free(registration);
            __ada_raise(ADA_STORAGE_ERROR);
            return 0;
        }
        cleanupRegistered = 1;
    }
    return 1;
}

static int registerTag(AdaTag* tag, int owned)
{
    __ada_mutex_lock(&tagMutex);
    int result = registerTagImpl(tag, owned);
    __ada_mutex_unlock(&tagMutex);
    return result;
}

void __ada_tag_register(void* value)
{
    registerTag(value, 0);
}

void* __ada_tag_create(const void* templateTag, void* parent, void* master)
{
    const AdaTag* source = templateTag;
    return __ada_tag_create_layout(templateTag, parent, master, source->size, source->layout);
}

void __ada_tag_set_layout(void* value, int64_t size, const int64_t* layout)
{
    AdaTag* tag = value;
    tag->size = size;
    tag->layout = layout;
}

void* __ada_tag_create_layout(const void* templateTag, void* parent, void* master, int64_t size, const int64_t* layout)
{
    const AdaTag* source = templateTag;
    return __ada_tag_create_full(templateTag, parent, master, size, layout, source->interfaceCount, source->interfaces);
}

void* __ada_tag_create_full(const void* templateTag, void* parent, void* master, int64_t size,
    const int64_t* layout, int64_t interfaceCount, const AdaInterfaceBinding* interfaces)
{
    const AdaTag* source = templateTag;
    AdaTag* tag = malloc(sizeof *tag);
    if (tag == NULL) {
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    *tag = *source;
    tag->size = size;
    tag->layout = layout;
    tag->slots = malloc((source->slotCount == 0 ? 1 : source->slotCount) * sizeof *tag->slots);
    if (tag->slots == NULL) {
        free(tag);
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    if (source->slotCount != 0) {
        memcpy(tag->slots, source->slots, source->slotCount * sizeof *tag->slots);
    }
    tag->interfaces = NULL;
    tag->interfaceCount = interfaceCount;
    if (interfaceCount != 0) {
        tag->interfaces = malloc((size_t)interfaceCount * sizeof *tag->interfaces);
        if (tag->interfaces == NULL) {
            free(tag->slots);
            free(tag);
            __ada_raise(ADA_STORAGE_ERROR);
            return NULL;
        }
        memcpy(tag->interfaces, interfaces, (size_t)interfaceCount * sizeof *tag->interfaces);
    }
    tag->parent = parent;
    tag->master = tag->equalityLink = master;
    if (!registerTag(tag, 1)) {
        free(tag->interfaces);
        free(tag->slots);
        free(tag);
        return NULL;
    }
    return tag;
}

void __ada_tag_check_accessibility(const void* object, int level)
{
    const AdaTag* tag = *(const AdaTag* const*)object;
    if (tag->level > level) {
        __ada_raise(ADA_PROGRAM_ERROR);
    }
}

void __ada_tag_check_level(const void* object, int level)
{
    const AdaTag* tag = *(const AdaTag* const*)object;
    if (tag->needsFinalization || tag->level > level) {
        __ada_raise(ADA_PROGRAM_ERROR);
    }
}

void __ada_tagged_result(void** result, const void* object)
{
    const AdaTag* tag = *(const AdaTag* const*)object;
    if (tag->needsFinalization) {
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
    return __ada_tag_has_ancestor(tag, ancestor);
}

/* All views retain the complete object's address. Interface maps translate a
   view's operation number to its concrete slot, with no record-layout offset. */
int __ada_tag_has_ancestor(const void* value, const void* ancestorValue)
{
    const AdaTag* tag = value;
    const AdaTag* ancestor = ancestorValue;
    for (; tag != NULL; tag = tag->parent) {
        if (tag == ancestor) { return 1; }
        for (int64_t i = 0; i < tag->interfaceCount; ++i) {
            if (tag->interfaces[i].tag == ancestor) { return 1; }
        }
    }
    return 0;
}

void* __ada_tag_dispatch(void* value, void* interfaceValue, int slot)
{
    AdaTag* tag = value;
    AdaTag* interfaceTag = interfaceValue;
    if (tag == NULL || interfaceTag == NULL || slot < 0 || slot >= interfaceTag->slotCount) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return NULL;
    }
    int64_t concreteSlot = -1;
    if (tag == interfaceTag) { concreteSlot = slot; }
    for (int64_t i = 0; concreteSlot < 0 && i < tag->interfaceCount; ++i) {
        if (tag->interfaces[i].tag == interfaceTag) { concreteSlot = tag->interfaces[i].slots[slot]; }
    }
    if (concreteSlot < 0 || concreteSlot >= tag->slotCount || tag->slots[concreteSlot].code == NULL) {
        __ada_raise(ADA_PROGRAM_ERROR);
        return NULL;
    }
    return &tag->slots[concreteSlot];
}

int __ada_tag_interface_count(void* value)
{
    AdaTag* tag = value;
    return requireTag(tag) ? (int)tag->interfaceCount : 0;
}

void* __ada_tag_interface_at(void* value, int position)
{
    AdaTag* tag = value;
    if (!requireTag(tag)) { return NULL; }
    if (position < 1 || position > tag->interfaceCount) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
        return NULL;
    }
    return tag->interfaces[position - 1].tag;
}

static void* tagInternal(const char* name, int length)
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

static void* tagDescendant(const char* name, int length, void* ancestor)
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
    return requireTag(tag) ? ((AdaTag*)tag)->isAbstract != 0 : 0;
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
    if (tag->needsFinalization) {
        __ada_raise(ADA_PROGRAM_ERROR);
    }
}

void* __ada_tag_internal(const char* name, int length)
{
    __ada_mutex_lock(&tagMutex);
    void* result = tagInternal(name, length);
    __ada_mutex_unlock(&tagMutex);
    return result;
}

void* __ada_tag_descendant(const char* name, int length, void* ancestor)
{
    __ada_mutex_lock(&tagMutex);
    void* result = tagDescendant(name, length, ancestor);
    __ada_mutex_unlock(&tagMutex);
    return result;
}
