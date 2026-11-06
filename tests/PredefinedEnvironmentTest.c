#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700
#define _DEFAULT_SOURCE 1
#define _DARWIN_C_SOURCE 1
#include "../runtime/adart.h"
#include "../runtime/adaio.h"
#include "../runtime/adathread.h"

#include <fenv.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>
#endif

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "predefined environment check failed at line %d\n", __LINE__); abort(); \
} } while (0)

enum { kWorkerCount = 4, kIterations = 200 };

static void* formatWorker(void* argument)
{
    (void)argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    int savedRounding = fegetround();
    CHECK(fesetround(FE_TONEAREST) == 0);
    for (int i = 0; i < kIterations; ++i) {
        char text[80];
        __ada_format_float(text, sizeof text, 1.25, 2, 2, 3);
        CHECK(strcmp(text, " 1.25E+00") == 0);
        __ada_format_float(text, sizeof text, 1.125, 2, 2, 0);
        CHECK(strcmp(text, " 1.12") == 0);
        __ada_format_float(text, sizeof text, -1.375, 2, 2, 0);
        CHECK(strcmp(text, "-1.38") == 0);
        __ada_format_float(text, sizeof text, -0.0, 2, 2, 0);
        CHECK(strcmp(text, "-0.00") == 0);
        CHECK(context.exception == NULL);
    }
    CHECK(fesetround(savedRounding) == 0);
    __ada_task_context_dispose(&context);
    __ada_task_context_bind(previous);
    return NULL;
}

#ifndef _WIN32
typedef struct TerminalRead
{
    AdaFile* file;
    char item;
} TerminalRead;

static tcflag_t persistentTerminalFlags(tcflag_t flags)
{
    // Kernels can mark pending input for reprocessing after a canonical-mode
    // transition; that transient flag is not a lost terminal setting.
#ifdef PENDIN
    flags &= ~PENDIN;
#endif
    return flags;
}

static void* terminalReader(void* argument)
{
    TerminalRead* read = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    __ada_text_get_immediate(&read->file, &read->item);
    CHECK(context.exception == NULL);
    __ada_task_context_dispose(&context);
    __ada_task_context_bind(previous);
    return NULL;
}

static void checkTerminal(void)
{
    int master = posix_openpt(O_RDWR | O_NOCTTY);
    CHECK(master >= 0 && grantpt(master) == 0 && unlockpt(master) == 0);
    const char* name = ptsname(master);
    CHECK(name != NULL);
    int slave = open(name, O_RDWR | O_NOCTTY);
    CHECK(slave >= 0);
    struct termios original, saved, observed;
    CHECK(tcgetattr(slave, &original) == 0);
    saved = original;
    saved.c_lflag |= ICANON | ECHO;
    saved.c_iflag &= ~(ICRNL | INLCR | IGNCR);
    CHECK(tcsetattr(slave, TCSANOW, &saved) == 0);
    AdaFile file = {0};
    file.stream = fdopen(slave, "r+");
    CHECK(file.stream != NULL);
    file.isOpen = file.text = 1;
    file.column = file.line = file.page = 1;
    CHECK(setvbuf(file.stream, NULL, _IONBF, 0) == 0);
    AdaFile* handle = &file;
    char item = '?';
    int available = -1;
    __ada_text_get_available(&handle, &item, &available);
    CHECK(__ada_exception == NULL && available == 0);
    CHECK(tcgetattr(slave, &observed) == 0
        && persistentTerminalFlags(observed.c_lflag) == persistentTerminalFlags(saved.c_lflag));
    TerminalRead read = {&file, '?'};
    AdaThread reader;
    CHECK(__ada_thread_create(&reader, terminalReader, &read) == 0);
    int raw = 0;
    for (int i = 0; i < 2000; ++i) {
        CHECK(tcgetattr(slave, &observed) == 0);
        if ((observed.c_lflag & (ICANON | ECHO)) == 0) { raw = 1; break; }
        struct timespec pause = {0, 1000000};
        nanosleep(&pause, NULL);
    }
    CHECK(raw);
    CHECK(write(master, "\r", 1) == 1);
    CHECK(__ada_thread_join(reader, NULL) == 0 && read.item == '\r');
    CHECK(tcgetattr(slave, &observed) == 0);
    CHECK(persistentTerminalFlags(observed.c_lflag) == persistentTerminalFlags(saved.c_lflag) && observed.c_iflag == saved.c_iflag
        && observed.c_oflag == saved.c_oflag && observed.c_cflag == saved.c_cflag);
    CHECK(memcmp(observed.c_cc, saved.c_cc, sizeof saved.c_cc) == 0);
    CHECK(file.column == 1 && file.line == 1 && file.page == 1);
    CHECK(tcsetattr(slave, TCSANOW, &original) == 0);
    CHECK(fclose(file.stream) == 0 && close(master) == 0);
}
#endif

int main(int argc, char** argv)
{
    if (argc == 2 && strcmp(argv[1], "terminal") == 0) {
#ifndef _WIN32
        checkTerminal();
        return 0;
#else
        return 1;
#endif
    }
    CHECK(setlocale(LC_NUMERIC, "C") != NULL);
    AdaThread workers[kWorkerCount];
    for (int i = 0; i < kWorkerCount; ++i) { CHECK(__ada_thread_create(&workers[i], formatWorker, NULL) == 0); }
    for (int i = 0; i < kWorkerCount; ++i) { CHECK(__ada_thread_join(workers[i], NULL) == 0); }
    return 0;
}
