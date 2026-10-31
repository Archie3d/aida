#include "Sema.h"
#include "SemaSupport.h"
#include "../../common/FloatAttributes.h"

#include <cmath>

namespace
{

ExactReal scaleExact(ExactReal value, long long exponent)
{
    // The bounded evaluator cannot represent larger powers. Multiplication
    // cancels factors before checking capacity in ExactReal::operation.
    if (value.numerator == 0) { return value; }
    if (exponent < -126 || exponent > 126) { return {}; }
    ExactReal factor = exponent >= 0 ? ExactReal::make((__int128)1 << exponent)
        : ExactReal::make(1, (__int128)1 << -exponent);
    return ExactReal::operation('*', value, factor);
}

__int128 integralPart(ExactReal value, FloatAttribute operation)
{
    __int128 whole = value.numerator / value.denominator;
    __int128 remainder = value.numerator % value.denominator;
    __int128 magnitude = remainder < 0 ? -remainder : remainder;
    bool half = magnitude == value.denominator - magnitude;
    bool greater = magnitude > value.denominator - magnitude;
    if ((operation == FloatFloor && remainder < 0)
        || (operation == FloatCeiling && remainder > 0)
        || (operation == FloatRounding && (greater || half))
        || ((operation == FloatUnbiasedRounding || operation == FloatMachineRounding)
            && (greater || (half && whole % 2 != 0)))) {
        whole += value.numerator < 0 ? -1 : 1;
    }
    return whole;
}

bool machineNumber(ExactReal value, bool narrow)
{
    if (value.numerator == 0) { return true; }
    if ((value.denominator & (value.denominator - 1)) != 0) { return false; }
    int exponent = 0;
    for (__int128 denominator = value.denominator; denominator > 1; denominator >>= 1) { --exponent; }
    unsigned __int128 magnitude = value.numerator < 0 ? -value.numerator : value.numerator;
    while (magnitude % 2 == 0) { magnitude >>= 1; ++exponent; }
    int bits = 0;
    for (; magnitude != 0; magnitude >>= 1) { ++bits; }
    return bits <= floatMantissa(narrow)
        && exponent >= floatMinimumExponent(narrow) - floatMantissa(narrow)
        && exponent + bits <= (narrow ? FLT_MAX_EXP : DBL_MAX_EXP);
}

int normalizedExponent(ExactReal value)
{
    if (value.numerator == 0) { return 0; }
    // Compare normalized fractions without overflowing a cross product.
    unsigned __int128 numerator = value.numerator < 0 ? -value.numerator : value.numerator;
    unsigned __int128 denominator = value.denominator;
    int exponent = 0;
    if (numerator >= denominator) {
        while (numerator >= denominator) { numerator >>= 1; ++exponent; }
    } else {
        while (numerator < denominator) { numerator <<= 1; --exponent; }
        ++exponent;
    }
    return exponent;
}

}

Type* Sema::analyzeFloatAttribute(AttributeExpr* expr, Scope* scope, Type* prefix, bool prefixIsType)
{
    const FloatAttributeInfo& info = *floatAttributeInfo(expr->lower.c_str());
    if (!prefixIsType || prefix == nullptr || prefix->kind != TypeKind::Float || !representationVisible(prefix)) {
        m_diagnostics.error(expr->location, "attribute '" + expr->name + "' requires a visible floating-point subtype mark");
        return nullptr;
    }
    if (expr->arguments.size() != static_cast<std::size_t>(info.arguments)) {
        m_diagnostics.error(expr->location, "attribute '" + expr->name + "' takes exactly "
            + std::to_string(info.arguments) + (info.arguments == 1 ? " argument" : " arguments"));
        return nullptr;
    }
    Type* base = m_types.scalarBaseType(prefix);
    expr->type = info.result == FloatIntegerResult ? m_types.universalInteger()
        : info.result == FloatBooleanResult ? m_types.booleanType()
        : info.result == FloatRealResult ? m_types.universalReal() : base;
    bool staticPrefix = prefix->scalarBoundsSymbol == nullptr;
    for (Type* type = prefix; type != nullptr; type = type->isSubtype ? type->base : type->parentType) {
        staticPrefix &= !type->genericFormal;
    }
    bool narrow = typeSize(base) == 4;
    if (info.arguments == 0) {
        expr->isStatic = staticPrefix;
        double value = floatAttributeProperty(info.operation, narrow);
        if (info.result == FloatRealResult) {
            expr->staticReal = value;
        } else {
            expr->staticValue = static_cast<long long>(value);
            if (staticPrefix) { expr->exactReal = ExactReal::make(expr->staticValue); }
        }
        return expr->type;
    }
    bool allStatic = staticPrefix;
    int errors = m_diagnostics.errorCount();
    for (std::size_t i = 0; i < expr->arguments.size(); ++i) {
        Expr* argument = expr->arguments[i].get();
        bool integer = i == 1 && info.integerArgument;
        Type* actual = analyzeExpr(argument, scope, integer ? nullptr : base);
        if (integer) {
            if (actual == nullptr || !representationVisible(actual)
                || (actual->kind != TypeKind::Integer && actual->kind != TypeKind::UniversalInteger)) {
                m_diagnostics.error(argument->location, "attribute '" + expr->name + "' requires an integer second argument");
            }
        } else {
            SemaSupport::adaptUniversal(argument, base);
            if (!typesCompatible(base, argument->type)) {
                m_diagnostics.error(argument->location, "attribute '" + expr->name + "' requires an argument of the prefix type");
            }
        }
        allStatic &= argument->isStatic;
    }
    if (m_diagnostics.errorCount() != errors || !allStatic) { return expr->type; }
    expr->isStatic = true;
    for (const ExprPtr& argument : expr->arguments) {
        if (argument->staticFailure != StaticFailure::None) {
            expr->staticFailure = StaticFailure::Dependent;
            return expr->type;
        }
    }
    long long n = 0;
    if (info.integerArgument) {
        __int128 value = 0;
        StaticFailure failure = StaticFailure::None;
        if (!foldWideInteger(expr->arguments[1].get(), value, failure)) {
            expr->staticFailure = StaticFailure::Capacity;
            return expr->type;
        }
        n = value < LLONG_MIN ? LLONG_MIN : value > LLONG_MAX ? LLONG_MAX : static_cast<long long>(value);
    }
    if (info.operation == FloatLeadingPart && n <= 0) {
        expr->staticFailure = StaticFailure::Check;
        return expr->type;
    }
    ExactReal x, y, result;
    bool exactX = exactValue(expr->arguments[0].get(), x);
    bool exactY = info.arguments < 2 || info.integerArgument || exactValue(expr->arguments[1].get(), y);
    if (info.operation == FloatRemainder && ((exactY && y.valid && y.numerator == 0)
        || expr->arguments[1]->staticReal == 0.0)) {
        expr->staticFailure = StaticFailure::Check;
        return expr->type;
    }
    if (info.operation == FloatAdjacent && exactX && exactY) {
        ExactReal direction = ExactReal::operation('-', y, x);
        double rounded = x.real(narrow ? 24 : 53);
        ExactReal difference = ExactReal::operation('-', ExactReal::binary(rounded), x);
        if (!direction.valid || !difference.valid) {
            expr->staticFailure = StaticFailure::Capacity;
        } else if (direction.numerator == 0) {
            expr->exactReal = x;
            expr->staticReal = x.numerator == 0 ? expr->arguments[0]->staticReal : x.real();
        } else {
            if (difference.numerator == 0 || (difference.numerator < 0) == (direction.numerator > 0)) {
                double towards = direction.numerator > 0 ? HUGE_VAL : -HUGE_VAL;
                rounded = floatAttributeValue(FloatAdjacent, rounded, towards, 0, narrow);
            }
            if (!std::isfinite(rounded)) { expr->staticFailure = StaticFailure::Check; }
            else { expr->staticReal = rounded; }
        }
        return expr->type;
    }
    bool machine = info.operation == FloatMachine || info.operation == FloatModel || info.operation == FloatAdjacent;
    if (exactX && exactY && !machine) {
        int exponent = normalizedExponent(x);
        switch (info.operation) {
        case FloatFloor:
        case FloatCeiling:
        case FloatRounding:
        case FloatUnbiasedRounding:
        case FloatMachineRounding:
        case FloatTruncation:
            result = ExactReal::make(integralPart(x, info.operation));
            break;
        case FloatExponent:
            expr->staticValue = exponent;
            expr->exactReal = ExactReal::make(exponent);
            return expr->type;
        case FloatFraction: result = scaleExact(x, -exponent); break;
        case FloatCompose:
            if (n < -252 || n > 252) { break; }
            result = scaleExact(x, n - exponent);
            break;
        case FloatScaling: result = scaleExact(x, n); break;
        case FloatCopySign:
            result = x;
            if ((x.numerator < 0) != std::signbit(expr->arguments[1]->staticReal)) { result.numerator = -result.numerator; }
            break;
        case FloatRemainder: {
            ExactReal quotient = ExactReal::operation('/', x, y);
            if (quotient.valid) {
                result = ExactReal::operation('-', x, ExactReal::operation('*', y,
                    ExactReal::make(integralPart(quotient, FloatUnbiasedRounding))));
                if (result.valid && !machineNumber(result, narrow)) { result = ExactReal::make(0); }
            }
            break;
        }
        case FloatLeadingPart: {
            unsigned __int128 magnitude = x.numerator < 0 ? -x.numerator : x.numerator;
            while (magnitude != 0 && magnitude % 2 == 0) { magnitude >>= 1; }
            int significantBits = 0;
            for (; magnitude != 0; magnitude >>= 1) { ++significantBits; }
            bool dyadic = (x.denominator & (x.denominator - 1)) == 0;
            if (dyadic && n >= significantBits) { result = x; break; }
            if (n > 252) { break; }
            ExactReal shifted = scaleExact(x, n - exponent);
            if (shifted.valid) {
                result = scaleExact(ExactReal::make(integralPart(shifted, FloatTruncation)), exponent - n);
            }
            break;
        }
        default: break;
        }
        if (!result.valid) {
            expr->staticFailure = StaticFailure::Capacity;
            return expr->type;
        }
        expr->exactReal = result;
        expr->staticReal = result.real();
        // Integral/zero attributes retain the specified IEEE zero sign.
        if (result.numerator == 0) {
            double sign = info.operation == FloatCopySign ? expr->arguments[1]->staticReal
                : info.operation == FloatFloor && x.numerator != 0 ? 1.0 : expr->arguments[0]->staticReal;
            expr->staticReal = std::copysign(0.0, sign);
        }
        return expr->type;
    }
    // Machine-oriented attributes and already-materialized machine constants
    // can be evaluated across the whole binary exponent range.
    double a = exactX ? x.real(narrow ? 24 : 53) : expr->arguments[0]->staticReal;
    if (a == 0.0) { a = std::copysign(0.0, expr->arguments[0]->staticReal); }
    double b = info.arguments == 2 && !info.integerArgument
        ? (exactY ? y.real(narrow ? 24 : 53) : expr->arguments[1]->staticReal) : 0.0;
    if (info.arguments == 2 && !info.integerArgument && b == 0.0) {
        b = std::copysign(0.0, expr->arguments[1]->staticReal);
    }
    double value = floatAttributeValue(info.operation, a, b, n, narrow);
    if (!std::isfinite(value)) {
        expr->staticFailure = StaticFailure::Check;
    } else if (info.result == FloatIntegerResult) {
        expr->staticValue = static_cast<long long>(value);
        expr->exactReal = ExactReal::make(expr->staticValue);
    } else {
        expr->staticReal = value;
    }
    return expr->type;
}
