#include "Sema.h"
#include "SemaSupport.h"

#include <algorithm>

using SemaSupport::isUniversal;

// This pass discovers possible types, without choosing symbols, adapting
// literals, folding expressions, or emitting diagnostics. Legality checks and
// AST annotation belong to the subsequent analysis of the selected context.
std::vector<Symbol*> Sema::expressionNames(Expr* expr, Scope* scope)
{
    if (expr->kind == ExprKind::Identifier) {
        return scope->lookup(static_cast<IdentifierExpr*>(expr)->lower);
    }
    if (expr->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(expr);
        for (Symbol* prefix : expressionNames(selected->prefix.get(), scope)) {
            if (prefix->kind == SymbolKind::Package && prefix->scope != nullptr) {
                return prefix->scope->lookupLocal(selected->selectorLower);
            }
        }
    }
    return {};
}

bool Sema::matchesExpression(Expr* expr, Scope* scope, Type* expected)
{
    return !expressionTypes(expr, scope, expected).empty();
}

bool Sema::matchCallArguments(CallExpr* expr, Symbol* candidate, Scope* scope,
                              std::vector<std::size_t>& positions)
{
    positions.clear();
    if (expr->arguments.size() > candidate->parameters.size()) {
        return false;
    }
    std::vector<bool> filled(candidate->parameters.size(), false);
    bool sawNamed = false;
    for (std::size_t i = 0; i < expr->arguments.size(); ++i) {
        const Association& argument = expr->arguments[i];
        std::size_t index = i;
        if (!argument.nameLower.empty()) {
            sawNamed = true;
            index = candidate->parameters.size();
            for (std::size_t p = 0; p < candidate->parameters.size(); ++p) {
                if (candidate->parameters[p]->name == argument.nameLower) {
                    index = p;
                    break;
                }
            }
        } else if (sawNamed) {
            return false;
        }
        if (index == candidate->parameters.size() || filled[index] || argument.high != nullptr) {
            return false;
        }
        filled[index] = true;
        positions.push_back(index);
    }
    for (std::size_t p = 0; p < filled.size(); ++p) {
        if (!filled[p] && !candidate->parameters[p]->hasDefault) {
            return false;
        }
    }
    for (std::size_t i = 0; i < positions.size(); ++i) {
        if (!matchesExpression(expr->arguments[i].value.get(), scope, candidate->parameters[positions[i]]->type)) {
            return false;
        }
    }
    return true;
}

std::vector<Type*> Sema::expressionTypes(Expr* expr, Scope* scope, Type* expected)
{
    if (m_resolutionDepth == 0) {
        m_resolutionTypes.clear();
    }
    auto expression = m_resolutionTypes.find(expr);
    if (expression != m_resolutionTypes.end()) {
        auto context = expression->second.find(expected);
        if (context != expression->second.end()) {
            return context->second;
        }
    }
    ++m_resolutionDepth;
    auto result = discoverExpressionTypes(expr, scope, expected);
    --m_resolutionDepth;
    m_resolutionTypes[expr][expected] = result;
    return result;
}

std::vector<Type*> Sema::discoverExpressionTypes(Expr* expr, Scope* scope, Type* expected)
{
    std::vector<Type*> result;
    auto add = [&](Type* type) {
        if (type != nullptr && (expected == nullptr || typesCompatible(expected, type))
            && std::find(result.begin(), result.end(), type) == result.end()) {
            result.push_back(type);
        }
    };
    if (expr == nullptr) {
        return result;
    }
    switch (expr->kind) {
    case ExprKind::IntegerLiteral:
        add(m_types.universalInteger());
        break;
    case ExprKind::RealLiteral:
        add(m_types.universalReal());
        break;
    case ExprKind::CharacterLiteral:
        add(m_types.isCharacter(expected) ? expected : m_types.characterType());
        break;
    case ExprKind::StringLiteral:
        add(m_types.isString(expected) ? expected : m_types.stringType());
        break;
    case ExprKind::Null:
        if (expected != nullptr && expected->kind == TypeKind::Access) {
            add(expected);
        }
        break;
    case ExprKind::Aggregate:
        if (expected != nullptr && (expected->kind == TypeKind::Array || expected->kind == TypeKind::Record)) {
            add(expected);
        }
        break;
    case ExprKind::Allocator: {
        auto* allocator = static_cast<AllocatorExpr*>(expr);
        Symbol* designated = lookupName(allocator->subtype->lower, scope);
        if (expected != nullptr && expected->kind == TypeKind::Access && designated != nullptr
            && designated->kind == SymbolKind::TypeName
            && rootType(expected->target) == rootType(designated->type)) {
            add(expected);
        }
        break;
    }
    case ExprKind::Identifier:
    case ExprKind::Selected: {
        std::vector<Symbol*> names = expressionNames(expr, scope);
        for (Symbol* symbol : names) {
            if (symbol->kind == SymbolKind::Subprogram) {
                if (symbol->returnType != nullptr
                    && std::all_of(symbol->parameters.begin(), symbol->parameters.end(), [](Symbol* parameter) {
                        return parameter->hasDefault;
                    })) {
                    add(symbol->returnType);
                }
            } else {
                add(symbol->type);
            }
        }
        if (names.empty() && expr->kind == ExprKind::Selected) {
            auto* selected = static_cast<SelectedExpr*>(expr);
            for (Type* prefix : expressionTypes(selected->prefix.get(), scope)) {
                Type* record = baseType(prefix);
                if (record->kind == TypeKind::Access) {
                    record = baseType(record->target);
                    if (selected->isDereference) {
                        add(record);
                        continue;
                    }
                }
                if (record != nullptr && record->kind == TypeKind::Record && !selected->isDereference) {
                    for (const FieldInfo& field : record->fields) {
                        if (field.name == selected->selectorLower) {
                            add(field.type);
                        }
                    }
                }
            }
        }
        break;
    }
    case ExprKind::Call: {
        auto* call = static_cast<CallExpr*>(expr);
        if (call->operatorExpression != nullptr) {
            return expressionTypes(call->operatorExpression.get(), scope, expected);
        }
        if (call->callee->kind == ExprKind::Identifier) {
            std::string name = static_cast<IdentifierExpr*>(call->callee.get())->lower;
            bool positional = std::all_of(call->arguments.begin(), call->arguments.end(),
                [](const Association& argument) { return argument.nameLower.empty() && argument.high == nullptr; });
            if (!operatorSymbol(name).empty() && positional) {
                std::vector<Expr*> operands;
                for (const Association& argument : call->arguments) {
                    operands.push_back(argument.value.get());
                }
                for (const OperatorCandidate& candidate : operatorCandidates(name, operands, scope, expected)) {
                    add(candidate.result);
                }
                break;
            }
        }
        std::vector<Symbol*> names = expressionNames(call->callee.get(), scope);
        bool callable = false;
        if (call->callee->kind == ExprKind::Identifier) {
            const std::string& name = static_cast<IdentifierExpr*>(call->callee.get())->lower;
            if (name.ends_with("'base") && call->arguments.size() == 1) {
                Symbol* symbol = lookupName(name.substr(0, name.size() - 5), scope);
                if (symbol != nullptr && symbol->kind == SymbolKind::TypeName) {
                    add(m_types.scalarBaseType(symbol->type));
                    callable = true;
                }
            }
        }
        for (Symbol* symbol : names) {
            if (symbol->kind == SymbolKind::Subprogram) {
                callable = true;
                std::vector<std::size_t> positions;
                if (symbol->returnType != nullptr && matchesResult(symbol, expected)
                    && matchCallArguments(call, symbol, scope, positions)) {
                    add(symbol->returnType);
                }
            } else if (symbol->kind == SymbolKind::TypeName) {
                callable = true;
                if (call->arguments.size() == 1) {
                    // Conversion legality is checked after selecting the context.
                    add(symbol->type);
                }
            }
        }
        if (!callable) {
            for (Type* prefix : expressionTypes(call->callee.get(), scope)) {
                Type* array = prefix;
                if (baseType(array)->kind == TypeKind::Access) {
                    array = baseType(array)->target;
                }
                if (array == nullptr || array->kind != TypeKind::Array) {
                    continue;
                }
                if (call->arguments.size() == 1 && call->arguments.front().high != nullptr) {
                    add(array);
                    continue;
                }
                if (call->arguments.size() != static_cast<std::size_t>(array->arrayRank)) {
                    continue;
                }
                bool matches = true;
                for (const Association& argument : call->arguments) {
                    matches = matches && matchesExpression(argument.value.get(), scope, array->index);
                    array = array->element;
                }
                if (matches) {
                    add(array);
                }
            }
        }
        break;
    }
    case ExprKind::Unary: {
        auto* unary = static_cast<UnaryExpr*>(expr);
        if (unary->operatorCall != nullptr) {
            add(expr->type);
        } else {
            for (const OperatorCandidate& candidate : operatorCandidates(operatorName(unary->op),
                    { unary->operand.get() }, scope, expected)) {
                add(candidate.result);
            }
        }
        break;
    }
    case ExprKind::Binary: {
        auto* binary = static_cast<BinaryExpr*>(expr);
        if (binary->operatorCall != nullptr) {
            add(expr->type);
        } else if (binary->op == BinaryOp::AndThen || binary->op == BinaryOp::OrElse) {
            if (matchesExpression(binary->left.get(), scope, m_types.booleanType())
                && matchesExpression(binary->right.get(), scope, m_types.booleanType())) {
                add(m_types.booleanType());
            }
        } else {
            for (const OperatorCandidate& candidate : operatorCandidates(operatorName(binary->op),
                    { binary->left.get(), binary->right.get() }, scope, expected)) {
                add(candidate.result);
            }
        }
        break;
    }
    case ExprKind::Qualified: {
        auto* qualified = static_cast<QualifiedExpr*>(expr);
        Symbol* symbol = lookupName(qualified->typeLower, scope);
        if (symbol != nullptr && symbol->kind == SymbolKind::TypeName) {
            add(symbol->type);
        }
        break;
    }
    case ExprKind::Membership:
        add(m_types.booleanType());
        break;
    case ExprKind::Attribute: {
        auto* attribute = static_cast<AttributeExpr*>(expr);
        const std::string& name = attribute->lower;
        if (name == "image") {
            add(m_types.stringType());
        } else if (name == "address") {
            add(m_addressType);
        } else if (name == "identity") {
            add(m_exceptionIdType);
        } else if (name == "pos" || name == "length" || name == "size" || name == "width" || name == "digits" || name == "modulus") {
            add(m_types.integerType());
        } else {
            for (Type* prefix : expressionTypes(attribute->prefix.get(), scope)) {
                if ((name == "first" || name == "last") && prefix->kind == TypeKind::Array) {
                    // The dimension expression is static; literal dimensions
                    // can be inspected before the normal attribute analysis.
                    long long dimension = 1;
                    if (!attribute->arguments.empty()) {
                        Expr* argument = attribute->arguments.front().get();
                        if (argument->kind == ExprKind::IntegerLiteral) {
                            dimension = static_cast<IntegerLiteralExpr*>(argument)->value;
                        } else if (!foldStatic(argument, dimension)) {
                            // Retain all index types until the dimension is analyzed.
                            int rank = prefix->arrayRank;
                            for (int i = 0; i < rank; ++i) {
                                add(prefix->index);
                                prefix = prefix->element;
                            }
                            continue;
                        }
                    }
                    for (long long i = 1; i < dimension && prefix->kind == TypeKind::Array; ++i) {
                        prefix = prefix->element;
                    }
                    if (prefix->kind == TypeKind::Array) {
                        add(prefix->index);
                    }
                } else if (name == "base") {
                    add(m_types.scalarBaseType(prefix));
                } else if (name == "first" || name == "last" || name == "val" || name == "succ"
                           || name == "pred" || name == "value" || name == "input") {
                    add(prefix);
                }
            }
        }
        break;
    }
    }
    return result;
}

Type* Sema::commonOperandType(Expr* left, Expr* right, Scope* scope, Type* expected)
{
    if (expected != nullptr && isNumeric(baseType(expected))) {
        return expected;
    }
    std::vector<Type*> types = expressionTypes(left, scope);
    auto rightTypes = expressionTypes(right, scope);
    types.insert(types.end(), rightTypes.begin(), rightTypes.end());
    Type* chosen = nullptr;
    for (Type* type : types) {
        if (isUniversal(type) || !matchesExpression(left, scope, type) || !matchesExpression(right, scope, type)) {
            continue;
        }
        if (chosen != nullptr && rootType(chosen) != rootType(type)) {
            return nullptr;
        }
        chosen = type;
    }
    return chosen;
}
