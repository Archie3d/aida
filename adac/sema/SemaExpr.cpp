#include "Sema.h"
#include "SemaSupport.h"

#include <utility>
#include <algorithm>
#include <cstdint>

using SemaSupport::isUniversal;
using SemaSupport::adaptUniversal;

namespace
{

bool isCharacterLiteralExpression(const Expr* expr)
{
    if (expr->kind == ExprKind::StringLiteral || expr->kind == ExprKind::CharacterLiteral) {
        return true;
    }
    if (expr->kind == ExprKind::Binary) {
        const auto* binary = static_cast<const BinaryExpr*>(expr);
        return binary->operatorCall == nullptr && binary->op == BinaryOp::Concatenate
            && isCharacterLiteralExpression(binary->left.get())
            && isCharacterLiteralExpression(binary->right.get());
    }
    return false;
}

}

Type* Sema::analyzeExpr(Expr* expr, Scope* scope, Type* expected)
{
    int errors = m_diagnostics.errorCount();
    ++m_expressionDepth;
    Type* result = analyzeExprValue(expr, scope, expected);
    if (result != nullptr && errors == m_diagnostics.errorCount()) {
        noteStaticValue(expr);
    }
    --m_expressionDepth;
    if (m_expressionDepth == 0 && errors == m_diagnostics.errorCount()) {
        validateStatic(expr, false, expected);
    }
    if (needsFinalization(result) || needsCollection(result) || (result != nullptr && result->m_tagged)) {
        if (m_currentSubprogram != nullptr) {
            m_currentSubprogram->m_usesFinalization = true;
        } else {
            m_finalizingUnits.insert(m_currentUnit);
        }
    }
    return result;
}

Type* Sema::analyzeExprValue(Expr* expr, Scope* scope, Type* expected)
{
    if (expr == nullptr) {
        return nullptr;
    }

    if (expected != nullptr && !representationVisible(expected)
        && (expr->kind == ExprKind::IntegerLiteral || expr->kind == ExprKind::RealLiteral)) {
        m_diagnostics.error(expr->location, "a numeric literal cannot denote a private type outside its defining package");
        expr->type = expected;
        return expected;
    }
    if (expr->m_implicitCall != nullptr) {
        expr->type = analyzeExpr(expr->m_implicitCall.get(), scope, expected);
        return expr->type;
    }
    if (expr->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(expr);
        if (selected->isDereference) {
            for (Type* type : expressionTypes(selected->prefix.get(), scope)) {
                if (type->m_accessProfile != nullptr) {
                    auto call = std::make_unique<CallExpr>();
                    call->location = expr->location;
                    call->callee = std::move(selected->prefix);
                    expr->m_implicitCall = std::move(call);
                    expr->type = analyzeExpr(expr->m_implicitCall.get(), scope, expected);
                    return expr->type;
                }
            }
        }
    }

    switch (expr->kind) {
    case ExprKind::IntegerLiteral: {
        auto* literal = static_cast<IntegerLiteralExpr*>(expr);
        Type* type = m_types.universalInteger();
        if (expected != nullptr && expected->kind == TypeKind::Integer) {
            type = expected;
        }
        expr->type = type;
        expr->isStatic = true;
        expr->staticValue = literal->value;
        return type;
    }
    case ExprKind::RealLiteral: {
        auto* literal = static_cast<RealLiteralExpr*>(expr);
        if (expected != nullptr && expected->kind == TypeKind::Fixed) {
            expr->type = expected;
            if (expected->m_formalFixed) {
                return expr->type;
            }
            expr->isStatic = true;
            expr->m_fixedInvalid = !literal->m_exactReal.scaled(expected->m_fixedScale, expr->staticValue);
            if (expr->m_fixedInvalid) {
                m_diagnostics.error(expr->location, "fixed-point literal exceeds exact evaluation or storage limits");
            }
            return expr->type;
        }
        expr->type = expected != nullptr && expected->kind == TypeKind::Float ? expected : m_types.universalReal();
        expr->isStatic = true;
        expr->staticReal = expr->m_exactReal.real();
        if (!expr->m_exactReal.m_valid) {
            expr->staticFailure = StaticFailure::Capacity;
        }
        return expr->type;
    }
    case ExprKind::CharacterLiteral: {
        auto* literal = static_cast<CharacterLiteralExpr*>(expr);
        expr->type = expected != nullptr && m_types.isCharacter(expected) && representationVisible(expected)
            ? expected : m_types.characterType();
        expr->isStatic = true;
        expr->staticValue = static_cast<unsigned char>(literal->value);
        return expr->type;
    }
    case ExprKind::StringLiteral: {
        auto* literal = static_cast<StringLiteralExpr*>(expr);
        Type* parent = m_types.isString(expected) && representationVisible(expected)
            && representationVisible(expected->element) ? expected : m_types.stringType();
        for (unsigned char character : literal->value) {
            if (character < parent->element->low || character > parent->element->high) {
                m_diagnostics.error(expr->location, "string literal element is outside the component subtype range");
                break;
            }
        }
        Type* type = m_types.makeSubtype(anonymousTypeName(), rootType(parent), 0, 0);
        type->constrained = true;
        type->indexLow = 1;
        type->indexHigh = static_cast<long long>(literal->value.size());
        expr->type = type;
        return type;
    }
    case ExprKind::Null:
        if (expected != nullptr && expected->kind == TypeKind::Access && !representationVisible(expected)) {
            m_diagnostics.error(expr->location, "null literal requires a visible access type");
        }
        expr->type = expected != nullptr && expected->kind == TypeKind::Access ? expected : nullptr;
        return expr->type;
    case ExprKind::Allocator:
        return analyzeAllocator(static_cast<AllocatorExpr*>(expr), scope, expected);
    case ExprKind::Identifier:
        return analyzeIdentifier(static_cast<IdentifierExpr*>(expr), scope, expected);
    case ExprKind::Selected:
        return analyzeSelected(static_cast<SelectedExpr*>(expr), scope, expected);
    case ExprKind::Call:
        return analyzeCall(static_cast<CallExpr*>(expr), scope, expected);
    case ExprKind::Attribute:
        return analyzeAttribute(static_cast<AttributeExpr*>(expr), scope, expected);
    case ExprKind::Aggregate:
        return analyzeAggregate(static_cast<AggregateExpr*>(expr), scope, expected);
    case ExprKind::Binary:
        return analyzeBinary(static_cast<BinaryExpr*>(expr), scope, expected);
    case ExprKind::Unary:
        return analyzeUnary(static_cast<UnaryExpr*>(expr), scope, expected);
    case ExprKind::Membership:
        return analyzeMembership(static_cast<MembershipExpr*>(expr), scope);
    case ExprKind::Qualified: {
        auto* qualified = static_cast<QualifiedExpr*>(expr);
        Type* type = resolveTypeName(qualified->typeLower, scope, qualified->location);
        Type* operand = analyzeExpr(qualified->operand.get(), scope, type);
        if (!typesCompatible(type, operand)) {
            m_diagnostics.error(expr->location, "a qualified expression must have the named type");
        }
        adaptUniversal(qualified->operand.get(), type);
        expr->type = type;
        expr->m_tagIndeterminate = qualified->operand->m_tagIndeterminate;
        expr->isStatic = type != nullptr && type->kind != TypeKind::Fixed && qualified->operand->isStatic && type != nullptr && type->m_scalarBoundsSymbol == nullptr;
        if (expr->isStatic && type->m_modulus != 0
            && (qualified->operand->staticValue < type->low || qualified->operand->staticValue > type->high)) {
            expr->isStatic = false;
        }
        expr->staticValue = qualified->operand->staticValue;
        expr->staticReal = qualified->operand->staticReal;
        if (type != nullptr && isScalar(type) && qualified->operand->isStatic
            && type->m_scalarBoundsSymbol == nullptr && !type->m_formalFixed) {
            expr->isStatic = true;
            expr->staticFailure = qualified->operand->staticFailure == StaticFailure::None
                ? StaticFailure::None : StaticFailure::Dependent;
            expr->m_exactReal = qualified->operand->m_exactReal;
            __int128 value = 0;
            StaticFailure failure = StaticFailure::None;
            bool valid = isReal(type)
                ? (!type->hasRealRange || (expr->staticReal >= type->lowReal && expr->staticReal <= type->highReal))
                : foldWideInteger(qualified->operand.get(), value, failure) && value >= type->low && value <= type->high;
            if (!valid && expr->staticFailure == StaticFailure::None) {
                expr->staticFailure = StaticFailure::Check;
            }
        }
        return type;
    }
    }

    return nullptr;
}

Type* Sema::analyzeAllocator(AllocatorExpr* expr, Scope* scope, Type* expected)
{
    // Like null, an allocator has no type of its own: the access type it is
    // being used as says what it makes.
    Type* access = expected != nullptr ? baseType(expected) : nullptr;
    if (access == nullptr || access->kind != TypeKind::Access) {
        m_diagnostics.error(expr->location, "the access type of an allocator has to be known from its context");
        return nullptr;
    }

    Type* designated = resolveSubtypeIndication(expr->subtype.get(), scope);
    if (designated == nullptr) {
        return nullptr;
    }
    if (designated->m_abstract && designated->m_classRoot == nullptr) {
        m_diagnostics.error(expr->location, "class-wide controlled or abstract allocators are not yet supported");
        return nullptr;
    }
    if (designated->m_classRoot != nullptr && expr->value == nullptr) {
        m_diagnostics.error(expr->location, "a class-wide allocator requires an initializer");
        return nullptr;
    }
    if (!typesCompatible(access->target, designated)) {
        m_diagnostics.error(expr->location, "an allocator for '" + access->name + "' has to make a '"
                                                + (access->target != nullptr ? access->target->name : "?") + "'");
        return nullptr;
    }
    if (designated->kind == TypeKind::Array && !designated->constrained) {
        m_diagnostics.error(expr->location, "an allocator for an array needs its bounds, as in 'new " + expr->subtype->name
                                                + " (1 .. 10)'");
        return nullptr;
    }

    expr->designated = designated;
    if (expr->value != nullptr) {
        if (hasIncompleteParts(designated)) {
            m_deferredProtectedUses.push_back({designated, expr->location});
        }
        if (hasProtected(designated)) {
            m_diagnostics.error(expr->location, "allocators with protected parts cannot have explicit initializers yet");
        }
        if (hasTasks(designated)) {
            m_diagnostics.error(expr->location, "allocators containing tasks cannot have explicit initializers yet");
        }
        if (baseType(designated) == m_exceptionOccurrenceType) {
            m_diagnostics.error(expr->value->location, "an exception occurrence cannot be copied by an allocator");
        }
        Type* value = analyzeExpr(expr->value.get(), scope, designated);
        if (!hasProtected(designated) && hasLimitedControlledParts(designated) && !buildsLimitedResult(expr->value.get())) {
            m_diagnostics.error(expr->location, "limited controlled objects cannot be copied");
        }
        if (!typesCompatible(designated, value)) {
            m_diagnostics.error(expr->value->location, "the allocator initializer has an incompatible type");
        }
        adaptUniversal(expr->value.get(), designated);
        if (designated->m_classRoot != nullptr && hasLimitedControlledParts(value)
            && value->m_classRoot == nullptr && buildsLimitedResult(expr->value.get())) {
            expr->designated = value;
        }
    }

    expr->type = expected;
    return expr->type;
}

// Outside the package that declared it, a private type carries assignment and
// equality and nothing else; a limited one carries not even those.
void Sema::checkPrivateOperands(BinaryExpr* expr)
{
    // Concatenation copies components; a visible array may have nonlimited
    // private components without exposing their representation or arithmetic.
    if (expr->op == BinaryOp::Concatenate) { return; }
    bool comparison = expr->op == BinaryOp::Equal || expr->op == BinaryOp::NotEqual;

    for (Expr* operand : { expr->left.get(), expr->right.get() }) {
        Type* type = operand != nullptr ? baseType(operand->type) : nullptr;
        if (comparison && hasIncompleteParts(type)) {
            m_deferredProtectedUses.push_back({type, expr->location});
        }
        if (hasProtected(type) && comparison) {
            m_diagnostics.error(expr->location, "types containing protected objects have no predefined equality");
            return;
        }
        if (hasTasks(type) && comparison) {
            m_diagnostics.error(expr->location, "types containing tasks have no predefined equality");
            return;
        }
        if (hasLimitedControlledParts(type) && comparison) {
            m_diagnostics.error(expr->location, "a limited controlled type has no predefined equality");
            return;
        }
        if (type == nullptr || type->privateTo == nullptr || withinPackage(type->privateTo)) {
            continue;
        }
        if (comparison && !type->isLimited) {
            continue;
        }
        m_diagnostics.error(expr->location, "'" + type->name + "' is "
                                                + (type->isLimited ? "limited private" : "private")
                                                + ", so '" + type->privateTo->displayName
                                                + "' has to be the one to say what this operator means");
        return;
    }
}

Type* Sema::analyzeBinary(BinaryExpr* expr, Scope* scope, Type* expected)
{
    if (expr->operatorCall != nullptr) {
        return expr->type;
    }
    std::string name = operatorName(expr->op);
    Type* operandContext = nullptr;
    Type* rightContext = nullptr;
    if (!name.empty()) {
        auto candidates = operatorCandidates(name, { expr->left.get(), expr->right.get() }, scope, expected, expr->m_standardFixed);
        if (expected != nullptr && expected->kind == TypeKind::Fixed
            && (candidates.empty() || (candidates.size() == 1 && candidates.front().symbol == nullptr))) {
            auto universal = expressionTypes(expr, scope);
            if (universal.size() == 1 && universal.front()->kind == TypeKind::UniversalReal) {
                analyzeBinaryOperation(expr, scope, nullptr, nullptr);
                adaptUniversal(expr, expected);
                if (expr->m_fixedInvalid) {
                    m_diagnostics.error(expr->location, "fixed-point expression exceeds exact evaluation or storage limits");
                }
                return expr->type;
            }
        }
        if (candidates.size() > 1) {
            m_diagnostics.error(expr->location, "ambiguous operator '" + name + "'");
            Type* result = candidates.front().result;
            bool sameResult = std::all_of(candidates.begin(), candidates.end(), [&](const OperatorCandidate& candidate) {
                return rootType(candidate.result) == rootType(result);
            });
            expr->type = sameResult ? result : nullptr;
            return expr->type;
        }
        if (candidates.size() == 1) {
            const OperatorCandidate& chosen = candidates.front();
            if (m_recordContract != nullptr) {
                m_recordContract->m_operators[operatorContractKey(name, { expr->left.get(), expr->right.get() })] = chosen.symbol;
            }
            if (chosen.symbol != nullptr) {
                std::vector<ExprPtr> operands;
                operands.push_back(std::move(expr->left));
                operands.push_back(std::move(expr->right));
                expr->operatorCall = bindOperator(chosen.symbol, std::move(operands), scope, expr->location);
                expr->type = chosen.result;
                expr->m_tagIndeterminate = expr->operatorCall->m_tagIndeterminate;
                expr->isStatic = false;
                return expr->type;
            }
            if ((expr->op == BinaryOp::Multiply || expr->op == BinaryOp::Divide)
                && (chosen.parameters[0]->kind == TypeKind::Fixed || chosen.parameters[1]->kind == TypeKind::Fixed)) {
                analyzeExpr(expr->left.get(), scope, chosen.parameters[0]);
                analyzeExpr(expr->right.get(), scope, chosen.parameters[1]);
                adaptUniversal(expr->left.get(), chosen.parameters[0]);
                adaptUniversal(expr->right.get(), chosen.parameters[1]);
                expr->type = chosen.result;
                expr->m_fixedResult = chosen.parameters[0]->kind == TypeKind::Fixed
                    && chosen.parameters[1]->kind == TypeKind::Fixed;
                if (expr->m_fixedResult && !expr->m_explicitFixedContext) {
                    bool requiresConversion = false;
                    for (Type* operand : chosen.parameters) {
                        for (Symbol* primitive : rootType(operand)->m_primitives) {
                            if (primitive->name == name && primitive->parameters.size() == 2
                                && primitive->parameters[0]->type->kind == TypeKind::Fixed
                                && primitive->parameters[1]->type->kind == TypeKind::Fixed) {
                                requiresConversion = true;
                                break;
                            }
                        }
                    }
                    if (requiresConversion) {
                        m_diagnostics.error(expr->location, "fixed-by-fixed arithmetic with a user-defined primitive requires an explicit result conversion");
                    }
                }
                noteStaticValue(expr);
                checkPrivateOperands(expr);
                return expr->type;
            }
            operandContext = chosen.parameters.front();
            rightContext = chosen.parameters.back();
            if (expected == nullptr) {
                expected = chosen.result;
            }
        }
    }
    Type* result = analyzeBinaryOperation(expr, scope, expected, operandContext, rightContext);
    checkPrivateOperands(expr);
    return result;
}

Type* Sema::analyzeBinaryOperation(BinaryExpr* expr, Scope* scope, Type* expected, Type* operandContext, Type* rightContext)
{
    switch (expr->op) {
    case BinaryOp::And:
    case BinaryOp::Or:
    case BinaryOp::Xor:
    case BinaryOp::AndThen:
    case BinaryOp::OrElse: {
        if (operandContext != nullptr && operandContext->m_modulus != 0
            && expr->op != BinaryOp::AndThen && expr->op != BinaryOp::OrElse) {
            Type* left = analyzeExpr(expr->left.get(), scope, operandContext);
            Type* right = analyzeExpr(expr->right.get(), scope, operandContext);
            if (!typesCompatible(left, right)) {
                m_diagnostics.error(expr->location, "logical operands must have the same modular type");
            }
            expr->type = operandContext;
            noteStaticValue(expr);
            return expr->type;
        }
        Type* context = operandContext != nullptr ? operandContext : expected;
        if (context == nullptr || !m_types.isBoolean(context)) {
            context = commonOperandType(expr->left.get(), expr->right.get(), scope, nullptr);
        }
        Type* left = analyzeExpr(expr->left.get(), scope, context);
        Type* right = analyzeExpr(expr->right.get(), scope, left);
        if (!m_types.isBoolean(left) || !m_types.isBoolean(right) || !typesCompatible(left, right)) {
            m_diagnostics.error(expr->location, "logical operators require Boolean operands");
            expr->type = m_types.booleanType();
            return expr->type;
        }
        expr->type = left;
        return expr->type;
    }

    case BinaryOp::Equal:
    case BinaryOp::NotEqual:
    case BinaryOp::Less:
    case BinaryOp::LessEqual:
    case BinaryOp::Greater:
    case BinaryOp::GreaterEqual: {
        bool contextualLeft = (expr->left->kind != ExprKind::CharacterLiteral
                               && isCharacterLiteralExpression(expr->left.get()))
            || expr->left->kind == ExprKind::Aggregate;
        Type* context = operandContext != nullptr ? operandContext
            : commonOperandType(expr->left.get(), expr->right.get(), scope, nullptr);
        Type* right = contextualLeft ? analyzeExpr(expr->right.get(), scope, context) : nullptr;
        Type* left = analyzeExpr(expr->left.get(), scope, context != nullptr ? context : right);
        if (!contextualLeft) {
            right = analyzeExpr(expr->right.get(), scope, isUniversal(left) ? nullptr : left);
        }
        // Compatibility is judged before adaptation, which would otherwise give
        // a literal the very type it is being compared against.
        if (!typesCompatible(left, right)) {
            m_diagnostics.error(expr->location, "the operands of a comparison must have the same type");
        }
        if (isUniversal(left) && !isUniversal(right)) {
            adaptUniversal(expr->left.get(), right);
        } else if (!isUniversal(left) && isUniversal(right)) {
            adaptUniversal(expr->right.get(), left);
        }
        // Records are equal or not; nothing says which of two comes first.  A
        // private one is left to the check below, which has more to say.
        if (expr->op != BinaryOp::Equal && expr->op != BinaryOp::NotEqual && baseType(left) != nullptr
            && baseType(left)->kind == TypeKind::Record && representationVisible(left)) {
            m_diagnostics.error(expr->location, "records can be compared for equality, but not put in order");
        }
        if (expr->op != BinaryOp::Equal && expr->op != BinaryOp::NotEqual
            && left != nullptr && left->kind == TypeKind::Array
            && !isDiscrete(left->element) && representationVisible(left)) {
            m_diagnostics.error(expr->location, "array ordering requires discrete components");
        }
        expr->type = m_types.booleanType();
        return expr->type;
    }

    case BinaryOp::Concatenate: {
        Type* context = expected != nullptr && expected->kind == TypeKind::Array ? baseType(expected) : nullptr;
        if (context == nullptr || context->arrayRank != 1 || context->isLimited || context->element->isLimited
            || hasLimitedControlledParts(context) || hasTasks(context) || hasProtected(context)
            || operandContext == nullptr || rightContext == nullptr) {
            analyzeExpr(expr->left.get(), scope);
            analyzeExpr(expr->right.get(), scope);
            m_diagnostics.error(expr->location, "concatenation requires a nonlimited one-dimensional array type and matching operands");
            expr->type = context != nullptr ? context : m_types.stringType();
            return expr->type;
        }
        Type* left = analyzeExpr(expr->left.get(), scope, operandContext);
        Type* right = analyzeExpr(expr->right.get(), scope, rightContext);
        adaptUniversal(expr->left.get(), operandContext);
        adaptUniversal(expr->right.get(), rightContext);
        if (!typesCompatible(operandContext, left) || !typesCompatible(rightContext, right)) {
            m_diagnostics.error(expr->location, "concatenation operands do not match the array or component type");
        }
        Type* ancestor = context;
        while (ancestor->m_parentType != nullptr) { ancestor = baseType(ancestor->m_parentType); }
        bool fixedAncestor = ancestor->constrained || ancestor->m_boundsSymbol != nullptr;
        bool knownIndex = fixedAncestor ? ancestor->constrained : context->index->m_scalarBoundsSymbol == nullptr;
        long long indexFirst = fixedAncestor ? ancestor->indexLow : context->index->low;
        auto measure = [&](Expr* operand, Type* formal, long long& first, long long& last, long long& count) {
            if (rootType(formal) != rootType(context)) {
                first = last = indexFirst;
                count = 1;
                return knownIndex;
            }
            Type* type = operand->type;
            if (!type->constrained) { return false; }
            first = type->indexLow;
            last = type->indexHigh;
            count = arrayLength(type);
            // A string literal is interpreted in the concatenation's array
            // context, rather than borrowing predefined String's lower bound.
            if (operand->kind == ExprKind::StringLiteral) {
                if (!knownIndex) { return false; }
                first = indexFirst;
                __int128 high = (__int128)first + count - 1;
                if (high < INT64_MIN || high > INT64_MAX) { return false; }
                last = (long long)high;
            }
            return true;
        };
        long long first = 0, last = 0, leftCount = 0, rightFirst = 0, rightLast = 0, rightCount = 0;
        bool knownLeft = measure(expr->left.get(), operandContext, first, last, leftCount);
        bool knownRight = measure(expr->right.get(), rightContext, rightFirst, rightLast, rightCount);
        Type* result = m_types.makeSubtype(anonymousTypeName(), context, 0, 0);
        result->m_boundsSymbol = nullptr;
        result->m_boundExpressions.clear();
        result->constrained = false;
        if (knownLeft && knownRight) {
            __int128 count = (__int128)leftCount + rightCount;
            if (count > INT64_MAX || count * typeSize(context->element) > INT64_MAX) {
                m_diagnostics.error(expr->location, "concatenation exceeds signed 64-bit storage capacity");
            } else if (leftCount == 0) {
                result->indexLow = rightFirst;
                result->indexHigh = rightLast;
                result->constrained = true;
            } else if (!fixedAncestor || knownIndex) {
                if (fixedAncestor) { first = indexFirst; }
                __int128 high = (__int128)first + count - 1;
                if (high >= INT64_MIN && high <= INT64_MAX) {
                    result->indexLow = first;
                    result->indexHigh = (long long)high;
                    result->constrained = true;
                }
            }
        }
        expr->type = result;
        return expr->type;
    }

    case BinaryOp::Power: {
        // Predefined exponentiation still requires an integer exponent.
        Type* left = analyzeExpr(expr->left.get(), scope, expected);
        Type* right = analyzeExpr(expr->right.get(), scope, m_types.integerType());
        adaptUniversal(expr->right.get(), m_types.integerType());
        if (left != nullptr && left->kind == TypeKind::Fixed) {
            m_diagnostics.error(expr->location, "exponentiation is not defined for fixed-point operands");
        }
        if (!isNumeric(baseType(left))) {
            m_diagnostics.error(expr->location, "arithmetic operators require numeric operands");
        }
        if (!isDiscrete(baseType(right))) {
            m_diagnostics.error(expr->right->location, "the exponent of '**' must be an integer");
        } else if (right != nullptr && !isUniversal(right) && rootType(right) != m_types.integerType()) {
            m_diagnostics.error(expr->right->location, "the exponent of '**' must have type Integer");
        }
        expr->type = left;
        noteStaticValue(expr);
        return expr->type;
    }

    default: {
        Type* context = commonOperandType(expr->left.get(), expr->right.get(), scope, expected);
        Type* left = analyzeExpr(expr->left.get(), scope, context);
        Type* right = analyzeExpr(expr->right.get(), scope, isUniversal(left) ? expected : left);
        Type* result = left;
        if ((expr->op == BinaryOp::Multiply || expr->op == BinaryOp::Divide)
            && ((left != nullptr && left->kind == TypeKind::Fixed) || (right != nullptr && right->kind == TypeKind::Fixed))) {
            m_diagnostics.error(expr->location, "fixed-point multiplication or division requires a resolvable fixed-point result context");
        }
        if (!typesCompatible(left, right)) {
            m_diagnostics.error(expr->location, "the operands of an arithmetic operator must have the same type");
        }
        if (isUniversal(left) && !isUniversal(right)) {
            adaptUniversal(expr->left.get(), right);
            result = right;
        } else if (!isUniversal(left) && isUniversal(right)) {
            adaptUniversal(expr->right.get(), left);
        }
        if (!isNumeric(baseType(result))) {
            m_diagnostics.error(expr->location, "arithmetic operators require numeric operands");
        }
        if ((expr->op == BinaryOp::Modulo || expr->op == BinaryOp::Remainder) && (isReal(baseType(result)) || (result != nullptr && result->kind == TypeKind::Fixed))) {
            m_diagnostics.error(expr->location, "'mod' and 'rem' require integer operands");
        }
        expr->type = result;
        noteStaticValue(expr);
        return expr->type;
    }
    }
}

Type* Sema::analyzeUnary(UnaryExpr* expr, Scope* scope, Type* expected)
{
    if (expr->operatorCall != nullptr) {
        return expr->type;
    }
    std::string name = operatorName(expr->op);
    auto candidates = operatorCandidates(name, { expr->operand.get() }, scope, expected);
    if (candidates.size() > 1) {
        m_diagnostics.error(expr->location, "ambiguous operator '" + name + "'");
        return nullptr;
    }
    if (candidates.size() == 1) {
        const OperatorCandidate& chosen = candidates.front();
        if (m_recordContract != nullptr) {
            m_recordContract->m_operators[operatorContractKey(name, { expr->operand.get() })] = chosen.symbol;
        }
        if (chosen.symbol != nullptr) {
            std::vector<ExprPtr> operands;
            operands.push_back(std::move(expr->operand));
            expr->operatorCall = bindOperator(chosen.symbol, std::move(operands), scope, expr->location);
            expr->type = chosen.result;
            expr->m_tagIndeterminate = expr->operatorCall->m_tagIndeterminate;
            expr->isStatic = false;
            return expr->type;
        }
        expected = chosen.parameters.front();
    }
    if (expected != nullptr && expected->kind == TypeKind::Fixed && expr->op != UnaryOp::Not) {
        ExactReal exact;
        if (exactValue(expr, exact)) {
            expr->type = expected;
            if (expected->m_formalFixed) {
                return expr->type;
            }
            expr->isStatic = true;
            expr->m_fixedInvalid = !exact.scaled(expected->m_fixedScale, expr->staticValue);
            if (expr->m_fixedInvalid) {
                m_diagnostics.error(expr->location, "fixed-point literal exceeds exact evaluation or storage limits");
            }
            return expr->type;
        }
    }
    Type* operand = analyzeExpr(expr->operand.get(), scope,
                                expr->op == UnaryOp::Not && expected == nullptr
                                    ? m_types.booleanType() : expected);
    if (operand != nullptr && operand->m_modulus != 0) {
        if (expr->op == UnaryOp::Abs) {
            m_diagnostics.error(expr->location, "abs is not defined for modular types");
        }
        expr->type = operand;
        noteStaticValue(expr);
        return expr->type;
    }
    if (expr->op == UnaryOp::Not) {
        if (!m_types.isBoolean(operand)) {
            m_diagnostics.error(expr->location, "'not' requires a Boolean operand");
            expr->type = m_types.booleanType();
            return expr->type;
        }
        expr->type = operand;
        return expr->type;
    }
    if (!isNumeric(baseType(operand))) {
        m_diagnostics.error(expr->location, "unary operators require a numeric operand");
    }
    expr->type = operand;
    noteStaticValue(expr);
    return expr->type;
}

Type* Sema::analyzeMembership(MembershipExpr* expr, Scope* scope)
{
    Type* mark = expr->typeLower.empty() ? nullptr
        : resolveTypeName(expr->typeLower, scope, expr->location);
    expr->m_membershipSubtype = mark;
    Type* operand = analyzeExpr(expr->operand.get(), scope, taggedViewVisible(mark) ? nullptr : mark);
    if (taggedViewVisible(mark) && taggedViewVisible(operand)) {
        Type* target = mark->m_classRoot != nullptr ? mark->m_classRoot : mark;
        Type* source = operand->m_classRoot != nullptr ? operand->m_classRoot : operand;
        if (!isTaggedAncestor(target, source)
            && !(operand->m_classRoot != nullptr && isTaggedAncestor(source, target))) {
            m_diagnostics.error(expr->location, "tagged membership requires convertible types");
        }
        expr->m_testedTaggedType = mark;
        expr->type = m_types.booleanType();
        return expr->type;
    }
    if (isUniversal(operand)) {
        operand = m_types.integerType();
        adaptUniversal(expr->operand.get(), operand);
    }

    if (!expr->typeLower.empty()) {
        Type* type = mark;
        if (type != nullptr) {
            if ((!isDiscrete(type) && type->kind != TypeKind::Fixed) || !typesCompatible(type, operand)) {
                m_diagnostics.error(expr->location, "membership subtype must match the operand's discrete type");
            }
            expr->low = scalarBoundExpr(type, true, expr->location);
            expr->high = scalarBoundExpr(type, false, expr->location);
        }
    } else {
        analyzeExpr(expr->low.get(), scope, operand);
        analyzeExpr(expr->high.get(), scope, operand);
        adaptUniversal(expr->low.get(), operand);
        adaptUniversal(expr->high.get(), operand);
    }

    expr->type = m_types.booleanType();
    return expr->type;
}

bool Sema::buildsLimitedResult(Expr* expression) const
{
    if (expression->m_implicitCall != nullptr) {
        return buildsLimitedResult(expression->m_implicitCall.get());
    }
    if (expression->kind == ExprKind::Qualified) {
        return buildsLimitedResult(static_cast<QualifiedExpr*>(expression)->operand.get());
    }
    if (expression->kind == ExprKind::Aggregate) {
        return true;
    }
    if (expression->kind == ExprKind::Identifier) {
        Symbol* symbol = static_cast<IdentifierExpr*>(expression)->symbol;
        return symbol != nullptr && symbol->kind == SymbolKind::Subprogram;
    }
    if (expression->kind == ExprKind::Selected) {
        Symbol* symbol = static_cast<SelectedExpr*>(expression)->symbol;
        return symbol != nullptr && symbol->kind == SymbolKind::Subprogram;
    }
    if (expression->kind == ExprKind::Call) {
        auto* call = static_cast<CallExpr*>(expression);
        return call->form == CallForm::Subprogram;
    }
    return false;
}
