#pragma once

#include "Ast.h"
#include "Type.h"

#include <string>
#include <algorithm>
#include <vector>

class Scope;

enum class SymbolKind
{
    Object,
    Number,
    TypeName,
    Subprogram,
    Parameter,
    Package,
    EnumerationLiteral,
    Exception,
    LoopParameter,
    Generic
};

enum class BuiltinKind
{
    None,

    // A plain call into the run time library.  The symbol names the C entry
    // point and the parameter list drives how the arguments are marshalled, so
    // that a whole package can be declared without new code in the emitter.
    Runtime
};

struct Symbol
{
    SymbolKind kind = SymbolKind::Object;
    std::string name;         // Lower-cased, used for lookup.
    std::string displayName;  // Original spelling.
    Type* type = nullptr;
    SourceLocation location;

    // Objects, constants and parameters.
    bool isConstant = false;
    bool aliased = false;
    bool awaitsValue = false;   // A deferred constant, until its value is given.
    bool isGlobal = false;
    bool byReference = false;
    bool genericObject = false;
    Type* discriminantRecord = nullptr;
    int discriminantIndex = -1;
    int shared = 0;
    Expr* sharedAlias = nullptr;
    bool classWideObject = false;
    bool objectReference = false; // A slot holding a renamed or generic actual object address.
    ParameterMode mode = ParameterMode::In;
    bool hasStaticValue = false;
    long long staticValue = 0;
    double staticReal = 0.0;
    ExactReal exactReal;
    std::string qbeName;   // Global symbol name, including the '$' sigil.

    // A parameter the caller may leave out.
    bool hasDefault = false;

    // The expression a parameter declared in Ada falls back on.  It belongs to
    // the declaration, so a call that leaves the parameter out simply points at
    // it rather than making a copy.
    Expr* defaultExpr = nullptr;
    std::vector<Token> defaultTokens;

    // Subprograms.
    std::vector<Symbol*> parameters;
    Type* returnType = nullptr;
    BuiltinKind builtin = BuiltinKind::None;
    bool hasBody = false;
    bool taskBody = false;
    Symbol* taskIdentity = nullptr;
    Type* protectedType = nullptr;
    Symbol* protectedReceiver = nullptr;
    bool protectedPrivate = false;
    bool entry = false;
    bool entryBarrierFunction = false;
    int protectedField = -1;
    bool requiresBody = false;
    bool missingBodyReported = false;
    // An inherited operation uses its own profile and the original implementation.
    Symbol* inheritedFrom = nullptr;
    Symbol* privatePrimitiveTo = nullptr;
    bool overrides = false;
    Type* controllingType = nullptr;
    int dispatchSlot = -1;
    int pendingOverride = 0;
    bool pendingControlledResult = false;
    Symbol* renamedSubprogram = nullptr;
    Symbol* renamedAccess = nullptr;
    bool intrinsicRenaming = false;
    Scope* declarationScope = nullptr;
    bool addressTaken = false;
    // Code pointer and static link, stored once in the enclosing frame.
    long long descriptorOffset = -1;
    // Body readiness belongs to the enclosing activation, beside captures.
    long long elaborationOffset = -1;
    Symbol* negatedEquality = nullptr; // Implicit Boolean "/=" delegates to "=".


    // Run time builtins.  The symbol names the C entry point, and a call into it
    // may leave an exception pending.
    std::string runtimeSymbol;
    bool importAda = false;
    bool canRaise = false;

    // Packages.  A child names the parent it was declared in, which is what
    // lets it see what its parent keeps private.
    Scope* scope = nullptr;
    Symbol* parentPackage = nullptr;
    std::string libraryName;
    bool hasSpecification = false;
    bool containsTasks = false;
    PackageSpecDecl* packageDeclaration = nullptr;

    // Generic units, which keep the declaration they came from so that an
    // instantiation can read the tokens again.
    struct GenericDecl* generic = nullptr;

    // Enumeration literals.
    long long enumerationValue = 0;

    // Exceptions.  Identity is the address of the object named here, which the
    // run time owns for the predefined exceptions and the declaring unit emits
    // for every other one.
    std::string exceptionObject;

    // Nesting: subprograms know their static level, objects know the subprogram
    // that owns them.  Objects referenced from a nested subprogram live in the
    // owner's frame block instead of a plain stack slot.
    int level = 0;
    int accessibilityLevel = 0;
    bool protectedAccessProfile = false;
    bool needsAccessibility = false;
    long long accessibilityOffset = -1;
    Symbol* owner = nullptr;
    bool isUplevel = false;
    long long frameOffset = -1;
    long long constrainedOffset = -1; // Actual's constrainedness for a writable record formal.
    long long frameSize = 8;
    bool needsFrame = false;
    bool usesFinalization = false;
};

inline bool needsConstrainedParameter(const Symbol* parameter)
{
    return parameter->mode != ParameterMode::In && hasDefaultDiscriminants(parameter->type)
        && !hasDiscriminantConstraint(parameter->type);
}

inline int sharedObjectKind(const Expr* expr);

inline int sharedSymbolKind(const Symbol* symbol)
{
    if (symbol == nullptr) {
        return 0;
    }
    int kind = std::max(symbol->shared, sharedTypeKind(symbol->type));
    return std::max(kind, sharedObjectKind(symbol->sharedAlias));
}

inline int sharedObjectKind(const Expr* expr)
{
    if (expr == nullptr) {
        return 0;
    }
    if (expr->kind == ExprKind::Identifier) {
        return sharedSymbolKind(static_cast<const IdentifierExpr*>(expr)->symbol);
    }
    if (expr->kind == ExprKind::Selected) {
        const auto* selected = static_cast<const SelectedExpr*>(expr);
        return std::max(sharedSymbolKind(selected->symbol), sharedTypeKind(expr->type));
    }
    return sharedTypeKind(expr->type);
}
