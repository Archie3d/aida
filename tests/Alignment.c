#include <stdint.h>

int __test_aligned(const void* address, int64_t alignment)
{
    return (uintptr_t)address % (uintptr_t)alignment == 0;
}
