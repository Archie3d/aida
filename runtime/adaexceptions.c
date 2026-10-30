#include "adart.h"
#include "adalock.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(sizeof(AdaExceptionOccurrence) == 760, "Ada.Exceptions occurrence ABI");
_Static_assert(offsetof(AdaExceptionOccurrence, ownedMessage) == 752, "owned occurrence message ABI");

typedef struct ExceptionRegistration
{
    struct ExceptionRegistration* next;
    const AdaException* identity;
    int owned;
} ExceptionRegistration;

static AdaMutex registryMutex = ADA_MUTEX_INITIALIZER;
static ExceptionRegistration* registrations;
static int cleanupRegistered;

static void releaseRegistry(void)
{
    __ada_mutex_lock(&registryMutex);
    while (registrations != NULL) {
        ExceptionRegistration* entry = registrations;
        registrations = entry->next;
        if (entry->owned) {
            free((void*)entry->identity->name);
            free((void*)entry->identity);
        }
        free(entry);
    }
    __ada_mutex_unlock(&registryMutex);
}

static int registerIdentity(const AdaException* identity, int owned)
{
    ExceptionRegistration* entry = malloc(sizeof *entry);
    if (entry == NULL) { __ada_raise(ADA_STORAGE_ERROR); return 0; }
    if (!cleanupRegistered) {
        if (atexit(releaseRegistry) != 0) {
            free(entry);
            __ada_raise(ADA_STORAGE_ERROR);
            return 0;
        }
        cleanupRegistered = 1;
    }
    *entry = (ExceptionRegistration){registrations, identity, owned};
    registrations = entry;
    return 1;
}

void __ada_exception_register(const AdaException* identity)
{
    if (identity == NULL) { return; }
    __ada_mutex_lock(&registryMutex);
    for (ExceptionRegistration* entry = registrations; entry != NULL; entry = entry->next) {
        if (entry->identity == identity) { __ada_mutex_unlock(&registryMutex); return; }
    }
    registerIdentity(identity, 0);
    __ada_mutex_unlock(&registryMutex);
}

const AdaException* __ada_exception_from_name(const char* name, int length)
{
    if (length == 0) { return NULL; }
    if (length < 0 || name == NULL || memchr(name, 0, (size_t)length) != NULL) {
        __ada_raise(ADA_DATA_ERROR);
        return NULL;
    }
    const AdaException* const kPredefined[] = {
        ADA_CONSTRAINT_ERROR, ADA_PROGRAM_ERROR, ADA_STORAGE_ERROR, ADA_TASKING_ERROR,
        &__ada_exc_numeric_error, ADA_STATUS_ERROR, ADA_MODE_ERROR, ADA_NAME_ERROR,
        ADA_USE_ERROR, ADA_DEVICE_ERROR, ADA_END_ERROR, ADA_DATA_ERROR,
        &__ada_exc_layout_error, &__ada_exc_tag_error
    };
    for (size_t i = 0; i < sizeof kPredefined / sizeof *kPredefined; ++i) {
        if (strlen(kPredefined[i]->name) == (size_t)length
            && memcmp(kPredefined[i]->name, name, (size_t)length) == 0) { return kPredefined[i]; }
    }
    __ada_mutex_lock(&registryMutex);
    for (ExceptionRegistration* entry = registrations; entry != NULL; entry = entry->next) {
        if (strlen(entry->identity->name) == (size_t)length
            && memcmp(entry->identity->name, name, (size_t)length) == 0) {
            const AdaException* result = entry->identity;
            __ada_mutex_unlock(&registryMutex);
            return result;
        }
    }
    AdaException* identity = malloc(sizeof *identity);
    char* copy = malloc((size_t)length + 1);
    if (identity == NULL || copy == NULL) {
        free(identity);
        free(copy);
        __ada_mutex_unlock(&registryMutex);
        __ada_raise(ADA_STORAGE_ERROR);
        return NULL;
    }
    memcpy(copy, name, (size_t)length);
    copy[length] = 0;
    identity->name = copy;
    if (!registerIdentity(identity, 1)) { free(copy); free(identity); identity = NULL; }
    __ada_mutex_unlock(&registryMutex);
    return identity;
}

void __ada_occurrence_initialize(AdaExceptionOccurrence* occurrence)
{
    memset(occurrence, 0, sizeof *occurrence);
}

void __ada_occurrence_release(AdaExceptionOccurrence* occurrence)
{
    free(occurrence->ownedMessage);
    memset(occurrence, 0, sizeof *occurrence);
}

void __ada_occurrence_restore(AdaExceptionOccurrence* target, const AdaException* identity,
    const char* message, int length)
{
    if (length < 0 || (length != 0 && (identity == NULL || message == NULL))) {
        __ada_raise(ADA_DATA_ERROR);
        return;
    }
    char* owned = NULL;
    if (length > ADA_SAVED_MESSAGE_CAPACITY) {
        owned = malloc((size_t)length);
        if (owned == NULL) { __ada_raise(ADA_STORAGE_ERROR); return; }
        memcpy(owned, message, (size_t)length);
    } else if (length != 0) {
        memmove(target->savedMessage, message, (size_t)length);
    }
    free(target->ownedMessage);
    target->ownedMessage = owned;
    target->identity = identity;
    target->length = length;
    target->message = length == 0 ? NULL : owned != NULL ? owned : target->savedMessage;
    target->origin = NULL;
    target->traceCount = 0;
    memset(target->trace, 0, sizeof target->trace);
}
