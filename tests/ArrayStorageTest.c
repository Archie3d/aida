/* Instrument allocation only in this test's runtime copy. This verifies actual
   frees, including ownership transfer failure, without a production debug API. */
#include <stdlib.h>
#include <stdio.h>
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
    CHECK(sizeof(AdaExceptionOccurrence) == 224);
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
    return 0;
}
