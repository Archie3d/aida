#define _POSIX_C_SOURCE 200809L
#include "../runtime/adart.h"
#include "../runtime/adaio.h"
#include <pthread.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "blocking I/O check failed at line %d\n", __LINE__); exit(1); \
} } while (0)

static AdaFile* input;
static int profile;
static int resetFile;

static void* readInput(void* unused)
{
    (void)unused;
    AdaTaskContext context = { 0 };
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    if (profile == 0) {
        char value = 0;
        __ada_get(&value);
        CHECK(value == '7');
    } else if (profile == 1) {
        char value = 0;
        __ada_stream_read(input, &value, 1);
        CHECK(value == '7');
    } else {
        CHECK(__ada_fixed_get(&input, 0, 0, 100, 0) == 7);
    }
    CHECK(__ada_exception == NULL);
    __ada_task_context_bind(previous);
    CHECK(__ada_task_context_dispose(&context));
    return NULL;
}

static void* changeInput(void* unused)
{
    (void)unused;
    AdaTaskContext context = { 0 };
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    if (resetFile) {
        __ada_file_reset(&input, ADA_MODE_IN);
    } else {
        __ada_file_close(&input);
    }
    CHECK(__ada_exception == NULL);
    __ada_task_context_bind(previous);
    CHECK(__ada_task_context_dispose(&context));
    return NULL;
}

/* Observe pins under their owning lock. A second pin proves Close/Reset has
   reached the file lock while the reader still owns it. CTest bounds failure. */
static void waitForPins(unsigned count)
{
    for (;;) {
        __ada_io_lock();
        unsigned users = input->m_users;
        int locked = 0;
        if (users >= count) {
            int status = pthread_mutex_trylock(&input->m_mutex);
            CHECK(status == 0 || status == EBUSY);
            locked = status == EBUSY;
            if (status == 0) { CHECK(pthread_mutex_unlock(&input->m_mutex) == 0); }
        }
        __ada_io_unlock();
        if (locked) { return; }
        struct timespec pause = { 0, 1000000 };
        nanosleep(&pause, NULL);
    }
}

int main(void)
{
    for (profile = 0; profile < 3; ++profile) {
        for (resetFile = 0; resetFile < 2; ++resetFile) {
            int ends[2];
            CHECK(pipe(ends) == 0);
            input = NULL;
            __ada_file_open(&input, ADA_MODE_IN, "", 0, 1);
            CHECK(input != NULL && __ada_exception == NULL);
            CHECK(fclose(input->stream) == 0);
            input->stream = fdopen(ends[0], "r");
            CHECK(input->stream != NULL);
            __ada_set_input(&input);
            pthread_t reader, changer;
            CHECK(pthread_create(&reader, NULL, readInput, NULL) == 0);
            waitForPins(1);
            CHECK(pthread_create(&changer, NULL, changeInput, NULL) == 0);
            waitForPins(2);

            /* All of this must finish before supplying the blocked input. */
            AdaFile* other = NULL;
            __ada_file_open(&other, ADA_MODE_INOUT, "", 0, 1);
            CHECK(other != NULL && other != input);
            __ada_set_output(&other);
            __ada_put_line("independent", 11);
            __ada_file_reset(&other, ADA_MODE_INOUT);
            __ada_set_input(&other);
            char text[16];
            int last = 0;
            __ada_get_line(text, sizeof text, &last);
            CHECK(last == 11 && memcmp(text, "independent", 11) == 0);
            __ada_file_close(&other);
            CHECK(__ada_exception == NULL);

            CHECK(write(ends[1], "7 ", 2) == 2);
            CHECK(close(ends[1]) == 0);
            CHECK(pthread_join(reader, NULL) == 0);
            CHECK(pthread_join(changer, NULL) == 0);
            if (resetFile) { __ada_file_close(&input); }
            CHECK(input == NULL && __ada_exception == NULL);
        }
    }
    return 0;
}
