#include "../runtime/adart.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "exception runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

static const AdaException kLocal = {"EXCEPTION_TEST.LOCAL"};

static void clearPending(void)
{
    AdaTaskContext* context = __ada_task_context();
    free(context->pendingMessage);
    context->pendingMessage = NULL;
    context->pendingMessageLength = 0;
    context->exception = NULL;
    context->pendingOrigin = NULL;
    context->pendingTraceCount = 0;
}

static void* worker(void* argument)
{
    const AdaException** result = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int i = 0; i < 200; ++i) {
        __ada_exception_register(&kLocal);
        CHECK(__ada_exception_from_name(kLocal.name, (int)strlen(kLocal.name)) == &kLocal);
        const AdaException* remote = __ada_exception_from_name("REMOTE.PACKAGE.FAILURE", 22);
        CHECK(remote != NULL && strcmp(remote->name, "REMOTE.PACKAGE.FAILURE") == 0);
        if (*result == NULL) { *result = remote; }
        CHECK(*result == remote);
        char message[5000];
        memset(message, 'x', sizeof message);
        message[13] = 0;
        AdaExceptionOccurrence occurrence = {0};
        __ada_occurrence_restore(&occurrence, remote, message, sizeof message);
        CHECK(context.exception == NULL && occurrence.ownedMessage != NULL
            && occurrence.length == sizeof message && memcmp(occurrence.message, message, sizeof message) == 0);
        __ada_reraise(&occurrence);
        CHECK(context.exception == remote && context.pendingMessageLength == sizeof message
            && memcmp(context.pendingMessage, message, sizeof message) == 0);
        clearPending();
        __ada_occurrence_restore(&occurrence, &kLocal, "small", 5);
        CHECK(occurrence.ownedMessage == NULL && occurrence.message == occurrence.savedMessage
            && memcmp(occurrence.message, "small", 5) == 0);
        __ada_occurrence_release(&occurrence);
        CHECK(occurrence.identity == NULL && occurrence.message == NULL && occurrence.length == 0);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    __ada_exception_register(&kLocal);
    CHECK(__ada_exception_from_name("CONSTRAINT_ERROR", 16) == ADA_CONSTRAINT_ERROR);
    CHECK(__ada_exception_from_name("ADA.IO_EXCEPTIONS.END_ERROR", 27) == ADA_END_ERROR);
    CHECK(__ada_exception_from_name("", 0) == NULL && context.exception == NULL);
    CHECK(__ada_exception_from_name("BAD\0NAME", 8) == NULL && context.exception == ADA_DATA_ERROR);
    clearPending();
    AdaExceptionOccurrence occurrence = {0};
    __ada_occurrence_restore(&occurrence, &kLocal, "previous", 8);
    __ada_occurrence_restore(&occurrence, NULL, "bad", 3);
    CHECK(context.exception == ADA_DATA_ERROR && occurrence.identity == &kLocal
        && memcmp(occurrence.message, "previous", 8) == 0);
    clearPending();
    __ada_occurrence_restore(&occurrence, NULL, "", 0);
    CHECK(occurrence.identity == NULL && occurrence.length == 0);
    __ada_reraise(&occurrence);
    CHECK(context.exception == NULL);
    __ada_occurrence_release(&occurrence);
    AdaThread threads[4];
    const AdaException* results[4] = {0};
    for (int i = 0; i < 4; ++i) { CHECK(__ada_thread_create(&threads[i], worker, &results[i]) == 0); }
    for (int i = 0; i < 4; ++i) { CHECK(__ada_thread_join(threads[i], NULL) == 0); }
    for (int i = 1; i < 4; ++i) { CHECK(results[i] == results[0]); }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
