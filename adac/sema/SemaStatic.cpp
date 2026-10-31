#include "Sema.h"
#include "../../common/Modular.h"

#include <cstdint>
#include <cmath>
#include <limits>

bool Sema::foldStatic(Expr* expr, long long& value) const
{
    if (expr != nullptr && expr->type != nullptr && expr->type->kind == TypeKind::Fixed) {
        return foldFixed(expr, value);
    }
    __int128 wide = 0;
    StaticFailure failure = StaticFailure::None;
    if (!foldWideInteger(expr, wide, failure) || wide < LLONG_MIN || wide > LLONG_MAX) {
        return false;
    }
    value = static_cast<long long>(wide);
    return true;
}

bool Sema::foldWideInteger(Expr* expr, __int128& value, StaticFailure& failure) const
{
    if (expr == nullptr || isReal(expr->type)) {
        return false;
    }
    if (expr->staticFailure != StaticFailure::None) {
        failure = StaticFailure::Dependent;
        return false;
    }
    if (expr->type != nullptr && expr->type->kind == TypeKind::Fixed) {
        long long counts;
        if (!foldFixed(expr, counts)) {
            return false;
        }
        value = counts;
        return true;
    }
    if (expr->exactReal.valid && expr->exactReal.denominator == 1) {
        value = expr->exactReal.numerator;
        return true;
    }
    if (expr->isStatic) {
        value = expr->staticValue;
        return true;
    }
    if (expr->kind == ExprKind::Binary && static_cast<BinaryExpr*>(expr)->fixedResult) {
        auto* binary = static_cast<BinaryExpr*>(expr);
        long long left, right, rounded;
        if (foldFixed(binary->left.get(), left) && foldFixed(binary->right.get(), right)) {
            int leftBits = binary->left->type->fixedScale, rightBits = binary->right->type->fixedScale;
            bool multiply = binary->op == BinaryOp::Multiply;
            if (fixedNumericOperation(multiply ? FixedMultiply : FixedDivide, left, leftBits,
                    right, rightBits, expr->type->modulus != 0, &rounded)) {
                value = expr->type->modulus != 0 ? (__int128)(uint64_t)rounded : (__int128)rounded;
                return true;
            }
            failure = binary->op == BinaryOp::Divide && right == 0 ? StaticFailure::ZeroDivisor : StaticFailure::Check;
        }
        return false;
    }
    auto checked = [&](bool overflow, __int128 result) {
        if (overflow || !ExactReal::make(result).valid) {
            failure = StaticFailure::Capacity;
            return false;
        }
        value = result;
        return true;
    };
    switch (expr->kind) {
    case ExprKind::IntegerLiteral:
        value = static_cast<IntegerLiteralExpr*>(expr)->value;
        return true;
    case ExprKind::CharacterLiteral:
        value = expr->isStatic ? expr->staticValue : static_cast<CharacterLiteralExpr*>(expr)->value;
        return true;
    case ExprKind::Membership: {
        auto* membership = static_cast<MembershipExpr*>(expr);
        __int128 operand, low, high;
        if (membership->testedTaggedType != nullptr
            || !foldWideInteger(membership->operand.get(), operand, failure)
            || !foldWideInteger(membership->low.get(), low, failure)
            || !foldWideInteger(membership->high.get(), high, failure)) {
            return false;
        }
        value = (operand >= low && operand <= high) != membership->negated;
        return true;
    }
    case ExprKind::Unary: {
        auto* unary = static_cast<UnaryExpr*>(expr);
        __int128 operand;
        if (unary->operatorCall || !foldWideInteger(unary->operand.get(), operand, failure)) {
            return false;
        }
        if (expr->type != nullptr && expr->type->modulus != 0) {
            long long result;
            if (operand < 0 || operand > (__int128)UINT64_MAX) {
                failure = StaticFailure::Capacity;
                return false;
            }
            if (!modularOperation(unary->op == UnaryOp::Not ? ModularNot : ModularSubtract,
                    expr->type->modulus, unary->op == UnaryOp::Not ? operand : 0,
                    unary->op == UnaryOp::Not ? 0 : operand, &result)) {
                return false;
            }
            value = unary->op == UnaryOp::Plus ? operand : (__int128)(uint64_t)result;
            return true;
        }
        value = unary->op == UnaryOp::Not ? operand == 0
            : unary->op == UnaryOp::Negate || (unary->op == UnaryOp::Abs && operand < 0) ? -operand : operand;
        return true;
    }
    case ExprKind::Binary: {
        auto* binary = static_cast<BinaryExpr*>(expr);
        if (binary->operatorCall) {
            return false;
        }
        __int128 left = 0, right = 0, result = 0;
        bool comparison = binary->op >= BinaryOp::Equal && binary->op <= BinaryOp::GreaterEqual;
        if (comparison && isReal(binary->left->type)) {
            ExactReal a, b;
            bool exactLeft = exactValue(binary->left.get(), a);
            bool exactRight = exactValue(binary->right.get(), b);
            if (exactLeft && exactRight) {
                // Compare reduced fractions by continued quotients. Cross
                // multiplication can exceed capacity even when both fit.
                int signA = a.numerator < 0 ? -1 : a.numerator > 0 ? 1 : 0;
                int signB = b.numerator < 0 ? -1 : b.numerator > 0 ? 1 : 0;
                if (signA != signB) {
                    left = signA < signB ? -1 : 1;
                } else if (signA != 0) {
                    __int128 an = signA < 0 ? -a.numerator : a.numerator;
                    __int128 bn = signB < 0 ? -b.numerator : b.numerator;
                    __int128 ad = a.denominator, bd = b.denominator;
                    int direction = signA;
                    for (;;) {
                        __int128 aq = an / ad, bq = bn / bd;
                        if (aq != bq) {
                            left = direction * (aq < bq ? -1 : 1);
                            break;
                        }
                        __int128 ar = an % ad, br = bn % bd;
                        if (ar == 0 || br == 0) {
                            left = direction * (ar == br ? 0 : ar == 0 ? -1 : 1);
                            break;
                        }
                        an = ad; ad = ar;
                        bn = bd; bd = br;
                        direction = -direction;
                    }
                }
            } else {
                auto machine = [](Expr* operand) {
                    return operand->isStatic && operand->staticFailure == StaticFailure::None
                        && operand->type != nullptr && isReal(operand->type)
                        && !operand->exactReal.valid && std::isfinite(operand->staticReal);
                };
                if (!exactLeft && !exactRight && machine(binary->left.get()) && machine(binary->right.get())) {
                    double x = binary->left->staticReal, y = binary->right->staticReal;
                    left = x < y ? -1 : x > y ? 1 : 0;
                } else {
                    Expr* operand = exactLeft ? binary->right.get() : binary->left.get();
                    const ExactReal& exact = exactLeft ? a : b;
                    bool comparable = exactLeft != exactRight && machine(operand);
                    double x = operand->staticReal;
                    int order = 0;
                    if (comparable && std::fabs(x) >= std::ldexp(1.0, 127)) {
                        order = x < 0 ? -1 : 1;
                    } else if (comparable && x != 0.0 && std::fabs(x) <= std::ldexp(1.0, -127)) {
                        order = exact.numerator == 0 ? (x < 0 ? -1 : 1)
                            : exact.numerator < 0 ? 1 : -1;
                    } else {
                        if (binary->left->isStatic && binary->right->isStatic) {
                            failure = binary->left->staticFailure != StaticFailure::None
                                || binary->right->staticFailure != StaticFailure::None
                                    ? StaticFailure::Dependent : StaticFailure::Capacity;
                        }
                        return false;
                    }
                    left = exactLeft ? -order : order;
                }
            }
        } else {
            if (!foldWideInteger(binary->left.get(), left, failure)) {
                return false;
            }
            if (binary->right->isStatic
                && ((binary->op == BinaryOp::AndThen && left == 0)
                    || (binary->op == BinaryOp::OrElse && left != 0))) {
                value = left != 0;
                return true;
            }
            if (!foldWideInteger(binary->right.get(), right, failure)) {
                return false;
            }
        }
        if (expr->type != nullptr && expr->type->modulus != 0) {
            ModularOperation operation = ModularInvalid;
            switch (binary->op) {
            case BinaryOp::Add: operation = ModularAdd; break;
            case BinaryOp::Subtract: operation = ModularSubtract; break;
            case BinaryOp::Multiply: operation = ModularMultiply; break;
            case BinaryOp::Divide: operation = ModularDivide; break;
            case BinaryOp::Remainder: operation = ModularRemainder; break;
            case BinaryOp::Modulo: operation = ModularModulo; break;
            case BinaryOp::Power: operation = ModularPower; break;
            case BinaryOp::And: operation = ModularAnd; break;
            case BinaryOp::Or: operation = ModularOr; break;
            case BinaryOp::Xor: operation = ModularXor; break;
            default: break;
            }
            long long modular;
            if (left < 0 || left > (__int128)UINT64_MAX || right < LLONG_MIN || right > (__int128)UINT64_MAX) {
                failure = StaticFailure::Capacity;
                return false;
            }
            if (!modularOperation(operation, expr->type->modulus, left, right, &modular)) {
                failure = right == 0 ? StaticFailure::ZeroDivisor : StaticFailure::NegativeExponent;
                return false;
            }
            value = (uint64_t)modular;
            return true;
        }
        switch (binary->op) {
        case BinaryOp::Add: {
            bool overflow = __builtin_add_overflow(left, right, &result);
            return checked(overflow, result);
        }
        case BinaryOp::Subtract: {
            bool overflow = __builtin_sub_overflow(left, right, &result);
            return checked(overflow, result);
        }
        case BinaryOp::Multiply: {
            bool overflow = __builtin_mul_overflow(left, right, &result);
            return checked(overflow, result);
        }
        case BinaryOp::Divide:
        case BinaryOp::Modulo:
        case BinaryOp::Remainder:
            if (right == 0) {
                failure = StaticFailure::ZeroDivisor;
                return false;
            }
            value = binary->op == BinaryOp::Divide ? left / right : left % right;
            if (binary->op == BinaryOp::Modulo && value != 0 && (value < 0) != (right < 0)) {
                value += right;
            }
            return true;
        case BinaryOp::Power:
            if (right < 0) {
                failure = StaticFailure::NegativeExponent;
                return false;
            }
            result = 1;
            while (right != 0) {
                if ((right & 1) && (__builtin_mul_overflow(result, left, &result)
                    || !ExactReal::make(result).valid)) {
                    failure = StaticFailure::Capacity;
                    return false;
                }
                right >>= 1;
                if (right != 0 && (__builtin_mul_overflow(left, left, &left)
                    || !ExactReal::make(left).valid)) {
                    failure = StaticFailure::Capacity;
                    return false;
                }
            }
            value = result;
            return true;
        case BinaryOp::Equal: value = left == right; return true;
        case BinaryOp::NotEqual: value = left != right; return true;
        case BinaryOp::Less: value = left < right; return true;
        case BinaryOp::LessEqual: value = left <= right; return true;
        case BinaryOp::Greater: value = left > right; return true;
        case BinaryOp::GreaterEqual: value = left >= right; return true;
        case BinaryOp::And:
        case BinaryOp::AndThen: value = left != 0 && right != 0; return true;
        case BinaryOp::Or:
        case BinaryOp::OrElse: value = left != 0 || right != 0; return true;
        case BinaryOp::Xor: value = (left != 0) != (right != 0); return true;
        default: return false;
        }
    }
    default: return false;
    }
}

// Validate after resolving a complete expression, so intermediate static values
// are not narrowed and unevaluated static short-circuit operands stay exempt.
void Sema::validateStatic(Expr* expr, bool enclosingStatic, Type* expected)
{
    if (expr == nullptr) {
        return;
    }
    auto visit = [&](Expr* child, bool parentStatic) { validateStatic(child, parentStatic); };
    bool nested = expr->isStatic;
    if (expr->implicitCall) {
        visit(expr->implicitCall.get(), false);
    }
    switch (expr->kind) {
    case ExprKind::Unary: {
        auto* unary = static_cast<UnaryExpr*>(expr);
        visit(unary->operatorCall ? unary->operatorCall.get() : unary->operand.get(), nested);
        break;
    }
    case ExprKind::Binary: {
        auto* binary = static_cast<BinaryExpr*>(expr);
        if (binary->operatorCall) {
            visit(binary->operatorCall.get(), false);
            break;
        }
        visit(binary->left.get(), nested);
        long long left;
        bool skipped = expr->isStatic && foldStatic(binary->left.get(), left)
            && ((binary->op == BinaryOp::AndThen && left == 0)
                || (binary->op == BinaryOp::OrElse && left != 0));
        if (!skipped) {
            visit(binary->right.get(), nested);
        }
        break;
    }
    case ExprKind::Call: {
        auto* call = static_cast<CallExpr*>(expr);
        if (call->operatorExpression) {
            visit(call->operatorExpression.get(), nested);
        } else {
            visit(call->callee.get(), false);
            for (auto& argument : call->arguments) {
                visit(argument.value.get(), call->form == CallForm::Conversion && nested);
                visit(argument.high.get(), false);
            }
        }
        break;
    }
    case ExprKind::Qualified:
        visit(static_cast<QualifiedExpr*>(expr)->operand.get(), nested);
        break;
    case ExprKind::Attribute: {
        auto* attribute = static_cast<AttributeExpr*>(expr);
        visit(attribute->prefix.get(), false);
        for (auto& argument : attribute->arguments) {
            visit(argument.get(), nested);
        }
        break;
    }
    case ExprKind::Selected:
        visit(static_cast<SelectedExpr*>(expr)->prefix.get(), false);
        break;
    case ExprKind::Aggregate: {
        auto* aggregate = static_cast<AggregateExpr*>(expr);
        visit(aggregate->ancestor.get(), false);
        for (auto& component : aggregate->components) {
            visit(component.value.get(), false);
            for (auto& low : component.choiceLows) { visit(low.get(), false); }
            for (auto& high : component.choiceHighs) { visit(high.get(), false); }
        }
        break;
    }
    case ExprKind::Membership: {
        auto* membership = static_cast<MembershipExpr*>(expr);
        visit(membership->operand.get(), nested);
        visit(membership->low.get(), nested);
        visit(membership->high.get(), nested);
        break;
    }
    case ExprKind::Allocator: {
        auto* allocator = static_cast<AllocatorExpr*>(expr);
        visit(allocator->value.get(), false);
        if (allocator->subtype) {
            auto& subtype = *allocator->subtype;
            visit(subtype.digits.get(), false);
            visit(subtype.delta.get(), false);
            visit(subtype.rangeLow.get(), false);
            visit(subtype.rangeHigh.get(), false);
            for (auto& low : subtype.indexLows) { visit(low.get(), false); }
            for (auto& high : subtype.indexHighs) { visit(high.get(), false); }
        }
        break;
    }
    default: break;
    }
    if (expr->staticFailure == StaticFailure::Capacity
        || (expr->kind == ExprKind::RealLiteral && !expr->exactReal.valid)) {
        m_diagnostics.error(expr->location, "static expression exceeds supported exact evaluation limits (127-bit magnitude)");
    } else if (expr->staticFailure == StaticFailure::ZeroDivisor) {
        m_diagnostics.error(expr->location, "division by zero in static expression");
    } else if (expr->staticFailure == StaticFailure::NegativeExponent) {
        m_diagnostics.error(expr->location, "negative exponent in static integer expression");
    } else if (expr->staticFailure == StaticFailure::Check) {
        m_diagnostics.error(expr->location, "constraint check fails in static expression");
    } else if (expr->staticFailure == StaticFailure::AttributeRange) {
        auto* attribute = static_cast<AttributeExpr*>(expr);
        m_diagnostics.error(attribute->arguments.front()->location, "attribute argument is outside the prefix base range");
    } else if (expr->staticFailure == StaticFailure::FixedCapacity) {
        m_diagnostics.error(expr->location, "static fixed-point expression exceeds supported 64-bit scaled evaluation limits");
    }
    Type* target = expected != nullptr && expr->type != nullptr
        && (expr->type->kind == TypeKind::UniversalInteger || expr->type->kind == TypeKind::UniversalReal)
        ? expected : expr->type;
    if (expr->isStatic && expr->staticFailure == StaticFailure::None && !enclosingStatic
        && isReal(target) && expr->exactReal.valid) {
        if (expr->exactReal.numerator != 0) {
            expr->staticReal = expr->exactReal.real(typeSize(target) == 4 ? 24 : 53);
        }
    }
    if (expr->isStatic && expr->staticFailure == StaticFailure::None && !enclosingStatic
        && target != nullptr && target->kind == TypeKind::Integer) {
        __int128 value;
        StaticFailure failure = StaticFailure::None;
        if (foldWideInteger(expr, value, failure)) {
            __int128 low = qbeClass(target) == 'w' ? INT_MIN : LLONG_MIN;
            __int128 high = qbeClass(target) == 'w' ? INT_MAX : LLONG_MAX;
            if (target->modulus != 0) {
                low = 0;
                high = target->modulus - 1;
            }
            if (value < low || value > high) {
                m_diagnostics.error(expr->location, "static expression is outside the base range of its expected type");
            }
        }
    }
}

bool Sema::foldStaticReal(Expr* expr, double& value) const
{
    if (expr == nullptr || expr->staticFailure != StaticFailure::None) {
        return false;
    }
    if (expr->isStatic && isReal(expr->type)) {
        value = expr->staticReal;
        return true;
    }
    if (expr->type != nullptr && expr->type->kind == TypeKind::UniversalReal && expr->exactReal.valid) {
        value = expr->exactReal.real();
        return true;
    }

    switch (expr->kind) {
    case ExprKind::RealLiteral:
        value = static_cast<RealLiteralExpr*>(expr)->value;
        return true;
    case ExprKind::IntegerLiteral:
        value = static_cast<double>(static_cast<IntegerLiteralExpr*>(expr)->value);
        return true;
    case ExprKind::Unary: {
        auto* unary = static_cast<UnaryExpr*>(expr);
        if (unary->operatorCall) {
            return false;
        }
        double operand = 0.0;
        if (!foldStaticReal(unary->operand.get(), operand)) {
            return false;
        }
        switch (unary->op) {
        case UnaryOp::Plus:
            value = operand;
            return true;
        case UnaryOp::Negate:
            value = -operand;
            return true;
        case UnaryOp::Abs:
            value = operand < 0.0 ? -operand : operand;
            return true;
        default:
            return false;
        }
    }
    case ExprKind::Binary: {
        auto* binary = static_cast<BinaryExpr*>(expr);
        if (binary->operatorCall) {
            return false;
        }
        double left = 0.0;
        double right = 0.0;
        if (!foldStaticReal(binary->left.get(), left) || !foldStaticReal(binary->right.get(), right)) {
            return false;
        }
        switch (binary->op) {
        case BinaryOp::Add:
            value = left + right;
            return true;
        case BinaryOp::Subtract:
            value = left - right;
            return true;
        case BinaryOp::Multiply:
            value = left * right;
            return true;
        case BinaryOp::Divide:
            if (right == 0.0) {
                return false;
            }
            value = left / right;
            return true;
        default:
            return false;
        }
    }
    default:
        return false;
    }
}

// Records whatever an expression folds to, choosing the representation that
// matches its type.
void Sema::noteStaticValue(Expr* expr)
{
    if (expr == nullptr || expr->isStatic) {
        return;
    }
    bool staticOperands = false;
    if (expr->kind == ExprKind::Unary) {
        auto* unary = static_cast<UnaryExpr*>(expr);
        staticOperands = !unary->operatorCall && unary->operand->isStatic;
        if (staticOperands && unary->operand->staticFailure != StaticFailure::None) {
            expr->isStatic = true;
            expr->staticFailure = StaticFailure::Dependent;
            return;
        }
    } else if (expr->kind == ExprKind::Binary) {
        auto* binary = static_cast<BinaryExpr*>(expr);
        staticOperands = !binary->operatorCall && binary->left->isStatic && binary->right->isStatic;
    } else if (expr->kind == ExprKind::Membership) {
        auto* membership = static_cast<MembershipExpr*>(expr);
        staticOperands = membership->operand->isStatic && membership->low != nullptr && membership->low->isStatic
            && membership->high != nullptr && membership->high->isStatic;
    }
    if (expr->type != nullptr && expr->type->kind == TypeKind::Fixed) {
        long long value = 0;
        if (foldFixed(expr, value)) {
            expr->isStatic = true;
            expr->staticValue = value;
        } else if (expr->kind == ExprKind::Binary) {
            auto* binary = static_cast<BinaryExpr*>(expr);
            if (!binary->operatorCall && binary->left->isStatic && binary->right->isStatic
                && !expr->type->formalFixed) {
                expr->isStatic = true;
                expr->staticFailure = binary->left->staticFailure != StaticFailure::None
                    || binary->right->staticFailure != StaticFailure::None ? StaticFailure::Dependent
                    : binary->op == BinaryOp::Divide && binary->right->staticValue == 0
                        ? StaticFailure::ZeroDivisor : StaticFailure::FixedCapacity;
            }
        } else if (staticOperands) {
            expr->isStatic = true;
            expr->staticFailure = StaticFailure::FixedCapacity;
        }
        return;
    }
    if (isReal(expr->type)) {
        ExactReal exact;
        if (exactValue(expr, exact)) {
            expr->exactReal = exact;
            expr->isStatic = true;
            expr->staticReal = exact.real();
            if (exact.numerator == 0 && expr->kind == ExprKind::Unary) {
                auto* unary = static_cast<UnaryExpr*>(expr);
                double zero = unary->operand->staticReal;
                expr->staticReal = unary->op == UnaryOp::Negate ? -zero
                    : unary->op == UnaryOp::Abs ? std::fabs(zero) : zero;
            } else if (exact.numerator == 0 && expr->kind == ExprKind::Binary) {
                auto* binary = static_cast<BinaryExpr*>(expr);
                double a = binary->left->staticReal, b = binary->right->staticReal;
                double zero = binary->op == BinaryOp::Add ? a + b
                    : binary->op == BinaryOp::Subtract ? a - b
                    : binary->op == BinaryOp::Multiply ? a * b
                    : binary->op == BinaryOp::Divide && b != 0.0 ? a / b
                    : binary->op == BinaryOp::Power && binary->right->staticValue % 2 != 0 ? a : 0.0;
                expr->staticReal = std::copysign(0.0, zero);
            }
        } else if (expr->kind == ExprKind::Binary) {
            auto* binary = static_cast<BinaryExpr*>(expr);
            if (!binary->operatorCall && binary->left->isStatic && binary->right->isStatic) {
                expr->isStatic = true;
                ExactReal exponent;
                ExactReal divisor;
                bool zeroPower = binary->op == BinaryOp::Power && binary->left->staticReal == 0.0
                    && exactValue(binary->right.get(), exponent) && exponent.numerator < 0;
                expr->staticFailure = binary->left->staticFailure != StaticFailure::None
                    || binary->right->staticFailure != StaticFailure::None ? StaticFailure::Dependent
                    : zeroPower || (binary->op == BinaryOp::Divide
                        && exactValue(binary->right.get(), divisor) && divisor.numerator == 0)
                        ? StaticFailure::ZeroDivisor : StaticFailure::Capacity;
            }
        } else if (staticOperands) {
            expr->isStatic = true;
            expr->staticFailure = StaticFailure::Capacity;
        }
        return;
    }
    __int128 value = 0;
    StaticFailure failure = StaticFailure::None;
    if (foldWideInteger(expr, value, failure)) {
        expr->isStatic = true;
        expr->exactReal = ExactReal::make(value);
        expr->exactReal.integer(expr->staticValue);
    } else if (failure != StaticFailure::None && staticOperands) {
        expr->isStatic = true;
        expr->staticFailure = failure;
    }
}


bool Sema::exactValue(Expr* expr, ExactReal& value) const
{
    if (expr == nullptr || expr->staticFailure != StaticFailure::None) {
        return false;
    }
    if (expr->type != nullptr && expr->type->kind == TypeKind::Fixed) {
        long long counts = 0;
        if (!foldFixed(expr, counts)) {
            return false;
        }
        int bits = expr->type->fixedScale;
        value = ExactReal::fixed(counts, bits);
        return true;
    }
    if (expr->exactReal.valid) {
        value = expr->exactReal;
        return true;
    }
    if (expr->kind == ExprKind::RealLiteral) {
        return false;
    }
    if (expr->type != nullptr && isReal(expr->type) && expr->isStatic) {
        double binary = typeSize(expr->type) == 4 ? static_cast<double>(static_cast<float>(expr->staticReal))
                                                 : expr->staticReal;
        value = ExactReal::binary(binary);
        return value.valid;
    }
    if (expr->isStatic && isDiscrete(expr->type)) {
        value = ExactReal::make(expr->staticValue);
        return true;
    }
    if (expr->kind == ExprKind::Unary) {
        auto* unary = static_cast<UnaryExpr*>(expr);
        if (unary->operatorCall || !exactValue(unary->operand.get(), value)) {
            return false;
        }
        if (unary->op == UnaryOp::Negate || (unary->op == UnaryOp::Abs && value.numerator < 0)) {
            value.numerator = -value.numerator;
        }
        return unary->op != UnaryOp::Not;
    }
    if (expr->kind == ExprKind::Binary) {
        auto* binary = static_cast<BinaryExpr*>(expr);
        ExactReal left, right;
        if (binary->operatorCall || !exactValue(binary->left.get(), left)
            || !exactValue(binary->right.get(), right)) {
            return false;
        }
        char op = binary->op == BinaryOp::Add ? '+' : binary->op == BinaryOp::Subtract ? '-'
            : binary->op == BinaryOp::Multiply ? '*' : binary->op == BinaryOp::Divide ? '/' : 0;
        if (op != 0) {
            value = ExactReal::operation(op, left, right);
        } else if (binary->op == BinaryOp::Power && right.denominator == 1) {
            __int128 exponent = right.numerator;
            if (exponent < 0) {
                left = ExactReal::make(left.denominator, left.numerator);
                exponent = -exponent;
            }
            value = ExactReal::make(1);
            while (exponent != 0 && value.valid && left.valid) {
                if (exponent & 1) {
                    value = ExactReal::operation('*', value, left);
                }
                exponent >>= 1;
                if (exponent != 0) {
                    left = ExactReal::operation('*', left, left);
                }
            }
            return value.valid && left.valid;
        } else {
            return false;
        }
        return value.valid;
    }
    return false;
}

bool Sema::foldFixed(Expr* expr, long long& value) const
{
    if (expr == nullptr || expr->staticFailure != StaticFailure::None || expr->fixedInvalid
        || (expr->type != nullptr && expr->type->formalFixed)) {
        return false;
    }
    if (expr->isStatic) {
        value = expr->staticValue;
        return true;
    }
    if (expr->kind == ExprKind::Unary) {
        auto* unary = static_cast<UnaryExpr*>(expr);
        long long operand;
        if (unary->operatorCall || !foldFixed(unary->operand.get(), operand)) {
            return false;
        }
        if (unary->op == UnaryOp::Plus || (unary->op == UnaryOp::Abs && operand >= 0)) {
            value = operand;
            return true;
        }
        return fixedOperation(FixedSubtract, 0, 0, operand, 0, 0, &value);
    }
    if (expr->kind == ExprKind::Binary) {
        auto* binary = static_cast<BinaryExpr*>(expr);
        long long left, right;
        if (binary->operatorCall || !foldStatic(binary->left.get(), left)
            || !foldStatic(binary->right.get(), right)) {
            return false;
        }
        FixedOperation op = FixedInvalid;
        switch (binary->op) {
        case BinaryOp::Add: op = FixedAdd; break;
        case BinaryOp::Subtract: op = FixedSubtract; break;
        case BinaryOp::Multiply: op = binary->roundFixed ? FixedMultiplyRounded : FixedMultiply; break;
        case BinaryOp::Divide: op = binary->roundFixed ? FixedDivideRounded : FixedDivide; break;
        default: break;
        }
        return fixedOperation(op, left, binary->left->type->fixedScale,
            right, binary->right->type->fixedScale, expr->type->fixedScale, &value);
    }
    if (expr->kind == ExprKind::Qualified) {
        auto* qualified = static_cast<QualifiedExpr*>(expr);
        return expr->type->scalarBoundsSymbol == nullptr && foldFixed(qualified->operand.get(), value)
            && value >= expr->type->low && value <= expr->type->high;
    }
    if (expr->kind == ExprKind::Call) {
        auto* call = static_cast<CallExpr*>(expr);
        if (call->form != CallForm::Conversion || expr->type->scalarBoundsSymbol != nullptr) {
            return false;
        }
        ExactReal exact;
        if (!exactValue(call->resolvedArguments.front(), exact)) {
            return false;
        }
        return exact.scaled(expr->type->fixedScale, value)
            && value >= expr->type->low && value <= expr->type->high;
    }
    return false;
}
