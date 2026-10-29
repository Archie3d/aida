#if defined(__APPLE__)
#define _DARWIN_C_SOURCE
#endif
#define _POSIX_C_SOURCE 200809L
#include "../runtime/adart.h"
#include "../runtime/adacalendar.h"
#include "../runtime/adahost.h"
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "host library check failed at line %d\n", __LINE__); abort(); \
} } while (0)

static void checkString(AdaArrayResult* result, const char* expected)
{
    CHECK(result->first == 1 && result->length == (int64_t)strlen(expected));
    CHECK(memcmp(result->data, expected, (size_t)result->length) == 0);
    free(result->data);
    memset(result, 0, sizeof *result);
}

static void* reader(void* argument)
{
    const char* root = argument;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    for (int i = 0; i < 100; ++i) {
        AdaArrayResult result = {0};
        __ada_host_env_value(&result, "AIDA_HOST_C_TEST", 16);
        CHECK(context.exception == NULL);
        checkString(&result, "a host value");
        HostSearch* search = __ada_host_search_start(root, (int)strlen(root), "*", 1, 7);
        CHECK(context.exception == NULL && search != NULL);
        while (__ada_host_search_more(search)) {
            __ada_host_search_name(&result, search);
            CHECK(context.exception == NULL && result.length > 0);
            free(result.data);
            __ada_host_search_step(search);
        }
        __ada_host_search_release(search);
        CHECK(__ada_calendar_clock() != INT64_MIN);
    }
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaArrayResult result = {0};
    const int64_t scale = INT64_C(1) << 30;
    int y, m, d, valid;
    int64_t seconds;
    for (int year = 1901; year <= 2399; ++year) {
        int64_t date = __ada_calendar_compose(year, 3, 1, scale / 4);
        CHECK(date != INT64_MIN);
        __ada_calendar_split(date, &y, &m, &d, &seconds, &valid);
        CHECK(valid && y == year && m == 3 && d == 1 && seconds == scale / 4);
        int leap = year % 4 == 0 && (year % 100 != 0 || year % 400 == 0);
        CHECK((__ada_calendar_compose(year, 2, 29, 0) != INT64_MIN) == leap);
    }
    __ada_calendar_split(INT64_MIN, &y, &m, &d, &seconds, &valid);
    CHECK(!valid);
    CHECK(__ada_calendar_add(__ada_calendar_compose(1901, 1, 1, 0), INT64_MIN, 1) != INT64_MIN);
    CHECK(__ada_calendar_compose(2399, 12, 31, 86400 * scale) == INT64_MIN);
    __ada_calendar_difference(INT64_MAX, INT64_MIN, &seconds, &valid);
    CHECK(!valid);
    __ada_calendar_delay_until(&context, __ada_calendar_clock() - 1);
    CHECK(context.exception == NULL);
    context.protectedActions = (AdaProtectedAction*)&context;
    __ada_calendar_delay_until(&context, 0);
    CHECK(context.exception == ADA_PROGRAM_ERROR);
    context.protectedActions = NULL;
    context.exception = NULL;
    __ada_host_env_set("AIDA_HOST_C_TEST", 16, "a host value", 12);
    __ada_host_env_value(&result, "AIDA_HOST_C_TEST", 16);
    checkString(&result, "a host value");
    __ada_host_env_set("bad=name", 8, "value", 5);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR);
    context.exception = NULL;
    __ada_host_exists("a\0b", 3);
    CHECK(context.exception == ADA_NAME_ERROR);
    context.exception = NULL;

    char root[] = "/tmp/aida-host-XXXXXX";
    CHECK(mkdtemp(root) != NULL);
    char nested[256], outside[256], linkName[256], source[256], target[256];
    snprintf(nested, sizeof nested, "%s/nested", root);
    snprintf(outside, sizeof outside, "%s/outside", root);
    snprintf(linkName, sizeof linkName, "%s/nested/link", root);
    snprintf(source, sizeof source, "%s/source", root);
    snprintf(target, sizeof target, "%s/target", root);
    CHECK(mkdir(nested, 0700) == 0 && mkdir(outside, 0700) == 0);
    CHECK(symlink(outside, linkName) == 0);
    FILE* file = fopen(source, "wb");
    CHECK(file != NULL && fwrite("payload", 1, 7, file) == 7 && fclose(file) == 0);
    CHECK(link(source, target) == 0);
    __ada_host_copy_file(source, (int)strlen(source), target, (int)strlen(target), "", 0);
    CHECK(context.exception == ADA_USE_ERROR && __ada_host_size(source, (int)strlen(source)) == 7);
    context.exception = NULL;
    AdaThread threads[4];
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_create(&threads[i], reader, root) == 0);
    }
    for (int i = 0; i < 4; ++i) {
        CHECK(__ada_thread_join(threads[i], NULL) == 0);
    }
    char throughLink[256];
    snprintf(throughLink, sizeof throughLink, "%s/../missing", linkName);
    __ada_host_full_name(&result, throughLink, (int)strlen(throughLink));
    char expected[256];
    snprintf(expected, sizeof expected, "%s/missing", root);
#if defined(__APPLE__)
    char canonical[256];
    snprintf(canonical, sizeof canonical, "/private%s", expected);
    checkString(&result, canonical);
#else
    checkString(&result, expected);
#endif
    __ada_host_delete_tree(nested, (int)strlen(nested));
    CHECK(context.exception == NULL && __ada_host_exists(outside, (int)strlen(outside)));
    CHECK(!__ada_host_exists(nested, (int)strlen(nested)));
    struct timespec oldTimes[2] = {{-INT64_C(2208988801), 0}, {-INT64_C(2208988801), 0}};
    CHECK(utimensat(AT_FDCWD, source, oldTimes, 0) == 0);
    HostSearch* oldSearch = __ada_host_search_start(root, (int)strlen(root), "source", 6, 7);
    CHECK(oldSearch != NULL && __ada_host_search_more(oldSearch));
    __ada_host_search_time(oldSearch, &y, &m, &d, &seconds, &valid);
    CHECK(!valid && context.exception == NULL && __ada_host_search_size(oldSearch) == 7);
    __ada_host_search_release(oldSearch);
    __ada_host_modification_time(source, (int)strlen(source), &y, &m, &d, &seconds);
    CHECK(context.exception == ADA_USE_ERROR);
    context.exception = NULL;
    for (int i = 0; i < 200; ++i) {
        HostSearch* search = __ada_host_search_start(root, (int)strlen(root), "*", 1, 7);
        CHECK(search != NULL);
        __ada_host_search_release(search);
    }
    __ada_host_delete_tree(root, (int)strlen(root));
    __ada_host_env_clear("AIDA_HOST_C_TEST", 16);
    CHECK(context.exception == NULL && __ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
