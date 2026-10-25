#pragma once

#include "Ast.h"
#include "Diagnostics.h"
#include "Scope.h"
#include "Type.h"

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

class Sema
{
public:
    explicit Sema(Diagnostics& diagnostics);

    void prepareLimitedViews(const std::vector<CompilationUnit*>& units);
    void analyze(CompilationUnit& unit);
    void finishAnalysis(bool requireLibraryBodies = true);

    TypeTable& typeTable() { return m_types; }
    Type* exceptionOccurrenceType() const { return m_exceptionOccurrenceType; }
    Symbol* mainSubprogram() const { return m_main; }

    // The exceptions declared in one file, whose objects that file's unit is
    // the one to emit.
    const std::vector<Symbol*>& exceptionsIn(const CompilationUnit* unit) const;
    Symbol* elaborationOwner(const CompilationUnit* unit) const { return m_elaborationOwners.at(unit); }
    bool usesFinalization(const CompilationUnit* unit) const { return m_finalizingUnits.contains(unit); }

private:
    // SemaLimited.cpp
    Type* completeView(Type* type, Scope* scope);
    bool checkCompleteView(Type* type, const SourceLocation& location, const char* use);
    void finishLimitedViews();
    std::unordered_map<std::string, Symbol*> m_limitedPackages;
    std::unordered_map<std::string, Symbol*> m_limitedPackageViews;
    std::unordered_map<const TypeDecl*, Type*> m_limitedCompletions;
    std::vector<Type*> m_limitedTypes;

    // Sema.cpp
    void setupStandardScope();
    Symbol* addException(Scope* scope, const std::string& displayName);
    Symbol* addTypeTo(Scope* scope, Type* type);
    void adoptLibraryUnit(PackageSpecDecl* decl, Symbol* package);
    std::string mangle(const std::string& name) const;
    std::string anonymousTypeName();

    // SemaDecl.cpp
    void analyzeDeclarativePart(DeclList& declarations, Scope* scope, bool reportIncomplete = true);
    void reportMissingBodies(const DeclList& declarations);
    void analyzeDecl(Decl* decl, Scope* scope);
    void analyzeProtected(TypeDecl* decl, Scope* scope);
    void declareProtectedOperation(Symbol* operation, Type* type);
    Symbol* protectedReceiver(Type* type);
    bool isProtectedCurrentInstance(Expr* expr, Type* type, Scope* scope);
    void bindProtectedCall(Expr* expr, Symbol* operation, Expr* object, Scope* scope);
    void analyzeObjectDecl(ObjectDecl* decl, Scope* scope);
    void analyzeNumberDecl(NumberDecl* decl, Scope* scope);
    void registerPrimitive(Symbol* symbol, Scope* scope);
    void inheritPrimitives(Type* type, Type* parent, Scope* scope, const SourceLocation& location);
    Scope* subunitScope(const std::shared_ptr<SubunitInfo>& info, Scope* parent);
    Symbol* declareSubprogram(SubprogramSpec& spec, Scope* scope, bool isBody, bool isFormal = false, Scope* profileScope = nullptr);
    void analyzeSubprogramRenaming(SubprogramDecl* decl, Scope* scope);
    void analyzeSubprogramBody(SubprogramBody* body, Scope* scope);
    void analyzeExceptionDecl(ExceptionDecl* decl, Scope* scope);
    void analyzePragma(PragmaDecl* decl, Scope* scope);

    // SemaPackages.cpp
    Symbol* declarePackagePath(const std::string& lower, const std::string& displayName, Scope* scope,
                               const SourceLocation& location, std::size_t& pushed);
    void analyzePackageSpec(PackageSpecDecl* decl, Scope* scope);
    void analyzePackageBody(PackageBodyDecl* decl, Scope* scope);
    void analyzeUseClause(UseDecl& decl, Scope* scope);
    bool withinPackage(Symbol* package) const;
    bool representationVisible(Type* type) const;
    bool taggedViewVisible(Type* type) const;
    bool buildsLimitedResult(Expr* expression) const;
    bool checkNotPrivate(Type* type, const SourceLocation& location, const char* what);

    // SemaGenerics.cpp
    // Retain contract ASTs because symbols refer to parameter defaults and
    // constraint expressions. Stable source keys reconnect reparsed instances
    // to the declarations and predefined/operator choices checked here.
    struct GenericContract
    {
        Scope* environment = nullptr;
        Scope* bindings = nullptr;
        std::string unitName;
        std::size_t tokenCount = 0;
        std::unordered_set<Symbol*> localSymbols;
        DeclList declarations;
        DeclList profiles;
        std::unordered_map<std::string, Symbol*> names;
        std::unordered_map<std::string, Symbol*> operators;
        std::unordered_map<std::string, Symbol*> subprograms;
        std::unordered_map<std::string, Symbol*> typeNames;
        std::unordered_map<std::string, Symbol*> unitNames;
        std::unordered_map<std::string, std::vector<Symbol*>> instances;
        bool valid = false;
    };
    // An instance specification must remain visible while its template body
    // is still unavailable. Resume in the original instance scope, retaining
    // its formal bindings and all specification symbol/type identities.
    struct PendingGenericInstance
    {
        GenericInstantiationDecl* instance = nullptr;
        Scope* bindings = nullptr;
        GenericContract* contract = nullptr;
        GenericContract* enclosingContract = nullptr;
        std::size_t tokenCount = 0;
        std::unordered_map<Symbol*, Symbol*> copies;
        Symbol* owner = nullptr;
        const CompilationUnit* unit = nullptr;
        std::vector<Symbol*> packages;
        std::vector<std::string> prefix;
        int accessibilityLevel = 0;
        int depth = 0;
    };
    std::vector<PendingGenericInstance> m_pendingGenericInstances;
    void completeGenericInstance(PendingGenericInstance pending);
    std::unordered_map<GenericDecl*, std::unique_ptr<GenericContract>> m_genericContracts;
    GenericContract* m_recordContract = nullptr;
    bool m_genericBodyContext = false;
    GenericContract* m_replayContract = nullptr;
    GenericContract* m_instanceOwnerContract = nullptr;
    std::size_t m_instanceFirstSymbol = 0;
    std::unordered_map<Symbol*, Symbol*> m_instanceCopies;
    static std::string contractKey(const SourceLocation& location);
    static std::string nameContractKey(Expr* expr);
    static std::string operatorContractKey(const std::string& name, const std::vector<Expr*>& operands);
    Symbol* instanceSymbol(Symbol* symbol);
    std::vector<Symbol*> contractNames(Expr* expr, std::vector<Symbol*> candidates);
    void recordContractName(Expr* expr, Symbol* symbol);
    void checkGenericContract(GenericDecl* decl, Scope* scope);
    void analyzeGenericDecl(GenericDecl* decl, Scope* scope);
    Symbol* declareUnitRenaming(const std::string& name, const std::string& lower,
                                const std::string& renamed, const std::string& renamedLower,
                                SourceLocation location, Scope* scope, bool isGeneric,
                                bool isPackage, bool isFunction);
    bool acceptsFormalType(const GenericFormal& formal, Type* actual, const std::string& genericName,
                           const SourceLocation& location);
    bool bindFormalSubprogram(GenericInstantiationDecl* decl, const GenericFormal& formal,
                              Expr* actual, Scope* bindings, Scope* actualScope, bool namedDefault,
                              GenericContract* contract, std::size_t firstSymbol);
    bool matchesFormalArray(const GenericFormal& formal, Type* actual, Scope* bindings,
                            const SourceLocation& location);
    bool bindGenericFormals(GenericInstantiationDecl* decl, Symbol* generic, Scope* bindings, Scope* scope);
    void analyzeGenericInstantiation(GenericInstantiationDecl* decl, Scope* scope);

    // SemaTypes.cpp
    void analyzeTypeDecl(TypeDecl* decl, Scope* scope);
    void analyzeSubtypeDecl(SubtypeDecl* decl, Scope* scope);
    void layoutRecord(TypeDecl* decl, TypeDefinition* definition, Type* type, Scope* scope);
    void reportIncompleteTypes(DeclList& declarations);
    void analyzeRecordRepresentation(RepresentationDecl* decl, Scope* scope, Type* type, Type* precedingType);
    void analyzeEnumerationRepresentation(RepresentationDecl* decl, Scope* scope, Type* type, Type* precedingType);
    void analyzeRepresentation(RepresentationDecl* decl, Scope* scope, Type* precedingType = nullptr);
    Type* resolveSubtypeIndication(SubtypeIndication* indication, Scope* scope, bool allowDynamic = false,
                                  bool allowDynamicScalar = false);
    void recordArrayConstraint(Type* type, SubtypeIndication* indication);
    void recordScalarConstraint(Type* type, const SourceLocation& location, bool objectConstraint);
    Type* constrainDiscriminants(SubtypeIndication* indication, Type* base, Scope* scope);
    bool hasKnownDiscriminants(Type* type) const;
    bool discriminantValue(Type* type, int index, long long& value) const;
    bool typesCompatible(Type* target, Type* source) const;

    // SemaStatements.cpp
    void analyzeStatements(StmtList& statements, Scope* scope);
    void declareStatementNames(StmtList& statements, Scope* scope);
    void analyzeBodyStatements(StmtList& statements, std::vector<ExceptionHandler>& handlers,
                               Scope* scope, const std::string& name, Symbol* identifier);
    void analyzeHandlers(std::vector<ExceptionHandler>& handlers, Scope* scope);
    void checkAssignable(Expr* target, Scope* scope, bool allowLimited = false, bool allowArrayView = false);
    void analyzeStatement(Stmt* statement, Scope* scope);
    void analyzeCaseStatement(CaseStmt* statement, Scope* scope);

    // SemaNames.cpp
    Symbol* lookupName(const std::string& lower, Scope* scope);
    std::vector<Symbol*> lookupAll(const std::string& lower, Scope* scope);
    Type* classWideType(Type* specific);
    void dispatchIndeterminate(Expr* expr, Type* controlling);
    Type* resolveTypeName(const std::string& lower, Scope* scope, const SourceLocation& location);
    void noteReference(Symbol* symbol);
    bool matchesResult(Symbol* subprogram, Type* expected) const;
    Symbol* resolveBareName(const std::vector<Symbol*>& candidates, Type* expected,
                            const SourceLocation& location, const std::string& name);
    Type* analyzeIdentifier(IdentifierExpr* expr, Scope* scope, Type* expected);
    Type* analyzeSelected(SelectedExpr* expr, Scope* scope, Type* expected);

    // SemaOperators.cpp
    struct OperatorCandidate
    {
        Symbol* symbol = nullptr; // Null denotes a predefined operator.
        std::vector<Type*> parameters;
        Type* result = nullptr;
        std::vector<std::size_t> argumentOrder; // Formal order for an explicit named call.
    };
    std::vector<OperatorCandidate> operatorCandidates(const std::string& name, const std::vector<Expr*>& operands,
                                                     Scope* scope, Type* expected, bool predefinedOnly = false,
                                                     const std::vector<std::string>& argumentNames = {},
                                                     Scope* selectedScope = nullptr);
    ExprPtr bindOperator(Symbol* symbol, std::vector<ExprPtr> operands, Scope* scope,
                         const SourceLocation& location);
    std::string explicitOperatorName(CallExpr* call, Scope* scope, bool& standard);
    std::vector<OperatorCandidate> explicitOperatorCandidates(CallExpr* call, Scope* scope, Type* expected);
    ExprPtr explicitOperator(CallExpr* call, Scope* scope, Type* expected = nullptr);

    // SemaExpr.cpp
    Type* analyzeCondition(Expr* expr, Scope* scope);
    Type* analyzeExpr(Expr* expr, Scope* scope, Type* expected = nullptr);
    Type* analyzeExprValue(Expr* expr, Scope* scope, Type* expected);
    Type* analyzeAllocator(AllocatorExpr* expr, Scope* scope, Type* expected);
    void checkPrivateOperands(BinaryExpr* expr);
    Type* analyzeBinary(BinaryExpr* expr, Scope* scope, Type* expected);
    Type* analyzeBinaryOperation(BinaryExpr* expr, Scope* scope, Type* expected, Type* operandContext = nullptr, Type* rightContext = nullptr);
    Type* analyzeUnary(UnaryExpr* expr, Scope* scope, Type* expected);
    Type* analyzeMembership(MembershipExpr* expr, Scope* scope);

    // SemaResolution.cpp: inspect interpretations without binding the AST.
    std::vector<Symbol*> prefixedOperations(SelectedExpr* expr, Scope* scope);
    Type* prefixedPrefixType(SelectedExpr* expr, Symbol* operation, Scope* scope, bool* ambiguous = nullptr);
    std::vector<Symbol*> expressionNames(Expr* expr, Scope* scope);
    std::vector<Type*> expressionTypes(Expr* expr, Scope* scope, Type* expected = nullptr);
    std::vector<Type*> discoverExpressionTypes(Expr* expr, Scope* scope, Type* expected);
    bool matchesExpression(Expr* expr, Scope* scope, Type* expected);
    bool matchCallArguments(CallExpr* expr, Symbol* candidate, Scope* scope,
                            std::vector<std::size_t>& positions, SelectedExpr* prefix = nullptr);
    Type* commonOperandType(Expr* left, Expr* right, Scope* scope, Type* expected);

    // SemaCalls.cpp
    Type* analyzeCall(CallExpr* expr, Scope* scope, Type* expected);

    // SemaAttributes.cpp
    Type* analyzeAttribute(AttributeExpr* expr, Scope* scope, Type* expected = nullptr);
    Type* resolveAnonymousAccess(SubtypeIndication* indication, Scope* scope);
    bool compatibleAnonymousAccess(Type* target, Type* source) const;
    void checkAnonymousAccess(Expr* expr, Type* expected, Type* actual);
    Type* analyzeObjectAccess(AttributeExpr* expr, Scope* scope, Type* expected);
    bool checkObjectAccessConversion(CallExpr* expr, Type* target, Type* source);
    bool matchesAccessProfile(Symbol* candidate, Type* access) const;

    // SemaAggregates.cpp
    Type* analyzeAggregate(AggregateExpr* expr, Scope* scope, Type* expected);
    bool aggregateDiscriminant(AggregateExpr* expr, Type* record, Scope* scope, int index, long long& value,
                               Expr** source = nullptr);
    bool checkAggregateDiscriminants(AggregateExpr* expr, Type* target, Scope* scope);

    // SemaStatic.cpp
    ExprPtr scalarBoundExpr(Type* type, bool first, const SourceLocation& location);
    bool foldStatic(Expr* expr, long long& value) const;
    bool foldWideInteger(Expr* expr, __int128& value, StaticFailure& failure) const;
    void validateStatic(Expr* expr, bool enclosingStatic = false, Type* expected = nullptr);
    bool exactValue(Expr* expr, ExactReal& value) const;
    bool foldFixed(Expr* expr, long long& value) const;
    bool foldStaticReal(Expr* expr, double& value) const;
    void noteStaticValue(Expr* expr);

    // SemaChoices.cpp
    bool resolveChoice(Expr* lowExpr, Expr* highExpr, Type* selectorType, const std::vector<CaseChoice>& covered,
                       Scope* scope, CaseChoice& choice);
    void reportUncovered(std::vector<CaseChoice> covered, Type* selectorType, const SourceLocation& location,
                         const char* what);
    Type* choiceSubtypeMark(Expr* expr, Scope* scope);
    std::string describeValue(Type* type, __int128 value) const;

    // A discovery pass uses one scope and never changes bindings. Discard its
    // memoized domains before the next pass, when declarations may have changed.
    std::unordered_map<Expr*, std::unordered_map<Type*, std::vector<Type*>>> m_resolutionTypes;
    int m_resolutionDepth = 0;
    int m_expressionDepth = 0;

    Diagnostics& m_diagnostics;
    TypeTable m_types;
    SymbolTable m_symbolTable;
    Scope* m_standardScope = nullptr;
    Scope* m_globalScope = nullptr;
    Scope* m_unitScope = nullptr;
    std::unordered_map<std::string, Scope*> m_libraryContexts;

    // The exceptions each file declared, since the object standing for one
    // belongs to the unit it was written in and to no other.
    const CompilationUnit* m_currentUnit = nullptr;
    std::unordered_map<const CompilationUnit*, Symbol*> m_elaborationOwners;
    std::unordered_set<const CompilationUnit*> m_finalizingUnits;
    std::unordered_map<const CompilationUnit*, std::vector<Symbol*>> m_unitExceptions;

    // The packages being analysed, innermost last.  A private type is only
    // transparent while one of them declared it.
    std::vector<Symbol*> m_packages;

    // Set while the visible part of a package specification is being analysed,
    // which is the only place a constant may be named without a value.
    bool m_inVisiblePart = false;
    Scope* m_packageSpecScope = nullptr;
    Type* m_textFileType = nullptr;
    Type* m_fieldType = nullptr;
    Type* m_numberBaseType = nullptr;
    Type* m_typeSetType = nullptr;
    Type* m_addressType = nullptr;
    Type* m_tagType = nullptr;
    Type* m_exceptionOccurrenceType = nullptr;
    Type* m_exceptionIdType = nullptr;
    Type* m_taskIdType = nullptr;
    Type* m_realTimeType = nullptr;
    std::vector<Expr*> m_sharedEscapes;
    std::vector<std::pair<Expr*, Type*>> m_sharedGenericActuals;
    Symbol* m_main = nullptr;
    Symbol* m_currentSubprogram = nullptr;
    Decl* m_scalarConstraintDeclaration = nullptr;
    bool m_allocatorDiscriminantConstraint = false;
    std::vector<std::pair<Type*, SourceLocation>> m_deferredProtectedUses;
    Type* m_currentProtectedType = nullptr;
    Scope* m_protectedBodyScope = nullptr;
    int m_accessibilityLevel = 0;
    int m_handlerDepth = 0;
    std::vector<Symbol*> m_extendedReturns;
    std::vector<LoopStmt*> m_loops;
    std::vector<StmtList*> m_statementSequences;
    std::unordered_set<std::string> m_statementIdentifiers;
    struct StatementRegion
    {
        std::string name;
        Scope* scope;
        Symbol* identifier;
    };
    std::vector<StatementRegion> m_statementRegions;
    std::vector<std::string> m_namePrefix;
    std::unordered_map<std::string, std::size_t> m_subprogramNames;
    std::unordered_map<std::string, std::size_t> m_tagNames;
    int m_anonymousCounter = 0;
    int m_instantiationDepth = 0;
};
