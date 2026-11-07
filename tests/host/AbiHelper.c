#include <stdint.h>
#include <string.h>

int32_t __ada_host_abi_check(int32_t narrow, int64_t wide, float single,
                           double real, void* address, int32_t tail1,
                           int64_t tail2, double tail3, int64_t tail4,
                           int32_t stack1, int64_t stack2, int32_t stack3, int64_t stack4)
{
    int64_t value;
    memcpy(&value, address, sizeof(value));
    if (narrow != INT32_MIN || wide != INT64_C(0x123456789abcdef)
        || single != 1.25f || real != -2.5 || value != INT64_C(-0x123456789)
        || tail1 != 37 || tail2 != INT64_C(-0x123456789abcdef)
        || tail3 != 9.75 || tail4 != INT64_MAX || stack1 != -77
        || stack2 != INT64_C(0x23456789abcdef) || stack3 != INT32_MAX || stack4 != INT64_MIN) {
        return 0;
    }
    value = INT64_C(0x102030405060708);
    memcpy(address, &value, sizeof(value));
    return 1;
}

uint64_t __ada_host_abi_unsigned(uint64_t value)
{
    return value ^ UINT64_C(0x8000000000000001);
}

void* __ada_host_abi_address(void* value)
{
    return value;
}
