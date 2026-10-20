#include "../runtime/adart.h"

void __test_shared_increment(int* value)
{
    ++*value;
}

void __test_shared_fail(int* value)
{
    *value = 99;
    __ada_raise_message(ADA_CONSTRAINT_ERROR, "shared import failed", 20);
}

void __test_shared_out(int* value)
{
    *value = 99;
}
