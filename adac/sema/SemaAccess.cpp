#include "Sema.h"
#include "SemaSupport.h"

#include <cctype>
#include <filesystem>

namespace
{

struct ObjectAccessView
{
    bool aliased = false;
    int level = 0;
    bool constrained = false;
    bool unstable = false;
    Symbol* object = nullptr;
    bool dynamic = false;
};

ObjectAccessView accessView(Expr* expression)
{
    if (expression == nullptr || expression->implicitCall != nullptr) { return {}; }
    Symbol* symbol = expression->kind == ExprKind::Identifier ? static_cast<IdentifierExpr*>(expression)->symbol
        : expression->kind == ExprKind::Selected ? static_cast<SelectedExpr*>(expression)->symbol : nullptr;
    if (symbol != nullptr) {
        if (symbol->sharedAlias != nullptr) {
            ObjectAccessView view = accessView(symbol->sharedAlias);
            if (view.object != nullptr && !view.object->isGlobal && view.object->owner != nullptr) {
                view.object->needsAccessibility = true;
                view.object->owner->needsFrame = true;
            }
            view.object = symbol;
            return view;
        }
        bool taggedFormal = symbol->type != nullptr && symbol->type->tagged
            && (symbol->kind == SymbolKind::Parameter || symbol->genericObject);
        return {symbol->aliased || taggedFormal, symbol->accessibilityLevel,
            symbol->isConstant || !hasDefaultDiscriminants(symbol->type)
                || hasDiscriminantConstraint(symbol->type), false, symbol};
    }
    if (expression->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(expression);
        Type* prefix = baseType(selected->prefix->type);
        bool dereference = prefix != nullptr && prefix->kind == TypeKind::Access;
        ObjectAccessView view = dereference ? ObjectAccessView {true, prefix->accessLevel, prefix->accessConstant || !prefix->generalAccess || !hasDefaultDiscriminants(prefix->target) || hasDiscriminantConstraint(prefix->target), false}
            : accessView(selected->prefix.get());
        if (dereference) { view.dynamic = prefix->accessParameter || prefix->accessDiscriminant; }
        if (selected->isDereference) { return view; }
        Type* record = dereference ? baseType(prefix->target) : prefix;
        if (selected->recordType != nullptr) { record = selected->recordType; }
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
            if (dereference) { view.dynamic = array->accessParameter || array->accessDiscriminant; array = baseType(array->target); }
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
    if (target->limitedView) { target = rootType(target); }
    if (source->limitedView) { source = rootType(source); }
    if (target->classRoot != nullptr || source->classRoot != nullptr
        || rootType(target) != rootType(source)
        || hasRuntimeLayout(target) || hasRuntimeLayout(source)) { return true; }
    return typeSize(target) == typeSize(source) && typeAlignment(source) >= typeAlignment(target);
}

}

Type* Sema::analyzeObjectAccess(AttributeExpr* expr, Scope* scope, Type* expected)
{
    bool unchecked = expr->lower == "unchecked_access";
    std::string attribute = unchecked ? "'Unchecked_Access" : "'Access";
    if (!representationVisible(expected) || !expected->generalAccess || !expr->arguments.empty()) {
        m_diagnostics.error(expr->location, "object " + attribute + " requires a general access type context and no arguments");
        return nullptr;
    }
    int errors = m_diagnostics.errorCount();
    // An object-access prefix is a name, never an implicit subprogram call.
    // In particular, an overloaded F.all must select an object designation
    // rather than invoke an access-to-subprogram alternative (RM 3.10.2(2.3)).
    auto* dereference = expr->prefix->kind == ExprKind::Selected
        ? static_cast<SelectedExpr*>(expr->prefix.get()) : nullptr;
    Type* source = dereference != nullptr && dereference->isDereference
        ? analyzeSelected(dereference, scope, expected->target)
        : analyzeExpr(expr->prefix.get(), scope, expected->target);
    if (source == nullptr) {
        if (m_diagnostics.errorCount() == errors) {
            m_diagnostics.error(expr->location, "object " + attribute + " requires an aliased object view");
        }
        return nullptr;
    }
    if (!checkCompleteView(source, expr->location, ("object " + attribute).c_str())) { return nullptr; }
    ObjectAccessView view = accessView(expr->prefix.get());
    if (!view.aliased || SemaSupport::objectView(expr->prefix.get()) == 0) {
        m_diagnostics.error(expr->location, "object " + attribute + " requires an aliased object view");
        return nullptr;
    }
    if (!expected->accessConstant && SemaSupport::objectView(expr->prefix.get()) != 2) {
        m_diagnostics.error(expr->location, "access-to-variable cannot designate a constant view");
        return nullptr;
    }
    if (view.unstable) {
        m_diagnostics.error(expr->location, attribute + " cannot designate a discriminant-dependent component of an unconstrained variable");
        return nullptr;
    }
    if (!matchingDesignated(expected->target, source)
        && !(expected->anonymousAccess && source->classRoot == rootType(expected->target))) {
        m_diagnostics.error(expr->location, "object view does not match the designated subtype");
        return nullptr;
    }
    if (!matchingStorage(completeView(expected->target, scope), source)) {
        m_diagnostics.error(expr->location, "object " + attribute + " requires matching designated storage representation");
        return nullptr;
    }
    if (!unchecked && view.level > expected->accessLevel && !view.dynamic && !expected->accessParameter && !expected->accessDiscriminant && m_replayContract == nullptr) {
        m_diagnostics.error(expr->location, "object is deeper than the access type");
        return nullptr;
    }
    expr->accessObject = view.object;
    if (view.object != nullptr && !view.object->isGlobal && view.object->owner != nullptr) {
        view.object->needsAccessibility = true;
        view.object->owner->needsFrame = true;
    }
    expr->objectAccess = true;
    expr->objectAccessLevel = unchecked ? 0 : view.level;
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
    } else if (source->accessLevel > target->accessLevel && !source->accessParameter && !source->accessDiscriminant && m_replayContract == nullptr) {
        m_diagnostics.error(expr->location, "source access type is deeper than the target access type");
    }
    return true;
}

Type* Sema::resolveAnonymousAccess(SubtypeIndication* indication, Scope* scope)
{
    if (indication->resolved != nullptr) { return indication->resolved; }
    TypeDefinition* definition = indication->accessDefinition.get();
    Type* type = m_types.create(TypeKind::Access, anonymousTypeName());
    indication->resolved = type;
    type->anonymousAccess = true;
    type->excludesNull = indication->nullExclusion;
    type->accessParameter = indication->accessParameter;
    type->accessDiscriminant = indication->accessDiscriminant;
    type->generalAccess = true;
    type->accessConstant = definition->accessConstant;
    type->accessProtected = definition->accessProtected;
    type->accessScope = scope;
    type->declarationScope = scope;
    type->accessLevel = m_accessibilityLevel;
    if (definition->accessProfile != nullptr) {
        SubprogramSpec& spec = *definition->accessProfile;
        Symbol* profile = m_symbolTable.createSymbol(SymbolKind::Subprogram, "", type->name);
        profile->returnType = spec.isFunction ? resolveSubtypeIndication(spec.returnType.get(), scope) : nullptr;
        if (profile->returnType != nullptr && profile->returnType->limitedView) {
            checkCompleteView(profile->returnType, indication->location, "an access-to-subprogram result");
        }
        for (ParameterDecl& declaration : spec.parameters) {
            Symbol* parameter = m_symbolTable.createSymbol(SymbolKind::Parameter, declaration.lower, declaration.name);
            parameter->type = resolveSubtypeIndication(declaration.subtype.get(), scope);
            if (parameter->type != nullptr && parameter->type->limitedView && !parameter->type->tagged) {
                checkCompleteView(parameter->type, declaration.location, "an access-to-subprogram parameter");
            }
            parameter->mode = declaration.mode;
            parameter->isConstant = declaration.mode == ParameterMode::In;
            parameter->byReference = declaration.mode != ParameterMode::In || isComposite(parameter->type);
            if (declaration.defaultValue != nullptr) {
                m_diagnostics.error(declaration.location, "access-to-subprogram parameters cannot have defaults");
            }
            if (spec.isFunction && declaration.mode != ParameterMode::In) {
                m_diagnostics.error(declaration.location, "access-to-function parameters must have mode in");
            }
            profile->parameters.push_back(parameter);
        }
        profile->protectedAccessProfile = type->accessProtected;
        type->accessProfile = profile;
    }
    if (definition->accessProfile == nullptr || type->accessProtected) {
        if (definition->parent != nullptr) { type->target = resolveSubtypeIndication(definition->parent.get(), scope, true, true); }
        type->collectionOwner = m_currentSubprogram;
        std::string identity = std::filesystem::path(m_diagnostics.fileName(indication->location.file)).filename().string();
        for (char& character : identity) {
            if (!std::isalnum(static_cast<unsigned char>(character))) { character = '_'; }
        }
        std::string name = "$" + mangle("anonymous_access_" + identity + "_"
            + std::to_string(indication->location.line) + "_" + std::to_string(indication->location.column));
        std::size_t ordinal = ++m_tagNames[name];
        type->collectionName = name + (ordinal > 1 ? "__" + std::to_string(ordinal) : "");
        if (m_currentSubprogram != nullptr) {
            m_currentSubprogram->needsFrame = true;
            m_currentSubprogram->usesFinalization = true;
        }
        if (m_scalarConstraintDeclaration != nullptr) {
            m_scalarConstraintDeclaration->anonymousAccessTypes.push_back(type);
        }
    }
    return type;
}

bool Sema::compatibleAnonymousAccess(Type* target, Type* source) const
{
    if (target == nullptr || source == nullptr || !target->anonymousAccess
        || source->kind != TypeKind::Access) { return false; }
    if (target->accessProfile != nullptr || source->accessProfile != nullptr) {
        return target->accessProfile != nullptr && source->accessProfile != nullptr
            && target->accessProtected == source->accessProtected
            && target->accessConventionC == source->accessConventionC && matchesAccessProfile(source->accessProfile, target);
    }
    bool designated = matchingDesignated(target->target, source->target);
    if (target->target->tagged && target->target->classRoot == nullptr && source->target->classRoot != nullptr) {
        designated = source->target->classRoot == rootType(target->target);
    }
    return (target->accessConstant || !source->accessConstant)
        && designated
        && matchingStorage(target->target, source->target);
}

void Sema::checkAnonymousAccess(Expr* expr, Type* expected, Type* actual)
{
    if (expr == nullptr || expected == nullptr || !expected->anonymousAccess
        || !compatibleAnonymousAccess(expected, actual) || expr->kind == ExprKind::Null) { return; }
    if (expected->accessProfile == nullptr && expected->target->tagged
        && expected->target->classRoot == nullptr && actual->target->classRoot != nullptr) {
        m_diagnostics.error(expr->location,
            "a dynamically tagged access value requires a class-wide access context or a controlling operand");
        return;
    }
    expr->accessTarget = expected;
    if (expected->accessParameter) { return; }
    if (actual->accessProfile != nullptr && actual->accessParameter) {
        m_diagnostics.error(expr->location, "an anonymous access-to-subprogram parameter cannot escape its call");
        return;
    }
    int level = actual->accessLevel;
    bool dynamic = actual->accessParameter || actual->accessDiscriminant;
    if (expr->kind == ExprKind::Attribute && static_cast<AttributeExpr*>(expr)->objectAccess) {
        level = static_cast<AttributeExpr*>(expr)->objectAccessLevel;
        dynamic = true; // A dereference may carry a run-time access-parameter level.
    }
    int targetLevel = expected->accessDiscriminant ? m_accessibilityLevel : expected->accessLevel;
    if (!dynamic && level > targetLevel && m_replayContract == nullptr) {
        m_diagnostics.error(expr->location, "source access value is deeper than the anonymous access type");
    } else {
        expr->accessCheckLevel = targetLevel;
    }
}
