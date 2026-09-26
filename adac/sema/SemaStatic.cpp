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
    if (expr == nullptr || isReal(expr->type)) {
        return false;
    }
    if (expr->isStatic) {
        value = expr->staticValue;
        return true;
    }

    switch (expr->kind) {
    case ExprKind::IntegerLiteral:
        value = static_cast<IntegerLiteralExpr*>(expr)->value;
        return true;
    case ExprKind::CharacterLiteral:
        value = static_cast<unsigned char>(static_cast<CharacterLiteralExpr*>(expr)->value);
        return true;
    case ExprKind::Unary: {
        auto* unary = static_cast<UnaryExpr*>(expr);
        if (unary->operatorCall) {
            return false;
        }
        long long operand = 0;
        if (!foldStatic(unary->operand.get(), operand)) {
            return false;
        }
        if (expr->type != nullptr && expr->type->m_modulus != 0) {
            long long modulus = expr->type->m_modulus;
            if (unary->op == UnaryOp::Negate) {
                return modularOperation(ModularSubtract, modulus, 0, operand, &value);
            }
            if (unary->op == UnaryOp::Not) {
                return modularOperation(ModularNot, modulus, operand, 0, &value);
            }
        }
        switch (unary->op) {
        case UnaryOp::Plus:
            value = operand;
            return true;
        case UnaryOp::Negate:
            return !__builtin_sub_overflow(0LL, operand, &value);
        case UnaryOp::Abs:
            if (operand < 0) {
                return !__builtin_sub_overflow(0LL, operand, &value);
            }
            value = operand;
            return true;
        case UnaryOp::Not:
            value = operand != 0 ? 0 : 1;
            return true;
        }
        return false;
    }
    case ExprKind::Binary: {
        auto* binary = static_cast<BinaryExpr*>(expr);
        if (binary->operatorCall) {
            return false;
        }
        long long left = 0;
        long long right = 0;
        if (!foldStatic(binary->left.get(), left) || !foldStatic(binary->right.get(), right)) {
            return false;
        }
        if (expr->type != nullptr && expr->type->m_modulus != 0) {
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
            if (operation != ModularInvalid) {
                return modularOperation(operation, expr->type->m_modulus, left, right, &value);
            }
        }
        switch (binary->op) {
        case BinaryOp::Add:
            return !__builtin_add_overflow(left, right, &value);
        case BinaryOp::Subtract:
            return !__builtin_sub_overflow(left, right, &value);
        case BinaryOp::Multiply:
            return !__builtin_mul_overflow(left, right, &value);
        case BinaryOp::Divide:
            if (right == 0 || (left == std::numeric_limits<long long>::min() && right == -1)) {
                return false;
            }
            value = left / right;
            return true;
        case BinaryOp::Modulo:
        case BinaryOp::Remainder:
            if (right == 0) {
                return false;
            }
            value = left == std::numeric_limits<long long>::min() && right == -1 ? 0 : left % right;
            if (binary->op == BinaryOp::Modulo && value != 0 && (value < 0) != (right < 0)) {
                value += right;
            }
            return true;
        case BinaryOp::Power: {
            if (right < 0) {
                return false;
            }
            long long result = 1;
            while (right != 0) {
                if ((right & 1) && __builtin_mul_overflow(result, left, &result)) {
                    return false;
                }
                right >>= 1;
                if (right != 0 && __builtin_mul_overflow(left, left, &left)) {
                    return false;
                }
            }
            value = result;
            return true;
        }
        default:
            return false;
        }
    }
    default:
        return false;
    }
}

bool Sema::foldStaticReal(Expr* expr, double& value) const
{
    if (expr == nullptr) {
        return false;
    }
    if (expr->isStatic && isReal(expr->type)) {
        value = expr->staticReal;
        return true;
    }
    if (expr->type != nullptr && expr->type->kind == TypeKind::UniversalReal && expr->m_exactReal.m_valid) {
        value = static_cast<double>(expr->m_exactReal.m_numerator)
            / static_cast<double>(expr->m_exactReal.m_denominator);
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
    if (expr->type != nullptr && expr->type->kind == TypeKind::Fixed) {
        long long value = 0;
        if (foldFixed(expr, value)) {
            expr->isStatic = true;
            expr->staticValue = value;
        }
        return;
    }
    if (isReal(expr->type)) {
        ExactReal exact;
        if (exactValue(expr, exact)) {
            expr->m_exactReal = exact;
        }
        double value = 0.0;
        if (foldStaticReal(expr, value)) {
            expr->isStatic = true;
            expr->staticReal = value;
        }
        return;
    }
    long long value = 0;
    if (foldStatic(expr, value)) {
        expr->isStatic = true;
        expr->staticValue = value;
    }
}


bool Sema::exactValue(Expr* expr, ExactReal& value) const
{
    if (expr == nullptr) {
        return false;
    }
    if (expr->type != nullptr && expr->type->kind == TypeKind::Fixed) {
        long long counts = 0;
        if (!foldFixed(expr, counts)) {
            return false;
        }
        value = ExactReal::make(counts, (__int128)1 << expr->type->m_fixedBits);
        return true;
    }
    if (expr->type != nullptr && expr->type->kind == TypeKind::Float) {
        if (!expr->isStatic) {
            return false;
        }
        double binary = typeSize(expr->type) == 4 ? static_cast<double>(static_cast<float>(expr->staticReal))
                                                 : expr->staticReal;
        if (!std::isfinite(binary)) {
            return false;
        }
        int exponent = 0;
        double fraction = std::frexp(binary, &exponent);
        __int128 numerator = static_cast<long long>(std::ldexp(fraction, 53));
        exponent -= 53;
        if (exponent < -126 || exponent > 73) {
            return false;
        }
        value = exponent >= 0 ? ExactReal::make(numerator * ((__int128)1 << exponent))
                              : ExactReal::make(numerator, (__int128)1 << -exponent);
        return value.m_valid;
    }
    if (expr->m_exactReal.m_valid) {
        value = expr->m_exactReal;
        return true;
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
        if (unary->op == UnaryOp::Negate || (unary->op == UnaryOp::Abs && value.m_numerator < 0)) {
            value.m_numerator = -value.m_numerator;
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
        } else if (binary->op == BinaryOp::Power && right.m_denominator == 1
                   && right.m_numerator >= -128 && right.m_numerator <= 128) {
            int exponent = static_cast<int>(right.m_numerator);
            value = ExactReal::make(1);
            for (int i = 0; i < (exponent < 0 ? -exponent : exponent); ++i) {
                value = ExactReal::operation(exponent < 0 ? '/' : '*', value, left);
            }
        } else {
            return false;
        }
        return value.m_valid;
    }
    return false;
}

bool Sema::foldFixed(Expr* expr, long long& value) const
{
    if (expr == nullptr || expr->m_fixedInvalid) {
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
        case BinaryOp::Multiply: op = FixedMultiply; break;
        case BinaryOp::Divide: op = FixedDivide; break;
        default: break;
        }
        return fixedOperation(op, left, binary->left->type->m_fixedBits,
            right, binary->right->type->m_fixedBits, expr->type->m_fixedBits, &value);
    }
    if (expr->kind == ExprKind::Qualified) {
        auto* qualified = static_cast<QualifiedExpr*>(expr);
        return foldFixed(qualified->operand.get(), value)
            && value >= expr->type->low && value <= expr->type->high;
    }
    if (expr->kind == ExprKind::Call) {
        auto* call = static_cast<CallExpr*>(expr);
        if (call->form != CallForm::Conversion) {
            return false;
        }
        ExactReal exact;
        if (!exactValue(call->resolvedArguments.front(), exact)) {
            return false;
        }
        return exact.scaled(expr->type->m_fixedBits, value)
            && value >= expr->type->low && value <= expr->type->high;
    }
    return false;
}
