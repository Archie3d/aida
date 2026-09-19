/* Instrument allocation only in this test's runtime copy. This verifies actual
   frees, including ownership transfer failure, without a production debug API. */
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
static int liveAllocations;
static int failNext;
static void* testMalloc(size_t size)
{
    if (failNext) {
        failNext = 0;
        return NULL;
    }
    void* result = malloc(size);
    if (result != NULL) {
        ++liveAllocations;
    }
    return result;
}
static void* testCalloc(size_t count, size_t size)
{
    if (failNext) {
        failNext = 0;
        return NULL;
    }
    void* result = calloc(count, size);
    if (result != NULL) {
        ++liveAllocations;
    }
    return result;
}
static void testFree(void* pointer)
{
    if (pointer != NULL) {
        --liveAllocations;
    }
    free(pointer);
}
#define malloc testMalloc
#define calloc testCalloc
#define free testFree
#include "../runtime/adart.c"
#undef malloc
#undef calloc
#undef free
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "line %d\n", __LINE__); return 1; } } while (0)
int main(void)
{
    void* owner = NULL;
    char* outer = __ada_array_local(&owner, 1, 20, 1);
    CHECK(outer != NULL && liveAllocations == 2);
    outer[0] = 'x';
    void* mark = owner;
    for (int i = 0; i < 1000; ++i) {
        CHECK(__ada_array_local(&owner, 5, 4, 1) != NULL);
        CHECK(__ada_array_local(&owner, 1, 100, 8) != NULL);
        __ada_array_rewind(&owner, mark);
        CHECK(owner == mark && liveAllocations == 2 && outer[0] == 'x');
    }
    void* transfer = testMalloc(100);
    __ada_array_adopt(&owner, transfer);
    CHECK(liveAllocations == 4);
    __ada_array_rewind(&owner, mark);
    CHECK(liveAllocations == 2);
    transfer = testMalloc(100);
    failNext = 1;
    __ada_array_adopt(&owner, transfer);
    CHECK(__ada_exception == ADA_STORAGE_ERROR && liveAllocations == 2 && owner == mark);
    __ada_array_release(&owner);
    CHECK(owner == NULL && liveAllocations == 0);
    __ada_array_release(&owner);
    CHECK(liveAllocations == 0);

    /* The occurrence ABI and both message owners must agree with generated
       code, including allocation failure while entering or leaving a handler. */
    CHECK(sizeof(AdaExceptionOccurrence) == 752);
    CHECK(sizeof(AdaTraceFrame) == 24);
    CHECK(offsetof(AdaExceptionOccurrence, origin) == 224);
    CHECK(offsetof(AdaExceptionOccurrence, trace) == 240);
    AdaExceptionOccurrence occurrence;
    char message[] = { 'a', '\0', 'b' };
    __ada_raise_message(ADA_PROGRAM_ERROR, message, sizeof message);
    message[0] = 'x';
    CHECK(liveAllocations == 1);
    __ada_exception_capture(&occurrence, &owner);
    CHECK(__ada_exception == NULL && liveAllocations == 2);
    CHECK(occurrence.identity == ADA_PROGRAM_ERROR && occurrence.length == 3);
    CHECK(memcmp(occurrence.message, "a\0b", 3) == 0);
    __ada_raise_message(ADA_CONSTRAINT_ERROR, "nested", 6);
    CHECK(liveAllocations == 3);
    __ada_reraise(&occurrence);
    __ada_array_release(&owner);
    CHECK(liveAllocations == 1 && __ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception_capture(&occurrence, &owner);
    CHECK(memcmp(occurrence.message, "a\0b", 3) == 0);
    failNext = 1;
    __ada_reraise(&occurrence);
    CHECK(__ada_exception == ADA_STORAGE_ERROR && liveAllocations == 2);
    __ada_array_release(&owner);
    CHECK(liveAllocations == 0);
    __ada_raise_message(ADA_PROGRAM_ERROR, "lost", 4);
    failNext = 1;
    __ada_exception_capture(&occurrence, &owner);
    CHECK(__ada_exception == ADA_STORAGE_ERROR && liveAllocations == 0 && owner == NULL);
    __ada_exception_capture(&occurrence, &owner);
    CHECK(occurrence.identity == ADA_STORAGE_ERROR && occurrence.length == 0);
    CHECK(__ada_exception == NULL && liveAllocations == 0);
    char longMessage[5000];
    memset(longMessage, 'x', sizeof longMessage);
    longMessage[10] = '\0';
    __ada_raise_message(ADA_PROGRAM_ERROR, longMessage, sizeof longMessage);
    __ada_exception_capture(&occurrence, &owner);
    AdaExceptionOccurrence saved = { 0 };
    __ada_save_occurrence(&saved, &occurrence);
    CHECK(saved.length == 200 && saved.message == saved.savedMessage);
    CHECK(memcmp(saved.message, longMessage, 200) == 0 && liveAllocations == 2);
    AdaExceptionOccurrence* heap = __ada_save_occurrence_new(&occurrence);
    CHECK(heap != NULL && heap->length == 5000 && liveAllocations == 3);
    __ada_save_occurrence(heap, heap);
    CHECK(heap->length == 5000);
    __ada_array_release(&owner);
    CHECK(liveAllocations == 1 && memcmp(heap->message, longMessage, 5000) == 0);
    __ada_save_occurrence(heap, &saved);
    CHECK(heap->length == 200 && heap->message == heap->savedMessage);
    __ada_deallocate(heap);
    CHECK(liveAllocations == 0 && saved.identity == ADA_PROGRAM_ERROR);
    failNext = 1;
    heap = __ada_save_occurrence_new(&saved);
    CHECK(heap == NULL && __ada_exception == ADA_STORAGE_ERROR && liveAllocations == 0);
    CHECK(saved.length == 200 && memcmp(saved.message, longMessage, 200) == 0);
    AdaExceptionOccurrence empty = { 0 };
    __ada_save_occurrence(&saved, &empty);
    CHECK(saved.identity == NULL && saved.message == NULL && saved.length == 0);
    AdaTraceFrame caller, callee;
    __ada_trace_enter(&caller, "Caller", "caller.adb:3:1");
    __ada_trace_enter(&callee, "Callee", "callee.adb:5:2");
    __ada_trace_location("callee.adb:8:4");
    __ada_raise_message(ADA_PROGRAM_ERROR, "a\0b", 3);
    __ada_exception_capture(&occurrence, &owner);
    CHECK(occurrence.traceCount == 2 && strcmp(occurrence.origin, "callee.adb:8:4") == 0);
    CHECK(strcmp(occurrence.trace[1].routine, "Caller") == 0);
    __ada_save_occurrence(&saved, &occurrence);
    __ada_trace_leave(&callee);
    __ada_trace_leave(&caller);
    CHECK(currentTrace == NULL);
    __ada_array_release(&owner);
    __ada_reraise(&saved);
    __ada_exception_capture(&occurrence, &owner);
    CHECK(occurrence.traceCount == 2 && occurrence.origin == saved.origin);
    char information[1024];
    int informationLength = __ada_exception_information_length(&occurrence);
    CHECK(informationLength > 0 && informationLength < (int)sizeof information);
    __ada_exception_information_copy(&occurrence, information, informationLength);
    const char expectedInformation[] = "PROGRAM_ERROR: a\0b\nraised at callee.adb:8:4\nAda traceback:"
        "\n  Callee at callee.adb:8:4\n  Caller at caller.adb:3:1";
    CHECK(informationLength == (int)sizeof expectedInformation - 1);
    CHECK(memcmp(information, expectedInformation, sizeof expectedInformation - 1) == 0);
    heap = __ada_save_occurrence_new(&occurrence);
    CHECK(heap != NULL && heap->traceCount == 2 && heap->origin == occurrence.origin);
    __ada_deallocate(heap);
    __ada_array_release(&owner);
    CHECK(liveAllocations == 0);
    __ada_trace_enter(&caller, "New_Caller", "new.adb:1:1");
    saved.identity = ADA_STORAGE_ERROR;
    failNext = 1;
    __ada_reraise(&saved);
    __ada_exception_capture(&occurrence, &owner);
    CHECK(occurrence.identity == ADA_STORAGE_ERROR && occurrence.traceCount == 1);
    CHECK(strcmp(occurrence.origin, "new.adb:1:1") == 0);
    __ada_trace_leave(&caller);
    __ada_save_occurrence(&saved, &empty);
    CHECK(saved.origin == NULL && saved.traceCount == 0 && liveAllocations == 0);
    return 0;
}
