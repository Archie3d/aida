#include <stdint.h>
#include <string.h>

int __test_enum_raw(const void* address, int bytes, int isSigned)
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
    int32_t value;
    memcpy(&value, address, sizeof(value));
    return value;
}

int __test_enum_echo(int code)
{
    return code;
}

int __test_enum_blue(void)
{
    return 250;
}

void __test_enum_set_blue(void* address)
{
    int16_t value = 250;
    memcpy(address, &value, sizeof(value));
}

void __test_enum_invalid(void* address)
{
    int16_t value = 44;
    memcpy(address, &value, sizeof(value));
}

int __test_enum_invalid_result(void)
{
    return 44;
}
