#include "Sema.h"
#include "SemaSupport.h"

#include <algorithm>

using SemaSupport::adaptUniversal;
using SemaSupport::isUniversal;

std::vector<Sema::OperatorCandidate> Sema::operatorCandidates(const std::string& name,
    const std::vector<Expr*>& operands, Scope* scope, Type* expected)
{
    std::vector<OperatorCandidate> candidates;
    auto symbols = scope->lookup(name);
    bool fixed = false;
    Symbol* fixedSymbol = nullptr;
    if (m_replayContract != nullptr) {
        auto found = m_replayContract->m_operators.find(operatorContractKey(name, operands));
        if (found != m_replayContract->m_operators.end()) {
            fixed = true;
            fixedSymbol = instanceSymbol(found->second);
            if (found->second != nullptr && fixedSymbol == nullptr) {
                m_diagnostics.error(operands.front()->location, "cannot map a resolved generic operator to its instance");
                return {};
            }
            symbols = fixedSymbol == nullptr ? std::vector<Symbol*> {} : std::vector<Symbol*> { fixedSymbol };
        }
    }
    for (Symbol* symbol : symbols) {
        if (symbol->kind != SymbolKind::Subprogram || symbol->parameters.size() != operands.size()
            || symbol->returnType == nullptr || !matchesResult(symbol, expected)) {
            continue;
        }
        OperatorCandidate candidate;
        candidate.symbol = symbol;
        candidate.result = symbol->returnType;
        bool matches = true;
        for (std::size_t i = 0; i < operands.size(); ++i) {
            Symbol* formal = symbol->parameters[i];
            matches = matches && formal->mode == ParameterMode::In && !formal->hasDefault
                && matchesExpression(operands[i], scope, formal->type);
            candidate.parameters.push_back(formal->type);
        }
        if (matches) {
            candidates.push_back(std::move(candidate));
        }
    }

    if (fixed && fixedSymbol != nullptr) {
        return candidates;
    }

    auto sameProfile = [&](const OperatorCandidate& a, const OperatorCandidate& b) {
        if (rootType(a.result) != rootType(b.result) || a.parameters.size() != b.parameters.size()) {
            return false;
        }
        for (std::size_t i = 0; i < a.parameters.size(); ++i) {
            if (rootType(a.parameters[i]) != rootType(b.parameters[i])) {
                return false;
            }
        }
        return true;
    };
    auto predefined = [&](std::vector<Type*> parameters, Type* result) {
        if (expected != nullptr && !typesCompatible(expected, result)) {
            return;
        }
        for (std::size_t i = 0; i < parameters.size(); ++i) {
            if (!matchesExpression(operands[i], scope, parameters[i])) {
                return;
            }
            // A root numeric operator applies to universal operands, not to
            // arbitrary concrete types that happen to accept numeric literals.
            if (isUniversal(parameters[i])) {
                auto types = expressionTypes(operands[i], scope);
                if (std::none_of(types.begin(), types.end(), [&](Type* type) {
                    return isUniversal(type) && typesCompatible(type, parameters[i]);
                })) {
                    return;
                }
            }
        }
        OperatorCandidate candidate { nullptr, std::move(parameters), result };
        for (const OperatorCandidate& existing : candidates) {
            // An explicit homograph replaces its predefined operation. A
            // different profile remains a competing interpretation.
            if (sameProfile(existing, candidate)) {
                return;
            }
        }
        candidates.push_back(std::move(candidate));
    };

    std::vector<Type*> types;
    auto addType = [&](Type* type) {
        if (type == nullptr) {
            return;
        }
        type = baseType(type);
        if (isUniversal(type) && expected != nullptr && isNumeric(baseType(expected))
            && typesCompatible(expected, type)) {
            type = baseType(expected);
        }
        if (std::find(types.begin(), types.end(), type) == types.end()) {
            types.push_back(type);
        }
    };
    addType(expected);
    for (Expr* operand : operands) {
        for (Type* type : expressionTypes(operand, scope)) {
            addType(type);
        }
    }
    if (operands.size() == 1) {
        if (name == "not") {
            predefined({ m_types.booleanType() }, m_types.booleanType());
            for (Type* type : types) {
                if (type->m_modulus != 0 && representationVisible(type)) {
                    predefined({ type }, type);
                }
            }
        } else if (name == "+" || name == "-" || name == "abs") {
            for (Type* type : types) {
                if (isNumeric(type) && representationVisible(type) && !(name == "abs" && type->m_modulus != 0)) {
                    predefined({ type }, type);
                }
            }
        }
    } else if (operands.size() == 2) {
        bool equality = name == "=" || name == "/=";
        bool ordering = name == "<" || name == "<=" || name == ">" || name == ">=";
        if (name == "and" || name == "or" || name == "xor") {
            predefined({ m_types.booleanType(), m_types.booleanType() }, m_types.booleanType());
            for (Type* type : types) {
                if (type->m_modulus != 0 && representationVisible(type)) {
                    predefined({ type, type }, type);
                }
            }
        } else if (name == "&") {
            addType(m_types.stringType());
            for (Type* type : types) {
                if (m_types.isString(type) && representationVisible(type)) {
                    Type* character = type->element;
                    predefined({ type, type }, type);
                    predefined({ type, character }, type);
                    predefined({ character, type }, type);
                    predefined({ character, character }, type);
                }
            }
        } else {
            for (Type* type : types) {
                if (equality || ordering) {
                    bool allowed = isNumeric(type) || isDiscrete(type) || type->kind == TypeKind::Array
                        || (equality && (type->kind == TypeKind::Record || type->kind == TypeKind::Access));
                    if (type->isLimited || (!equality && !representationVisible(type))) {
                        allowed = false;
                    }
                    if (ordering && type->kind == TypeKind::Array && !isDiscrete(type->element)) {
                        allowed = false;
                    }
                    if (allowed) {
                        predefined({ type, type }, m_types.booleanType());
                    }
                } else if (isNumeric(type) && representationVisible(type) && !(name == "abs" && type->m_modulus != 0)) {
                    if ((name == "mod" || name == "rem") && isReal(type)) {
                        continue;
                    }
                    if (name == "+" || name == "-" || name == "*" || name == "/" || name == "mod"
                        || name == "rem" || name == "**") {
                        predefined({ type, name == "**" ? m_types.integerType() : type }, type);
                    }
                }
            }
        }
    }
    // Preserve Ada's preference for root numeric operations in universal
    // contexts (for example a named number or a comparison of literals).
    auto rootNumeric = [](const OperatorCandidate& candidate) {
        return candidate.symbol == nullptr && std::any_of(candidate.parameters.begin(), candidate.parameters.end(),
            [](Type* type) { return isUniversal(type); });
    };
    if (std::any_of(candidates.begin(), candidates.end(), rootNumeric)) {
        std::erase_if(candidates, [&](const OperatorCandidate& candidate) { return !rootNumeric(candidate); });
    }
    return candidates;
}

ExprPtr Sema::bindOperator(Symbol* symbol, std::vector<ExprPtr> operands, Scope* scope,
                           const SourceLocation& location)
{
    auto call = std::make_unique<CallExpr>();
    call->location = location;
    call->form = CallForm::Subprogram;
    call->subprogram = symbol;
    call->type = symbol->returnType;
    auto callee = std::make_unique<IdentifierExpr>();
    callee->location = location;
    callee->name = symbol->displayName;
    callee->lower = symbol->name;
    callee->symbol = symbol;
    call->callee = std::move(callee);
    for (std::size_t i = 0; i < operands.size(); ++i) {
        analyzeExpr(operands[i].get(), scope, symbol->parameters[i]->type);
        adaptUniversal(operands[i].get(), symbol->parameters[i]->type);
        call->resolvedArguments.push_back(operands[i].get());
        Association argument;
        argument.value = std::move(operands[i]);
        call->arguments.push_back(std::move(argument));
    }
    return call;
}

ExprPtr Sema::explicitOperator(CallExpr* call)
{
    if (call->callee->kind != ExprKind::Identifier) {
        return nullptr;
    }
    std::string name = static_cast<IdentifierExpr*>(call->callee.get())->lower;
    if (operatorSymbol(name).empty() || std::any_of(call->arguments.begin(), call->arguments.end(),
            [](const Association& argument) { return !argument.nameLower.empty() || argument.high != nullptr; })) {
        return nullptr;
    }
    if (call->arguments.size() == 1) {
        for (UnaryOp op : { UnaryOp::Plus, UnaryOp::Negate, UnaryOp::Not, UnaryOp::Abs }) {
            if (name == operatorName(op)) {
                auto unary = std::make_unique<UnaryExpr>();
                unary->location = call->location;
                unary->op = op;
                unary->operand = std::move(call->arguments[0].value);
                return unary;
            }
        }
    } else if (call->arguments.size() == 2) {
        for (int i = static_cast<int>(BinaryOp::Add); i <= static_cast<int>(BinaryOp::GreaterEqual); ++i) {
            auto op = static_cast<BinaryOp>(i);
            if (name == operatorName(op)) {
                auto binary = std::make_unique<BinaryExpr>();
                binary->location = call->location;
                binary->op = op;
                binary->left = std::move(call->arguments[0].value);
                binary->right = std::move(call->arguments[1].value);
                return binary;
            }
        }
    }
    return nullptr;
}
