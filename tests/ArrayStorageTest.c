/* Instrument allocation only in this test's runtime copy. This verifies actual
   frees, including ownership transfer failure, without a production debug API. */
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
static int liveAllocations;
static int failNext;
static int failAfter = -1;
static void* testMalloc(size_t size)
{
    if (failNext || failAfter == 0) {
        failNext = 0;
        failAfter = -1;
        return NULL;
    }
    if (failAfter > 0) {
        --failAfter;
    }
    void* result = malloc(size);
    if (result != NULL) {
        ++liveAllocations;
    }
    return result;
}
static void* testCalloc(size_t count, size_t size)
{
    if (failNext || failAfter == 0) {
        failNext = 0;
        failAfter = -1;
        return NULL;
    }
    if (failAfter > 0) {
        --failAfter;
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
#include "../runtime/adatags.c"
#undef malloc
#undef calloc
#undef free
#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "line %d\n", __LINE__); return 1; } } while (0)
static int finalizationOrder;
static void finalizeValue(void* object)
{
    int value = *(int*)object;
    finalizationOrder = finalizationOrder * 10 + value;
    if (value == 2) {
        __ada_raise(ADA_CONSTRAINT_ERROR);
    }
}

static int adjustments;
static void adjustValue(void* object)
{
    if (*(int*)object == 2) {
        __ada_raise_message(ADA_CONSTRAINT_ERROR, "adjustment", 10);
    } else {
        ++adjustments;
    }
}

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
    /* Registration and cleanup work even when the next heap allocation fails.
       A suffix rewind retains earlier records; failing callbacks do not stop it. */
    AdaFinalization* finalizations = NULL;
    AdaFinalization firstRecord, secondRecord;
    int firstObject = 1;
    int secondObject = 3;
    __ada_exception = NULL;
    failNext = 1;
    __ada_finalization_push(&finalizations, &firstRecord, &firstObject, finalizeValue);
    __ada_finalization_push(&finalizations, &secondRecord, &secondObject, finalizeValue);
    __ada_finalize_to(&finalizations, &firstRecord);
    CHECK(finalizations == &firstRecord && finalizationOrder == 3 && failNext == 1);
    __ada_finalize_to(&finalizations, NULL);
    CHECK(finalizations == NULL && finalizationOrder == 31 && liveAllocations == 0);
    failNext = 0;
    finalizationOrder = 0;
    __ada_raise_message(ADA_STORAGE_ERROR, "original", 8);
    __ada_finalization_push(&finalizations, &firstRecord, &firstObject, finalizeValue);
    __ada_finalize_to(&finalizations, NULL);
    CHECK(__ada_exception == ADA_STORAGE_ERROR && liveAllocations == 1);
    __ada_exception_capture(&occurrence, &owner);
    CHECK(occurrence.length == 8 && memcmp(occurrence.message, "original", 8) == 0);
    __ada_array_release(&owner);
    finalizationOrder = 0;
    secondObject = 2;
    __ada_raise_message(ADA_STORAGE_ERROR, "replaced", 8);
    __ada_finalization_push(&finalizations, &firstRecord, &firstObject, finalizeValue);
    __ada_finalization_push(&finalizations, &secondRecord, &secondObject, finalizeValue);
    __ada_finalize_to(&finalizations, NULL);
    CHECK(finalizations == NULL && finalizationOrder == 21);
    CHECK(__ada_exception == ADA_PROGRAM_ERROR && liveAllocations == 0);
    __ada_exception = NULL;
    /* Reservations fail before construction, and inactive or finalized values
       are never finalized again when their containing scope unwinds. */
    failNext = 1;
    __ada_finalization_reserve(&finalizations, &owner, &firstObject, finalizeValue);
    CHECK(finalizations == NULL && __ada_exception == ADA_STORAGE_ERROR && liveAllocations == 0);
    __ada_exception = NULL;
    finalizationOrder = 0;
    __ada_finalization_reserve(&finalizations, &owner, &firstObject, finalizeValue);
    __ada_finalize_to(&finalizations, NULL);
    CHECK(finalizationOrder == 0 && __ada_exception == NULL);
    __ada_array_release(&owner);
    CHECK(liveAllocations == 0 && registeredFinalizations == NULL);
    __ada_finalization_reserve(&finalizations, &owner, &firstObject, finalizeValue);
    __ada_finalization_reserve(&finalizations, &owner, &secondObject, finalizeValue);
    CHECK(liveAllocations == 4);
    CHECK(__ada_controlled_adjust(&firstObject, adjustValue) == 0 && adjustments == 1);
    CHECK(__ada_controlled_adjust(&secondObject, adjustValue) == 1 && __ada_exception == NULL);
    CHECK(liveAllocations == 4);
    CHECK(__ada_controlled_finalize(&firstObject) == 0 && finalizationOrder == 1);
    __ada_finalize_to(&finalizations, NULL);
    CHECK(finalizationOrder == 1 && registeredFinalizations == NULL);
    __ada_array_release(&owner);
    CHECK(liveAllocations == 0);
    /* Library registrations outlive elaboration arenas. Shutdown must skip
       failed construction, preserve pending occurrences, and release all
       registration storage even when a finalizer fails. */
    finalizationOrder = 0;
    failNext = 1;
    __ada_library_reserve(&firstObject, finalizeValue);
    CHECK(libraryFinalizations == NULL && liveAllocations == 0 && __ada_exception == ADA_STORAGE_ERROR);
    __ada_exception = NULL;
    __ada_library_reserve(&firstObject, finalizeValue);
    __ada_controlled_activate(&firstObject, finalizeValue);
    __ada_library_reserve(&secondObject, finalizeValue);
    CHECK(liveAllocations == 4);
    __ada_raise_message(ADA_STORAGE_ERROR, "library failure", 15);
    __ada_library_finalize();
    CHECK(finalizationOrder == 1 && registeredFinalizations == NULL && libraryFinalizations == NULL);
    CHECK(libraryFinalizationArena == NULL && __ada_exception == ADA_STORAGE_ERROR);
    __ada_exception_capture(&occurrence, &owner);
    CHECK(occurrence.length == 15 && memcmp(occurrence.message, "library failure", 15) == 0);
    __ada_array_release(&owner);
    CHECK(liveAllocations == 0);
    __ada_library_finalize();
    CHECK(finalizationOrder == 1 && liveAllocations == 0 && __ada_exception == NULL);
    finalizationOrder = 0;
    __ada_library_reserve(&firstObject, finalizeValue);
    __ada_controlled_activate(&firstObject, finalizeValue);
    __ada_library_reserve(&secondObject, finalizeValue);
    __ada_controlled_activate(&secondObject, finalizeValue);
    failNext = 1;
    __ada_library_finalize();
    CHECK(finalizationOrder == 21 && liveAllocations == 0 && failNext == 1);
    CHECK(__ada_exception == ADA_PROGRAM_ERROR && registeredFinalizations == NULL);
    failNext = 0;
    __ada_exception = NULL;
    /* Every allocation point in collection creation, object allocation and
       part registration must unwind without leaving registry or heap nodes. */
    for (int failure = 0; failure <= 8; ++failure) {
        failAfter = failure;
        void* collection = __ada_collection_create(&finalizations, &owner);
        if (collection != NULL) {
            int* object = __ada_collection_allocate(collection, sizeof *object);
            if (object != NULL) {
                *object = 1;
                __ada_allocation_reserve(object, object, finalizeValue);
                if (__ada_exception == NULL) {
                    __ada_controlled_activate(object, finalizeValue);
                }
            }
        }
        CHECK(failure == 8 || __ada_exception == ADA_STORAGE_ERROR);
        failAfter = -1;
        __ada_finalize_to(&finalizations, NULL);
        __ada_array_release(&owner);
        CHECK(liveAllocations == 0 && registeredAllocations == NULL && registeredFinalizations == NULL);
        __ada_exception = NULL;
    }
    void* collection = __ada_collection_create(&finalizations, &owner);
    CHECK(collection != NULL);
    int* allocatedFirst = __ada_collection_allocate(collection, sizeof *allocatedFirst);
    int* allocatedSecond = __ada_collection_allocate(collection, sizeof *allocatedSecond);
    CHECK(allocatedFirst != NULL && allocatedSecond != NULL);
    *allocatedFirst = 1;
    *allocatedSecond = 2;
    __ada_allocation_reserve(allocatedFirst, allocatedFirst, finalizeValue);
    __ada_allocation_reserve(allocatedSecond, allocatedSecond, finalizeValue);
    __ada_controlled_activate(allocatedFirst, finalizeValue);
    __ada_controlled_activate(allocatedSecond, finalizeValue);
    finalizationOrder = 0;
    __ada_deallocate(allocatedSecond);
    CHECK(finalizationOrder == 2 && __ada_exception == ADA_PROGRAM_ERROR);
    __ada_finalize_to(&finalizations, NULL);
    __ada_array_release(&owner);
    CHECK(finalizationOrder == 21 && __ada_exception == ADA_PROGRAM_ERROR);
    CHECK(liveAllocations == 0 && registeredAllocations == NULL && registeredFinalizations == NULL);
    __ada_exception = NULL;
    /* Tagged results use the same ownership transfer and rewind boundaries. */
    AdaDispatchEntry slots[] = { { NULL, NULL } };
    AdaTag template = { NULL, 32, 8, slots, NULL, NULL, "TEST.ROOT", 1, NULL, 1, 0, 0 };
    failNext = 1;
    CHECK(__ada_tag_create(&template, NULL, &owner) == NULL);
    CHECK(__ada_exception == ADA_STORAGE_ERROR && liveAllocations == 0);
    __ada_exception = NULL;
    AdaTag* tag = __ada_tag_create(&template, NULL, &owner);
    CHECK(tag != NULL && tag->master == &owner && tag->slots != slots);
    CHECK(liveAllocations == 3);
    struct TaggedObject
    {
        AdaTag* tag;
        long long fields[3];
    } object = { tag, { 11, 22, 33 } };
    for (int i = 0; i < 1000; ++i) {
        void* result = NULL;
        __ada_tagged_result(&result, &object);
        CHECK(result != NULL && liveAllocations == 4);
        CHECK(memcmp(result, &object, sizeof object) == 0);
        __ada_array_adopt(&owner, result);
        CHECK(liveAllocations == 5);
        __ada_array_release(&owner);
        CHECK(liveAllocations == 3);
    }
    transfer = NULL;
    failNext = 1;
    __ada_tagged_result(&transfer, &object);
    CHECK(transfer == NULL && liveAllocations == 3 && __ada_exception == ADA_STORAGE_ERROR);
    __ada_exception = NULL;
    __ada_tagged_result(&transfer, &object);
    failNext = 1;
    __ada_array_adopt(&owner, transfer);
    CHECK(liveAllocations == 3 && owner == NULL && __ada_exception == ADA_STORAGE_ERROR);
    __ada_exception = NULL;
    __ada_tag_check_level(&object, 0);
    CHECK(__ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
    tag->m_needsFinalization = 1;
    __ada_tag_check_copy(&object);
    CHECK(__ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
    transfer = NULL;
    __ada_tagged_result(&transfer, &object);
    CHECK(transfer == NULL && __ada_exception == ADA_PROGRAM_ERROR && liveAllocations == 3);
    releaseTags();
    CHECK(liveAllocations == 0);
    return 0;
}
