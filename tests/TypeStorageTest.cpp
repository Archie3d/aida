#include "../adac/Type.h"
#include "../adac/Symbol.h"

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
    derived->parentType = types.longIntegerType();
    derived->byteSize = 1;
    CHECK(qbeClass(derived) == 'l');
    Type* code = types.create(TypeKind::Enumeration, "Code");
    code->low = 0;
    code->high = 2;
    code->literals = {"Red", "Green", "Blue"};
    code->enumCodes = {-1000, 250, 65535};
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
    record->compositeSize = 16;
    CHECK(typeSize(record) == 16 && typeLayoutSize(record) == 8 && typeAlignment(record) == 4);
    Type* roomy = types.makeSubtype("Roomy", record, 0, 0);
    roomy->compositeSize = 32;
    Type* array = types.create(TypeKind::Array, "Array");
    array->arrayRank = 1;
    array->constrained = true;
    array->indexLow = 1;
    array->indexHigh = 3;
    array->element = roomy;
    array->compositeSize = 128;
    array->sizeBounds = {{1, 3}};
    CHECK(typeSize(array) == 128 && typeLayoutSize(array) == 96);
    Type* slice = types.makeSubtype("Slice", array, 0, 0);
    slice->indexHigh = 2;
    CHECK(typeSize(slice) == 64 && typeLayoutSize(slice) == 64);
    slice->indexHigh = 0;
    CHECK(typeSize(slice) == 0);

    // The hidden actual-constraint argument depends on the nominal subtype
    // and mode; a constrained view must not inherit a mutable formal's ABI.
    Type* discriminated = types.create(TypeKind::Record, "Discriminated");
    IntegerLiteralExpr defaultValue;
    FieldInfo discriminant;
    discriminant.type = types.integerType();
    discriminant.isDiscriminant = true;
    discriminant.defaultValue = &defaultValue;
    discriminated->fields = {discriminant};
    discriminated->discriminantCount = 1;
    Type* alias = types.makeSubtype("Alias", discriminated, 0, 0);
    Symbol formal;
    formal.type = alias;
    CHECK(hasDefaultDiscriminants(alias));
    CHECK(!needsConstrainedParameter(&formal));
    formal.mode = ParameterMode::InOut;
    CHECK(needsConstrainedParameter(&formal));
    formal.mode = ParameterMode::Out;
    CHECK(needsConstrainedParameter(&formal));
    Type* fixed = types.makeSubtype("Fixed", alias, 0, 0);
    fixed->discriminantsKnown = true;
    fixed->discriminantValues = {1};
    formal.type = fixed;
    CHECK(!needsConstrainedParameter(&formal));
    Type* inherited = types.makeSubtype("Inherited", alias, 0, 0);
    inherited->discriminantConstraintSource = fixed;
    formal.type = inherited;
    CHECK(!needsConstrainedParameter(&formal));
    discriminated->fields.front().defaultValue = nullptr;
    formal.type = alias;
    CHECK(!hasDefaultDiscriminants(alias) && !needsConstrainedParameter(&formal));

    Type* perObject = types.create(TypeKind::Record, "PerObject");
    perObject->dependentStorage = true;
    FieldInfo length;
    length.type = types.integerType();
    length.offset = 16;
    FieldInfo text;
    text.type = types.stringType();
    text.offset = 24;
    text.dependentBounds = true;
    perObject->fields = {length, text};
    CHECK(typeSize(perObject) == 64 && typeAlignment(perObject) == 8);
    CHECK(needsFinalization(perObject) && hasDependentStorage(perObject));
    CHECK(!hasRuntimeLayout(perObject));
    Type* perObjectAlias = types.makeSubtype("PerObjectAlias", perObject, 0, 0);
    CHECK(perObjectAlias->dependentStorage && typeSize(perObjectAlias) == 64);
    text.offset = 64;
    text.dependentBounds = false;
    perObject->fields.push_back(text);
    CHECK(hasRuntimeLayout(perObject));
    // Explicit slots determine extent independently of a component's value size.
    Type* represented = types.create(TypeKind::Record, "Represented");
    FieldInfo slot;
    slot.type = types.integerType();
    slot.offset = 16;
    slot.representationSize = 12;
    represented->fields = {slot};
    CHECK(fieldStorageSize(slot) == 12 && typeLayoutSize(represented) == 28);
    slot.type = types.stringType();
    slot.dependentBounds = true;
    slot.inlineDependent = true;
    slot.representationSize = 8;
    represented->fields = {slot};
    CHECK(typeSize(represented) == 24 && !hasRuntimeLayout(represented));
    CHECK(!needsFinalization(represented) && !hasDependentStorage(represented));
    Type* variants = types.create(TypeKind::Record, "Variants");
    variants->variants = {
        {-1, 0, {}, false}, {-1, 0, {}, true},
        {0, 1, {}, false}, {0, 1, {}, true}
    };
    CHECK(variantsExclusive(variants, 0, 1));
    CHECK(variantsExclusive(variants, 2, 1));
    CHECK(variantsExclusive(variants, 2, 3));
    CHECK(!variantsExclusive(variants, -1, 2));
    CHECK(!variantsExclusive(variants, 0, 2));
    CHECK(!variantsExclusive(variants, 2, 2));
    return 0;
}
