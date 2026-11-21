#include <stddef.h>

typedef struct Sample
{
    int value;
    short code;
    unsigned char padding[10];
} Sample;

_Static_assert(sizeof(Sample) == 16, "C sample stride");
_Static_assert(offsetof(Sample, code) == 4, "C sample field layout");

static Sample samples[] = {
    {7, 10, {1}},
    {9, 20, {2}},
    {0, 0, {3}}
};

Sample* aida_c_pointer_sample(ptrdiff_t offset)
{
    return samples + offset;
}

int aida_c_pointer_change(Sample* first, Sample* selected, ptrdiff_t distance)
{
    if (first != samples || selected != samples + 1 || distance != 1) {
        return 0;
    }
    selected->value = 31;
    return 1;
}

char** aida_c_pointer_names(void)
{
    static char first[] = "first";
    static char second[] = "second";
    static char* names[] = {first, second, NULL};
    return names;
}
