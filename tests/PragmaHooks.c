#include <stdlib.h>

void __ada_test_no_return_returns(int value)
{
    if (value != 42) {
        abort();
    }
}

void __ada_test_no_return_invoke(void (*action)(int), int value)
{
    action(value);
}
