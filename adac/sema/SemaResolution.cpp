#include "Sema.h"
#include "SemaSupport.h"

#include <algorithm>

using SemaSupport::isUniversal;

// RM 4.1.3: selected operations belong to the type's declaration region,
// including class-wide operations in an ancestor's region. A visible component
// with the same name takes precedence, even when it is itself callable.
std::vector<Symbol*> Sema::prefixedOperations(SelectedExpr* expr, Scope* scope)
{
    if (expr->isDereference || expr->prefix == nullptr) { return {}; }
    for (Symbol* name : expressionNames(expr->prefix.get(), scope)) {
        if (name->kind == SymbolKind::Package || name->kind == SymbolKind::TypeName) { return {}; }
    }
    std::vector<Symbol*> result;
    for (Type* prefix : expressionTypes(expr->prefix.get(), scope)) {
        Type* type = completeView(designatedViewType(prefix), scope);
        if (type != nullptr && type->limitedView) { continue; }
        type = rootType(type != nullptr && type->classRoot != nullptr ? type->classRoot : type);
        if (type == nullptr || !taggedViewVisible(type)) { continue; }
        if (representationVisible(type) && std::any_of(type->fields.begin(), type->fields.end(), [&](const FieldInfo& field) {
                return field.name == expr->selectorLower;
            })) { continue; }
        for (Type* ancestor = type; ancestor != nullptr; ancestor = ancestor->parentType) {
            if (ancestor->declarationScope == nullptr) { continue; }
            for (Symbol* operation : ancestor->declarationScope->lookupVisibleLocal(expr->selectorLower, scope)) {
                if (prefixedPrefixType(expr, operation, scope) != nullptr
                    && std::find(result.begin(), result.end(), operation) == result.end()) {
                    result.push_back(operation);
                }
            }
        }
    }
    return result;
}

Type* Sema::prefixedPrefixType(SelectedExpr* expr, Symbol* operation, Scope* scope, bool* ambiguous)
{
    if (expr->isDereference || operation->kind != SymbolKind::Subprogram || operation->parameters.empty()) { return nullptr; }
    Type* formal = operation->parameters.front()->type;
    if (formal->kind == TypeKind::Access) {
        if (!formal->anonymousAccess || !formal->accessParameter) { return nullptr; }
        formal = formal->target;
    }
    if (!formal->tagged) { return nullptr; }
    Type* chosen = nullptr;
    for (Type* prefix : expressionTypes(expr->prefix.get(), scope)) {
        Type* actual = completeView(designatedViewType(prefix), scope);
        if (actual != nullptr && actual->limitedView) { continue; }
        if (actual == nullptr || !taggedViewVisible(actual)) { continue; }
        Type* specific = actual->classRoot != nullptr ? actual->classRoot : rootType(actual);
        if (representationVisible(specific) && std::any_of(specific->fields.begin(), specific->fields.end(), [&](const FieldInfo& field) {
                return field.name == expr->selectorLower;
            })) { continue; }
        bool regionMatches = false;
        for (Type* ancestor = specific; ancestor != nullptr; ancestor = ancestor->parentType) {
            regionMatches = regionMatches || ancestor->declarationScope == operation->declarationScope;
            if (!ancestor->parentVisible && !representationVisible(ancestor)) { break; }
        }
        if (!regionMatches) { continue; }
        if ((formal->classRoot != nullptr && isTaggedAncestor(formal->classRoot, specific))
            || (formal->classRoot == nullptr && rootType(formal) == specific)) {
            if (chosen != nullptr && chosen != prefix && ambiguous != nullptr) { *ambiguous = true; }
            chosen = prefix;
        }
    }
    return chosen;
}

// This pass discovers possible types, without choosing symbols, adapting
// literals, folding expressions, or emitting diagnostics. Legality checks and
// AST annotation belong to the subsequent analysis of the selected context.
std::vector<Symbol*> Sema::expressionNames(Expr* expr, Scope* scope)
{
    if (expr == nullptr) {
        return {};
    }
    if (expr->kind == ExprKind::Identifier) {
        const std::string& lower = static_cast<IdentifierExpr*>(expr)->lower;
        // A Base conversion synthesizes its type mark in analyzeCall; its
        // prefix is preserved through resolveTypeName instead of name lookup.
        if (lower.ends_with("'base") || lower.ends_with("'class")) {
            return {};
        }
        return contractNames(expr, scope->lookup(lower));
    }
    if (expr->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(expr);
        auto prefixes = expressionNames(selected->prefix.get(), scope);
        if (prefixes.size() != 1 && std::any_of(prefixes.begin(), prefixes.end(), [](Symbol* prefix) {
                return prefix->kind == SymbolKind::Package;
            })) {
            return {};
        }
        for (Symbol* prefix : prefixes) {
            if (prefix->kind == SymbolKind::Package && prefix->scope != nullptr) {
                return contractNames(expr, prefix->scope->lookupVisibleLocal(selected->selectorLower, scope));
            }
        }
        std::vector<Symbol*> operations = prefixedOperations(selected, scope);
        for (Type* prefix : expressionTypes(selected->prefix.get(), scope)) {
            Type* type = baseType(prefix);
            if (type != nullptr && type->kind == TypeKind::Access) {
                type = baseType(type->target);
            }
            if (type != nullptr && type->protectedScope != nullptr && representationVisible(type)
                && !selected->isDereference) {
                for (Symbol* operation : type->protectedScope->lookupLocal(selected->selectorLower)) {
                    bool current = isProtectedCurrentInstance(selected->prefix.get(), type, scope);
                    if ((operation->kind == SymbolKind::Subprogram && !operation->protectedPrivate)
                        || current) {
                        operations.push_back(operation);
                    }
                }
            }
        }
        return contractNames(expr, std::move(operations));
    }
    return {};
}

bool Sema::matchesExpression(Expr* expr, Scope* scope, Type* expected)
{
    return !expressionTypes(expr, scope, expected).empty();
}

bool Sema::matchCallArguments(CallExpr* expr, Symbol* candidate, Scope* scope,
                              std::vector<std::size_t>& positions, SelectedExpr* prefix)
{
    positions.clear();
    if (prefix == nullptr && expr->prefixedSubprogram == nullptr && expr->callee != nullptr
        && expr->callee->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(expr->callee.get());
        if (prefixedPrefixType(selected, candidate, scope) != nullptr) { prefix = selected; }
    }
    std::size_t offset = prefix != nullptr ? 1 : 0;
    if (expr->arguments.size() + offset > candidate->parameters.size()) {
        return false;
    }
    std::vector<bool> filled(candidate->parameters.size(), false);
    if (offset != 0) { filled[0] = true; }
    bool sawNamed = false;
    for (std::size_t i = 0; i < expr->arguments.size(); ++i) {
        const Association& argument = expr->arguments[i];
        std::size_t index = i + offset;
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
            Type* controlling = candidate->controllingType;
            bool dispatch = false;
            if (controlling != nullptr && controllingParameterType(candidate->parameters[positions[i]]->type) == controlling) {
                for (Type* actual : expressionTypes(expr->arguments[i].value.get(), scope)) {
                    dispatch = dispatch || designatedViewType(actual)->classRoot == controlling;
                }
            }
            if (!dispatch) {
                return false;
            }
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
    if (expr->implicitCall != nullptr) {
        return expressionTypes(expr->implicitCall.get(), scope, expected);
    }
    switch (expr->kind) {
    case ExprKind::IntegerLiteral:
        add(m_types.universalInteger());
        break;
    case ExprKind::RealLiteral:
        add(m_types.universalReal());
        break;
    case ExprKind::CharacterLiteral: {
        add(m_types.isCharacter(expected) && representationVisible(expected) ? expected : m_types.characterType());
        std::string name = "'" + std::string(1, static_cast<CharacterLiteralExpr*>(expr)->value) + "'";
        for (Symbol* literal : scope->lookup(name)) {
            if (literal->kind == SymbolKind::EnumerationLiteral && representationVisible(literal->type)) {
                add(literal->type);
            }
        }
        break;
    }
    case ExprKind::StringLiteral:
        add(m_types.isString(expected) && representationVisible(expected)
            && representationVisible(expected->element) ? expected : m_types.stringType());
        break;
    case ExprKind::Box:
        break;
    case ExprKind::Null:
        if (expected != nullptr && expected->kind == TypeKind::Access) {
            add(expected);
        }
        break;
    case ExprKind::Aggregate: {
        auto* aggregate = static_cast<AggregateExpr*>(expr);
        if (expected != nullptr && (expected->kind == TypeKind::Array || expected->kind == TypeKind::Record)) {
            if (aggregate->ancestor != nullptr) {
                for (Type* ancestor : expressionTypes(aggregate->ancestor.get(), scope)) {
                    if (isTaggedAncestor(ancestor, expected) && rootType(ancestor) != rootType(expected)) {
                        add(expected);
                    }
                }
            } else if (!expected->tagged || rootType(expected)->parentType == nullptr) {
                add(expected);
            }
        }
        break;
    }
    case ExprKind::Allocator: {
        auto* allocator = static_cast<AllocatorExpr*>(expr);
        Symbol* designated = lookupName(allocator->subtype->lower, scope);
        if (expected != nullptr && expected->kind == TypeKind::Access && designated != nullptr
            && designated->kind == SymbolKind::TypeName
            && typesCompatible(expected->target, designated->type)) {
            add(expected);
        }
        break;
    }
    case ExprKind::Identifier:
    case ExprKind::Selected: {
        std::vector<Symbol*> names = expressionNames(expr, scope);
        for (Symbol* symbol : names) {
            if (symbol->kind == SymbolKind::Subprogram) {
                bool prefixed = expr->kind == ExprKind::Selected
                    && prefixedPrefixType(static_cast<SelectedExpr*>(expr), symbol, scope) != nullptr;
                CallExpr call;
                std::vector<std::size_t> positions;
                if (prefixed && symbol->returnType != nullptr
                    && matchCallArguments(&call, symbol, scope, positions, static_cast<SelectedExpr*>(expr))) {
                    Type* resultType = symbol->returnType;
                    Type* prefix = designatedViewType(prefixedPrefixType(static_cast<SelectedExpr*>(expr), symbol, scope));
                    if (symbol->controllingType != nullptr && prefix->classRoot == symbol->controllingType
                        && rootType(resultType) == symbol->controllingType) {
                        resultType = classWideType(symbol->controllingType);
                    }
                    add(resultType);
                }
                if (!prefixed && symbol->returnType != nullptr
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
                    if (record->accessProfile != nullptr && selected->isDereference) {
                        if (record->accessProfile->parameters.empty()) {
                            add(record->accessProfile->returnType);
                        }
                        continue;
                    }
                    record = baseType(completeView(record->target, scope));
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
        bool standardOperator = false;
        if (!explicitOperatorName(call, scope, standardOperator).empty()) {
            for (const OperatorCandidate& candidate : explicitOperatorCandidates(call, scope, expected)) {
                add(candidate.result);
            }
            break;
        }
        std::vector<Symbol*> names = call->prefixedSubprogram != nullptr
            ? std::vector<Symbol*> {call->prefixedSubprogram} : expressionNames(call->callee.get(), scope);
        bool callable = false;
        if (call->callee->kind == ExprKind::Identifier) {
            const std::string& name = static_cast<IdentifierExpr*>(call->callee.get())->lower;
            if (name.ends_with("'class") && call->arguments.size() == 1) {
                Symbol* symbol = lookupName(name.substr(0, name.size() - 6), scope);
                if (symbol != nullptr && symbol->kind == SymbolKind::TypeName && symbol->type->tagged) {
                    add(resolveTypeName(name, scope, call->location));
                    callable = true;
                }
            }
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
                    Type* resultType = symbol->returnType;
                    if (symbol->controllingType != nullptr && rootType(resultType) == symbol->controllingType) {
                        if (call->prefixedSubprogram == nullptr && call->callee->kind == ExprKind::Selected) {
                            Type* prefix = prefixedPrefixType(static_cast<SelectedExpr*>(call->callee.get()), symbol, scope);
                            if (prefix != nullptr && designatedViewType(prefix)->classRoot == symbol->controllingType) {
                                resultType = classWideType(symbol->controllingType);
                            }
                        }
                        for (std::size_t i = 0; i < positions.size(); ++i) {
                            if (controllingParameterType(symbol->parameters[positions[i]]->type) == symbol->controllingType) {
                                for (Type* actual : expressionTypes(call->arguments[i].value.get(), scope)) {
                                    if (designatedViewType(actual)->classRoot == symbol->controllingType) {
                                        resultType = classWideType(symbol->controllingType);
                                    }
                                }
                            }
                        }
                    }
                    add(resultType);
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
            Expr* callee = call->callee.get();
            if (callee->kind == ExprKind::Selected) {
                auto* selected = static_cast<SelectedExpr*>(callee);
                if (selected->isDereference) {
                    for (Type* type : expressionTypes(selected->prefix.get(), scope)) {
                        if (type->accessProfile != nullptr) {
                            callee = selected->prefix.get();
                            break;
                        }
                    }
                }
            }
            for (Type* prefix : expressionTypes(callee, scope)) {
                if (prefix->accessProfile != nullptr) {
                    std::vector<std::size_t> positions;
                    if (matchCallArguments(call, prefix->accessProfile, scope, positions)) {
                        add(prefix->accessProfile->returnType);
                    }
                    continue;
                }
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
                    { unary->operand.get() }, scope, expected, unary->predefinedOnly)) {
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
            for (Type* type : expressionTypes(binary->left.get(), scope, expected)) {
                if (m_types.isBoolean(type) && representationVisible(type)
                    && matchesExpression(binary->right.get(), scope, type)) {
                    add(type);
                }
            }
        } else {
            for (const OperatorCandidate& candidate : operatorCandidates(operatorName(binary->op),
                    { binary->left.get(), binary->right.get() }, scope, expected, binary->standardFixed || binary->predefinedOnly)) {
                add(candidate.result);
            }
        }
        break;
    }
    case ExprKind::Qualified: {
        auto* qualified = static_cast<QualifiedExpr*>(expr);
        if (qualified->typeLower.ends_with("'class")) {
            add(resolveTypeName(qualified->typeLower, scope, expr->location));
            break;
        }
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
        if (name == "access") {
            if (attribute->prefixedAccessType != nullptr) {
                add(attribute->prefixedAccessType);
                break;
            }
            if (expected != nullptr && expected->kind == TypeKind::Access && expected->accessProfile == nullptr) {
                for (Type* prefix : expressionTypes(attribute->prefix.get(), scope)) {
                    if (typesCompatible(expected->target, prefix)
                        || (expected->anonymousAccess && prefix->classRoot == rootType(expected->target))) {
                        add(expected);
                    }
                }
            } else if (expected != nullptr) {
                for (Symbol* candidate : expressionNames(attribute->prefix.get(), scope)) {
                    if (matchesAccessProfile(candidate, expected)) {
                        add(expected);
                    }
                }
            }
        } else if (name == "small" || name == "delta") {
            add(m_types.universalReal());
        } else if (name == "image") {
            add(m_types.stringType());
        } else if (name == "address") {
            add(m_addressType);
        } else if (name == "tag") {
            add(m_tagType);
        } else if (name == "external_tag") {
            add(m_types.stringType());
        } else if (name == "class") {
            for (Type* prefix : expressionTypes(attribute->prefix.get(), scope)) {
                if (taggedViewVisible(prefix) && prefix->classRoot == nullptr) {
                    add(classWideType(prefix));
                }
            }
        } else if (name == "identity") {
            bool task = false;
            for (Type* prefix : expressionTypes(attribute->prefix.get(), scope)) {
                if (prefix->kind == TypeKind::Access) {
                    prefix = prefix->target;
                }
                prefix = baseType(prefix);
                task |= prefix != nullptr && prefix->taskBody != nullptr;
            }
            add(task ? m_taskIdType : m_exceptionIdType);
        } else if (name == "caller") {
            add(m_taskIdType);
        } else if (name == "constrained" || name == "callable" || name == "terminated" || name == "machine_overflows" || name == "machine_rounds") {
            add(m_types.booleanType());
        } else if (name == "modulus" || name == "pos" || name == "length" || name == "size"
            || name == "alignment" || name == "position" || name == "first_bit" || name == "last_bit") {
            add(m_types.universalInteger());
        } else if (name == "scale" || name == "width" || name == "digits" || name == "fore" || name == "aft" || name == "machine_radix") {
            add(m_types.integerType());
        } else {
            for (Type* prefix : expressionTypes(attribute->prefix.get(), scope)) {
                if ((name == "first" || name == "last") && prefix->kind == TypeKind::Access
                    && prefix->target != nullptr && prefix->target->kind == TypeKind::Array) {
                    prefix = prefix->target;
                }
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
                } else if (name == "min" || name == "max" || name == "mod" || name == "round" || name == "base" || name == "val" || name == "succ" || name == "pred") {
                    add(m_types.scalarBaseType(prefix));
                } else if (name == "first" || name == "last" || name == "value" || name == "input") {
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
