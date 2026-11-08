/* Diagnostic, single-thread benchmark census only; never timed or installed. */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { kCounterCapacity = 64 };
struct Counter
{
    const char* name;
    uint64_t calls;
    uint64_t empty;
};
static struct Counter counters[kCounterCapacity];
static size_t counterCount;

static void dumpCounters(void)
{
    printf("{");
    for (size_t i = 0; i < counterCount; ++i) {
        printf("%s\"%s\":{\"calls\":%" PRIu64 ",\"empty\":%" PRIu64 "}",
               i == 0 ? "" : ",", counters[i].name, counters[i].calls, counters[i].empty);
    }
    printf("}\n");
}

void __ada_profile_count(const char* name, int empty)
{
    if (counterCount == 0 && atexit(dumpCounters) != 0) { abort(); }
    size_t index = 0;
    while (index < counterCount && strcmp(counters[index].name, name) != 0) { ++index; }
    if (index == counterCount) {
        if (counterCount == kCounterCapacity) { abort(); }
        counters[counterCount++].name = name;
    }
    ++counters[index].calls;
    counters[index].empty += empty != 0;
}
