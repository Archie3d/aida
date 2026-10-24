#include "Sema.h"
#include "SemaSupport.h"

namespace
{

struct ObjectAccessView
{
    bool aliased = false;
    int level = 0;
    bool constrained = false;
    bool unstable = false;
};

ObjectAccessView accessView(Expr* expression)
{
    if (expression == nullptr || expression->implicitCall != nullptr) { return {}; }
    Symbol* symbol = expression->kind == ExprKind::Identifier ? static_cast<IdentifierExpr*>(expression)->symbol
        : expression->kind == ExprKind::Selected ? static_cast<SelectedExpr*>(expression)->symbol : nullptr;
    if (symbol != nullptr) {
        if (symbol->sharedAlias != nullptr) { return accessView(symbol->sharedAlias); }
        bool taggedFormal = symbol->type != nullptr && symbol->type->tagged
            && (symbol->kind == SymbolKind::Parameter || symbol->genericObject);
        return {symbol->aliased || taggedFormal, symbol->accessibilityLevel,
            symbol->isConstant || !hasDefaultDiscriminants(symbol->type)
                || hasDiscriminantConstraint(symbol->type), false};
    }
    if (expression->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(expression);
        Type* prefix = baseType(selected->prefix->type);
        bool dereference = prefix != nullptr && prefix->kind == TypeKind::Access;
        ObjectAccessView view = dereference ? ObjectAccessView {true, prefix->accessLevel, prefix->accessConstant || !prefix->generalAccess || !hasDefaultDiscriminants(prefix->target) || hasDiscriminantConstraint(prefix->target), false}
            : accessView(selected->prefix.get());
        if (selected->isDereference) { return view; }
        Type* record = dereference ? baseType(prefix->target) : prefix;
        if (selected->fieldIndex >= 0 && record != nullptr) {
            const FieldInfo& field = record->fields[selected->fieldIndex];
            view.unstable |= !view.constrained && (field.variant >= 0 || field.dependentBounds
                || field.type->discriminantRecord != nullptr);
            view.aliased = field.aliased || (field.isDiscriminant && view.aliased);
            view.constrained = !hasDefaultDiscriminants(field.type) || hasDiscriminantConstraint(field.type);
        }
        return view;
    }
    if (expression->kind == ExprKind::Call) {
        auto* call = static_cast<CallExpr*>(expression);
        if (call->form == CallForm::Conversion && expression->type->tagged) {
            return accessView(call->resolvedArguments.front());
        }
        if (call->form == CallForm::Indexing) {
            Type* array = baseType(call->callee->type);
            bool dereference = array != nullptr && array->kind == TypeKind::Access;
            ObjectAccessView view = dereference ? ObjectAccessView {true, array->accessLevel, true, false}
                : accessView(call->callee.get());
            if (dereference) { array = baseType(array->target); }
            view.aliased = array->aliasedElements;
            view.constrained = !hasDefaultDiscriminants(expression->type) || hasDiscriminantConstraint(expression->type);
            return view;
        }
    }
    return {};
}

bool matchingDesignated(Type* target, Type* source)
{
    if (target == nullptr || source == nullptr) { return false; }
    if (target->tagged) {
        Type* targetRoot = target->classRoot != nullptr ? target->classRoot : rootType(target);
        Type* sourceRoot = source->classRoot != nullptr ? source->classRoot : rootType(source);
        return target->classRoot != nullptr ? isTaggedAncestor(targetRoot, sourceRoot)
            : source->classRoot == nullptr && rootType(target) == rootType(source);
    }
    if (rootType(target) != rootType(source)) { return false; }
    if (target->kind == TypeKind::Record && target->discriminantCount > 0
        && !hasDiscriminantConstraint(target)) { return true; }
    return SemaSupport::staticallyMatches(source, target);
}

bool matchingStorage(Type* target, Type* source)
{
    if (target->classRoot != nullptr || source->classRoot != nullptr
        || rootType(target) != rootType(source)
        || hasRuntimeLayout(target) || hasRuntimeLayout(source)) { return true; }
    return typeSize(target) == typeSize(source) && typeAlignment(source) >= typeAlignment(target);
}

}

Type* Sema::analyzeObjectAccess(AttributeExpr* expr, Scope* scope, Type* expected)
{
    if (!representationVisible(expected) || !expected->generalAccess || !expr->arguments.empty()) {
        m_diagnostics.error(expr->location, "object 'Access requires a general access type context and no arguments");
        return nullptr;
    }
    Type* source = analyzeExpr(expr->prefix.get(), scope, expected->target);
    if (source == nullptr) { return nullptr; }
    ObjectAccessView view = accessView(expr->prefix.get());
    if (!view.aliased || SemaSupport::objectView(expr->prefix.get()) == 0) {
        m_diagnostics.error(expr->location, "object 'Access requires an aliased object view");
        return nullptr;
    }
    if (!expected->accessConstant && SemaSupport::objectView(expr->prefix.get()) != 2) {
        m_diagnostics.error(expr->location, "access-to-variable cannot designate a constant view");
        return nullptr;
    }
    if (view.unstable) {
        m_diagnostics.error(expr->location, "'Access cannot designate a discriminant-dependent component of an unconstrained variable");
        return nullptr;
    }
    if (!matchingDesignated(expected->target, source)) {
        m_diagnostics.error(expr->location, "object view does not match the designated subtype");
        return nullptr;
    }
    if (!matchingStorage(expected->target, source)) {
        m_diagnostics.error(expr->location, "object 'Access requires matching designated storage representation");
        return nullptr;
    }
    if (view.level > expected->accessLevel && m_replayContract == nullptr) {
        m_diagnostics.error(expr->location, "object is deeper than the access type");
        return nullptr;
    }
    expr->objectAccess = true;
    expr->objectAccessLevel = view.level;
    expr->prefixType = source;
    expr->type = expected;
    m_sharedEscapes.push_back(expr->prefix.get());
    return expected;
}

bool Sema::checkObjectAccessConversion(CallExpr* expr, Type* target, Type* source)
{
    if (target->accessProfile != nullptr || source == nullptr || source->kind != TypeKind::Access
        || source->accessProfile != nullptr) { return false; }
    bool related = false;
    Type* left = rootType(target);
    Type* right = rootType(source);
    while (left->parentType != nullptr) { left = left->parentType; }
    while (right->parentType != nullptr) { right = right->parentType; }
    related = left == right;
    bool designated = matchingDesignated(target->target, source->target)
        || (target->target->kind == TypeKind::Record && source->target->kind == TypeKind::Record
            && matchingDesignated(source->target, target->target));
    if (target->target->tagged && source->target->tagged) {
        Type* a = target->target->classRoot != nullptr ? target->target->classRoot : target->target;
        Type* b = source->target->classRoot != nullptr ? source->target->classRoot : source->target;
        designated = isTaggedAncestor(a, b) || (source->target->classRoot != nullptr && isTaggedAncestor(b, a));
    }
    if (((!representationVisible(target) || !representationVisible(source)) && rootType(target) != rootType(source))
        || (!target->generalAccess && !related) || !designated || (!target->accessConstant && source->accessConstant)) {
        m_diagnostics.error(expr->location, "this type conversion is not allowed");
    } else if (!matchingStorage(target->target, source->target)) {
        m_diagnostics.error(expr->location, "object access conversion requires matching designated storage representation");
    } else if (source->accessLevel > target->accessLevel && m_replayContract == nullptr) {
        m_diagnostics.error(expr->location, "source access type is deeper than the target access type");
    }
    return true;
}
