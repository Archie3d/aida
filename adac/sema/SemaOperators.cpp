#include "Sema.h"
#include "SemaSupport.h"

#include <algorithm>

using SemaSupport::adaptUniversal;
using SemaSupport::isUniversal;

std::vector<Sema::OperatorCandidate> Sema::operatorCandidates(const std::string& name,
    const std::vector<Expr*>& operands, Scope* scope, Type* expected, bool predefinedOnly,
    const std::vector<std::string>& argumentNames)
{
    std::vector<OperatorCandidate> candidates;
    auto symbols = scope->lookup(name);
    auto matchesNames = [&](const std::vector<std::string>& formals) {
        for (std::size_t i = 0; i < argumentNames.size(); ++i) {
            if (!argumentNames[i].empty() && argumentNames[i] != formals[i]) {
                return false;
            }
        }
        return true;
    };
    std::vector<OperatorCandidate> hiddenProfiles;
    bool fixed = false;
    Symbol* fixedSymbol = nullptr;
    if (m_replayContract != nullptr) {
        auto found = m_replayContract->operators.find(operatorContractKey(name, operands));
        if (found != m_replayContract->operators.end()) {
            fixed = true;
            fixedSymbol = instanceSymbol(found->second);
            if (found->second != nullptr && fixedSymbol == nullptr) {
                m_diagnostics.error(operands.front()->location, "cannot map a resolved generic operator to its instance");
                return {};
            }
            symbols = fixedSymbol == nullptr ? std::vector<Symbol*> {} : std::vector<Symbol*> { fixedSymbol };
        }
    }
    if (predefinedOnly) { symbols.clear(); }
    for (Symbol* symbol : symbols) {
        if (symbol->kind != SymbolKind::Subprogram || symbol->parameters.size() != operands.size()
            || symbol->returnType == nullptr || !matchesResult(symbol, expected)) {
            continue;
        }
        OperatorCandidate candidate;
        candidate.symbol = symbol;
        candidate.result = symbol->returnType;
        bool matches = true;
        bool dispatching = false;
        for (std::size_t i = 0; i < operands.size(); ++i) {
            Symbol* formal = symbol->parameters[i];
            Type* context = formal->type;
            if (symbol->controllingType != nullptr && rootType(formal->type) == symbol->controllingType) {
                for (Type* actual : expressionTypes(operands[i], scope)) {
                    if (actual->classRoot == symbol->controllingType) {
                        context = actual;
                        dispatching = true;
                    }
                }
            }
            matches = matches && formal->mode == ParameterMode::In && !formal->hasDefault
                && matchesExpression(operands[i], scope, context);
            candidate.parameters.push_back(context);
        }
        if (dispatching && (name == "=" || name == "/=") && symbol->parameters.size() == 2
            && rootType(symbol->parameters[0]->type) == symbol->controllingType
            && rootType(symbol->parameters[1]->type) == symbol->controllingType) {
            continue; // Class-wide equality dispatches the complete specific equality.
        }
        if (dispatching && rootType(candidate.result) == symbol->controllingType) {
            candidate.result = classWideType(symbol->controllingType);
        }
        if (matches) {
            // A homograph hides the predefined profile even when its formal
            // names do not match this particular named call.
            hiddenProfiles.push_back(candidate);
            std::vector<std::string> formals;
            for (Symbol* formal : symbol->parameters) {
                formals.push_back(formal->name);
            }
            if (matchesNames(formals)) {
                candidates.push_back(std::move(candidate));
            }
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
            if (!SemaSupport::profileTypeMatches(a.parameters[i], b.parameters[i])) {
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
        for (const OperatorCandidate& existing : hiddenProfiles) {
            // An explicit homograph replaces its predefined operation. A
            // different profile remains a competing interpretation.
            if (sameProfile(existing, candidate)) {
                return;
            }
        }
        if (std::any_of(candidates.begin(), candidates.end(), [&](const OperatorCandidate& existing) {
                return sameProfile(existing, candidate);
            })) {
            return;
        }
        if (matchesNames(candidate.parameters.size() == 1 ? std::vector<std::string> { "right" }
                                                       : std::vector<std::string> { "left", "right" })) {
            candidates.push_back(std::move(candidate));
        }
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
                if ((type->modulus != 0 || m_types.isBoolean(type)) && representationVisible(type)) {
                    predefined({ type }, type);
                }
            }
        } else if (name == "+" || name == "-" || name == "abs") {
            for (Type* type : types) {
                if (isNumeric(type) && representationVisible(type) && !(name == "abs" && type->modulus != 0)) {
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
                if ((type->modulus != 0 || m_types.isBoolean(type)) && representationVisible(type)) {
                    predefined({ type, type }, type);
                }
            }
        } else if (name == "&") {
            addType(m_types.stringType());
            for (Type* type : types) {
                if (type->kind == TypeKind::Array && type->arrayRank == 1 && !type->isLimited
                    && !type->element->isLimited && !hasLimitedControlledParts(type)
                    && !hasTasks(type) && !hasProtected(type) && representationVisible(type)) {
                    Type* component = type->element;
                    predefined({ type, type }, type);
                    predefined({ type, component }, type);
                    predefined({ component, type }, type);
                    predefined({ component, component }, type);
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
                } else if (type->kind == TypeKind::Fixed && (name == "*" || name == "/")) {
                    for (Type* other : types) {
                        if (rootType(other) == m_types.integerType() || other->kind == TypeKind::UniversalInteger) {
                            predefined({ type, m_types.integerType() }, type);
                            if (name == "*") {
                                predefined({ m_types.integerType(), type }, type);
                            }
                        } else if (other->kind == TypeKind::Fixed && expected != nullptr
                                   && isNumeric(expected)) {
                            predefined({ type, other }, expected);
                        }
                    }
                } else if (isNumeric(type) && representationVisible(type) && !(name == "abs" && type->modulus != 0)) {
                    if (((name == "mod" || name == "rem") && (isReal(type) || type->kind == TypeKind::Fixed))
                        || (name == "**" && type->kind == TypeKind::Fixed)) {
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
            [](Type* type) { return isUniversal(type); })
            && std::none_of(candidate.parameters.begin(), candidate.parameters.end(),
                [](Type* type) { return type->kind == TypeKind::Fixed; });
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
        Type* context = symbol->parameters[i]->type;
        if (symbol->controllingType != nullptr && rootType(context) == symbol->controllingType) {
            for (Type* actual : expressionTypes(operands[i].get(), scope)) {
                if (actual->classRoot == symbol->controllingType) {
                    context = actual;
                    call->dispatching = true;
                }
            }
        }
        analyzeExpr(operands[i].get(), scope, context);
        adaptUniversal(operands[i].get(), symbol->parameters[i]->type);
        call->resolvedArguments.push_back(operands[i].get());
        Association argument;
        argument.value = std::move(operands[i]);
        call->arguments.push_back(std::move(argument));
    }
    call->tagIndeterminate = symbol->controllingType != nullptr
        && rootType(call->type) == symbol->controllingType && !call->dispatching;
    for (std::size_t i = 0; i < call->resolvedArguments.size(); ++i) {
        if (rootType(symbol->parameters[i]->type) == symbol->controllingType) {
            call->tagIndeterminate = call->tagIndeterminate && call->resolvedArguments[i]->tagIndeterminate;
        }
    }
    if (call->dispatching) {
        if (rootType(call->type) == symbol->controllingType) {
            call->type = classWideType(symbol->controllingType);
        }
        for (std::size_t i = 0; i < call->resolvedArguments.size(); ++i) {
            if (call->resolvedArguments[i]->tagIndeterminate) {
                dispatchIndeterminate(call->resolvedArguments[i], symbol->controllingType);
            }
            if (rootType(symbol->parameters[i]->type) == symbol->controllingType
                && call->resolvedArguments[i]->type != nullptr && call->resolvedArguments[i]->type->classRoot == nullptr) {
                m_diagnostics.error(location, "all controlling operands of a dispatching call must be class-wide");
            }
        }
    }
    return call;
}

std::string Sema::explicitOperatorName(CallExpr* call, Scope* scope, bool& standard)
{
    std::string name;
    standard = false;
    if (call->callee->kind == ExprKind::Identifier) {
        name = static_cast<IdentifierExpr*>(call->callee.get())->lower;
    } else if (call->callee->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(call->callee.get());
        auto prefixes = expressionNames(selected->prefix.get(), scope);
        standard = prefixes.size() == 1 && prefixes.front()->scope == m_standardScope;
        if (standard && (selected->selectorLower == "*" || selected->selectorLower == "/")) {
            name = selected->selectorLower;
        }
    }
    return operatorSymbol(name);
}

std::vector<Sema::OperatorCandidate> Sema::explicitOperatorCandidates(CallExpr* call, Scope* scope, Type* expected)
{
    bool standard = false;
    std::string name = explicitOperatorName(call, scope, standard);
    std::size_t count = call->arguments.size();
    if (name.empty() || count == 0 || count > 2) {
        return {};
    }
    std::vector<OperatorCandidate> result;
    // At most two operands: try each formal order without mutating the AST.
    // A positional actual is fixed to its source position; named actuals are
    // checked against each user-defined or predefined candidate's own names.
    for (bool reverse : { false, true }) {
        if (reverse && count != 2) {
            continue;
        }
        std::vector<Expr*> operands(count);
        std::vector<std::string> names(count);
        std::vector<std::size_t> order(count);
        bool valid = true;
        bool sawNamed = false;
        for (std::size_t i = 0; i < count; ++i) {
            const Association& argument = call->arguments[i];
            std::size_t position = reverse ? count - i - 1 : i;
            if (argument.high != nullptr || (argument.nameLower.empty() && (sawNamed || position != i))) {
                valid = false;
            }
            sawNamed = sawNamed || !argument.nameLower.empty();
            operands[position] = argument.value.get();
            names[position] = argument.nameLower;
            order[position] = i;
        }
        if (!valid) {
            continue;
        }
        for (OperatorCandidate candidate : operatorCandidates(name, operands, scope, expected, standard, names)) {
            candidate.argumentOrder = order;
            result.push_back(std::move(candidate));
        }
    }
    return result;
}

ExprPtr Sema::explicitOperator(CallExpr* call, Scope* scope, Type* expected)
{
    bool standard = false;
    std::string name = explicitOperatorName(call, scope, standard);
    if (name.empty()) {
        return nullptr;
    }
    bool named = std::any_of(call->arguments.begin(), call->arguments.end(),
        [](const Association& argument) { return !argument.nameLower.empty(); });
    bool predefinedOnly = false;
    ExprPtr boundCall;
    Type* boundResult = nullptr;
    if (named) {
        bool sawNamed = false;
        std::vector<std::string> names;
        for (const Association& argument : call->arguments) {
            if (argument.nameLower.empty() && sawNamed) {
                m_diagnostics.error(argument.value->location, "a positional argument cannot follow a named argument");
                return nullptr;
            }
            if (!argument.nameLower.empty()) {
                sawNamed = true;
                if (std::find(names.begin(), names.end(), argument.nameLower) != names.end()) {
                    m_diagnostics.error(argument.value->location, "a parameter cannot be supplied more than once");
                    return nullptr;
                }
                names.push_back(argument.nameLower);
            }
        }
        auto candidates = explicitOperatorCandidates(call, scope, expected);
        if (candidates.size() != 1) {
            m_diagnostics.error(call->location, candidates.empty()
                ? "no visible operator matches the argument names and types in this call"
                : "ambiguous operator '" + name + "'");
            return nullptr;
        }
        const OperatorCandidate& chosen = candidates.front();
        std::vector<ExprPtr> operands;
        for (std::size_t index : chosen.argumentOrder) {
            operands.push_back(std::move(call->arguments[index].value));
        }
        if (chosen.symbol != nullptr) {
            std::vector<Expr*> pointers;
            for (const ExprPtr& operand : operands) {
                pointers.push_back(operand.get());
            }
            if (m_recordContract != nullptr) {
                m_recordContract->operators[operatorContractKey(name, pointers)] = chosen.symbol;
            }
            boundCall = bindOperator(chosen.symbol, std::move(operands), scope, call->location);
            boundResult = chosen.result;
        } else {
            for (std::size_t i = 0; i < operands.size(); ++i) {
                call->arguments[i].value = std::move(operands[i]);
            }
            predefinedOnly = true;
        }
    }
    if (operatorSymbol(name).empty() || std::any_of(call->arguments.begin(), call->arguments.end(),
            [](const Association& argument) { return argument.high != nullptr; })) {
        return nullptr;
    }
    if (call->arguments.size() == 1) {
        for (UnaryOp op : { UnaryOp::Plus, UnaryOp::Negate, UnaryOp::Not, UnaryOp::Abs }) {
            if (name == operatorName(op)) {
                auto unary = std::make_unique<UnaryExpr>();
                unary->location = call->location;
                unary->op = op;
                unary->predefinedOnly = predefinedOnly || standard;
                unary->operand = std::move(call->arguments[0].value);
                if (boundCall != nullptr) {
                    unary->type = boundResult;
                    unary->tagIndeterminate = boundCall->tagIndeterminate;
                    unary->operatorCall = std::move(boundCall);
                }
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
                binary->predefinedOnly = predefinedOnly;
                binary->standardFixed = standard;
                binary->explicitFixedContext = standard;
                binary->left = std::move(call->arguments[0].value);
                binary->right = std::move(call->arguments[1].value);
                if (boundCall != nullptr) {
                    binary->type = boundResult;
                    binary->tagIndeterminate = boundCall->tagIndeterminate;
                    binary->operatorCall = std::move(boundCall);
                }
                return binary;
            }
        }
    }
    return nullptr;
}
