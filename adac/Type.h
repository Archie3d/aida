#pragma once

#include <memory>
#include <string>
#include <utility>
#include <vector>

enum class TypeKind
{
    Void,
    UniversalInteger,
    UniversalReal,
    Integer,
    Enumeration,
    Float,
    Fixed,
    Array,
    Record,
    Access
};

struct Type;
struct Symbol;
class Scope;

struct FieldInfo
{
    std::string name;          // Lowered, which is how a selection looks it up.
    std::string displayName;   // As written, which is how a message spells it.
    Type* type = nullptr;
    int index = 0;
    long long offset = 0;

    // What the component declaration gives it, used when an object of the
    // record is made without a value of its own.
    struct Expr* defaultValue = nullptr;

    // A leading component named in the discriminant part. Only whole-object
    // assignment to an unconstrained variable may change it.
    bool isDiscriminant = false;
    bool aliased = false;
    // An explicit slot includes any reserved trailing bytes. Dependent arrays
    // use inline payload storage in represented records instead of a view slot.
    long long representationSize = -1;
    bool inlineDependent = false;
    bool dependentBounds = false; // This field stores an object-owned array view.

    // Which alternative of the variant part the component belongs to, or -1
    // when every value of the record has it.
    int variant = -1;
};

// One value the discriminant may hold to select a variant.  A single value is
// a range of one.
struct VariantChoice
{
    long long low = 0;
    long long high = 0;
};

// Alternatives are stored in preorder. Siblings share storage; a nested
// alternative also requires its parent to be active.
struct VariantInfo
{
    int parent = -1;
    int discriminant = -1;
    std::vector<VariantChoice> choices;
    bool isOthers = false;
};

struct Type
{
    Type(TypeKind typeKind, std::string typeName)
        : kind(typeKind)
        , name(std::move(typeName))
    {
    }

    TypeKind kind;
    std::string name;

    // Subtypes share identity through base. Derivation keeps a distinct identity.
    Type* base = nullptr;
    bool isSubtype = false;
    // Derivation ancestry and primitive declarations are independent of layout.
    Type* parentType = nullptr;
    // A private completion's derivation ancestry is hidden outside its package.
    bool parentVisible = true;
    // Tagged records reserve the leading pointer and retain a parent prefix.
    bool tagged = false;
    // The partial view can hide tagging while the full layout remains tagged.
    bool privateViewTagged = false;
    bool privateViewAbstract = false;
    bool abstract = false;
    bool controlled = false;
    // Class-wide identity is distinct from subtype identity and derivation.
    bool dispatchFrozen = false;
    Symbol* tagOwner = nullptr;
    Symbol* collectionOwner = nullptr;
    std::string collectionName;
    long long collectionOffset = -1;
    long long tagOffset = -1;
    int accessLevel = 0;
    Type* classRoot = nullptr;
    Type* classWide = nullptr;
    // Includes hidden inherited slots, independently of source visibility.
    std::vector<Symbol*> dispatchSlots;
    int parentFieldCount = 0;
    std::string tagName;
    Scope* declarationScope = nullptr;
    std::vector<Symbol*> primitives;
    Type* scalarBase = nullptr;

    // Named but not yet described.  'type Node;' declares one so that an access
    // type can point at a record declared further down, and the full definition
    // fills this very object in, which keeps every pointer already taken good.
    bool isIncomplete = false;
    bool limitedView = false;
    bool incompleteInPrivatePart = false;
    struct TypeDecl* incompleteDeclaration = nullptr;
    std::string limitedPackage;

    // Set for a type declared private: what it is made of is only in reach
    // inside the package that declared it.  A limited one lends not even
    // assignment or equality to the outside.
    Symbol* privateTo = nullptr;
    int privateDiscriminantCount = 0;
    bool isLimited = false;
    // Explicit limitedness of the full view, inherited by subtypes/derivations.
    bool limitedRecord = false;
    Scope* protectedScope = nullptr;
    std::string protectedName;
    Symbol* protectedObject = nullptr;
    bool protectedBodySeen = false;
    bool incompleteLayout = false;
    Symbol* taskBody = nullptr; // Task identity occupies one pointer-sized slot.

    // Discriminants and the variant part they may select.  The discriminants
    // are the first components of the record; variantOn identifies the outer
    // selector, or -1 when there is no variant part. Each nested alternative
    // records its own selector and parent in variants.
    int discriminantCount = 0;
    bool recordRepresentationSpecified = false;
    int variantOn = -1;
    std::vector<VariantInfo> variants;

    // Set on a subtype whose discriminants a constraint has pinned down, as in
    // 'subtype Round is Shape (Circle)'.
    // An inherited profile keeps the parent's constraint, independently of
    // the derived first subtype reached through its type-identity base link.
    Type* discriminantConstraintSource = nullptr;
    bool discriminantsKnown = false;
    std::vector<long long> discriminantValues;
    Symbol* discriminantSymbol = nullptr;
    std::vector<struct Expr*> discriminantExpressions;
    std::vector<bool> discriminantPerObject;
    bool dependentStorage = false;
    bool discriminantPerEvaluation = false;
    bool discriminantObjectConstraint = false;

    // Semantic bounds include the unsigned 64-bit endpoint. Runtime values
    // still occupy at most 64 bits.
    __int128 low = 0;
    __int128 high = 0;
    // Zero denotes signed integers; modular types retain this across subtypes.
    __int128 modulus = 0;

    // Scalar constraints use full-width bounds in the owning
    // activation. Aliases share the symbol; only the declaration owns expressions.
    Symbol* scalarBoundsSymbol = nullptr;
    bool scalarObjectConstraint = false;
    struct Expr* scalarLow = nullptr;
    struct Expr* scalarHigh = nullptr;
    Type* scalarConstraintBase = nullptr;
    // Decimal digits limits are checked independently of range compatibility.
    Type* decimalConstraint = nullptr;

    // Enumeration types.
    std::vector<std::string> literals;
    // Expressions and the Ada scalar call ABI use positions; storage and C
    // scalar calls use these signed 32-bit codes. Empty means positional codes.
    std::vector<long long> enumCodes;
    bool enumCodesSpecified = false;

    // Floating point types.  The requested number of decimal digits selects the
    // machine representation, and the bounds are kept separately from the
    // discrete ones because they do not fit in an integer.
    int digits = 0;
    // Binary fractional bits, or ADA_FIXED_DECIMAL_TAG + decimal scale.
    int fixedScale = 0;
    int fixedAft = 1;
    int fixedRadix = 2;
    bool fixedRadixSpecified = false;
    // Contract-only fixed-point types have no scale or bounds until instantiation.
    bool formalFixed = false;
    struct Expr* delta = nullptr;
    struct Expr* smallClause = nullptr;
    bool smallPending = false;
    bool hasRealRange = false;
    double lowReal = 0.0;
    double highReal = 0.0;

    // Array types.
    // Multidimensional arrays use nested row layouts internally.
    int arrayRank = 1;
    bool isArrayRow = false;
    Type* element = nullptr;
    Type* index = nullptr;
    long long indexLow = 0;
    long long indexHigh = 0;
    bool constrained = true;

    // A dynamically constrained declaration keeps its bounds in its owner's
    // activation. `constrained` continues to mean a static storage layout.
    Symbol* boundsSymbol = nullptr;
    std::vector<std::pair<struct Expr*, struct Expr*>> boundExpressions;
    Type* discriminantRecord = nullptr;
    bool partialDiscriminantBounds = false;

    // Runtime record layout: total size followed by one offset per field.
    Symbol* layoutSymbol = nullptr;

    // Record types.
    std::vector<FieldInfo> fields;

    // Access types.
    Type* target = nullptr;
    std::string accessIdentity;
    bool excludesNull = false;
    bool anonymousAccess = false;
    bool accessParameter = false;
    bool accessDiscriminant = false;
    bool generalAccess = false;
    bool accessConstant = false;
    bool accessProtected = false;
    bool accessConventionC = false;
    bool accessConventionFrozen = false;
    bool accessConventionSpecified = false;
    bool aliasedElements = false;
    // Subprogram access uses a signature instead of an object target.
    Symbol* accessProfile = nullptr;
    Scope* accessScope = nullptr;

    // Objects of this type start out cleared.  A File_Type has to, because the
    // run time reads the handle before anything has opened it.
    bool needsZeroInit = false;
    int shared = 0; // 1: Volatile, 2: Atomic (also volatile).

    // A width the type carries instead of the one its kind implies, which is
    // how a Stream_Element comes out a byte wide.
    long long byteSize = 0;
    long long compositeSize = 0;
    std::vector<std::pair<long long, long long>> sizeBounds;
    // -1 selects the default; zero specifies no additional alignment promise.
    long long alignment = -1;
    long long specifiedSize = -1;
    bool alignmentSpecified = false;
};

// True when an object of this type, or one holding it, must be cleared.
bool hasRuntimeLayout(const Type* type);
bool needsZeroInit(const Type* type);
bool needsFinalization(const Type* type);
bool needsCollection(const Type* type);
bool hasAccessDiscriminants(const Type* type);
Type* controllingParameterType(Type* type);
Type* controllingResultType(Type* type);
Type* designatedViewType(Type* type);
bool hasTasks(const Type* type);
bool hasProtected(const Type* type);
bool hasIncompleteParts(const Type* type);
bool hasLimitedControlledParts(const Type* type);
bool hasLimitedParts(const Type* type);
long long fieldStorageSize(const FieldInfo& field);
bool variantsExclusive(const Type* type, int left, int right);

// The type identity at the root of a subtype chain.
Type* rootType(Type* type);

// Which alternative of a variant part a discriminant value picks, or -1 when
// it picks none.
int variantFor(const Type* record, long long discriminant, int parent = -1);
// -1 means unknown, 0 absent, 1 present, including all enclosing variants.
int variantPresence(Type* type, int variant);

// What a constraint along the subtype chain fixed a discriminant to, if
// anything did.
bool discriminantValueOf(const Type* type, int index, long long& value);
bool hasDefaultDiscriminants(const Type* type);
bool hasDependentStorage(const Type* type);
bool hasDiscriminantConstraint(const Type* type);

// The type that carries the value representation (skips subtypes only).
Type* baseType(Type* type);

bool isTaggedAncestor(Type* ancestor, Type* type);
bool isDiscrete(const Type* type);
bool isReal(const Type* type);
bool isNumeric(const Type* type);
bool isScalar(const Type* type);
bool isComposite(const Type* type);

long long typeSize(const Type* type);
long long typeLayoutSize(const Type* type);
bool scalarStorageSigned(const Type* type);
long long typeAlignment(const Type* type);

// The QBE base type letter used for values of this type.
char qbeClass(const Type* type);

// The QBE load/store instruction suffix for values of this type.
const char* qbeLoadInstruction(const Type* type);
const char* qbeStoreInstruction(const Type* type);

long long arrayLength(const Type* type);

class TypeTable
{
public:
    TypeTable();

    Type* create(TypeKind kind, const std::string& name);
    Type* makeSubtype(const std::string& name, Type* parent, __int128 low, __int128 high);
    void refreshRecordSubtypes(Type* completed);
    void refreshEnumerationRepresentation(Type* type);
    void refreshStorageRepresentation(Type* type);

    Type* scalarBaseType(Type* type);

    Type* voidType() const { return m_void; }
    Type* universalInteger() const { return m_universalInteger; }
    Type* universalReal() const { return m_universalReal; }
    Type* integerType() const { return m_integer; }
    Type* longIntegerType() const { return m_longInteger; }
    Type* naturalType() const { return m_natural; }
    Type* positiveType() const { return m_positive; }
    Type* booleanType() const { return m_boolean; }
    Type* characterType() const { return m_character; }
    Type* floatType() const { return m_float; }
    Type* longFloatType() const { return m_longFloat; }
    Type* stringType() const { return m_string; }
    Type* durationType() const { return m_duration; }

    bool isBoolean(const Type* type) const;
    bool isCharacter(const Type* type) const;
    bool isString(const Type* type) const;

private:
    std::vector<std::unique_ptr<Type>> m_types;
    Type* m_void = nullptr;
    Type* m_universalInteger = nullptr;
    Type* m_universalReal = nullptr;
    Type* m_integer = nullptr;
    Type* m_longInteger = nullptr;
    Type* m_natural = nullptr;
    Type* m_positive = nullptr;
    Type* m_boolean = nullptr;
    Type* m_character = nullptr;
    Type* m_float = nullptr;
    Type* m_longFloat = nullptr;
    Type* m_string = nullptr;
    Type* m_duration = nullptr;
};

// Conservative by-reference components for array view conversions.
bool hasPrivateOrTaggedParts(const Type* type);

int sharedTypeKind(const Type* type);
bool hasSharedParts(const Type* type);
bool hasSharedAccessParts(const Type* type);

// QBE constants carry the low 64 bits; semantic bounds retain unsigned endpoints.
inline std::string scalarText(__int128 value)
{
    return std::to_string(static_cast<long long>(value));
}
