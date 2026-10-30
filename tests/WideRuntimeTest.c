#include "../runtime/adart.h"
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(condition) do { if (!(condition)) { fprintf(stderr, "wide runtime check failed at %d\n", __LINE__); abort(); } } while (0)

static void exercise(void)
{
    uint32_t words[32];
    uint16_t wide[32];
    char bytes[32];
    for (int code = 0; code <= 65535; ++code) {
        int length = __ada_wide_character_image(wide, 32, code, 2);
        CHECK(length >= 2 && length <= 12);
        CHECK(__ada_wide_character_value(wide, length, 2, 65535) == code);
        if (code < 256) {
            length = __ada_image_character(bytes, 32, code);
            CHECK(__ada_value_character(bytes, length) == code);
        }
    }
    const int kValues[] = {65536, 0x1F600, 0x10FFFF, 0x7FFFFFFF};
    for (size_t i = 0; i < sizeof kValues / sizeof kValues[0]; ++i) {
        int length = __ada_wide_character_image(words, 32, kValues[i], 4);
        CHECK(__ada_wide_character_value(words, length, 4, INT_MAX) == kValues[i]);
        length = __ada_wide_character_image(bytes, 32, kValues[i], 1);
        CHECK(__ada_wide_character_value(bytes, length, 1, INT_MAX) == kValues[i]);
    }
    CHECK(__ada_wide_character_width(0, 65535, 2) == 12);
    CHECK(__ada_wide_character_width(0, INT_MAX, 4) == 12);
    CHECK(__ada_wide_character_width(0x391, 0x3A9, 2) == 3);
    CHECK(__ada_wide_character_width(2, 1, 4) == 0);
    CHECK(__ada_identifier_equal("Stra\xC3\x9F" "e", 7, "STRASSE", 7, 1));
    CHECK(__ada_identifier_equal("\xCE\xB1", 2, "\xCE\x91", 2, 1));
    CHECK(!__ada_identifier_equal("\xCE\xB1", 2, "\xCE\x91", 2, 0));
    CHECK(__ada_identifier_equal("A\xE2\x80\x8C" "B", 5, "AB", 2, 1));
    CHECK(!__ada_identifier_equal("\xE2\x80\x8C" "AB", 5, "AB", 2, 1));
    const char* names[] = {"mix_\xCE\x91", "'a'"};
    CHECK(__ada_image_enum(bytes, 32, 0, names, 2) == 6);
    CHECK(memcmp(bytes, "MIX_\xCE\x91", 6) == 0);
    CHECK(__ada_image_enum(bytes, 32, 1, names, 2) == 3);
    CHECK(memcmp(bytes, "'a'", 3) == 0);
    CHECK(__ada_image_widen(wide, 32, "A\xF0\x9F\x98\x80", 5, 2) == 13);
    CHECK(__ada_image_widen(words, 32, "A\xF0\x9F\x98\x80", 5, 4) == 2 && words[1] == 0x1F600);
    AdaArrayResult result = {0};
    __ada_wide_to_utf8(&result, words, 2, 4);
    CHECK(result.length == 5 && memcmp(result.data, "A\xF0\x9F\x98\x80", 5) == 0);
    free(result.data);
    CHECK(__ada_exception == NULL);
    __ada_wide_character_value("'\t'", 3, 1, 255);
    CHECK(__ada_exception == ADA_CONSTRAINT_ERROR);
    __ada_exception = NULL;
    __ada_wide_character_value("Hex_FFFFFFFF", 12, 1, INT_MAX);
    CHECK(__ada_exception == ADA_CONSTRAINT_ERROR);
    __ada_exception = NULL;
    __ada_wide_character_image(bytes, 1, 65, 1);
    CHECK(__ada_exception == ADA_STORAGE_ERROR);
    __ada_exception = NULL;
    words[0] = 0xD800;
    __ada_wide_to_utf8(&result, words, 1, 4);
    CHECK(__ada_exception == ADA_CONSTRAINT_ERROR);
    __ada_exception = NULL;
}

static void* worker(void* unused)
{
    (void)unused;
    AdaTaskContext context = {0};
    AdaTaskContext* previous = __ada_task_context_bind(&context);
    exercise();
    CHECK(__ada_task_context_dispose(&context));
    __ada_task_context_bind(previous);
    return NULL;
}

int main(void)
{
    exercise();
    pthread_t threads[4];
    for (int i = 0; i < 4; ++i) { CHECK(pthread_create(&threads[i], NULL, worker, NULL) == 0); }
    for (int i = 0; i < 4; ++i) { CHECK(pthread_join(threads[i], NULL) == 0); }
    return 0;
}
