#include "SemaSupport.h"

#include "Ast.h"
#include "Type.h"

namespace SemaSupport
{

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
