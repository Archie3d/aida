#include "QbeSupport.h"

#include "QbeEmitter.h"

#include <cctype>
#include <cstdio>

namespace QbeSupport {

Value constantValue(long long value, char type)
{
    return Value { std::to_string(value), type };
}

// QBE reads these with scanf, so the full precision of the value is written out
// rather than the six decimals std::to_string would give.
std::string realLiteral(double value, char type)
{
    char buffer[40];
    std::snprintf(buffer, sizeof buffer, "%.17g", value);
    return (type == 's' ? "s_" : "d_") + std::string(buffer);
}

bool isFloatClass(char type)
{
    return type == 's' || type == 'd';
}

bool isUnconstrainedArray(const Type* type)
{
    return type != nullptr && type->kind == TypeKind::Array && !type->constrained;
}

bool isLiteralOperand(const std::string& operand)
{
    if (operand.empty()) {
        return false;
    }
    std::size_t start = operand[0] == '-' ? 1 : 0;
    if (start >= operand.size()) {
        return false;
    }
    for (std::size_t i = start; i < operand.size(); ++i) {
        if (std::isdigit(static_cast<unsigned char>(operand[i])) == 0) {
            return false;
        }
    }
    return true;
}

std::string cScalarClass(const Type* type)
{
    if (type->kind == TypeKind::Integer || type->cCharacter != CCharacterConvention::None) {
        const Type* storage = type;
        while (storage->isSubtype && storage->base != nullptr) { storage = storage->base; }
        bool signedValue = type->kind == TypeKind::Integer ? scalarStorageSigned(storage)
            : type->cCharacter == CCharacterConvention::Signed;
        switch (typeSize(type)) {
        case 1: return signedValue ? "sb" : "ub";
        case 2: return signedValue ? "sh" : "uh";
        case 4: return "w";
        case 8: return "l";
        }
    }
    return std::string(1, qbeClass(type));
}

const char* comparisonInstruction(BinaryOp op, char type, bool isUnsigned)
{
    if (isUnsigned) {
        switch (op) {
        case BinaryOp::Less: return type == 'l' ? "cultl" : "cultw";
        case BinaryOp::LessEqual: return type == 'l' ? "culel" : "culew";
        case BinaryOp::Greater: return type == 'l' ? "cugtl" : "cugtw";
        case BinaryOp::GreaterEqual: return type == 'l' ? "cugel" : "cugew";
        default: break;
        }
    }
    bool isFloat = type == 's' || type == 'd';
    switch (op) {
    case BinaryOp::Equal:
        return isFloat ? (type == 's' ? "ceqs" : "ceqd") : (type == 'l' ? "ceql" : "ceqw");
    case BinaryOp::NotEqual:
        return isFloat ? (type == 's' ? "cnes" : "cned") : (type == 'l' ? "cnel" : "cnew");
    case BinaryOp::Less:
        return isFloat ? (type == 's' ? "clts" : "cltd") : (type == 'l' ? "csltl" : "csltw");
    case BinaryOp::LessEqual:
        return isFloat ? (type == 's' ? "cles" : "cled") : (type == 'l' ? "cslel" : "cslew");
    case BinaryOp::Greater:
        return isFloat ? (type == 's' ? "cgts" : "cgtd") : (type == 'l' ? "csgtl" : "csgtw");
    case BinaryOp::GreaterEqual:
        return isFloat ? (type == 's' ? "cges" : "cged") : (type == 'l' ? "csgel" : "csgew");
    default:
        return "ceqw";
    }
}

} // namespace QbeSupport
