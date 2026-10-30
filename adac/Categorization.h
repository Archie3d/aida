#pragma once

#include "Ast.h"
#include "Diagnostics.h"

class Scope;

// The view at the point of use matters: a private completion is not a promise
// to clients. fullType is used only when checking the promise's full view.
bool hasPreelaborableInitialization(Type* type, Symbol* owner = nullptr, Type* fullType = nullptr, Scope* viewScope = nullptr);
bool preelaborableExpression(const Expr* expression, Symbol* owner = nullptr);
bool supportsExternalStreaming(Type* type);
bool pureExpression(const Expr* expression, Symbol* owner = nullptr);
void checkCategorizedDeclarations(const DeclList& declarations, int category,
    Diagnostics& diagnostics, Symbol* owner = nullptr);
void checkCategorizedStatements(const StmtList& statements, Diagnostics& diagnostics, Symbol* owner = nullptr);
