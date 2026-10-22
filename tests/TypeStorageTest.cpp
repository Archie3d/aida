#include "../adac/Type.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

#define CHECK(condition) do { if (!(condition)) { \
    std::fprintf(stderr, "type storage check failed at line %d\n", __LINE__); std::abort(); \
} } while (0)

int main()
{
    TypeTable types;
    Type* byte = types.makeSubtype("Byte", types.longIntegerType(), -128, 127);
    byte->byteSize = 1;
    CHECK(typeSize(byte) == 1 && typeAlignment(byte) == 1 && qbeClass(byte) == 'l');
    CHECK(std::strcmp(qbeLoadInstruction(byte), "loadsb") == 0);
    Type* wide = types.makeSubtype("Wide", types.integerType(), -128, 127);
    wide->byteSize = 8;
    CHECK(typeSize(wide) == 8 && qbeClass(wide) == 'w');
    Type* derived = types.create(TypeKind::Integer, "Derived");
    derived->m_parentType = types.longIntegerType();
    derived->byteSize = 1;
    CHECK(qbeClass(derived) == 'l');
    Type* code = types.create(TypeKind::Enumeration, "Code");
    code->low = 0;
    code->high = 2;
    code->literals = {"Red", "Green", "Blue"};
    code->m_enumCodes = {-1000, 250, 65535};
    code->byteSize = 4;
    Type* green = types.makeSubtype("Green", code, 1, 1);
    green->byteSize = 1;
    CHECK(!scalarStorageSigned(green));
    CHECK(std::strcmp(qbeLoadInstruction(green), "loadub") == 0);
    Type* record = types.create(TypeKind::Record, "Record");
    FieldInfo first;
    first.type = byte;
    FieldInfo second;
    second.type = types.integerType();
    second.offset = 4;
    record->fields = {first, second};
    record->m_compositeSize = 16;
    CHECK(typeSize(record) == 16 && typeLayoutSize(record) == 8 && typeAlignment(record) == 4);
    Type* roomy = types.makeSubtype("Roomy", record, 0, 0);
    roomy->m_compositeSize = 32;
    Type* array = types.create(TypeKind::Array, "Array");
    array->arrayRank = 1;
    array->constrained = true;
    array->indexLow = 1;
    array->indexHigh = 3;
    array->element = roomy;
    array->m_compositeSize = 128;
    array->m_sizeBounds = {{1, 3}};
    CHECK(typeSize(array) == 128 && typeLayoutSize(array) == 96);
    Type* slice = types.makeSubtype("Slice", array, 0, 0);
    slice->indexHigh = 2;
    CHECK(typeSize(slice) == 64 && typeLayoutSize(slice) == 64);
    slice->indexHigh = 0;
    CHECK(typeSize(slice) == 0);
    return 0;
}
