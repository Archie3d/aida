#include <stdlib.h>

typedef int (*Callback)(int);

int __test_callback_call(Callback callback, int value)
{
    return callback(value);
}

Callback __test_callback_get(void)
{
    return abs;
}

float __test_callback_float(float value)
{
    return value + 0.5f;
}

double __test_callback_double(double value)
{
    return value + 0.25;
}

void __test_callback_update(int* value)
{
    *value += 7;
}
