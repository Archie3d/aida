#pragma once

#include "Ast.h"
#include "../common/Array.h"
#include "../common/Master.h"
#include "../common/Modular.h"
#include "../common/Real.h"
#include "Diagnostics.h"
#include "Sema.h"

#include <functional>
#include <ostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// A value is a QBE operand.  Array values additionally carry their bounds, so
// that unconstrained arrays keep working at run time.
struct Value
{
    std::string name;   // QBE operand spelling: a %temporary, $symbol or literal.
    char type = 'w';    // QBE class letter: w/l (32/64-bit integer), s/d (float/double).
                       // Addresses use l; the letter is emitted directly into IR.
    std::string first;  // QBE operand for the inclusive lower bound of array axis 1.
    std::string last;   // Inclusive upper-bound operand; both bounds are empty for scalars.

    // Bounds for dimensions 2 through rank; ordinary arrays of arrays keep
    // their component bounds in the component type instead.
    std::vector<std::pair<std::string, std::string>> innerBounds;

    std::vector<std::string> lengths;
    std::vector<std::string> strides;

    const std::string& axisFirst(int dimension) const
    {
        return dimension == 0 ? first : innerBounds[dimension - 1].first;
    }
    const std::string& axisLast(int dimension) const
    {
        return dimension == 0 ? last : innerBounds[dimension - 1].second;
    }

    std::string accessLevel;
    std::string accessOwner;

    bool declaredLayout = false; // Destination under construction; its tag may name an ancestor.
    bool shared = false; // This value denotes shared scalar storage.

    // Containing object for an access component constrained by discriminants.
    std::shared_ptr<Value> constraintObject;
    Type* constraintLayout = nullptr;

    Type* scalarStorageType = nullptr; // Physical type of a discriminant used by an inherited expression.
    int bitOffset = 0;
    int bitWidth = 0;
    std::string dynamicBitOffset;

    bool hasBounds() const { return !first.empty() && !last.empty(); }
};

class QbeEmitter
{
public:
    QbeEmitter(Sema& sema, Diagnostics& diagnostics);

    // The intermediate language for one library unit, which is what a single
    // object file is built from.
    void emitUnit(const LibraryUnit& unit, std::ostream& out);

private:
    static constexpr long long kArrayStackLimit = 65536;
    static constexpr long long kInlineScratchLimit = 32;
    static constexpr long long kFrameStackLimit = INT32_MAX - 16;

    struct ArrayAggregatePlan
    {
        std::unordered_map<Expr*, Value> choices;
        std::unordered_map<Expr*, Value> shapes;
    };

    struct StorageCheckpoint
    {
        std::string local;
        std::string temporary;
        std::string finalization;
        std::string temporaryFinalization;
    };

    struct FunctionContext
    {
        std::unordered_map<Type*, Value> discriminantBounds;
        struct DiscriminantObject
        {
            Value address;
            Type* layout;
        };
        std::unordered_map<Type*, DiscriminantObject> discriminantObjects;
        Symbol* symbol = nullptr;
        std::ostringstream prologue;
        std::ostringstream body;
        std::vector<std::size_t> returnOffsets;
        std::string frameAllocation;
        std::string arrayArena;
        std::string temporaryArena;
        std::unordered_map<std::string, long long> stackAlignments;
        std::string finalizationChain;
        bool hasMaster = false;
        struct BlockMaster
        {
            std::string record;
            StorageCheckpoint storage;
        };
        std::vector<BlockMaster> blockMasters;
        std::size_t masterObservingCalls = 0;
        std::vector<std::string> elidedBlockMasters;
        std::unordered_map<std::string, std::size_t> transferMasterDepth;
        std::string temporaryFinalizationChain;
        bool initializingTemporary = false;
        bool initializingStorage = false;
        std::string extendedReturnLabel;
        bool cleansArrayResult = false;
        struct ReturnCleanup
        {
            std::string ownerMark;
            std::string arenaMark;
            std::size_t masterDepth;
            std::size_t handlerDepth;
            std::string taskMark;
        };
        std::vector<ReturnCleanup> returnCleanups;
        std::string resultTarget;
        Value resultTargetShape;
        std::string resultDescriptor;
        std::string resultProvided;
        std::string resultTargetOwner;
        std::string resultTargetArena;
        AllocatorExpr* pendingExpression = nullptr;
        std::string pendingAllocation;
        std::string pendingCollection;
        std::string coextensionLevel;
        std::string constructionOwner;
        std::string constructionArena;
        std::string constructionAccessLevel;
        std::string taskMaster = "0";
        std::string taskCollection = "0";
        std::string taskGroup = "0";
        std::unordered_map<Symbol*, std::string> taskGroups;
        bool arrayArenaUsed = false;
        bool temporaryArenaUsed = false;
        std::string frameTemp;
        bool hasFrame = false;
        bool terminated = false;
        long long frameSize = 8;
        std::unordered_map<Symbol*, std::string> locals;
        std::unordered_map<Symbol*, Value> bounds;
        std::unordered_map<Symbol*, std::string> streamConstraints;
        std::unordered_map<const AttributeExpr*, Value> rangePrefixes;
        std::vector<std::string> handlerLabels;
        std::unordered_map<std::string, StorageCheckpoint> handlerStorage;
        struct GotoTarget
        {
            std::string label;
            StorageCheckpoint storage;
            std::size_t masterDepth;
            std::size_t handlerDepth;
            std::size_t returnDepth;
        };
        std::unordered_map<const Stmt*, GotoTarget> gotoTargets;
        std::unordered_map<const LoopStmt*, StorageCheckpoint> loopStorage;
        std::unordered_map<const LoopStmt*, std::size_t> loopHandlerDepth;
        std::unordered_map<const LoopStmt*, std::size_t> loopReturnDepth;
        std::vector<std::string> activeExceptions;
        std::unordered_map<const LoopStmt*, std::string> loopExits;
        std::string propagateLabel;
        bool usesPropagate = false;
        std::vector<SubprogramBody*> nested;
        SourceLocation sourceLocation;
        std::string traceName;
        std::string controllingTag;
    };

    // Output and data (QbeEmitter.cpp).
    std::string newTemp();
    std::string newLabel(const char* prefix);
    void line(const std::string& text);
    enum class CallEffects
    {
        DeferredCheck, // Caller must check after its transaction/lock cleanup.
        CheckException, // Inspect pending status immediately after the call.
        RuntimeHook, // Keep the triggering location; caller controls checks.
        CheckedRuntimeHook // Keep the triggering location and check immediately.
    };
    enum class MasterEffects
    {
        MayObserve, // Includes unknown/source calls and callbacks; retain ancestry.
        Independent, // Audited primitive: cannot observe/publish master ancestry.
        ScopeHook // Candidate lifecycle bookkeeping, not evidence of scope use.
    };
    void emitCallInstruction(const std::string& instruction, CallEffects effects,
                             MasterEffects masters = MasterEffects::MayObserve);
    void emitSourceLocation();
    void emitReturn(const std::string& operand = "");
    std::string functionExitActions(const FunctionContext& context);
    std::string sourceLocationData(const SourceLocation& location);
    void label(const std::string& name);
    void jump(const std::string& target);
    void branch(const Value& condition, const std::string& ifTrue, const std::string& ifFalse);
    std::string stringData(const std::string& text);
    std::string enumTableFor(const Type* type);
    std::string enumCodeTableFor(Type* type);
    Value enumCode(const Value& position, Type* type);
    Value enumPosition(const Value& code, Type* type);

    // Declarations (QbeDecl.cpp).
    void emitTypeTag(Type* type);
    void initializeTypeTag(Type* type);
    void initializeConstructorMaster(Symbol* symbol);
    Value typeTag(Type* type);
    Value taggedSize(const Value& object);
    void checkTagLevel(const Value& object, int level);
    void emitClassWideObject(ObjectDecl* object, Symbol* symbol);
    void collectGlobals(DeclList& declarations);
    void emitElaborationDeclarations(DeclList& declarations);
    void emitLocalDeclarations(DeclList& declarations);

    // Functions (QbeFunctions.cpp).
    void emitElaboration(const LibraryUnit& unit);
    void emitSubprogramsIn(DeclList& declarations);
    void emitSubprogram(SubprogramBody* body);
    void emitImportGuard(Symbol* symbol);
    void emitInheritedAccessEntries(Type* type);
    void initializeAccessDescriptor(Symbol* symbol);
    std::unordered_set<Symbol*> m_emittedAccessEntries;
    std::unordered_set<Symbol*> m_importedAccessDescriptors;
    void prepareElaborationFlags(DeclList& declarations);
    Value elaborationFlag(Symbol* symbol);
    void emitElaborationCheck(Symbol* symbol);
    void finishFunction(const std::string& signature);

    // Statements (QbeStatements.cpp).
    void emitStatements(StmtList& statements);
    std::string beginTaskTemporary();
    void activateTaskTemporary(const std::string& collection);
    void emitStatement(Stmt* statement);

    // Exceptions and checks (QbeExceptions.cpp).
    void emitExceptionObjects(const std::vector<Symbol*>& exceptions);
    void emitRaise(Symbol* exception, const SourceLocation& location, Expr* message = nullptr);
    void emitExceptionCheck();
    void emitHandlers(std::vector<ExceptionHandler>& handlers, const std::string& afterLabel,
                      const std::string& dispatchLabel);
    void emitAccessConstraintCheck(const Value& value, Type* type, const Value* address = nullptr);
    void emitRecordConstraintCheck(const Value& value, Type* type);
    Value recordConstraintMatches(const Value& value, Type* type);
    void emitDiscreteRangeCheck(Expr* low, const Value& first, const Value& last);
    void emitRangeCheck(const Value& value, Type* type, const SourceLocation& location, const Value* address = nullptr);
    Value variantCondition(const Value& address, Type* record, int variant);
    void checkVariant(const Value& address, Type* record, int variant);
    std::string rangeTest(const Value& value, long long low, long long high);
    void checkNotNull(const Value& pointer);
    void raiseConstraintError();
    Value taggedMembership(const Value& object, Type* target);

    // Storage and assignment (QbeStorage.cpp).
    std::string runtimeSize(Type* type);
    std::string allocTypeScratch(Type* type);
    Value layoutAddress(Type* type);
    void emitRecordLayout(Type* type);
    std::string fieldOffset(Type* type, const FieldInfo& field);
    Value fieldAddress(const Value& object, Type* type, const FieldInfo& field);
    Value dependentFieldBounds(const Value& object, Type* type, const FieldInfo& field);
    Value allocatedArrayView(Value pointer, Type* type);
    long long reserveFrame(long long size, long long alignment = 8);
    std::string allocScratch(long long size, long long alignment = 1);
    void emitScratchZero(const std::string& buffer, long long size, long long alignment);
    bool emitScratchCopy(const std::string& destination, const std::string& source, long long size);
    void emitArrayOffset(const std::string& offset, const Value& originalIndex, const Value& index,
        const Value& axis, const std::string& stride);
    std::string storageArena(bool temporary, bool allocate = false);
    StorageCheckpoint storageCheckpoint();
    void rewindStorage(const StorageCheckpoint& checkpoint, bool checkException = true);
    void emitBlockExits(std::size_t depth);
    void pushHandler(const std::string& label);
    void emitControlledCall(const Value& object, Type* type, const std::string& operation, const std::string& explicitTag = "");
    void initializeFinalization();
    void initializeCollection(Type* type, bool poolClause = false);
    void emitStoragePoolClause(RepresentationDecl* decl);
    int storagePoolSlot(const std::string& name);
    Value collectionFor(Type* type);
    void emitExtendedReturnBody(ReturnStmt* statement);
    void emitControlledResult(Expr* expression, ObjectDecl* object = nullptr, ReturnStmt* extended = nullptr);
    std::string finalizerName(Type* type);
    void prepareControlledObject(const Value& object, Type* type, bool library = false);
    void activateControlledObject(const Value& object, Type* type);
    void walkControlled(const Value& object, Type* type, bool parentFirst,
                        const std::function<void(const Value&, Type*)>& action, bool activeOnly = true);
    void adjustControlledObject(const Value& object, Type* type, const std::string& viewTag = "");
    void finalizeControlledObject(const Value& object, Type* type, const std::string& viewTag = "");
    void copyControlledObject(const Value& target, const Value& source, Type* type, bool initialize);
    std::string objectBytes(const Value& object, Type* type);
    void emitFinalizer(Type* type);
    void emitControlledParts(Type* type);
    Value staticLinkFor(int targetLevel);
    Value addressOf(Symbol* symbol);
    Value loadBitField(const Value& address, Type* type);
    void storeBitField(const Value& address, const Value& value, Type* type);
    Value loadFrom(const Value& address, Type* type);
    void storeInto(const Value& address, const Value& value, Type* type);
    struct StorageCopyBack
    {
        Value actual;
        Value temporary;
        Type* type;
        Type* temporaryType;
        std::string constrained;
    };
    void repackRecord(const Value& destination, Type* target, const Value& source, Type* sourceType);
    Value adaptCompositeStorage(Value actual, Type* actualType, Type* formal, ParameterMode mode,
                                std::vector<StorageCopyBack>& copies, const std::string& constrained = "1");
    void emitStorageCopyBacks(const std::vector<StorageCopyBack>& copies);
    void copyInto(const Value& destination, const Value& source, Type* type);
    void assignInto(const Value& address, Type* type, Expr* value, bool initialize = false,
                    const std::string& constrained = "1");
    Value recordConstrained(Expr* object, const Value* evaluated = nullptr);
    Value discriminantConstraint(Type* type, int index);
    Value fieldDiscriminantConstraint(Type* type, int fieldIndex);
    void emitDiscriminantConstraint(Type* type);
    Value fieldStorage(const Value& object, Type* type, const FieldInfo& field);
    void initializeDependentField(const Value& object, Type* type, const FieldInfo& field);
    void cloneDependentStorage(const Value& object, Type* type);
    std::string dependentArena(const Value& object, Type* type);
    void initializeDependentStorage(const Value& object, Type* type);
    void checkRecordAssignment(const Value& address, const Value& source, Type* type,
                               bool initialize, const std::string& constrained);

    // Arrays (QbeArrays.cpp).
    void emitDynamicArray(ObjectDecl* object, Symbol* symbol);
    void emitObjectReference(ObjectDecl* object, Symbol* symbol);
    void initializeObject(const Value& address, Symbol* symbol, Expr* initializer);
    void emitArrayFill(const Value& address, Type* type, Expr* value);
    void emitScalarSubtype(Type* type, const SourceLocation& location);
    Value scalarBounds(Type* type);
    Value scalarBoundsAddress(Type* type);
    void emitTypeBounds(Type* type, const SourceLocation& location);
    Value wideBound(Value value);
    Value narrowArrayLength(Value value);
    Value arrayView(Value value, Type* type);
    Value loadArrayView(const std::string& descriptor, int rank, bool result);
    void storeArrayView(const std::string& descriptor, Value value, Type* type, bool result);
    Value boundsFor(Symbol* symbol);
    Value withBounds(const Value& address, Type* type, Symbol* symbol);
    Value arrayRow(const Value& array, Type* type);
    std::string arrayElementSize(const Value& array, Type* type);
    Value arrayElementAddress(const Value& array, Type* type, const std::string& offset);
    void checkArrayShape(const Value& target, Type* targetType, const Value& source, Type* sourceType);
    Value convertArrayBounds(Value value, Type* source, Type* target, const SourceLocation& location);
    struct ArrayCopyBack
    {
        Value actual;
        Value temporary;
        Type* sourceType;
        Type* targetType;
        SourceLocation location;
    };
    Value emitArrayArgument(Expr* argument, ParameterMode mode, std::vector<ArrayCopyBack>& copies);
    void emitArrayCopyBacks(const std::vector<ArrayCopyBack>& copies);
    Value lengthOf(const Value& array, Type* type);
    Value emitSlice(CallExpr* expr);
    Value emitConcatenation(BinaryExpr* expr);
    Value emitArrayLogical(Type* type, Value left, const Value* right, const std::string& operation);

    // Expressions and initialization (QbeExpr.cpp).
    Value emitExpr(Expr* expr);
    Value emitExprValue(Expr* expr);
    Value emitAddress(Expr* expr);
    Value emitAllocator(AllocatorExpr* expr);
    Value emitAllocatorValue(AllocatorExpr* expr);
    std::string currentAccessLevel();
    std::string accessLevelFor(Type* type);
    std::string accessLevelFor(Symbol* symbol);
    void captureAccessLevel(Symbol* symbol);
    void preserveAccessView(const Value& value, Type* target, Type* source);
    void checkAccessDiscriminants(const Value& value, Type* type, const std::string& level, const std::string& owner);
    bool hasComponentDefaults(Type* type);
    void emitDefaultInit(const Value& address, Type* type);

    // Calls (QbeCalls.cpp).
    Value emitCall(CallExpr* expr, Type* inheritedControllingType = nullptr);
    Value emitCallValue(CallExpr* expr, Type* inheritedControllingType);
    Value consumeArrayResult(const std::string& resultStorage, int rank, bool adopt);
    Value emitRuntimeCall(CallExpr* expr, Symbol* subprogram);

    // Operators (QbeOperators.cpp).
    Value emitBinary(BinaryExpr* expr);
    Value emitUnary(UnaryExpr* expr);
    Value emitUncheckedConversion(CallExpr* expr);
    Value emitStorageIo(CallExpr* expr, bool reading);
    void emitStorageIoComponents(const Value& target, const Value& source, Type* type, bool reading);
    Value emitDiscreteConversion(const Value& value, Type* source, Type* target);
    Value emitModularOperation(ModularOperation operation, const Value& left, const Value& right, Type* type);
    Value emitIntegerOperation(int operation, const Value& left, const Value& right, char type);
    Value emitRealOperation(RealOperation operation, const Value& left, const Value& right, char type);
    Value emitShortCircuit(BinaryExpr* expr);
    Value emitModulo(const Value& left, const Value& right, char type);
    // Floating ** Integer; integer powers use emitIntegerOperation instead.
    Value emitPower(const Value& left, const Value& right, char type);

    // Comparisons (QbeComparisons.cpp).
    Value compareObjects(const Value& left, const Value& right, Type* type);
    Value compareArrays(BinaryOp op, const Value& left, Type* leftType, const Value& right, Type* rightType);
    Value comparePrimitiveRecord(const Value& left, const Value& right, Type* type);
    Value compareRecords(const Value& left, const Value& right, Type* type);

    // Attributes (QbeAttributes.cpp).
    Value emitAttribute(AttributeExpr* expr);
    Value emitStreamAttribute(AttributeExpr* expr);
    Value emitStreamCall(Symbol* operation, const Value& stream, const Value* item, Type* type,
                         const std::string& constrained = "1", Expr* actual = nullptr);
    Type streamStorageType(Type* type);
    void emitStreamScalarCheck(const Value& value, Type* type, const Type& storage);
    void emitStreamWrappers(Type* type);
    Value emitStreamRecordInput(const Value& stream, Type* type);
    void emitStreamComponents(const Value& stream, bool reading, Value object, Type* type,
                              bool skipDiscriminants = false, const std::string& constrained = "1");
    std::string widenToDouble(const Value& value);
    static int defaultAft(const Type* type);

    // Aggregates (QbeAggregates.cpp).
    Value prepareArrayAggregate(Expr* expr, const Value& context, Type* type, ArrayAggregatePlan& plan);
    Value emitDynamicAggregateInto(AggregateExpr* expr, const Value& address, Type* type,
                                   ArrayAggregatePlan* plan = nullptr);
    void emitAggregateInto(AggregateExpr* expr, const Value& address, Type* type);
    Value emitAggregate(AggregateExpr* expr);

    void emitPragma(PragmaDecl* pragma);
    uint32_t m_suppressedChecks = 0;
    Sema& m_sema;
    Diagnostics& m_diagnostics;
    std::ostringstream m_data;
    std::vector<std::string> m_functions;
    std::vector<SubprogramBody*> m_pendingSubprograms;
    std::unordered_map<std::string, std::string> m_stringPool;
    std::unordered_map<const Type*, std::string> m_enumTables;
    std::unordered_map<const Type*, std::string> m_enumCodeTables;
    std::unordered_map<Type*, bool> m_emittedTags;
    std::vector<Type*> m_streamWrapperTypes;
    std::unordered_map<Type*, bool> m_emittedCollections;
    std::unordered_map<Type*, bool> m_initializedCollections;
    FunctionContext* m_context = nullptr;

    // Strings and tables are private to the object they end up in, so their
    // names carry the unit they were emitted for and no two units collide.
    std::string m_unitTag;
    int m_tempCounter = 0;
    int m_labelCounter = 0;
    int m_dataCounter = 0;

    void beginUnit(const std::string& key);
    void writeUnit(std::ostream& out);
};
