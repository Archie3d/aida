#include "SemaSupport.h"
#include "Symbol.h"

#include "Ast.h"
#include "Type.h"

namespace SemaSupport
{

bool conformingExpressions(const Expr* left, const Expr* right)
{
    if (left == nullptr || right == nullptr) {
        return left == right;
    }
    auto normalized = [](const Expr* expression) -> const Expr* {
        for (;;) {
            const Expr* replacement = nullptr;
            if (expression->kind == ExprKind::Call) {
                replacement = static_cast<const CallExpr*>(expression)->operatorExpression.get();
            } else if (expression->kind == ExprKind::Binary) {
                replacement = static_cast<const BinaryExpr*>(expression)->operatorCall.get();
            } else if (expression->kind == ExprKind::Unary) {
                replacement = static_cast<const UnaryExpr*>(expression)->operatorCall.get();
            }
            if (replacement == nullptr) {
                return expression;
            }
            expression = replacement;
        }
    };
    left = normalized(left);
    right = normalized(right);
    auto named = [](const Expr* expression) -> Symbol* {
        if (expression->kind == ExprKind::Identifier) {
            return static_cast<const IdentifierExpr*>(expression)->symbol;
        }
        if (expression->kind == ExprKind::Selected) {
            return static_cast<const SelectedExpr*>(expression)->symbol;
        }
        return nullptr;
    };
    if (Symbol* symbol = named(left)) {
        return symbol == named(right);
    }
    if (left->kind != right->kind) {
        return false;
    }
    auto sameList = [](const auto& a, const auto& b) {
        if (a.size() != b.size()) {
            return false;
        }
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (!conformingExpressions(a[i].get(), b[i].get())) {
                return false;
            }
        }
        return true;
    };
    switch (left->kind) {
    case ExprKind::IntegerLiteral: {
        ExactReal a = left->exactReal.valid ? left->exactReal
            : ExactReal::make(static_cast<const IntegerLiteralExpr*>(left)->value);
        ExactReal b = right->exactReal.valid ? right->exactReal
            : ExactReal::make(static_cast<const IntegerLiteralExpr*>(right)->value);
        return a.numerator == b.numerator;
    }
    case ExprKind::RealLiteral:
        return left->exactReal.valid && right->exactReal.valid
            && left->exactReal.numerator == right->exactReal.numerator
            && left->exactReal.denominator == right->exactReal.denominator;
    case ExprKind::StringLiteral:
        return static_cast<const StringLiteralExpr*>(left)->value == static_cast<const StringLiteralExpr*>(right)->value;
    case ExprKind::CharacterLiteral:
        return left->type == right->type
            && static_cast<const CharacterLiteralExpr*>(left)->value == static_cast<const CharacterLiteralExpr*>(right)->value;
    case ExprKind::Box:
    case ExprKind::Null:
        return true;
    case ExprKind::Identifier:
        return false;
    case ExprKind::Selected: {
        auto* a = static_cast<const SelectedExpr*>(left);
        auto* b = static_cast<const SelectedExpr*>(right);
        return a->selectorLower == b->selectorLower && conformingExpressions(a->prefix.get(), b->prefix.get());
    }
    case ExprKind::Binary: {
        auto* a = static_cast<const BinaryExpr*>(left);
        auto* b = static_cast<const BinaryExpr*>(right);
        return a->op == b->op && conformingExpressions(a->operatorCall.get(), b->operatorCall.get())
            && conformingExpressions(a->left.get(), b->left.get()) && conformingExpressions(a->right.get(), b->right.get());
    }
    case ExprKind::Unary: {
        auto* a = static_cast<const UnaryExpr*>(left);
        auto* b = static_cast<const UnaryExpr*>(right);
        return a->op == b->op && conformingExpressions(a->operatorCall.get(), b->operatorCall.get())
            && conformingExpressions(a->operand.get(), b->operand.get());
    }
    case ExprKind::Call: {
        auto* a = static_cast<const CallExpr*>(left);
        auto* b = static_cast<const CallExpr*>(right);
        if (a->form != b->form || a->subprogram != b->subprogram || a->arguments.size() != b->arguments.size()
            || (a->subprogram == nullptr && !conformingExpressions(a->callee.get(), b->callee.get()))) {
            return false;
        }
        for (std::size_t i = 0; i < a->arguments.size(); ++i) {
            if (a->arguments[i].nameLower != b->arguments[i].nameLower
                || !conformingExpressions(a->arguments[i].value.get(), b->arguments[i].value.get())
                || !conformingExpressions(a->arguments[i].high.get(), b->arguments[i].high.get())) {
                return false;
            }
        }
        return true;
    }
    case ExprKind::Attribute: {
        auto* a = static_cast<const AttributeExpr*>(left);
        auto* b = static_cast<const AttributeExpr*>(right);
        return a->lower == b->lower && a->prefixType == b->prefixType
            && a->exceptionSymbol == b->exceptionSymbol && a->accessSubprogram == b->accessSubprogram
            && (a->accessSubprogram != nullptr || a->exceptionSymbol != nullptr
                || conformingExpressions(a->prefix.get(), b->prefix.get()))
            && sameList(a->arguments, b->arguments);
    }
    case ExprKind::Qualified: {
        auto* a = static_cast<const QualifiedExpr*>(left);
        auto* b = static_cast<const QualifiedExpr*>(right);
        return a->type == b->type && conformingExpressions(a->operand.get(), b->operand.get());
    }
    case ExprKind::Aggregate: {
        auto* a = static_cast<const AggregateExpr*>(left);
        auto* b = static_cast<const AggregateExpr*>(right);
        bool ancestorMatches = a->ancestorIsType && b->ancestorIsType
            ? a->ancestorType == b->ancestorType
            : conformingExpressions(a->ancestor.get(), b->ancestor.get());
        if (a->components.size() != b->components.size() || a->nullRecord != b->nullRecord
            || a->ancestorIsType != b->ancestorIsType || !ancestorMatches) {
            return false;
        }
        for (std::size_t i = 0; i < a->components.size(); ++i) {
            const auto& x = a->components[i];
            const auto& y = b->components[i];
            bool record = left->type != nullptr && baseType(left->type)->kind == TypeKind::Record;
            if ((record && x.names != y.names) || x.isOthers != y.isOthers
                || (!record && (!sameList(x.choiceLows, y.choiceLows) || !sameList(x.choiceHighs, y.choiceHighs)))
                || !conformingExpressions(x.value.get(), y.value.get())) {
                return false;
            }
        }
        return true;
    }
    case ExprKind::Membership: {
        auto* a = static_cast<const MembershipExpr*>(left);
        auto* b = static_cast<const MembershipExpr*>(right);
        return a->negated == b->negated && a->membershipSubtype == b->membershipSubtype
            && conformingExpressions(a->operand.get(), b->operand.get())
            && conformingExpressions(a->low.get(), b->low.get()) && conformingExpressions(a->high.get(), b->high.get());
    }
    case ExprKind::Allocator: {
        auto* a = static_cast<const AllocatorExpr*>(left);
        auto* b = static_cast<const AllocatorExpr*>(right);
        if (!staticallyMatches(a->designated, b->designated)
            || !conformingExpressions(a->value.get(), b->value.get()) || bool(a->subtype) != bool(b->subtype)) {
            return false;
        }
        if (a->subtype == nullptr) {
            return true;
        }
        const auto& x = a->subtype;
        const auto& y = b->subtype;
        return conformingExpressions(x->digits.get(), y->digits.get())
            && conformingExpressions(x->delta.get(), y->delta.get())
            && conformingExpressions(x->rangeLow.get(), y->rangeLow.get())
            && conformingExpressions(x->rangeHigh.get(), y->rangeHigh.get())
            && x->discriminantNames == y->discriminantNames
            && sameList(x->indexLows, y->indexLows) && sameList(x->indexHighs, y->indexHighs);
    }
    }
    return false;
}

bool profileTypeMatches(Type* actual, Type* formal)
{
    if (rootType(actual) == rootType(formal)) { return true; }
    if (actual == nullptr || formal == nullptr || !actual->anonymousAccess || !formal->anonymousAccess) { return false; }
    Type a = *actual;
    Type b = *formal;
    a.excludesNull = false;
    b.excludesNull = false;
    return staticallyMatches(&a, &b);
}

// Subtype constraints are part of the contract for array indexes/components.
bool staticallyMatches(Type* actual, Type* formal)
{
    if (actual != nullptr && formal != nullptr && actual->excludesNull != formal->excludesNull) { return false; }
    if (actual != nullptr && formal != nullptr && actual->anonymousAccess && formal->anonymousAccess) {
        if (actual->accessConstant != formal->accessConstant || actual->accessProtected != formal->accessProtected
            || actual->accessConventionC != formal->accessConventionC) { return false; }
        if (actual->accessProfile != nullptr || formal->accessProfile != nullptr) {
            Symbol* a = actual->accessProfile;
            Symbol* b = formal->accessProfile;
            if (a == nullptr || b == nullptr || a->parameters.size() != b->parameters.size()
                || (a->returnType != b->returnType && !staticallyMatches(a->returnType, b->returnType))) { return a == b; }
            for (std::size_t i = 0; i < a->parameters.size(); ++i) {
                if (a->parameters[i]->mode != b->parameters[i]->mode
                    || !staticallyMatches(a->parameters[i]->type, b->parameters[i]->type)) { return false; }
            }
            return true;
        }
        return staticallyMatches(actual->target, formal->target);
    }
    if (actual == nullptr || formal == nullptr || baseType(actual) != baseType(formal)) {
        return false;
    }
    if (actual == formal) {
        return true;
    }
    if (isScalar(actual)) {
        return actual->scalarBoundsSymbol == formal->scalarBoundsSymbol
            && actual->low == formal->low && actual->high == formal->high
            && (actual->kind != TypeKind::Fixed || actual->digits == formal->digits)
            && actual->hasRealRange == formal->hasRealRange
            && actual->lowReal == formal->lowReal && actual->highReal == formal->highReal;
    }
    if (actual->kind == TypeKind::Array) {
        if (actual->constrained != formal->constrained || actual->boundsSymbol != formal->boundsSymbol) {
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
    auto constraint = [](Type* type) {
        while (type != nullptr) {
            if (type->discriminantConstraintSource != nullptr) { type = type->discriminantConstraintSource; }
            else if (type->discriminantsKnown || type->discriminantSymbol != nullptr) { return type; }
            else { type = type->isSubtype ? type->base : nullptr; }
        }
        return static_cast<Type*>(nullptr);
    };
    Type* actualConstraint = constraint(actual);
    Type* formalConstraint = constraint(formal);
    if (actualConstraint == nullptr || formalConstraint == nullptr) { return actualConstraint == formalConstraint; }
    return actualConstraint->discriminantSymbol == formalConstraint->discriminantSymbol
        && actualConstraint->discriminantsKnown == formalConstraint->discriminantsKnown
        && actualConstraint->discriminantValues == formalConstraint->discriminantValues;
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
        if (type->formalFixed) {
            expr->type = type;
            expr->isStatic = false;
            return;
        }
        expr->fixedInvalid = !expr->exactReal.scaled(type->fixedScale, expr->staticValue);
        expr->isStatic = !expr->fixedInvalid;
        expr->type = type;
        return;
    }
    if (isReal(type) && expr->type->kind == TypeKind::UniversalInteger && expr->exactReal.valid) {
        expr->staticReal = expr->exactReal.real(typeSize(type) == 4 ? 24 : 53);
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

// 0 is not an object name, 1 is a constant view, 2 is a variable view.
int objectView(Expr* expr)
{
    if (expr->implicitCall != nullptr) {
        return 0;
    }
    Symbol* symbol = nullptr;
    if (expr->kind == ExprKind::Identifier) {
        symbol = static_cast<IdentifierExpr*>(expr)->symbol;
    } else if (expr->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(expr);
        symbol = selected->symbol;
        if (symbol == nullptr) {
            Type* prefix = baseType(selected->prefix->type);
            bool access = prefix != nullptr && prefix->kind == TypeKind::Access;
            Type* record = access ? baseType(prefix->target) : prefix;
            int view = access ? (prefix->accessConstant ? 1 : 2) : objectView(selected->prefix.get());
            if (view != 0 && selected->fieldIndex >= 0 && record != nullptr
                && record->fields[selected->fieldIndex].isDiscriminant) {
                return 1;
            }
            return view;
        }
    } else if (expr->kind == ExprKind::Call) {
        auto* call = static_cast<CallExpr*>(expr);
        if (call->form == CallForm::Conversion && call->type != nullptr && call->type->tagged) {
            return objectView(call->resolvedArguments.front());
        }
        if (call->form == CallForm::Indexing || call->form == CallForm::Slice) {
            Type* prefix = baseType(call->callee->type);
            return prefix != nullptr && prefix->kind == TypeKind::Access
                ? (prefix->accessConstant ? 1 : 2) : objectView(call->callee.get());
        }
    }
    if (symbol == nullptr) {
        return 0;
    }
    if (symbol->protectedField >= 0 && expr->protectedReceiver != nullptr
        && expr->protectedReceiver->mode == ParameterMode::In) {
        return 1;
    }
    if (symbol->kind == SymbolKind::Object || symbol->kind == SymbolKind::Parameter
        || symbol->kind == SymbolKind::LoopParameter) {
        return symbol->isConstant || symbol->kind == SymbolKind::LoopParameter
            || (symbol->kind == SymbolKind::Parameter && symbol->mode == ParameterMode::In) ? 1 : 2;
    }
    return 0;
}

}
