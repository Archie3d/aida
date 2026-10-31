#include "Sema.h"
#include "SemaSupport.h"

#include <algorithm>
#include <cmath>

using SemaSupport::adaptUniversal;

Type* Sema::analyzeCall(CallExpr* expr, Scope* scope, Type* expected)
{
    expr->accessCreationLevel = m_accessibilityLevel;
    if (expr->operatorExpression == nullptr) {
        int errors = m_diagnostics.errorCount();
        expr->operatorExpression = explicitOperator(expr, scope, expected);
        if (m_diagnostics.errorCount() != errors) {
            return nullptr;
        }
    }
    if (expr->operatorExpression != nullptr) {
        expr->type = analyzeExpr(expr->operatorExpression.get(), scope, expected);
        expr->isStatic = expr->operatorExpression->isStatic;
        expr->tagIndeterminate = expr->operatorExpression->tagIndeterminate;
        expr->staticValue = expr->operatorExpression->staticValue;
        expr->staticReal = expr->operatorExpression->staticReal;
        expr->exactReal = expr->operatorExpression->exactReal;
        expr->staticFailure = expr->operatorExpression->staticFailure;
        return expr->type;
    }
    expr->resolvedArguments.clear();
    expr->dispatching = false;
    // Collect the entities the callee may denote.
    std::vector<Symbol*> candidates;
    if (expr->prefixedSubprogram != nullptr) {
        candidates = {expr->prefixedSubprogram};
    } else if (expr->callee->kind == ExprKind::Identifier) {
        auto* identifier = static_cast<IdentifierExpr*>(expr->callee.get());
        candidates = expressionNames(expr->callee.get(), scope);
        if (identifier->lower.ends_with("'base") || identifier->lower.ends_with("'class")) {
            Type* type = resolveTypeName(identifier->lower, scope, identifier->location);
            if (type == nullptr) {
                return nullptr;
            }
            Symbol* mark = m_symbolTable.createSymbol(SymbolKind::TypeName, identifier->lower, identifier->name);
            mark->type = type;
            mark->location = identifier->location;
            candidates = { mark };
        }
        if (candidates.empty()) {
            m_diagnostics.error(expr->callee->location, "'" + identifier->name + "' is not declared");
            return nullptr;
        }
    } else if (expr->callee->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(expr->callee.get());
        bool ambiguous = false;
        Symbol* prefixSymbol = expandedPrefix(selected->prefix.get(), scope, &ambiguous);
        if (ambiguous) {
            m_diagnostics.error(expr->location, "ambiguous expanded-name prefix");
            return nullptr;
        }
        if (prefixSymbol != nullptr) {
            recordContractName(selected->prefix.get(), prefixSymbol);
            if (selected->prefix->kind == ExprKind::Identifier) {
                static_cast<IdentifierExpr*>(selected->prefix.get())->symbol = prefixSymbol;
            } else {
                static_cast<SelectedExpr*>(selected->prefix.get())->symbol = prefixSymbol;
            }
            candidates = expandedDeclarations(prefixSymbol, selected->selectorLower, scope);
            if (candidates.empty()) {
                m_diagnostics.error(expr->callee->location, "'" + selected->selector + "' is not declared in '"
                                                                + prefixSymbol->displayName + "'");
                return nullptr;
            }
        }
    }

    if (candidates.empty() && expr->callee->kind == ExprKind::Selected) {
        candidates = expressionNames(expr->callee.get(), scope);
    }
    // An explicit dereference and an implicit one use the same indirect call.
    if (expr->callee->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(expr->callee.get());
        if (selected->isDereference) {
            for (Type* type : expressionTypes(selected->prefix.get(), scope)) {
                if (type->accessProfile != nullptr) {
                    ExprPtr prefix = std::move(selected->prefix);
                    expr->callee = std::move(prefix);
                    break;
                }
            }
        }
    }
    bool direct = std::any_of(candidates.begin(), candidates.end(), [](Symbol* candidate) {
        return candidate->kind == SymbolKind::Subprogram || candidate->kind == SymbolKind::TypeName;
    });
    if (!direct) {
        for (Type* type : expressionTypes(expr->callee.get(), scope)) {
            if (type->accessProfile != nullptr) {
                analyzeExpr(expr->callee.get(), scope, type);
                candidates = { type->accessProfile };
                expr->indirect = true;
                break;
            }
        }
    }

    if (!expr->indirect) {
        candidates = contractNames(expr->callee.get(), std::move(candidates));
    }

    // A parameterless function can supply the array prefix of an indexed
    // component. Resolve its result using the index and component contexts.
    auto indexedPrefixes = indexedPrefixTypes(expr, scope, expected);
    if (indexedPrefixes.size() > 1) {
        m_diagnostics.error(expr->location, "ambiguous array prefix");
        return nullptr;
    }
    Type* indexedContext = indexedPrefixes.empty() ? nullptr : indexedPrefixes.front();

    // A range as the only argument selects a slice of an array.
    if (expr->arguments.size() == 1 && expr->arguments.front().high) {
        Type* prefixType = analyzeExpr(expr->callee.get(), scope, indexedContext);
        if (prefixType != nullptr && prefixType->kind == TypeKind::Access) { prefixType = prefixType->target; }
        if (prefixType == nullptr || prefixType->kind != TypeKind::Array) {
            m_diagnostics.error(expr->location, "only an array can be sliced");
            return nullptr;
        }
        if (prefixType->arrayRank > 1) {
            m_diagnostics.error(expr->location, "only one-dimensional arrays can be sliced");
            return nullptr;
        }
        Expr* low = expr->arguments.front().value.get();
        Expr* high = expr->arguments.front().high.get();
        analyzeExpr(low, scope, prefixType->index);
        analyzeExpr(high, scope, prefixType->index);
        adaptUniversal(low, prefixType->index != nullptr ? prefixType->index : m_types.integerType());
        adaptUniversal(high, prefixType->index != nullptr ? prefixType->index : m_types.integerType());

        Type* result = m_types.makeSubtype(anonymousTypeName(), rootType(prefixType), 0, 0);
        result->element = prefixType->element;
        result->index = prefixType->index;
        // A slice describes its components, not the parent's trailing storage.
        // Its first element can also be less aligned than the complete array.
        result->compositeSize = 0;
        result->sizeBounds.clear();
        result->alignment = -1;
        long long lowBound = 0;
        long long highBound = 0;
        if (foldStatic(low, lowBound) && foldStatic(high, highBound)) {
            result->constrained = true;
            result->indexLow = lowBound;
            result->indexHigh = highBound;
        } else {
            result->constrained = false;
        }

        expr->form = CallForm::Slice;
        expr->resolvedArguments = { low, high };
        expr->type = result;
        return expr->type;
    }

    // Reject malformed associations before selecting a profile. In particular,
    // a repeated name must not overwrite an earlier actual argument.
    bool sawNamed = false;
    std::vector<std::string> names;
    for (const Association& association : expr->arguments) {
        if (association.nameLower.empty()) {
            if (sawNamed) {
                m_diagnostics.error(association.value->location, "a positional argument cannot follow a named argument");
                return nullptr;
            }
        } else {
            sawNamed = true;
            if (std::find(names.begin(), names.end(), association.nameLower) != names.end()) {
                m_diagnostics.error(association.value->location, "a parameter cannot be supplied more than once");
                return nullptr;
            }
            names.push_back(association.nameLower);
        }
    }

    // Result context can eliminate profiles before their arguments are resolved.
    bool hasSubprograms = std::any_of(candidates.begin(), candidates.end(), [](Symbol* candidate) {
        return candidate->kind == SymbolKind::Subprogram;
    });
    if (expected != nullptr && hasSubprograms) {
        std::erase_if(candidates, [&](Symbol* candidate) {
            return candidate->kind == SymbolKind::Subprogram
                && !matchesResult(candidate, expected);
        });
        if (indexedContext == nullptr && candidates.empty() && (expr->callee->kind == ExprKind::Identifier
                                  || expr->callee->kind == ExprKind::Selected)) {
            m_diagnostics.error(expr->location, "no visible subprogram matches this call");
            return nullptr;
        }
    }

    // A type mark applied to one argument is a type conversion.
    if (candidates.size() == 1 && candidates.front()->kind == SymbolKind::TypeName) {
        if (expr->arguments.size() != 1) {
            m_diagnostics.error(expr->location, "a type conversion takes exactly one operand");
            return nullptr;
        }
        if (!checkCompleteView(candidates.front()->type, expr->location, "a type conversion")) { return nullptr; }
        Expr* source = expr->arguments.front().value.get();
        if (candidates.front()->type->kind == TypeKind::Array
            && (source->kind == ExprKind::Aggregate || source->kind == ExprKind::StringLiteral)) {
            m_diagnostics.error(source->location,
                "array conversion requires an independently typed operand; use a qualified expression");
            return nullptr;
        }
        if (source->kind == ExprKind::Aggregate || source->kind == ExprKind::Allocator
            || source->kind == ExprKind::CharacterLiteral || source->kind == ExprKind::StringLiteral) {
            m_diagnostics.error(source->location,
                "type conversion requires an independently typed operand; use a qualified expression");
            return nullptr;
        }
        if (source->type == nullptr && source->kind != ExprKind::Aggregate) {
            Type* conversion = candidates.front()->type;
            Type* context = source->kind == ExprKind::Null && conversion->kind == TypeKind::Access ? conversion : nullptr;
            Expr* operation = source;
            if (source->kind == ExprKind::Call) {
                auto* call = static_cast<CallExpr*>(source);
                if (call->operatorExpression == nullptr) { call->operatorExpression = explicitOperator(call, scope); }
                if (call->operatorExpression != nullptr) { operation = call->operatorExpression.get(); }
            }
            if (isNumeric(conversion) && operation->kind == ExprKind::Binary) {
                auto* binary = static_cast<BinaryExpr*>(operation);
                if (binary->op == BinaryOp::Multiply || binary->op == BinaryOp::Divide) {
                    auto left = expressionTypes(binary->left.get(), scope);
                    auto right = expressionTypes(binary->right.get(), scope);
                    auto fixed = [](Type* type) { return type->kind == TypeKind::Fixed; };
                    auto realOperand = [&](Type* type) { return fixed(type) || type->kind == TypeKind::UniversalReal; };
                    if ((std::any_of(left.begin(), left.end(), fixed) && std::any_of(right.begin(), right.end(), realOperand))
                        || (std::any_of(right.begin(), right.end(), fixed) && std::any_of(left.begin(), left.end(), realOperand))) {
                        context = conversion;
                        binary->explicitFixedContext = true;
                    }
                }
            }
            analyzeExpr(source, scope, context);
        }
        recordContractName(expr->callee.get(), candidates.front());
        expr->form = CallForm::Conversion;
        expr->type = candidates.front()->type;
        Expr* operand = expr->arguments.front().value.get();
        expr->resolvedArguments.push_back(operand);
        // A literal only takes the type it is converted to when both belong to
        // the same family; crossing families is what the conversion is for.
        if (expr->type->kind != TypeKind::Fixed && operand->type != nullptr && operand->type->kind != TypeKind::Fixed
            && isReal(expr->type) == isReal(operand->type)) {
            adaptUniversal(operand, expr->type);
        }

        if (hasIncompleteParts(expr->type)) {
            m_deferredProtectedUses.push_back({expr->type, expr->location});
        }
        if (hasProtected(expr->type)) {
            m_diagnostics.error(expr->location, "conversions containing protected objects are not yet supported");
        }
        if (hasTasks(expr->type)) {
            m_diagnostics.error(expr->location, "conversions of types containing tasks are not yet supported");
        }
        if (expr->type->kind == TypeKind::Array && hasLimitedParts(expr->type)) {
            m_diagnostics.error(expr->location, "array conversions cannot copy limited controlled components");
        }
        if (expr->type->kind == TypeKind::Access && checkObjectAccessConversion(expr, expr->type, operand->type)) {
            return expr->type;
        }
        bool numeric = isNumeric(baseType(expr->type)) && isNumeric(baseType(operand->type))
            && representationVisible(expr->type) && representationVisible(operand->type);
        // Explicit array conversions are separate from implicit compatibility.
        // Retain the supported conversion between arrays with identical component
        // subtypes and corresponding compatible index types.
        Type* targetAxis = expr->type;
        Type* sourceAxis = operand->type;
        bool arrayConversion = targetAxis != nullptr && sourceAxis != nullptr
            && targetAxis->kind == TypeKind::Array && sourceAxis->kind == TypeKind::Array
            && targetAxis->arrayRank == sourceAxis->arrayRank;
        if (arrayConversion) {
            int rank = targetAxis->arrayRank;
            for (int dimension = 0; dimension < rank; ++dimension) {
                bool integerIndices = targetAxis->index->kind == TypeKind::Integer
                    && sourceAxis->index->kind == TypeKind::Integer;
                if (!integerIndices && !typesCompatible(targetAxis->index, sourceAxis->index)) {
                    arrayConversion = false;
                }
                targetAxis = targetAxis->element;
                sourceAxis = sourceAxis->element;
            }
            arrayConversion = arrayConversion && targetAxis == sourceAxis;
        }
        Type* sourceRoot = operand->type == nullptr ? nullptr : operand->type->classRoot;
        Type* targetRoot = expr->type->classRoot != nullptr ? expr->type->classRoot : expr->type;
        bool relatedUntagged = false;
        if (operand->type != nullptr && !expr->type->tagged) {
            Type* targetAncestor = rootType(expr->type);
            Type* sourceAncestor = rootType(operand->type);
            while (targetAncestor->parentType != nullptr
                && (targetAncestor->parentVisible || representationVisible(targetAncestor))) {
                targetAncestor = targetAncestor->parentType;
            }
            while (sourceAncestor->parentType != nullptr
                && (sourceAncestor->parentVisible || representationVisible(sourceAncestor))) {
                sourceAncestor = sourceAncestor->parentType;
            }
            relatedUntagged = targetAncestor == sourceAncestor;
        }
        bool classConversion = sourceRoot != nullptr
            && (isTaggedAncestor(sourceRoot, targetRoot) || isTaggedAncestor(targetRoot, sourceRoot)
                || (expr->type->classRoot != nullptr
                    && (sourceRoot->interfaceKind != InterfaceKind::None || rootType(targetRoot)->interfaceKind != InterfaceKind::None)));
        if (classConversion && (sourceRoot->interfaceKind != InterfaceKind::None
            || rootType(targetRoot)->interfaceKind != InterfaceKind::None)) {
            classConversion = taggedViewVisible(expr->type) && taggedViewVisible(operand->type);
        }
        bool ancestorView = isTaggedAncestor(expr->type, operand->type)
            && (rootType(expr->type) == rootType(operand->type) || representationVisible(operand->type)
                || (rootType(targetRoot)->interfaceKind != InterfaceKind::None && taggedViewVisible(operand->type)));
        if (!numeric && !arrayConversion && !relatedUntagged && !classConversion && !ancestorView && !typesCompatible(expr->type, operand->type)) {
            m_diagnostics.error(expr->location, "this type conversion is not allowed");
        }
        if (expr->type->kind == TypeKind::Fixed) {
            if (expr->type->formalFixed || expr->type->scalarBoundsSymbol != nullptr) {
                return expr->type;
            }
            ExactReal exact;
            long long value = 0;
            if (exactValue(operand, exact) && exact.scaled(expr->type->fixedScale, value)
                && value >= expr->type->low && value <= expr->type->high) {
                expr->isStatic = true;
                expr->staticValue = value;
            } else if (operand->isStatic) {
                expr->isStatic = true;
                expr->staticFailure = operand->staticFailure != StaticFailure::None
                    ? StaticFailure::Dependent : exact.valid ? StaticFailure::Check : StaticFailure::Capacity;
            }
            return expr->type;
        }
        // Enumeration conversions preserve positions, including conversions
        // between a parent enumeration and its derived types (RM 4.9).
        if (!numeric && relatedUntagged && isDiscrete(expr->type) && operand->isStatic
            && expr->type->scalarBoundsSymbol == nullptr) {
            expr->isStatic = true;
            expr->staticValue = operand->staticValue;
            expr->staticFailure = operand->staticFailure != StaticFailure::None ? StaticFailure::Dependent
                : operand->staticValue < expr->type->low || operand->staticValue > expr->type->high
                    ? StaticFailure::Check : StaticFailure::None;
        }
        if (numeric && operand->isStatic && expr->type->scalarBoundsSymbol == nullptr) {
            ExactReal exact;
            expr->isStatic = true;
            if (operand->staticFailure != StaticFailure::None) {
                expr->staticFailure = StaticFailure::Dependent;
            } else if (!exactValue(operand, exact)) {
                expr->staticFailure = StaticFailure::Capacity;
            } else if (isDiscrete(expr->type)) {
                long long rounded = 0;
                __int128 value = exact.numerator;
                bool integral = exact.denominator == 1;
                if (isReal(operand->type) || operand->type->kind == TypeKind::Fixed) {
                    if (expr->type->modulus != 0) {
                        value = exact.numerator / exact.denominator;
                        __int128 remainder = exact.numerator % exact.denominator;
                        __int128 magnitude = remainder < 0 ? -remainder : remainder;
                        if (magnitude >= (exact.denominator / 2 + exact.denominator % 2)) {
                            value += exact.numerator < 0 ? -1 : 1;
                        }
                        integral = true;
                    } else {
                        integral = exact.scaled(0, rounded);
                        value = rounded;
                    }
                }
                if (!integral || value < expr->type->low || value > expr->type->high) {
                    expr->staticFailure = StaticFailure::Check;
                } else {
                    expr->staticValue = value;
                    expr->exactReal = ExactReal::make(value);
                }
            } else if (isReal(expr->type)) {
                expr->staticReal = exact.numerator == 0 ? std::copysign(0.0, operand->staticReal)
                    : exact.real(typeSize(expr->type) == 4 ? 24 : 53);
                if (!std::isfinite(expr->staticReal) || (expr->type->hasRealRange
                    && (expr->staticReal < expr->type->lowReal || expr->staticReal > expr->type->highReal))) {
                    expr->staticFailure = StaticFailure::Check;
                }
            }
        }
        return expr->type;
    }

    std::vector<Symbol*> subprograms;
    for (Symbol* candidate : candidates) {
        if (candidate->kind == SymbolKind::Subprogram) {
            subprograms.push_back(candidate);
        }
    }

    if (indexedContext != nullptr && !subprograms.empty()) {
        for (Symbol* candidate : subprograms) {
            std::vector<std::size_t> positions;
            if (matchCallArguments(expr, candidate, scope, positions)) {
                m_diagnostics.error(expr->location, "ambiguous call or indexed component");
                return nullptr;
            }
        }
        subprograms.clear();
    }

    if (!subprograms.empty()) {
        Symbol* chosen = nullptr;
        for (Symbol* candidate : subprograms) {
            std::vector<std::size_t> positions;
            bool matches = matchCallArguments(expr, candidate, scope, positions);
            if (matches) {
                if (chosen != nullptr) {
                    m_diagnostics.error(expr->location, "ambiguous subprogram call");
                    return nullptr;
                }
                chosen = candidate;
            }
        }

        if (chosen == nullptr) {
            // Prefer an undeclared-name or malformed-expression diagnostic when
            // an actual has no interpretation even without a formal context.
            int errors = m_diagnostics.errorCount();
            for (const Association& association : expr->arguments) {
                Expr* argument = association.value.get();
                if (argument->kind != ExprKind::Aggregate && argument->kind != ExprKind::Allocator
                    && argument->kind != ExprKind::Null && expressionTypes(argument, scope).empty()) {
                    analyzeExpr(argument, scope, nullptr);
                }
            }
            if (m_diagnostics.errorCount() != errors) {
                return nullptr;
            }
            m_diagnostics.error(expr->location, "no visible subprogram matches this call");
            return nullptr;
        }

        // Lower only after overload resolution. This preserves a single prefix
        // evaluation and lets the ordinary call path enforce modes, accessibility,
        // defaults, dispatching and cleanup (RM 6.4(10.1)).
        if (expr->prefixedSubprogram == nullptr && expr->callee->kind == ExprKind::Selected) {
            auto* selected = static_cast<SelectedExpr*>(expr->callee.get());
            bool ambiguousPrefix = false;
            Type* prefixType = prefixedPrefixType(selected, chosen, scope, &ambiguousPrefix);
            if (ambiguousPrefix) {
                m_diagnostics.error(expr->location, "ambiguous prefix of prefixed call");
                return nullptr;
            }
            if (prefixType != nullptr) {
                ExprPtr receiver = std::move(selected->prefix);
                if (baseType(prefixType)->kind == TypeKind::Access) {
                    auto dereference = std::make_unique<SelectedExpr>();
                    dereference->location = receiver->location;
                    dereference->selector = dereference->selectorLower = "all";
                    dereference->isDereference = true;
                    dereference->prefix = std::move(receiver);
                    receiver = std::move(dereference);
                }
                Type* formal = chosen->parameters.front()->type;
                if (formal->kind == TypeKind::Access) {
                    auto access = std::make_unique<AttributeExpr>();
                    access->location = receiver->location;
                    access->name = "Access";
                    access->lower = "access";
                    Type* accessType = m_types.create(TypeKind::Access, anonymousTypeName());
                    *accessType = *formal;
                    accessType->target = designatedViewType(prefixType);
                    access->prefixedAccessType = accessType;
                    access->prefix = std::move(receiver);
                    receiver = std::move(access);
                }
                Association first;
                first.value = std::move(receiver);
                expr->arguments.insert(expr->arguments.begin(), std::move(first));
                auto callee = std::make_unique<IdentifierExpr>();
                callee->location = expr->callee->location;
                callee->name = chosen->displayName;
                callee->lower = chosen->name;
                callee->symbol = chosen;
                expr->callee = std::move(callee);
                expr->prefixedSubprogram = chosen;
                return analyzeCall(expr, scope, expected);
            }
        }

        expr->form = CallForm::Subprogram;
        expr->subprogram = chosen;
        if (hasProtected(chosen->returnType)) {
            m_diagnostics.error(expr->location, "function results containing protected objects are not yet supported");
        }
        if (hasIncompleteParts(chosen->returnType)) {
            m_deferredProtectedUses.push_back({chosen->returnType, expr->location});
        }
        Expr* protectedObject = expr->callee->kind == ExprKind::Selected
            ? static_cast<SelectedExpr*>(expr->callee.get())->prefix.get() : nullptr;
        bindProtectedCall(expr, chosen, protectedObject, scope);
        if (!expr->indirect) {
            recordContractName(expr->callee.get(), chosen);
        }
        expr->resolvedArguments.assign(chosen->parameters.size(), nullptr);
        for (std::size_t i = 0; i < expr->arguments.size(); ++i) {
            std::size_t index = i;
            if (!expr->arguments[i].nameLower.empty()) {
                for (std::size_t p = 0; p < chosen->parameters.size(); ++p) {
                    if (chosen->parameters[p]->name == expr->arguments[i].nameLower) {
                        index = p;
                        break;
                    }
                }
            }
            Expr* argument = expr->arguments[i].value.get();
            Type* formal = chosen->parameters[index]->type;
            Type* context = formal;
            if (controllingParameterType(formal) == chosen->controllingType) {
                for (Type* actual : expressionTypes(argument, scope)) {
                    if (designatedViewType(actual)->classRoot == chosen->controllingType) {
                        context = actual;
                        expr->dispatching = true;
                    }
                }
            }
            if (formal->kind == TypeKind::Access && formal->anonymousAccess
                && chosen->controllingType != nullptr && controllingParameterType(formal) == chosen->controllingType
                && argument->kind == ExprKind::Attribute) {
                auto* attribute = static_cast<AttributeExpr*>(argument);
                if ((attribute->lower == "access" || attribute->lower == "unchecked_access")
                    && attribute->prefixedAccessType == nullptr) {
                    for (Type* actual : expressionTypes(attribute->prefix.get(), scope)) {
                        if (actual->classRoot == chosen->controllingType) {
                            context = m_types.create(TypeKind::Access, anonymousTypeName());
                            *context = *formal;
                            context->target = actual;
                            expr->dispatching = true;
                            break;
                        }
                    }
                }
            }
            analyzeExpr(argument, scope, context);
            adaptUniversal(argument, chosen->parameters[index]->type);
            if (chosen->parameters[index]->mode != ParameterMode::In) {
                // Passing a variable by reference does not copy a limited value.
                checkAssignable(argument, scope, true, true);
            }
            expr->resolvedArguments[index] = argument;
        }
        // Whatever the caller left out stands in as the expression its
        // declaration gives.
        for (std::size_t p = 0; p < chosen->parameters.size(); ++p) {
            if (expr->resolvedArguments[p] == nullptr) {
                expr->resolvedArguments[p] = chosen->parameters[p]->defaultExpr;
                if (chosen->controllingType != nullptr && chosen->controllingType->abstract
                    && controllingParameterType(chosen->parameters[p]->type) == chosen->controllingType) {
                    m_abstractDefaultCalls.push_back(expr);
                }
            }
        }
        // A class-wide actual supplies dispatching wrappers even though the
        // checked template used a specific formal type. A determinate operand
        // supplies the tag to the other, tag-indeterminate controlling operands.
        if (chosen->classWideWrapper || (chosen->genericFormalSubprogram && chosen->abstract
            && chosen->controllingType != nullptr && chosen->controllingType->classRoot != nullptr)) {
            for (std::size_t i = 0; i < chosen->parameters.size(); ++i) {
                if (controllingParameterType(chosen->parameters[i]->type) == chosen->controllingType
                    && expr->resolvedArguments[i] != nullptr && !expr->resolvedArguments[i]->tagIndeterminate) {
                    expr->dispatching = true;
                }
            }
        }
        expr->tagIndeterminate = chosen->controllingType != nullptr
            && controllingResultType(chosen->returnType) == chosen->controllingType
            && !expr->dispatching;
        for (std::size_t i = 0; i < chosen->parameters.size(); ++i) {
            if (controllingParameterType(chosen->parameters[i]->type) == chosen->controllingType) {
                expr->tagIndeterminate = expr->tagIndeterminate && expr->resolvedArguments[i] != nullptr
                    && expr->resolvedArguments[i]->tagIndeterminate;
            }
        }
        if (expr->dispatching) {
            for (std::size_t i = 0; i < chosen->parameters.size(); ++i) {
                if (controllingParameterType(chosen->parameters[i]->type) == chosen->controllingType
                    && expr->resolvedArguments[i] != nullptr && expr->resolvedArguments[i]->tagIndeterminate) {
                    dispatchIndeterminate(expr->resolvedArguments[i], chosen->controllingType);
                }
            }
            for (std::size_t i = 0; i < chosen->parameters.size(); ++i) {
                if (controllingParameterType(chosen->parameters[i]->type) == chosen->controllingType
                    && expr->resolvedArguments[i] != nullptr
                    && expr->resolvedArguments[i]->type != nullptr
                    && designatedViewType(expr->resolvedArguments[i]->type)->classRoot == nullptr) {
                    m_diagnostics.error(expr->location, "all controlling operands of a dispatching call must be class-wide");
                }
            }
        }
        if (chosen->abstract) { m_abstractCalls.push_back(expr); }
        expr->type = chosen->returnType;
        if (expr->dispatching && chosen->returnType != nullptr
            && controllingResultType(chosen->returnType) == chosen->controllingType) {
            expr->type = dispatchingResultType(chosen);
        }
        return expr->type;
    }

    // Otherwise this is an indexed component.
    Type* prefixType = analyzeExpr(expr->callee.get(), scope, indexedContext);
    Type* array = prefixType;
    // Indexing an access value reads the array it designates, without '.all'.
    if (array != nullptr && baseType(array)->kind == TypeKind::Access) {
        array = baseType(array)->target;
    }
    if (array == nullptr || array->kind != TypeKind::Array) {
        m_diagnostics.error(expr->location, "only arrays and subprograms can be applied to arguments");
        return nullptr;
    }
    if (expr->arguments.size() != static_cast<std::size_t>(array->arrayRank)) {
        m_diagnostics.error(expr->location, "array indexing requires " + std::to_string(array->arrayRank) + " index value(s)");
        return nullptr;
    }
    expr->form = CallForm::Indexing;
    int rank = array->arrayRank;
    for (int dimension = 0; dimension < rank; ++dimension) {
        Expr* index = expr->arguments[dimension].value.get();
        if (!expr->arguments[dimension].nameLower.empty()) {
            m_diagnostics.error(index->location, "array indexing does not allow named associations");
        }
        Type* indexType = analyzeExpr(index, scope, array->index);
        if (!typesCompatible(array->index, indexType)) {
            m_diagnostics.error(index->location, "index value has an incompatible type");
        }
        adaptUniversal(index, array->index != nullptr ? array->index : m_types.integerType());
        expr->resolvedArguments.push_back(index);
        array = array->element;
    }
    expr->type = array;
    (void)expected;
    return expr->type;
}
