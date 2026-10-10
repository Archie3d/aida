#include "../runtime/adart.h"
#include "../runtime/adaio.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "shared tables check failed at line %d\n", __LINE__); exit(1); \
} } while (0)

enum { workerCount = 4, iterations = 100 };
static pthread_mutex_t barrierMutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t barrierCondition = PTHREAD_COND_INITIALIZER;
static int arrivals;
static int generation;
static AdaFile* files[workerCount];
static int lines[workerCount];
static AdaTag root = { .name = "SHARED.ROOT" };
static AdaTag templates[workerCount];
static char tagNames[workerCount][40];
static int finalizations;
static void* libraryCollection;

static void meet(void)
{
    CHECK(pthread_mutex_lock(&barrierMutex) == 0);
    int previous = generation;
    if (++arrivals == workerCount) {
        arrivals = 0;
        ++generation;
        CHECK(pthread_cond_broadcast(&barrierCondition) == 0);
    } else {
        while (generation == previous) {
            CHECK(pthread_cond_wait(&barrierCondition, &barrierMutex) == 0);
        }
    }
    CHECK(pthread_mutex_unlock(&barrierMutex) == 0);
}

static void finalize(void* object)
{
    ++finalizations;
    CHECK(__ada_task_context() == &__ada_main_context);
    CHECK(__ada_exception == NULL);
    /* Finalizers may use both shared services. A recursive library shutdown
       must be rejected rather than recursively unwinding the same arena. */
    AdaFile* scratch = NULL;
    __ada_file_open(&scratch, ADA_MODE_OUT, "", 0, 1);
    __ada_file_close(&scratch);
    __ada_tag_register(&root);
    if (*(int*)object) {
        __ada_library_finalize();
    }
}

static void checkLibraryRejection(void)
{
    int object = 0;
    CHECK(__ada_collection_allocate(libraryCollection, 16) == NULL && __ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
    CHECK(__ada_collection_begin(libraryCollection) == NULL && __ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
    __ada_library_reserve(&object, finalize);
    CHECK(__ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
    __ada_library_finalize();
    CHECK(__ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
    CHECK(__ada_collection_create(NULL, NULL) == NULL && __ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
    CHECK(__ada_construction_owner(NULL) == NULL && __ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
    CHECK(__ada_construction_arena(NULL) == NULL && __ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
    CHECK(__ada_tagged_owned_copy(NULL, NULL, NULL) == NULL && __ada_exception == ADA_PROGRAM_ERROR);
    __ada_exception = NULL;
}

static void* worker(void* argument)
{
    int id = *(int*)argument;
    AdaTaskContext context = { 0 };
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    meet();
    /* The first file/table/tag initialization happens concurrently. */
    __ada_file_open(&files[id], ADA_MODE_OUT, "", 0, 1);
    CHECK(files[id] != NULL && __ada_exception == NULL);
    __ada_tag_register(&root);
    CHECK(__ada_tag_descendant(root.name, (int)strlen(root.name), &root) == &root);
    checkLibraryRejection();
    meet();
    for (int i = 0; i < iterations; ++i) {
        AdaFile* scratch = NULL;
        __ada_file_open(&scratch, ADA_MODE_INOUT, "", 0, 1);
        CHECK(__ada_exception == NULL && scratch != NULL);
        int value = id * iterations + i;
        __ada_write_element(&scratch, &value, sizeof value);
        __ada_file_reset(&scratch, ADA_MODE_INOUT);
        int back = -1;
        __ada_read_element(&scratch, &back, sizeof back);
        CHECK(back == value);
        __ada_file_close(&scratch);
        CHECK(scratch == NULL && __ada_exception == NULL);

        __ada_set_output(&files[id]);
        /* Another thread may change the selection between these operations;
           each complete line must still reach exactly one of the open files. */
        __ada_put_line("shared line", 11);
        AdaFileRef snapshot = __ada_current_output();
        __ada_text_put_line(snapshot, "snapshot line", 13);

        __ada_tag_register(&root);
        AdaTag* tag = __ada_tag_create(&templates[id], &root, &context);
        CHECK(tag != NULL && __ada_exception == NULL);
        CHECK(__ada_tag_internal(tagNames[id], (int)strlen(tagNames[id])) == tag);
        CHECK(__ada_tag_is_descendant(tag, tag));
        CHECK(__ada_tag_descendant(root.name, (int)strlen(root.name), &root) == &root);
    }
    meet();
    __ada_file_reset(&files[id], ADA_MODE_IN);
    while (!__ada_file_end_of_file(&files[id])) {
        char text[32];
        int last = 0;
        __ada_text_get_line(&files[id], text, sizeof text, &last);
        CHECK((last == 11 && memcmp(text, "shared line", 11) == 0)
              || (last == 13 && memcmp(text, "snapshot line", 13) == 0));
        ++lines[id];
    }
    CHECK(__ada_exception == NULL);
    __ada_file_close(&files[id]);
    __ada_task_context_bind(previous);
    CHECK(__ada_task_context_dispose(&context));
    return NULL;
}

static void checkSnapshots(void)
{
    AdaFile* first = NULL;
    AdaFile* second = NULL;
    __ada_file_open(&first, ADA_MODE_OUT, "snapshot-first.tmp", 18, 1);
    __ada_file_open(&second, ADA_MODE_OUT, "snapshot-second.tmp", 19, 1);
    __ada_set_output(&first);
    AdaFileRef snapshot = __ada_current_output();
    __ada_set_output(&second);
    CHECK(*snapshot == first && *__ada_current_output() == second);
    AdaArrayResult name;
    __ada_file_name(&name, &first);
    CHECK(__ada_exception == NULL && name.m_last == 18);
    __ada_file_delete(&first);
    __ada_file_open(&first, ADA_MODE_OUT, "reused-slot.tmp", 15, 1);
    CHECK(memcmp(name.m_data, "snapshot-first.tmp", 18) == 0);
    __ada_deallocate(name.m_data);
    __ada_file_delete(&first);
    __ada_file_delete(&second);
    CHECK(*__ada_current_output() == *__ada_standard_output());
}

int main(void)
{
    libraryCollection = __ada_collection_create(NULL, NULL);
    CHECK(libraryCollection != NULL);
    int object = 0;
    __ada_library_reserve(&object, finalize);
    __ada_controlled_activate(&object, finalize);
    pthread_t workers[workerCount];
    int ids[workerCount];
    for (int i = 0; i < workerCount; ++i) {
        ids[i] = i;
        snprintf(tagNames[i], sizeof tagNames[i], "SHARED.WORKER%d", i);
        templates[i].name = tagNames[i];
        CHECK(pthread_create(&workers[i], NULL, worker, &ids[i]) == 0);
    }
    int total = 0;
    for (int i = 0; i < workerCount; ++i) {
        CHECK(pthread_join(workers[i], NULL) == 0);
        total += lines[i];
    }
    CHECK(total == workerCount * iterations * 2 && finalizations == 0);
    checkSnapshots();
    __ada_raise_message(ADA_TASKING_ERROR, "preserved", 9);
    __ada_library_finalize();
    CHECK(finalizations == 1 && __ada_exception == ADA_TASKING_ERROR);
    CHECK(__ada_main_context.m_pendingMessageLength == 9);
    CHECK(memcmp(__ada_main_context.m_pendingMessage, "preserved", 9) == 0);
    CHECK(__ada_task_context_dispose(&__ada_main_context));
    object = 1;
    __ada_library_reserve(&object, finalize);
    __ada_controlled_activate(&object, finalize);
    __ada_library_finalize();
    CHECK(finalizations == 2 && __ada_exception == ADA_PROGRAM_ERROR);
    CHECK(__ada_main_context.m_libraryFinalizations == NULL);
    CHECK(__ada_main_context.m_libraryFinalizationArena == NULL);
    CHECK(__ada_task_context_dispose(&__ada_main_context));
    return 0;
}
