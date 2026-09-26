#include "Sema.h"
#include "SemaSupport.h"

#include <algorithm>

using SemaSupport::adaptUniversal;

Type* Sema::analyzeCall(CallExpr* expr, Scope* scope, Type* expected)
{
    if (expr->operatorExpression == nullptr) {
        expr->operatorExpression = explicitOperator(expr);
    }
    if (expr->operatorExpression != nullptr) {
        expr->type = analyzeExpr(expr->operatorExpression.get(), scope, expected);
        expr->isStatic = expr->operatorExpression->isStatic;
        expr->staticValue = expr->operatorExpression->staticValue;
        expr->staticReal = expr->operatorExpression->staticReal;
        return expr->type;
    }
    expr->resolvedArguments.clear();
    // Collect the entities the callee may denote.
    std::vector<Symbol*> candidates;
    if (expr->callee->kind == ExprKind::Identifier) {
        auto* identifier = static_cast<IdentifierExpr*>(expr->callee.get());
        candidates = expressionNames(expr->callee.get(), scope);
        if (identifier->lower.ends_with("'base")) {
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
        Symbol* prefixSymbol = nullptr;
        if (selected->prefix->kind == ExprKind::Identifier) {
            auto* identifier = static_cast<IdentifierExpr*>(selected->prefix.get());
            for (Symbol* candidate : scope->lookup(identifier->lower)) {
                if (candidate->kind == SymbolKind::Package) {
                    prefixSymbol = candidate;
                    identifier->symbol = candidate;
                    break;
                }
            }
        } else if (selected->prefix->kind == ExprKind::Selected) {
            auto* nested = static_cast<SelectedExpr*>(selected->prefix.get());
            analyzeSelected(nested, scope, nullptr);
            if (nested->symbol != nullptr && nested->symbol->kind == SymbolKind::Package) {
                prefixSymbol = nested->symbol;
            }
        }
        if (prefixSymbol != nullptr && prefixSymbol->scope != nullptr) {
            candidates = prefixSymbol->scope->lookupLocal(selected->selectorLower);
            if (candidates.empty()) {
                m_diagnostics.error(expr->callee->location, "'" + selected->selector + "' is not declared in '"
                                                                + prefixSymbol->displayName + "'");
                return nullptr;
            }
        }
    }

    candidates = contractNames(expr->callee.get(), std::move(candidates));

    // A range as the only argument selects a slice of an array.
    if (expr->arguments.size() == 1 && expr->arguments.front().high) {
        Type* prefixType = analyzeExpr(expr->callee.get(), scope, nullptr);
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
        if (candidates.empty() && (expr->callee->kind == ExprKind::Identifier
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
        Expr* source = expr->arguments.front().value.get();
        if (source->type == nullptr && source->kind != ExprKind::Aggregate) {
            Type* conversion = candidates.front()->type;
            Type* context = nullptr;
            if (conversion->kind == TypeKind::Fixed && source->kind == ExprKind::Binary) {
                auto* binary = static_cast<BinaryExpr*>(source);
                if (binary->op == BinaryOp::Multiply || binary->op == BinaryOp::Divide) {
                    for (Expr* part : { binary->left.get(), binary->right.get() }) {
                        for (Type* type : expressionTypes(part, scope)) {
                            if (type->kind == TypeKind::Fixed) {
                                context = conversion;
                            }
                        }
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

        bool numeric = isNumeric(baseType(expr->type)) && isNumeric(baseType(operand->type));
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
        if (!numeric && !arrayConversion && !typesCompatible(expr->type, operand->type)) {
            m_diagnostics.error(expr->location, "this type conversion is not allowed");
        }
        if (expr->type->kind == TypeKind::Fixed) {
            ExactReal exact;
            long long value = 0;
            if (exactValue(operand, exact) && exact.scaled(expr->type->m_fixedBits, value)
                && value >= expr->type->low && value <= expr->type->high) {
                expr->isStatic = true;
                expr->staticValue = value;
            } else if (operand->type != nullptr && operand->type->kind == TypeKind::UniversalReal) {
                m_diagnostics.error(expr->location, "fixed-point conversion requires an exact static real within the target range");
            }
            return expr->type;
        }
        if (operand->type != nullptr && operand->type->kind == TypeKind::Fixed) {
            return expr->type;
        }
        if (operand->isStatic && expr->type->m_scalarBoundsSymbol == nullptr
            && isReal(expr->type) == isReal(operand->type)
            && (expr->type->m_modulus == 0
                || (operand->staticValue >= expr->type->low && operand->staticValue <= expr->type->high))) {
            expr->isStatic = true;
            expr->staticValue = operand->staticValue;
            expr->staticReal = operand->staticReal;
        }
        return expr->type;
    }

    std::vector<Symbol*> subprograms;
    for (Symbol* candidate : candidates) {
        if (candidate->kind == SymbolKind::Subprogram) {
            subprograms.push_back(candidate);
        }
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

        expr->form = CallForm::Subprogram;
        expr->subprogram = chosen;
        recordContractName(expr->callee.get(), chosen);
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
            analyzeExpr(argument, scope, chosen->parameters[index]->type);
            adaptUniversal(argument, chosen->parameters[index]->type);
            if (chosen->parameters[index]->mode != ParameterMode::In) {
                // Passing a variable by reference does not copy a limited value.
                checkAssignable(argument, scope, true);
            }
            expr->resolvedArguments[index] = argument;
        }
        // Whatever the caller left out stands in as the expression its
        // declaration gives.
        for (std::size_t p = 0; p < chosen->parameters.size(); ++p) {
            if (expr->resolvedArguments[p] == nullptr) {
                expr->resolvedArguments[p] = chosen->parameters[p]->defaultExpr;
            }
        }
        expr->type = chosen->returnType;
        return expr->type;
    }

    // Otherwise this is an indexed component.
    Type* prefixType = analyzeExpr(expr->callee.get(), scope, nullptr);
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
