#pragma once

#include "Ast.h"
#include "Diagnostics.h"
#include "Scope.h"
#include "Type.h"

#include <string>
#include <unordered_map>
#include <vector>

class Sema
{
public:
    explicit Sema(Diagnostics& diagnostics);

    void analyze(CompilationUnit& unit);

    TypeTable& typeTable() { return m_types; }
    Type* exceptionOccurrenceType() const { return m_exceptionOccurrenceType; }
    Symbol* mainSubprogram() const { return m_main; }

    // The exceptions declared in one file, whose objects that file's unit is
    // the one to emit.
    const std::vector<Symbol*>& exceptionsIn(const CompilationUnit* unit) const;

private:
    // Sema.cpp
    void setupStandardScope();
    Symbol* addException(Scope* scope, const std::string& displayName);
    Symbol* addTypeTo(Scope* scope, Type* type);
    void adoptLibraryUnit(PackageSpecDecl* decl, Symbol* package);
    std::string mangle(const std::string& name) const;
    std::string anonymousTypeName();

    // SemaDecl.cpp
    void analyzeDeclarativePart(DeclList& declarations, Scope* scope, bool reportIncomplete = true);
    void analyzeDecl(Decl* decl, Scope* scope);
    void analyzeObjectDecl(ObjectDecl* decl, Scope* scope);
    void analyzeNumberDecl(NumberDecl* decl, Scope* scope);
    Symbol* declareSubprogram(SubprogramSpec& spec, Scope* scope, bool isBody);
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
    bool checkNotPrivate(Type* type, const SourceLocation& location, const char* what);

    // SemaGenerics.cpp
    void analyzeGenericDecl(GenericDecl* decl, Scope* scope);
    bool acceptsFormalType(const GenericFormal& formal, Type* actual, const std::string& genericName,
                           const SourceLocation& location);
    bool bindGenericFormals(GenericInstantiationDecl* decl, Symbol* generic, Scope* bindings, Scope* scope);
    void analyzeGenericInstantiation(GenericInstantiationDecl* decl, Scope* scope);

    // SemaTypes.cpp
    void analyzeTypeDecl(TypeDecl* decl, Scope* scope);
    void analyzeSubtypeDecl(SubtypeDecl* decl, Scope* scope);
    void layoutRecord(TypeDecl* decl, TypeDefinition* definition, Type* type, Scope* scope);
    void reportIncompleteTypes(DeclList& declarations);
    void analyzeRepresentation(RepresentationDecl* decl, Scope* scope);
    Type* resolveSubtypeIndication(SubtypeIndication* indication, Scope* scope, bool allowDynamic = false,
                                  bool allowDynamicScalar = false);
    Type* constrainDiscriminants(SubtypeIndication* indication, Type* base, Scope* scope);
    bool hasKnownDiscriminants(Type* type) const;
    bool discriminantValue(Type* type, int index, long long& value) const;
    int knownVariant(Type* type) const;
    bool typesCompatible(Type* target, Type* source) const;

    // SemaStatements.cpp
    void analyzeStatements(StmtList& statements, Scope* scope);
    void analyzeHandlers(std::vector<ExceptionHandler>& handlers, Scope* scope);
    void checkAssignable(Expr* target, Scope* scope, bool allowLimited = false);
    void analyzeStatement(Stmt* statement, Scope* scope);
    void analyzeCaseStatement(CaseStmt* statement, Scope* scope);

    // SemaNames.cpp
    Symbol* lookupName(const std::string& lower, Scope* scope);
    std::vector<Symbol*> lookupAll(const std::string& lower, Scope* scope);
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
    };
    std::vector<OperatorCandidate> operatorCandidates(const std::string& name, const std::vector<Expr*>& operands,
                                                     Scope* scope, Type* expected);
    ExprPtr bindOperator(Symbol* symbol, std::vector<ExprPtr> operands, Scope* scope,
                         const SourceLocation& location);
    ExprPtr explicitOperator(CallExpr* call);

    // SemaExpr.cpp
    Type* analyzeExpr(Expr* expr, Scope* scope, Type* expected = nullptr);
    Type* analyzeAllocator(AllocatorExpr* expr, Scope* scope, Type* expected);
    void checkPrivateOperands(BinaryExpr* expr);
    Type* analyzeBinary(BinaryExpr* expr, Scope* scope, Type* expected);
    Type* analyzeBinaryOperation(BinaryExpr* expr, Scope* scope, Type* expected, Type* operandContext = nullptr);
    Type* analyzeUnary(UnaryExpr* expr, Scope* scope, Type* expected);
    Type* analyzeMembership(MembershipExpr* expr, Scope* scope);

    // SemaResolution.cpp: inspect interpretations without binding the AST.
    std::vector<Symbol*> expressionNames(Expr* expr, Scope* scope);
    std::vector<Type*> expressionTypes(Expr* expr, Scope* scope, Type* expected = nullptr);
    std::vector<Type*> discoverExpressionTypes(Expr* expr, Scope* scope, Type* expected);
    bool matchesExpression(Expr* expr, Scope* scope, Type* expected);
    bool matchCallArguments(CallExpr* expr, Symbol* candidate, Scope* scope,
                            std::vector<std::size_t>& positions);
    Type* commonOperandType(Expr* left, Expr* right, Scope* scope, Type* expected);

    // SemaCalls.cpp
    Type* analyzeCall(CallExpr* expr, Scope* scope, Type* expected);

    // SemaAttributes.cpp
    Type* analyzeAttribute(AttributeExpr* expr, Scope* scope);

    // SemaAggregates.cpp
    Type* analyzeAggregate(AggregateExpr* expr, Scope* scope, Type* expected);
    bool aggregateDiscriminant(AggregateExpr* expr, Type* record, Scope* scope, int index, long long& value,
                               Expr** source = nullptr);
    bool checkAggregateDiscriminants(AggregateExpr* expr, Type* target, Scope* scope);

    // SemaStatic.cpp
    ExprPtr scalarBoundExpr(Type* type, bool first, const SourceLocation& location);
    bool foldStatic(Expr* expr, long long& value) const;
    bool foldStaticReal(Expr* expr, double& value) const;
    void noteStaticValue(Expr* expr);

    // SemaChoices.cpp
    bool resolveChoice(Expr* lowExpr, Expr* highExpr, Type* selectorType, const std::vector<CaseChoice>& covered,
                       Scope* scope, CaseChoice& choice);
    void reportUncovered(std::vector<CaseChoice> covered, Type* selectorType, const SourceLocation& location,
                         const char* what);
    Type* choiceSubtypeMark(Expr* expr, Scope* scope);
    std::string describeValue(Type* type, long long value) const;

    // A discovery pass uses one scope and never changes bindings. Discard its
    // memoized domains before the next pass, when declarations may have changed.
    std::unordered_map<Expr*, std::unordered_map<Type*, std::vector<Type*>>> m_resolutionTypes;
    int m_resolutionDepth = 0;

    Diagnostics& m_diagnostics;
    TypeTable m_types;
    SymbolTable m_symbolTable;
    Scope* m_standardScope = nullptr;
    Scope* m_globalScope = nullptr;

    // The exceptions each file declared, since the object standing for one
    // belongs to the unit it was written in and to no other.
    const CompilationUnit* m_currentUnit = nullptr;
    std::unordered_map<const CompilationUnit*, std::vector<Symbol*>> m_unitExceptions;

    // The packages being analysed, innermost last.  A private type is only
    // transparent while one of them declared it.
    std::vector<Symbol*> m_packages;

    // Set while the visible part of a package specification is being analysed,
    // which is the only place a constant may be named without a value.
    bool m_inVisiblePart = false;
    Type* m_textFileType = nullptr;
    Type* m_fieldType = nullptr;
    Type* m_numberBaseType = nullptr;
    Type* m_typeSetType = nullptr;
    Type* m_addressType = nullptr;
    Type* m_exceptionOccurrenceType = nullptr;
    Type* m_exceptionIdType = nullptr;
    Symbol* m_main = nullptr;
    Symbol* m_currentSubprogram = nullptr;
    int m_handlerDepth = 0;
    std::vector<LoopStmt*> m_loops;
    std::vector<std::string> m_namePrefix;
    std::unordered_map<std::string, std::size_t> m_subprogramNames;
    int m_anonymousCounter = 0;
    int m_instantiationDepth = 0;
};
