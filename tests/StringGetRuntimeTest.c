#include "../runtime/adart.h"
#include "../runtime/adaio.h"
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { \
    fprintf(stderr, "string Get runtime check failed at line %d\n", __LINE__); abort(); \
} } while (0)

int main(void)
{
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    AdaFile* file = NULL;
    char text[6] = {0};
    __ada_text_get_string(&file, NULL, 0);
    CHECK(context.exception == NULL);
    __ada_text_get_string(&file, text, 1);
    CHECK(context.exception == ADA_STATUS_ERROR);
    context.exception = NULL;
    __ada_file_open(&file, ADA_MODE_OUT, "", 0, 1);
    CHECK(context.exception == NULL);
    __ada_text_get_string(&file, NULL, 0);
    CHECK(context.exception == NULL && ftell(file->stream) == 0);
    __ada_text_get_string(&file, text, 1);
    CHECK(context.exception == ADA_MODE_ERROR);
    context.exception = NULL;
    __ada_text_get_string(&file, text, -1);
    CHECK(context.exception == ADA_CONSTRAINT_ERROR);
    context.exception = NULL;
    const char payload[] = {'\n', '\f', 'a', ' ', 'b', '\n', '\f', 0, 'c'};
    CHECK(fwrite(payload, 1, sizeof payload, file->stream) == sizeof payload);
    __ada_file_reset(&file, ADA_MODE_IN);
    __ada_set_input(&file);
    __ada_get_string(text, 5);
    CHECK(context.exception == NULL && memcmp(text, "a b\0c", 5) == 0);
    __ada_get_string(NULL, 0);
    CHECK(context.exception == NULL);
    __ada_get_string(text, 1);
    CHECK(context.exception == ADA_END_ERROR);
    context.exception = NULL;
    __ada_file_delete(&file);
    CHECK(context.exception == NULL && file == NULL);

    /* A valid Ada input handle over a write-only host stream simulates a
       backend read failure. It must report Device_Error rather than EOF. */
    static const char kPath[] = "string-get-runtime.tmp";
    __ada_file_open(&file, ADA_MODE_OUT, kPath, sizeof kPath - 1, 1);
    CHECK(context.exception == NULL && fclose(file->stream) == 0);
    file->stream = fopen(kPath, "wb");
    CHECK(file->stream != NULL);
    file->mode = ADA_MODE_IN;
    __ada_text_get_string(&file, text, 1);
    CHECK(context.exception == ADA_DEVICE_ERROR && ferror(file->stream));
    context.exception = NULL;
    __ada_file_delete(&file);
    CHECK(context.exception == NULL && file == NULL);
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return 0;
}
