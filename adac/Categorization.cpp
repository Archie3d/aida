#include "Categorization.h"
#include "Scope.h"

#include <unordered_set>

namespace
{

bool constraints(const Type* type, Symbol* owner);

bool initialization(Type* type, Symbol* owner, Type* fullType, std::unordered_set<Type*>& visiting, Scope* viewScope)
{
    if (type == nullptr) { return false; }
    if (type->isSubtype && type->base != nullptr) { return initialization(type->base, owner, fullType, visiting, viewScope); }
    if (type->taskBody != nullptr || type->classRoot != nullptr) { return false; }
    if (type != fullType && ((type->privateTo != nullptr && type->privateTo != owner
            && (viewScope == nullptr || !viewScope->privateVisible(type->privateTo)))
        || type->protectedScope != nullptr || (type->genericFormal && (type->formalDerived || type->kind == TypeKind::Record)))) {
        return type->preelaborableInitialization;
    }
    if (type->isIncomplete) { return type != fullType && type->preelaborableInitialization; }
    if (type->protectedScope != nullptr) {
        for (const auto& [name, symbols] : type->protectedScope->declarations()) {
            for (const Symbol* symbol : symbols) { if (symbol->entry) { return false; } }
        }
    }
    // Recursion through an access component terminates at its elementary type.
    if (!visiting.insert(type).second) { return false; }
    bool result = constraints(type, owner);
    if (type->parentType != nullptr) { result = result && initialization(type->parentType, owner, nullptr, visiting, viewScope); }
    if (type->controlled) {
        for (const Symbol* operation : type->primitives) {
            if (operation->name == "initialize" && operation->overrides && operation->inheritedFrom == nullptr) {
                result = false;
            }
        }
    }
    if (type->kind == TypeKind::Array) { result = result && initialization(type->element, owner, nullptr, visiting, viewScope); }
    if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : type->fields) {
            result = result && (field.defaultValue != nullptr
                ? preelaborableExpression(field.defaultValue, owner)
                : initialization(field.type, owner, nullptr, visiting, viewScope));
        }
    }
    visiting.erase(type);
    return result;
}

bool objectName(const Symbol* symbol)
{
    return symbol != nullptr && (symbol->kind == SymbolKind::Object || symbol->kind == SymbolKind::Parameter
        || symbol->kind == SymbolKind::LoopParameter);
}

bool staticScalarType(const Type* type)
{
    if (type == nullptr || !isScalar(type) || type->kind == TypeKind::Access) { return false; }
    for (const Type* view = type; view != nullptr; view = view->isSubtype ? view->base : view->parentType) {
        if (view->genericFormal) { return false; }
    }
    return true;
}

bool stringType(const Type* type)
{
    if (type == nullptr || type->kind != TypeKind::Array || type->arrayRank != 1) { return false; }
    const Type* element = rootType(type->element);
    while (element != nullptr && element->parentType != nullptr) { element = element->parentType; }
    // The predefined character types have implicit literal sets; ordinary
    // enumeration types carry their declared literals explicitly.
    return element != nullptr && element->kind == TypeKind::Enumeration
        && element->literals.empty() && !element->genericFormal;
}

bool staticFunction(const Symbol* symbol)
{
    if (symbol == nullptr || symbol->genericFormalSubprogram) { return false; }
    if (symbol->kind == SymbolKind::EnumerationLiteral) { return true; }
    if (!symbol->genericIntrinsic.empty()) {
        const std::string& operation = symbol->genericIntrinsic;
        if (operation == "&" && stringType(symbol->returnType)) { return true; }
        if (!staticScalarType(symbol->returnType)) { return false; }
        for (const Symbol* parameter : symbol->parameters) { if (!staticScalarType(parameter->type)) { return false; } }
        if (operation.starts_with("attribute:")) {
            Type* prefix = symbol->intrinsicPrefix;
            return staticScalarType(prefix) && prefix->scalarBoundsSymbol == nullptr;
        }
        return true;
    }
    if (symbol->genericActualEntity != nullptr) { return staticFunction(symbol->genericActualEntity); }
    return symbol->renamedSubprogram != nullptr && staticFunction(symbol->renamedSubprogram);
}

bool preelaborableName(const Expr* expression, Symbol* owner)
{
    if (expression == nullptr || expression->implicitCall != nullptr) { return false; }
    if (expression->kind == ExprKind::Identifier) { return true; }
    if (expression->kind == ExprKind::Selected) {
        const auto* selected = static_cast<const SelectedExpr*>(expression);
        return selected->isDereference ? preelaborableExpression(selected->prefix.get(), owner)
            : preelaborableName(selected->prefix.get(), owner);
    }
    if (expression->kind == ExprKind::Call) {
        const auto* call = static_cast<const CallExpr*>(expression);
        if (call->form != CallForm::Indexing && call->form != CallForm::Slice) { return false; }
        if (!preelaborableName(call->callee.get(), owner)) { return false; }
        for (const auto& argument : call->arguments) {
            if (!preelaborableExpression(argument.value.get(), owner)
                || !preelaborableExpression(argument.high.get(), owner)) { return false; }
        }
        return true;
    }
    return false;
}

bool constraints(const Type* type, Symbol* owner)
{
    if (type == nullptr) { return true; }
    if (!preelaborableExpression(type->scalarLow, owner) || !preelaborableExpression(type->scalarHigh, owner)) { return false; }
    for (const auto& [low, high] : type->boundExpressions) {
        if (!preelaborableExpression(low, owner) || !preelaborableExpression(high, owner)) { return false; }
    }
    for (const Expr* value : type->discriminantExpressions) {
        if (!preelaborableExpression(value, owner)) { return false; }
    }
    return true;
}

}

bool hasPreelaborableInitialization(Type* type, Symbol* owner, Type* fullType, Scope* viewScope)
{
    std::unordered_set<Type*> visiting;
    return initialization(type, owner, fullType, visiting, viewScope);
}

bool preelaborableExpression(const Expr* expression, Symbol* owner)
{
    if (expression == nullptr) { return true; }
    if (expression->implicitCall != nullptr) { return preelaborableExpression(expression->implicitCall.get(), owner); }
    if (expression->isStatic) { return true; }
    switch (expression->kind) {
    case ExprKind::Identifier: {
        const Symbol* symbol = static_cast<const IdentifierExpr*>(expression)->symbol;
        return symbol == nullptr || ((symbol->kind != SymbolKind::Subprogram || staticFunction(symbol))
            && (!objectName(symbol) || symbol->discriminantIndex >= 0));
    }
    case ExprKind::Selected: {
        const auto* selected = static_cast<const SelectedExpr*>(expression);
        return !selected->isDereference && (selected->symbol == nullptr || (selected->symbol->kind != SymbolKind::Subprogram || staticFunction(selected->symbol))) && !objectName(selected->symbol) && selected->fieldIndex < 0
            && preelaborableExpression(selected->prefix.get(), owner);
    }
    case ExprKind::Call: {
        const auto* call = static_cast<const CallExpr*>(expression);
        if (call->operatorExpression != nullptr) { return preelaborableExpression(call->operatorExpression.get(), owner); }
        if (call->indirect || (call->form == CallForm::Subprogram && !staticFunction(call->subprogram))) { return false; }
        if (call->form != CallForm::Subprogram && !preelaborableExpression(call->callee.get(), owner)) { return false; }
        for (const Expr* argument : call->resolvedArguments) {
            if (!preelaborableExpression(argument, owner)) { return false; }
        }
        for (const Association& argument : call->arguments) {
            if (!preelaborableExpression(argument.value.get(), owner) || !preelaborableExpression(argument.high.get(), owner)) { return false; }
        }
        return true;
    }
    case ExprKind::Unary: {
        const auto* unary = static_cast<const UnaryExpr*>(expression);
        return unary->operatorCall == nullptr && staticScalarType(unary->operand->type)
            && staticScalarType(expression->type) && preelaborableExpression(unary->operand.get(), owner);
    }
    case ExprKind::Binary: {
        const auto* binary = static_cast<const BinaryExpr*>(expression);
        bool function = binary->op == BinaryOp::AndThen || binary->op == BinaryOp::OrElse
            || (binary->op == BinaryOp::Concatenate && stringType(expression->type))
            || (staticScalarType(binary->left->type) && staticScalarType(binary->right->type) && staticScalarType(expression->type));
        return binary->operatorCall == nullptr && function && preelaborableExpression(binary->left.get(), owner)
            && preelaborableExpression(binary->right.get(), owner);
    }
    case ExprKind::Aggregate: {
        const auto* aggregate = static_cast<const AggregateExpr*>(expression);
        if (aggregate->ancestorIsType && !hasPreelaborableInitialization(aggregate->ancestorType, owner, nullptr)) { return false; }
        if (!preelaborableExpression(aggregate->ancestor.get(), owner)) { return false; }
        for (const auto& component : aggregate->components) {
            if (component.value->kind != ExprKind::Box && !preelaborableExpression(component.value.get(), owner)) { return false; }
            if (component.value->kind == ExprKind::Box && expression->type != nullptr && expression->type->kind == TypeKind::Array
                && !hasPreelaborableInitialization(expression->type->element, owner, nullptr)) { return false; }
            for (const auto& choice : component.choiceLows) { if (!preelaborableExpression(choice.get(), owner)) { return false; } }
            for (const auto& choice : component.choiceHighs) { if (!preelaborableExpression(choice.get(), owner)) { return false; } }
        }
        for (std::size_t i = 0; i < aggregate->resolvedFields.size(); ++i) {
            const Expr* value = aggregate->resolvedFields[i];
            if (value == nullptr) { continue; }
            if (value->kind == ExprKind::Box && expression->type != nullptr) {
                const FieldInfo& field = expression->type->fields[i];
                if (field.defaultValue != nullptr) {
                    if (!preelaborableExpression(field.defaultValue, owner)) { return false; }
                } else if (!hasPreelaborableInitialization(field.type, owner, nullptr)) { return false; }
            } else if (!preelaborableExpression(value, owner)) { return false; }
        }
        return true;
    }
    case ExprKind::Qualified:
        return preelaborableExpression(static_cast<const QualifiedExpr*>(expression)->operand.get(), owner);
    case ExprKind::Attribute: {
        const auto* attribute = static_cast<const AttributeExpr*>(expression);
        // An attribute evaluates its prefix as a name, not as an object
        // primary. Index expressions and calls in that name still execute.
        const Expr* prefix = attribute->prefix.get();
        if (!preelaborableName(prefix, owner)
            && !preelaborableExpression(prefix, owner)) { return false; }
        for (const auto& argument : attribute->arguments) { if (!preelaborableExpression(argument.get(), owner)) { return false; } }
        return attribute->lower != "input" && attribute->lower != "output" && attribute->lower != "read"
            && attribute->lower != "write" && attribute->lower != "image" && attribute->lower != "value";
    }
    case ExprKind::Membership: {
        const auto* membership = static_cast<const MembershipExpr*>(expression);
        return preelaborableExpression(membership->operand.get(), owner)
            && preelaborableExpression(membership->low.get(), owner) && preelaborableExpression(membership->high.get(), owner);
    }
    case ExprKind::Allocator: {
        const auto* allocator = static_cast<const AllocatorExpr*>(expression);
        return (allocator->value != nullptr ? preelaborableExpression(allocator->value.get(), owner)
            : hasPreelaborableInitialization(allocator->designated, owner)) && constraints(allocator->designated, owner);
    }
    case ExprKind::Box:
        return hasPreelaborableInitialization(expression->type, owner, nullptr);
    default:
        return true;
    }
}


bool supportsExternalStreaming(Type* type)
{
    if (type == nullptr) { return false; }
    if (type->isSubtype && type->base != nullptr) { return supportsExternalStreaming(type->base); }
    if (type->visibleStreamReadWrite) { return true; }
    if (type->kind == TypeKind::Access) { return false; }
    if (type->isLimited && (!streamAvailable(type, 0) || !streamAvailable(type, 1))) { return false; }
    if (type->kind == TypeKind::Array) { return supportsExternalStreaming(type->element); }
    if (type->parentType != nullptr && !supportsExternalStreaming(type->parentType)) { return false; }
    for (const FieldInfo& field : type->fields) {
        if (!supportsExternalStreaming(field.type)) { return false; }
    }
    return true;
}

namespace
{

bool pureInitialization(Type* type, Symbol* owner)
{
    if (type == nullptr) { return false; }
    if (type->privateTo != nullptr && type->privateTo != owner) { return false; }
    if (type->kind == TypeKind::Array) { return pureInitialization(type->element, owner); }
    for (const FieldInfo& field : type->fields) {
        if (field.defaultValue != nullptr ? !pureExpression(field.defaultValue, owner) : !pureInitialization(field.type, owner)) { return false; }
    }
    return true;
}

}

bool pureExpression(const Expr* expression, Symbol* owner)
{
    if (expression == nullptr) { return true; }
    switch (expression->kind) {
    case ExprKind::Allocator: {
        const auto* allocator = static_cast<const AllocatorExpr*>(expression);
        return expression->type != nullptr && expression->type->accessConstant
            && (allocator->value != nullptr ? pureExpression(allocator->value.get(), owner) : pureInitialization(allocator->designated, owner));
    }
    case ExprKind::Aggregate: {
        const auto* aggregate = static_cast<const AggregateExpr*>(expression);
        if (aggregate->ancestorIsType && aggregate->ancestorType != nullptr
            && aggregate->ancestorType->privateTo != nullptr && aggregate->ancestorType->privateTo != owner) { return false; }
        if (!pureExpression(aggregate->ancestor.get(), owner)) { return false; }
        for (const auto& component : aggregate->components) { if (!pureExpression(component.value.get(), owner)) { return false; } }
        for (std::size_t i = 0; i < aggregate->resolvedFields.size(); ++i) {
            const Expr* value = aggregate->resolvedFields[i];
            if (value != nullptr && value->kind == ExprKind::Box && expression->type != nullptr) {
                const FieldInfo& field = expression->type->fields[i];
                if (field.defaultValue != nullptr ? !pureExpression(field.defaultValue, owner) : !pureInitialization(field.type, owner)) { return false; }
            } else if (!pureExpression(value, owner)) { return false; }
        }
        if (expression->type != nullptr && expression->type->kind == TypeKind::Array) {
            for (const auto& component : aggregate->components) {
                if (component.value->kind == ExprKind::Box && !pureInitialization(expression->type->element, owner)) { return false; }
            }
        }
        return true;
    }
    case ExprKind::Box:
        return expression->type == nullptr || expression->type->privateTo == nullptr || expression->type->privateTo == owner;
    case ExprKind::Qualified:
        return pureExpression(static_cast<const QualifiedExpr*>(expression)->operand.get(), owner);
    case ExprKind::Call: {
        const auto* call = static_cast<const CallExpr*>(expression);
        for (const auto& argument : call->arguments) { if (!pureExpression(argument.value.get(), owner)) { return false; } }
        return pureExpression(call->callee.get(), owner);
    }
    case ExprKind::Attribute: {
        const auto* attribute = static_cast<const AttributeExpr*>(expression);
        if (!pureExpression(attribute->prefix.get(), owner)) { return false; }
        for (const auto& argument : attribute->arguments) { if (!pureExpression(argument.get(), owner)) { return false; } }
        return true;
    }
    case ExprKind::Selected:
        return pureExpression(static_cast<const SelectedExpr*>(expression)->prefix.get(), owner);
    case ExprKind::Unary:
        return pureExpression(static_cast<const UnaryExpr*>(expression)->operand.get(), owner);
    case ExprKind::Binary: {
        const auto* binary = static_cast<const BinaryExpr*>(expression);
        return pureExpression(binary->left.get(), owner) && pureExpression(binary->right.get(), owner);
    }
    default:
        return true;
    }
}

void checkCategorizedStatements(const StmtList& statements, Diagnostics& diagnostics, Symbol* owner)
{
    for (const auto& statement : statements) {
        if (statement->kind == StmtKind::Pragma) {
            const auto* pragma = static_cast<const PragmaStmt*>(statement.get())->pragma.get();
            if (!pragma->assertionsEnabled || (preelaborableExpression(pragma->assertion, owner)
                && (pragma->assertion != nullptr && pragma->assertion->isStatic && pragma->assertion->staticValue != 0
                    ? true : preelaborableExpression(pragma->message, owner)))) { continue; }
        }
        if (statement->kind != StmtKind::Null) {
            diagnostics.error(statement->location, "preelaborated package elaboration permits only null statements");
        }
    }
}

void checkCategorizedDeclarations(const DeclList& declarations, int category, Diagnostics& diagnostics, Symbol* owner)
{
    if (category == 0) { return; }
    for (const auto& declaration : declarations) {
        const Decl* item = declaration.get();
        bool valid = true;
        for (Type* type : item->constraintOrder) { valid = valid && constraints(type, owner); }
        switch (item->kind) {
        case DeclKind::Object: {
            const auto* object = static_cast<const ObjectDecl*>(item);
            if (object->awaitsValue) { break; }
            bool reference = object->isRenaming || (!object->symbols.empty()
                && object->symbols.front()->genericObject && object->symbols.front()->objectReference);
            if (category == 2 && !object->isConstant && !reference) {
                diagnostics.error(item->location, "a pure library unit cannot elaborate a variable declaration");
            }
            valid = valid && object->preelaborableCreation && (reference
                ? preelaborableName(object->initializer.get(), owner)
                : preelaborableExpression(object->initializer.get(), owner));
            if (category == 2 && (!object->pureDefaultCreation || !pureExpression(object->initializer.get(), owner))) { valid = false; }
            if (!reference && object->initializer == nullptr) {
                for (const Symbol* symbol : object->symbols) {
                    if (category == 2) { valid = valid && pureInitialization(symbol->type, owner); }
                }
            }
            break;
        }
        case DeclKind::Type: {
            Type* type = static_cast<const TypeDecl*>(item)->declaredType;
            valid = valid && constraints(type, owner);
            if (category == 2 && type != nullptr && type->kind == TypeKind::Access
                && !type->anonymousAccess && type->accessProfile == nullptr && !type->accessConstant && !type->zeroStorage) {
                diagnostics.error(item->location, "a named access-to-variable type in a pure unit requires Storage_Size zero");
            }
            checkCategorizedDeclarations(static_cast<const TypeDecl*>(item)->taskObjects, category, diagnostics, owner);
            break;
        }
        case DeclKind::Subtype:
            valid = valid && constraints(static_cast<const SubtypeDecl*>(item)->declaredType, owner);
            break;
        case DeclKind::PackageSpecification: {
            const auto* package = static_cast<const PackageSpecDecl*>(item);
            checkCategorizedDeclarations(package->publicPart, category, diagnostics, package->symbol);
            checkCategorizedDeclarations(package->privatePart, category, diagnostics, package->symbol);
            break;
        }
        case DeclKind::PackageBody: {
            const auto* package = static_cast<const PackageBodyDecl*>(item);
            checkCategorizedDeclarations(package->declarations, category, diagnostics, package->symbol);
            checkCategorizedStatements(package->body, diagnostics, package->symbol);
            for (const auto& handler : package->handlers) { checkCategorizedStatements(handler.body, diagnostics, package->symbol); }
            break;
        }
        case DeclKind::GenericInstantiation:
            checkCategorizedDeclarations(static_cast<const GenericInstantiationDecl*>(item)->expansion, category, diagnostics, owner);
            break;
        case DeclKind::Pragma: {
            const auto* pragma = static_cast<const PragmaDecl*>(item);
            if (pragma->lower == "assert" && pragma->assertionsEnabled) {
                valid = valid && preelaborableExpression(pragma->assertion, owner);
                if (pragma->assertion == nullptr || !pragma->assertion->isStatic || pragma->assertion->staticValue == 0) {
                    valid = valid && preelaborableExpression(pragma->message, owner);
                }
            }
            break;
        }
        default:
            break; // Subprogram bodies and component defaults are not executed now.
        }
        if (!valid) { diagnostics.error(item->location, "declaration performs an action forbidden during preelaboration"); }
    }
}
