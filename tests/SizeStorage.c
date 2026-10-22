#include <stdint.h>
#include <string.h>

int64_t __test_size_value(const void* address, int bytes, int isSigned)
{
    if (bytes == 1) {
        uint8_t value;
        memcpy(&value, address, sizeof(value));
        return isSigned ? (int8_t)value : value;
    }
    if (bytes == 2) {
        uint16_t value;
        memcpy(&value, address, sizeof(value));
        return isSigned ? (int16_t)value : value;
    }
    if (bytes == 4) {
        int32_t value;
        memcpy(&value, address, sizeof(value));
        return value;
    }
    int64_t value;
    memcpy(&value, address, sizeof(value));
    return value;
}

int64_t __test_size_offset(const void* first, const void* second)
{
    return (const unsigned char*)second - (const unsigned char*)first;
}

void __test_size_touch_roomy(void* address)
{
    int32_t value = 77;
    memcpy(address, &value, sizeof(value));
    ((unsigned char*)address)[31] = 90;
}

int __test_size_touch_derived(void* address)
{
    ((unsigned char*)address)[63] = 90;
    return 1;
}

void __test_size_set_long(void* address)
{
    int64_t value = -99;
    memcpy(address, &value, sizeof(value));
}

void __test_size_bad_long(void* address)
{
    int64_t value = 1000;
    memcpy(address, &value, sizeof(value));
}

void __test_size_touch(void* address, int64_t count)
{
    ((unsigned char*)address)[count - 1] = 90;
}
