#include "SemaSupport.h"

#include "Ast.h"
#include "Type.h"

namespace SemaSupport
{

// Subtype constraints are part of the contract for array indexes/components.
bool staticallyMatches(Type* actual, Type* formal)
{
    if (actual == nullptr || formal == nullptr || baseType(actual) != baseType(formal)) {
        return false;
    }
    if (actual == formal) {
        return true;
    }
    if (isScalar(actual)) {
        return actual->m_scalarBoundsSymbol == formal->m_scalarBoundsSymbol
            && actual->low == formal->low && actual->high == formal->high
            && actual->hasRealRange == formal->hasRealRange
            && actual->lowReal == formal->lowReal && actual->highReal == formal->highReal;
    }
    if (actual->kind == TypeKind::Array) {
        if (actual->constrained != formal->constrained || actual->m_boundsSymbol != formal->m_boundsSymbol) {
            return false;
        }
        int rank = actual->arrayRank;
        for (int dimension = 0; dimension < rank; ++dimension) {
            if (actual->constrained && (actual->indexLow != formal->indexLow || actual->indexHigh != formal->indexHigh)) {
                return false;
            }
            actual = actual->element;
            formal = formal->element;
        }
        return true;
    }
    return actual->discriminantsKnown == formal->discriminantsKnown
        && actual->discriminantValues == formal->discriminantValues;
}

bool isUniversal(const Type* type)
{
    return type != nullptr
           && (type->kind == TypeKind::UniversalInteger || type->kind == TypeKind::UniversalReal);
}

// Gives literals and expressions built purely from literals the type imposed by
// their context.
void adaptUniversal(Expr* expr, Type* type)
{
    if (expr == nullptr || type == nullptr || !isUniversal(expr->type)) {
        return;
    }
    if (type->kind == TypeKind::Fixed) {
        if (type->m_formalFixed) {
            expr->type = type;
            expr->isStatic = false;
            return;
        }
        expr->m_fixedInvalid = !expr->m_exactReal.scaled(type->m_fixedBits, expr->staticValue);
        expr->isStatic = !expr->m_fixedInvalid;
        expr->type = type;
        return;
    }
    expr->type = type;
    if (expr->kind == ExprKind::Unary) {
        adaptUniversal(static_cast<UnaryExpr*>(expr)->operand.get(), type);
    } else if (expr->kind == ExprKind::Binary) {
        auto* binary = static_cast<BinaryExpr*>(expr);
        if (binary->operatorCall) {
            return;
        }
        adaptUniversal(binary->left.get(), type);
        adaptUniversal(binary->right.get(), type);
    }
}

std::vector<std::string> splitDottedName(const std::string& name)
{
    std::vector<std::string> parts;
    std::string current;
    for (char c : name) {
        if (c == '.') {
            parts.push_back(current);
            current.clear();
        } else {
            current.push_back(c);
        }
    }
    parts.push_back(current);
    return parts;
}

}
