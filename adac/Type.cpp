#include "Type.h"
#include "../common/Array.h"
#include "../common/Fixed.h"

#include <limits>
#include <algorithm>
#include <unordered_set>

Type* rootType(Type* type)
{
    while (type != nullptr && type->base != nullptr) {
        type = type->base;
    }
    return type;
}

Type* baseType(Type* type)
{
    while (type != nullptr && type->isSubtype && type->base != nullptr) {
        type = type->base;
    }
    return type;
}

bool isTaggedAncestor(Type* ancestor, Type* type)
{
    ancestor = rootType(ancestor);
    type = rootType(type);
    if (ancestor == nullptr || type == nullptr || !ancestor->tagged || !type->tagged) {
        return false;
    }
    for (; type != nullptr; type = type->parentType) {
        if (type == ancestor) {
            return true;
        }
    }
    return false;
}

bool isDiscrete(const Type* type)
{
    if (type == nullptr) {
        return false;
    }
    return type->kind == TypeKind::Integer || type->kind == TypeKind::Enumeration
           || type->kind == TypeKind::UniversalInteger;
}

bool isReal(const Type* type)
{
    if (type == nullptr) {
        return false;
    }
    return type->kind == TypeKind::Float || type->kind == TypeKind::UniversalReal;
}

bool isNumeric(const Type* type)
{
    if (type == nullptr) {
        return false;
    }
    return type->kind == TypeKind::Integer || type->kind == TypeKind::Float || type->kind == TypeKind::Fixed
           || type->kind == TypeKind::UniversalInteger || type->kind == TypeKind::UniversalReal;
}

bool isScalar(const Type* type)
{
    return isDiscrete(type) || (type != nullptr && (type->kind == TypeKind::Float || type->kind == TypeKind::Fixed || type->kind == TypeKind::Access
                                                    || type->kind == TypeKind::UniversalReal));
}

bool isComposite(const Type* type)
{
    return type != nullptr && (type->kind == TypeKind::Array || type->kind == TypeKind::Record);
}

bool needsZeroInit(const Type* type)
{
    if (type == nullptr) {
        return false;
    }
    if (type->needsZeroInit) {
        return true;
    }
    // An access object starts out null, so anything holding one has to be
    // cleared before it is used.
    if (type->kind == TypeKind::Access) {
        return true;
    }
    if (type->kind == TypeKind::Array) {
        return needsZeroInit(type->element);
    }
    if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : type->fields) {
            if (needsZeroInit(field.type)) {
                return true;
            }
        }
    }
    return false;
}

long long typeSize(const Type* type)
{
    if (type == nullptr) {
        return 0;
    }
    if (type->tagged && type->isSubtype && type->base != nullptr) {
        return typeSize(type->base);
    }
    if (type->compositeSize > 0) {
        bool matches = type->kind == TypeKind::Record;
        if (type->kind == TypeKind::Array && type->constrained
            && type->sizeBounds.size() == static_cast<std::size_t>(type->arrayRank)) {
            matches = true;
            const Type* axis = type;
            for (const auto& bounds : type->sizeBounds) {
                matches = matches && axis->constrained && axis->indexLow == bounds.first && axis->indexHigh == bounds.second;
                axis = axis->element;
            }
        }
        if (matches) { return type->compositeSize; }
    }
    if (type->byteSize > 0) {
        return type->byteSize;
    }
    switch (type->kind) {
    case TypeKind::Void:
        return 0;
    case TypeKind::UniversalInteger:
        return 8;
    case TypeKind::UniversalReal:
        return 8;
    case TypeKind::Integer:
        return type->base != nullptr ? typeSize(type->base)
            : (type->low < -2147483648LL || type->high > 2147483647LL ? 8 : 4);
    case TypeKind::Enumeration:
        return type->literals.size() > 256 ? 4 : 1;
    case TypeKind::Float:
        // Six decimal digits are what a single precision number holds.
        return type->digits > 6 ? 8 : 4;
    case TypeKind::Fixed:
    case TypeKind::Access:
        return 8;
    case TypeKind::Array: {
        long long count = arrayLength(type);
        long long elementSize = typeSize(type->element);
        long long alignment = typeAlignment(type->element);
        if (elementSize % alignment != 0) {
            if (__builtin_add_overflow(elementSize, alignment - elementSize % alignment, &elementSize)) {
                return std::numeric_limits<long long>::max();
            }
        }
        long long size;
        return __builtin_mul_overflow(count, elementSize, &size) ? std::numeric_limits<long long>::max() : size;
    }
    case TypeKind::Record: {
        long long size = type->tagged || type->protectedScope != nullptr ? 8 : 0;
        long long alignment = typeAlignment(type);
        for (const FieldInfo& field : type->fields) {
            // The alternatives of a variant part share their storage, so the
            // record reaches as far as the largest of them and no further.
            long long end;
            long long fieldSize = field.dependentBounds ? adaArrayLocalSize(field.type->arrayRank) : typeSize(field.type);
            if (__builtin_add_overflow(field.offset, fieldSize, &end)) {
                return std::numeric_limits<long long>::max();
            }
            if (end > size) {
                size = end;
            }
        }
        if (alignment > 0 && size % alignment != 0) {
            if (__builtin_add_overflow(size, alignment - size % alignment, &size)) {
                return std::numeric_limits<long long>::max();
            }
        }
        return size;
    }
    }
    return 0;
}

// Size clauses add trailing storage without changing component offsets.
long long typeLayoutSize(const Type* type)
{
    if (type == nullptr || !isComposite(type)) { return typeSize(type); }
    Type layout = *type;
    layout.compositeSize = 0;
    return typeSize(&layout);
}

bool scalarStorageSigned(const Type* type)
{
    if (type->kind == TypeKind::Integer) { return type->low < 0; }
    return !type->enumCodes.empty() && type->low >= 0
        && type->low < static_cast<long long>(type->enumCodes.size())
        && type->enumCodes[static_cast<std::size_t>(type->low)] < 0;
}

long long typeAlignment(const Type* type)
{
    if (type == nullptr) {
        return 1;
    }
    if (type->alignment > 0) {
        return type->alignment;
    }
    if (type->taskBody != nullptr) {
        return 8;
    }
    switch (type->kind) {
    case TypeKind::Array:
        return typeAlignment(type->element);
    case TypeKind::Record: {
        long long alignment = type->tagged || type->dependentStorage || type->protectedScope != nullptr ? 8 : 1;
        for (const FieldInfo& field : type->fields) {
            long long fieldAlignment = typeAlignment(field.type);
            if (fieldAlignment > alignment) {
                alignment = fieldAlignment;
            }
        }
        return alignment;
    }
    default:
        return typeSize(type) == 0 ? 1 : typeSize(type);
    }
}

long long arrayLength(const Type* type)
{
    if (type == nullptr || type->kind != TypeKind::Array || !type->constrained) {
        return 0;
    }
    if (type->indexHigh < type->indexLow) { return 0; }
    __int128 length = (__int128)type->indexHigh - type->indexLow + 1;
    return length > std::numeric_limits<long long>::max() ? std::numeric_limits<long long>::max() : (long long)length;
}

char qbeClass(const Type* type)
{
    if (type == nullptr) {
        return 'w';
    }
    switch (type->kind) {
    case TypeKind::Float:
        return typeSize(type) == 8 ? 'd' : 's';
    case TypeKind::UniversalReal:
        return 'd';
    case TypeKind::Integer: {
        const Type* valueType = type;
        while (valueType->isSubtype && valueType->base != nullptr) { valueType = valueType->base; }
        return typeSize(valueType) == 8
            || (valueType->parentType != nullptr && qbeClass(valueType->parentType) == 'l') ? 'l' : 'w';
    }
    case TypeKind::UniversalInteger:
    case TypeKind::Fixed:
    case TypeKind::Access:
    case TypeKind::Array:
    case TypeKind::Record:
        return 'l';
    default:
        return 'w';
    }
}

const char* qbeLoadInstruction(const Type* type)
{
    switch (typeSize(type)) {
    case 1:
        return scalarStorageSigned(type) ? "loadsb" : "loadub";
    case 2:
        return scalarStorageSigned(type) ? "loadsh" : "loaduh";
    case 8:
        if (isReal(type)) {
            return "loadd";
        }
        return "loadl";
    default:
        if (isReal(type)) {
            return "loads";
        }
        return "loadsw";
    }
}

const char* qbeStoreInstruction(const Type* type)
{
    switch (typeSize(type)) {
    case 1:
        return "storeb";
    case 2:
        return "storeh";
    case 8:
        if (isReal(type)) {
            return "stored";
        }
        return "storel";
    default:
        if (isReal(type)) {
            return "stores";
        }
        return "storew";
    }
}

TypeTable::TypeTable()
{
    m_void = create(TypeKind::Void, "void");

    m_universalInteger = create(TypeKind::UniversalInteger, "universal_integer");
    m_universalInteger->low = std::numeric_limits<long long>::min();
    m_universalInteger->high = std::numeric_limits<long long>::max();

    m_universalReal = create(TypeKind::UniversalReal, "universal_real");

    m_integer = create(TypeKind::Integer, "Integer");
    m_integer->low = -2147483648LL;
    m_integer->high = 2147483647LL;

    m_longInteger = create(TypeKind::Integer, "Long_Integer");
    m_longInteger->low = std::numeric_limits<long long>::min();
    m_longInteger->high = std::numeric_limits<long long>::max();

    m_natural = makeSubtype("Natural", m_integer, 0, 2147483647LL);
    m_positive = makeSubtype("Positive", m_integer, 1, 2147483647LL);

    m_boolean = create(TypeKind::Enumeration, "Boolean");
    m_boolean->literals = { "false", "true" };
    m_boolean->low = 0;
    m_boolean->high = 1;

    m_character = create(TypeKind::Enumeration, "Character");
    m_character->low = 0;
    m_character->high = 255;

    m_float = create(TypeKind::Float, "Float");
    m_float->digits = 6;

    m_longFloat = create(TypeKind::Float, "Long_Float");
    m_longFloat->digits = 15;

    // Runtime time ABI: signed 64-bit counts of 2**(-30) seconds.
    m_duration = create(TypeKind::Fixed, "Duration");
    m_duration->fixedScale = 30;
    m_duration->fixedAft = 10;
    m_duration->low = std::numeric_limits<long long>::min();
    m_duration->high = std::numeric_limits<long long>::max();

    m_string = create(TypeKind::Array, "String");
    m_string->element = m_character;
    m_string->index = m_positive;
    m_string->constrained = false;
    m_string->indexLow = 1;
    m_string->indexHigh = 0;
}

Type* TypeTable::create(TypeKind kind, const std::string& name)
{
    m_types.push_back(std::make_unique<Type>(kind, name));
    return m_types.back().get();
}

Type* TypeTable::makeSubtype(const std::string& name, Type* parent, __int128 low, __int128 high)
{
    Type* subtype = create(parent->kind, name);
    subtype->alignment = parent->alignment;
    subtype->compositeSize = parent->compositeSize;
    subtype->sizeBounds = parent->sizeBounds;
    subtype->base = parent;
    subtype->isSubtype = true;
    subtype->tagged = parent->tagged;
    subtype->privateViewTagged = parent->privateViewTagged;
    subtype->abstract = parent->abstract;
    subtype->controlled = parent->controlled;
    subtype->tagName = parent->tagName;
    subtype->parentFieldCount = parent->parentFieldCount;
    subtype->modulus = parent->modulus;
    subtype->low = low;
    subtype->high = high;
    subtype->scalarBoundsSymbol = parent->scalarBoundsSymbol;
    subtype->byteSize = parent->kind == TypeKind::Integer ? typeSize(parent) : parent->byteSize;
    subtype->needsZeroInit = parent->needsZeroInit;
    subtype->literals = parent->literals;
    subtype->enumCodes = parent->enumCodes;
    subtype->digits = parent->digits;
    subtype->fixedScale = parent->fixedScale;
    subtype->fixedAft = parent->fixedAft;
    subtype->fixedRadix = parent->fixedRadix;
    subtype->formalFixed = parent->formalFixed;
    subtype->delta = parent->delta;
    subtype->hasRealRange = parent->hasRealRange;
    subtype->lowReal = parent->lowReal;
    subtype->highReal = parent->highReal;
    subtype->arrayRank = parent->arrayRank;
    subtype->isArrayRow = parent->isArrayRow;
    subtype->element = parent->element;
    subtype->index = parent->index;
    subtype->indexLow = parent->indexLow;
    subtype->indexHigh = parent->indexHigh;
    subtype->constrained = parent->constrained;
    subtype->boundsSymbol = parent->boundsSymbol;
    subtype->fields = parent->fields;
    subtype->layoutSymbol = parent->layoutSymbol;
    subtype->target = parent->target;
    subtype->accessProfile = parent->accessProfile;
    subtype->accessScope = parent->accessScope;
    subtype->privateTo = parent->privateTo;
    subtype->isLimited = parent->isLimited;
    subtype->taskBody = parent->taskBody;
    subtype->protectedScope = parent->protectedScope;
    subtype->incompleteLayout = parent->incompleteLayout;
    subtype->discriminantCount = parent->discriminantCount;
    subtype->dependentStorage = parent->dependentStorage;
    subtype->discriminantRecord = parent->discriminantRecord;
    subtype->variantOn = parent->variantOn;
    subtype->variants = parent->variants;
    return subtype;
}

// Subtypes declared against a private view keep that view's identity, but
// their cached representation and lifetime properties must follow completion.
void TypeTable::refreshRecordSubtypes(Type* completed)
{
    for (const auto& stored : m_types) {
        Type* subtype = stored.get();
        if (!subtype->isSubtype || rootType(subtype) != completed) {
            continue;
        }
        subtype->tagged = completed->tagged;
        subtype->privateViewTagged = completed->privateViewTagged;
        subtype->controlled = completed->controlled;
        subtype->dependentStorage = completed->dependentStorage;
        subtype->tagName = completed->tagName;
        subtype->parentFieldCount = completed->parentFieldCount;
        subtype->isLimited = subtype->isLimited || completed->isLimited;
        subtype->fields = completed->fields;
        subtype->layoutSymbol = completed->layoutSymbol;
        subtype->taskBody = completed->taskBody;
        subtype->protectedScope = completed->protectedScope;
        if (completed->taskBody != nullptr) {
            subtype->byteSize = completed->byteSize;
        }
    }
}

// Inherited primitive profiles create corresponding subtypes during the type
// declaration, before adjacent representation items settle its storage.
void TypeTable::refreshEnumerationRepresentation(Type* type)
{
    for (const auto& stored : m_types) {
        Type* subtype = stored.get();
        if (subtype->isSubtype && baseType(subtype) == type) {
            subtype->byteSize = type->byteSize;
            subtype->enumCodes = type->enumCodes;
            subtype->alignment = type->alignment;
        }
    }
}

void TypeTable::refreshStorageRepresentation(Type* type)
{
    for (const auto& stored : m_types) {
        Type* subtype = stored.get();
        if (subtype == type || !subtype->isSubtype) { continue; }
        Type* ancestor = subtype->base;
        while (ancestor != nullptr && ancestor != type && ancestor->isSubtype) { ancestor = ancestor->base; }
        if (ancestor == type && subtype->specifiedSize < 0) {
            subtype->byteSize = type->byteSize;
            subtype->compositeSize = type->compositeSize;
            subtype->sizeBounds = type->sizeBounds;
            subtype->alignment = type->alignment;
        }
    }
}

bool hasDependentStorage(const Type* type)
{
    if (type == nullptr) { return false; }
    if (type->dependentStorage) { return true; }
    if (type->kind == TypeKind::Array) { return hasDependentStorage(type->element); }
    if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : type->fields) {
            if (hasDependentStorage(field.type)) { return true; }
        }
    }
    return false;
}

bool hasDefaultDiscriminants(const Type* type)
{
    const Type* record = type;
    while (record != nullptr && record->isSubtype) { record = record->base; }
    return record != nullptr && record->discriminantCount > 0
        && record->fields.front().defaultValue != nullptr;
}

bool hasDiscriminantConstraint(const Type* type)
{
    for (const Type* walk = type; walk != nullptr; walk = walk->isSubtype ? walk->base : nullptr) {
        if (walk->discriminantConstraintSource != nullptr) {
            return hasDiscriminantConstraint(walk->discriminantConstraintSource);
        }
        if (walk->discriminantsKnown || walk->discriminantSymbol != nullptr) { return true; }
    }
    return false;
}

bool discriminantValueOf(const Type* type, int index, long long& value)
{
    for (const Type* walk = type; walk != nullptr; walk = walk->isSubtype ? walk->base : nullptr) {
        if (walk->discriminantConstraintSource != nullptr) {
            return discriminantValueOf(walk->discriminantConstraintSource, index, value);
        }
        if (!walk->discriminantsKnown || index < 0
            || static_cast<std::size_t>(index) >= walk->discriminantValues.size()) {
            continue;
        }
        value = walk->discriminantValues[static_cast<std::size_t>(index)];
        return true;
    }
    return false;
}

int variantFor(const Type* record, long long discriminant, int parent)
{
    int fallback = -1;
    for (std::size_t v = 0; v < record->variants.size(); ++v) {
        if (record->variants[v].parent != parent) {
            continue;
        }
        if (record->variants[v].isOthers) {
            fallback = static_cast<int>(v);
            continue;
        }
        for (const VariantChoice& choice : record->variants[v].choices) {
            if (discriminant >= choice.low && discriminant <= choice.high) {
                return static_cast<int>(v);
            }
        }
    }
    return fallback;
}

int variantPresence(Type* type, int variant)
{
    Type* record = baseType(type);
    bool unknown = false;
    for (int current = variant; current >= 0; current = record->variants[current].parent) {
        const VariantInfo& info = record->variants[current];
        long long value = 0;
        if (!discriminantValueOf(type, info.discriminant, value)) {
            unknown = true;
        } else if (variantFor(record, value, info.parent) != current) {
            return 0;
        }
    }
    return unknown ? -1 : 1;
}

bool TypeTable::isBoolean(const Type* type) const
{
    type = rootType(const_cast<Type*>(type));
    while (type != nullptr && type->parentType != nullptr) {
        type = type->parentType;
    }
    return type == m_boolean;
}

bool TypeTable::isCharacter(const Type* type) const
{
    type = rootType(const_cast<Type*>(type));
    while (type != nullptr && type->parentType != nullptr) {
        type = type->parentType;
    }
    return type == m_character;
}

bool TypeTable::isString(const Type* type) const
{
    const Type* root = rootType(const_cast<Type*>(type));
    return root != nullptr && root->kind == TypeKind::Array && isCharacter(root->element)
        && typeSize(root->element) == 1;
}

// The base subtype keeps type identity and representation, but drops the
// first subtype's constraint. Cache it on the type, not on each subtype.
Type* TypeTable::scalarBaseType(Type* type)
{
    type = baseType(type);
    if (type == nullptr || (!isDiscrete(type) && !isReal(type) && type->kind != TypeKind::Fixed)) {
        return nullptr;
    }
    if (type->scalarBase != nullptr) {
        return type->scalarBase;
    }
    Type* result = makeSubtype(type->name + "'Base", type, type->low, type->high);
    result->scalarBoundsSymbol = nullptr;
    if (type->parentType != nullptr) {
        Type* parentBase = scalarBaseType(type->parentType);
        result->low = parentBase->low;
        result->high = parentBase->high;
        result->hasRealRange = parentBase->hasRealRange;
        result->lowReal = parentBase->lowReal;
        result->highReal = parentBase->highReal;
    } else if (type->modulus != 0) {
        result->low = 0;
        result->high = type->modulus - 1;
    } else if (type->kind == TypeKind::Integer || type->kind == TypeKind::Fixed) {
        bool wide = typeSize(type) == 8;
        result->low = wide ? std::numeric_limits<long long>::min() : -2147483648LL;
        result->high = wide ? std::numeric_limits<long long>::max() : 2147483647LL;
    } else if (type->kind == TypeKind::Float) {
        result->hasRealRange = false;
    }
    if (type->kind == TypeKind::Fixed && fixedIsDecimal(type->fixedScale)) { result->digits = 18; }
    type->scalarBase = result;
    return result;
}

bool needsFinalization(const Type* type)
{
    if (type == nullptr) {
        return false;
    }
    if (type->protectedScope != nullptr || type->dependentStorage || type->controlled || type->classRoot != nullptr) {
        return true;
    }
    if (type->kind == TypeKind::Array) {
        return needsFinalization(type->element);
    }
    if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : type->fields) {
            if (needsFinalization(field.type)) {
                return true;
            }
        }
    }
    return false;
}

bool hasTasks(const Type* type)
{
    if (type == nullptr) {
        return false;
    }
    if (type->taskBody != nullptr) {
        return true;
    }
    if (type->kind == TypeKind::Array) {
        return hasTasks(type->element);
    }
    if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : type->fields) {
            if (hasTasks(field.type)) {
                return true;
            }
        }
    }
    return false;
}

bool needsCollection(const Type* type)
{
    // Every named object access type owns a collection, including scalar and
    // plain composite designations. Access-to-subprogram values own no storage.
    return type != nullptr && type->kind == TypeKind::Access && type->target != nullptr
        && type->accessProfile == nullptr;
}

bool hasLimitedControlledParts(const Type* type)
{
    if (type == nullptr) {
        return false;
    }
    if (type->isLimited && needsFinalization(type)) {
        return true;
    }
    if (type->kind == TypeKind::Array) {
        return hasLimitedControlledParts(type->element);
    }
    if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : type->fields) {
            if (hasLimitedControlledParts(field.type)) {
                return true;
            }
        }
    }
    return false;
}

bool hasPrivateOrTaggedParts(const Type* type)
{
    if (type == nullptr) {
        return false;
    }
    if (type->tagged || type->privateTo != nullptr) {
        return true;
    }
    if (type->kind == TypeKind::Array) {
        return hasPrivateOrTaggedParts(type->element);
    }
    if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : type->fields) {
            if (hasPrivateOrTaggedParts(field.type)) {
                return true;
            }
        }
    }
    return false;
}

bool hasProtected(const Type* type)
{
    if (type == nullptr) {
        return false;
    }
    if (type->protectedScope != nullptr) {
        return true;
    }
    if (type->kind == TypeKind::Array) {
        return hasProtected(type->element);
    }
    if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : type->fields) {
            if (hasProtected(field.type)) {
                return true;
            }
        }
    }
    return false;
}

bool hasIncompleteParts(const Type* type)
{
    type = baseType(const_cast<Type*>(type));
    if (type == nullptr) {
        return false;
    }
    if (type->isIncomplete || type->incompleteLayout) {
        return true;
    }
    return type->kind == TypeKind::Array && hasIncompleteParts(type->element);
}

int sharedTypeKind(const Type* type)
{
    int kind = 0;
    for (; type != nullptr; type = type->base != nullptr ? type->base : type->parentType) {
        kind = std::max(kind, type->shared);
    }
    return kind;
}

bool hasSharedParts(const Type* type)
{
    if (type == nullptr) {
        return false;
    }
    if (sharedTypeKind(type) != 0) {
        return true;
    }
    if (type->kind == TypeKind::Array) {
        return hasSharedParts(type->element);
    }
    if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : type->fields) {
            if (hasSharedParts(field.type)) {
                return true;
            }
        }
    }
    return false;
}

namespace
{

bool hasSharedAccessParts(const Type* type, std::unordered_set<const Type*>& visited)
{
    if (type == nullptr || !visited.insert(type).second) {
        return false;
    }
    if (type->kind == TypeKind::Access) {
        return hasSharedParts(type->target) || hasSharedAccessParts(type->target, visited);
    }
    if (type->kind == TypeKind::Array) {
        return hasSharedAccessParts(type->element, visited);
    }
    if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : type->fields) {
            if (hasSharedAccessParts(field.type, visited)) {
                return true;
            }
        }
    }
    return false;
}

}

bool hasSharedAccessParts(const Type* type)
{
    std::unordered_set<const Type*> visited;
    return hasSharedAccessParts(type, visited);
}

bool hasRuntimeLayout(const Type* type)
{
    if (type == nullptr) { return false; }
    if (type->kind == TypeKind::Array) {
        return !type->constrained || hasRuntimeLayout(type->element);
    }
    if (type->kind == TypeKind::Record) {
        if (type->layoutSymbol != nullptr) { return true; }
        for (const FieldInfo& field : type->fields) {
            if (!field.dependentBounds && hasRuntimeLayout(field.type)) { return true; }
        }
    }
    return false;
}
