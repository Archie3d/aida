#include "Sema.h"
#include "SemaSupport.h"

#include <algorithm>

using SemaSupport::splitDottedName;

Scope* Sema::expandedScope(Symbol* prefix, Scope* from) const
{
    if (prefix->constructScope != nullptr && from->immediateDepth(prefix->constructScope) >= 0
        && prefix->renamedSubprogram == nullptr && prefix->renamedUnit == nullptr
        && prefix->inheritedFrom == nullptr) {
        return prefix->constructScope;
    }
    return prefix->kind == SymbolKind::Package ? prefix->scope : nullptr;
}

Symbol* Sema::expandedPrefix(Expr* prefix, Scope* scope, bool* ambiguous)
{
    if (prefix->kind != ExprKind::Identifier && prefix->kind != ExprKind::Selected) { return nullptr; }
    Symbol* result = nullptr;
    for (Symbol* candidate : expressionNames(prefix, scope)) {
        if (expandedScope(candidate, scope) == nullptr) { continue; }
        if (result != nullptr && result != candidate) {
            if (ambiguous != nullptr) { *ambiguous = true; }
            return nullptr;
        }
        result = candidate;
    }
    return result;
}

std::vector<Symbol*> Sema::expandedDeclarations(Symbol* prefix, const std::string& name, Scope* from) const
{
    Scope* region = expandedScope(prefix, from);
    if (region == nullptr) { return {}; }
    auto result = region->lookupVisibleLocal(name, from);
    if (prefix->kind == SymbolKind::Package && region != prefix->scope) {
        auto declared = result;
        auto specification = prefix->scope->lookupVisibleLocal(name, from);
        declared.insert(declared.end(), specification.begin(), specification.end());
        result = region->lookup(name);
        std::erase_if(result, [&](Symbol* symbol) {
            return std::find(declared.begin(), declared.end(), symbol) == declared.end();
        });
    }
    // Generic formals share the generic's region, but are not exported as
    // declarations of an instance. Only its current instance can select them.
    Scope* bindings = prefix->kind == SymbolKind::Package ? prefix->scope->parent() : prefix->declarationScope;
    if (result.empty() && from->immediateDepth(region) >= 0
        && bindings != nullptr && bindings->m_genericBindings) {
        result = bindings->lookupVisibleLocal(name, from);
        std::erase_if(result, [&](Symbol* symbol) { return expandedScope(symbol, from) == region; });
    }
    return result;
}

Symbol* Sema::lookupName(const std::string& lower, Scope* scope)
{
    std::vector<std::string> names = splitDottedName(lower);

    std::vector<Symbol*> candidates = scope->lookup(names.front());
    if (m_replayContract != nullptr && names.front() == m_replayContract->unitName
        && std::any_of(candidates.begin(), candidates.end(), [](Symbol* symbol) { return symbol->kind == SymbolKind::Generic; })) {
        for (Symbol* original : m_replayContract->bindings->lookupLocal(names.front())) {
            Symbol* instance = instanceSymbol(original);
            if (instance != nullptr && expandedScope(instance, scope) != nullptr) {
                candidates = { instance };
                break;
            }
        }
    }
    if (candidates.empty()) {
        return nullptr;
    }
    if (candidates.size() > 1 && std::any_of(candidates.begin(), candidates.end(), [](Symbol* symbol) {
            return symbol->kind != SymbolKind::Subprogram && symbol->kind != SymbolKind::EnumerationLiteral;
        })) {
        return nullptr;
    }
    for (std::size_t i = 1; i < names.size(); ++i) {
        Symbol* prefix = nullptr;
        for (Symbol* candidate : candidates) {
            Scope* selected = expandedScope(candidate, scope);
            if (selected != nullptr) {
                if (prefix != nullptr) { return nullptr; }
                prefix = candidate;
            }
        }
        if (prefix == nullptr) { return nullptr; }
        candidates = expandedDeclarations(prefix, names[i], scope);
        if (candidates.empty()) { return nullptr; }
    }
    return candidates.front();
}

std::vector<Symbol*> Sema::lookupAll(const std::string& lower, Scope* scope)
{
    return scope->lookup(lower);
}

Type* Sema::resolveTypeName(const std::string& lower, Scope* scope, const SourceLocation& location)
{
    if (lower.ends_with("'class")) {
        Type* specific = resolveTypeName(lower.substr(0, lower.size() - 6), scope, location);
        if (specific == nullptr) {
            return nullptr;
        }
        if (!taggedViewVisible(specific) || specific->classRoot != nullptr) {
            m_diagnostics.error(location, "'Class requires a specific tagged type");
            return nullptr;
        }
        return classWideType(specific);
    }
    if (lower.ends_with("'base")) {
        Type* prefix = resolveTypeName(lower.substr(0, lower.size() - 5), scope, location);
        Type* result = m_types.scalarBaseType(prefix);
        if (prefix != nullptr && result == nullptr) {
            m_diagnostics.error(location, "'Base requires a scalar subtype mark");
        }
        return result;
    }
    Symbol* symbol = nullptr;
    std::string key = contractKey(location) + ":" + lower;
    if (m_replayContract != nullptr) {
        auto found = m_replayContract->typeNames.find(key);
        if (found != m_replayContract->typeNames.end()) {
            symbol = instanceSymbol(found->second);
            if (symbol == nullptr) {
                m_diagnostics.error(location, "cannot map a resolved generic type name to its instance");
                return nullptr;
            }
        }
    }
    if (symbol == nullptr) {
        symbol = lookupName(lower, scope);
    }
    if (symbol == nullptr) {
        m_diagnostics.error(location, "'" + lower + "' is not declared");
        return nullptr;
    }
    if (symbol->kind != SymbolKind::TypeName) {
        m_diagnostics.error(location, "'" + symbol->displayName + "' is not a type");
        return nullptr;
    }
    if (m_recordContract != nullptr) {
        m_recordContract->typeNames[key] = symbol;
    }
    if (symbol->type != nullptr && symbol->type->protectedScope != nullptr
        && m_currentProtectedType == symbol->type && symbol->name == symbol->type->protectedName) {
        m_diagnostics.error(location, "the current protected instance cannot be used as a subtype mark");
        return nullptr;
    }
    return completeView(symbol->type, scope);
}

void Sema::noteReference(Symbol* symbol)
{
    if (symbol == nullptr || symbol->discriminantRecord != nullptr || symbol->owner == nullptr || symbol->isGlobal) {
        return;
    }
    if (symbol->owner != m_currentSubprogram) {
        symbol->isUplevel = true;
        symbol->owner->needsFrame = true;
    }
}

bool Sema::matchesResult(Symbol* subprogram, Type* expected) const
{
    if (expected == nullptr) {
        return true;
    }
    if (expected->kind == TypeKind::Void) {
        return subprogram->returnType == nullptr;
    }
    return subprogram->returnType != nullptr && typesCompatible(expected, subprogram->returnType);
}

Symbol* Sema::resolveBareName(const std::vector<Symbol*>& candidates, Type* expected,
                              const SourceLocation& location, const std::string& name)
{
    // Ordinary objects and type/package names continue through their existing
    // semantic paths. Overloadable names must have exactly one interpretation.
    for (Symbol* candidate : candidates) {
        if (candidate->kind != SymbolKind::Subprogram && candidate->kind != SymbolKind::EnumerationLiteral) {
            if (candidates.size() != 1) {
                m_diagnostics.error(location, "ambiguous name '" + name + "'");
                return nullptr;
            }
            if (candidate->kind == SymbolKind::Statement) {
                m_diagnostics.error(location, "statement identifier '" + name + "' cannot be used as a value");
                return nullptr;
            }
            return candidate;
        }
    }
    Symbol* chosen = nullptr;
    for (Symbol* candidate : candidates) {
        if (candidate->kind == SymbolKind::Subprogram) {
            if (!matchesResult(candidate, expected)
                || std::any_of(candidate->parameters.begin(), candidate->parameters.end(), [](Symbol* parameter) {
                    return !parameter->hasDefault;
                })) {
                continue;
            }
        } else if (expected != nullptr && !typesCompatible(expected, candidate->type)) {
            continue;
        }
        if (chosen != nullptr) {
            m_diagnostics.error(location, "ambiguous name '" + name + "'");
            return nullptr;
        }
        chosen = candidate;
    }
    if (chosen == nullptr) {
        m_diagnostics.error(location, "no visible interpretation of '" + name + "' matches this context");
    }
    return chosen;
}

Type* Sema::analyzeIdentifier(IdentifierExpr* expr, Scope* scope, Type* expected)
{
    std::vector<Symbol*> candidates = expressionNames(expr, scope);
    if (candidates.empty()) {
        m_diagnostics.error(expr->location, "'" + expr->name + "' is not declared");
        return nullptr;
    }

    Symbol* chosen = resolveBareName(candidates, expected, expr->location, expr->name);
    if (chosen == nullptr) {
        return nullptr;
    }
    if (chosen->type != nullptr && chosen->type->protectedScope != nullptr
        && (chosen->kind == SymbolKind::TypeName || chosen == chosen->type->protectedObject)
        && chosen->name == chosen->type->protectedName) {
        if (Symbol* receiver = protectedReceiver(chosen->type)) {
            chosen = receiver;
        }
    }
    expr->symbol = chosen;
    if (chosen->protectedField >= 0) {
        expr->protectedReceiver = protectedReceiver(chosen->protectedType);
        if (expr->protectedReceiver == nullptr) {
            m_diagnostics.error(expr->location, "protected components require the current protected instance");
        } else {
            noteReference(expr->protectedReceiver);
        }
    } else {
        bindProtectedCall(expr, chosen, nullptr, scope);
    }
    recordContractName(expr, chosen);

    switch (chosen->kind) {
    case SymbolKind::Object:
    case SymbolKind::Parameter:
    case SymbolKind::LoopParameter:
        noteReference(chosen);
        expr->type = chosen->type;
        if (chosen->hasStaticValue && sharedSymbolKind(chosen) == 0) {
            expr->isStatic = true;
            expr->staticValue = chosen->staticValue;
            expr->staticReal = chosen->staticReal;
            expr->exactReal = chosen->exactReal;
        }
        return expr->type;
    case SymbolKind::Number:
        expr->type = chosen->type;
        expr->isStatic = chosen->hasStaticValue;
        expr->staticValue = chosen->staticValue;
        expr->staticReal = chosen->staticReal;
        expr->exactReal = chosen->exactReal;
        return expr->type;
    case SymbolKind::EnumerationLiteral:
        expr->type = chosen->type;
        expr->isStatic = true;
        expr->staticValue = chosen->enumerationValue;
        return expr->type;
    case SymbolKind::Subprogram:
        if (chosen->abstract) { m_abstractCalls.push_back(expr); }
        if (chosen->controllingType != nullptr && chosen->controllingType->abstract
            && std::any_of(chosen->parameters.begin(), chosen->parameters.end(), [&](Symbol* parameter) {
                return parameter->hasDefault && controllingParameterType(parameter->type) == chosen->controllingType;
            })) { m_abstractDefaultCalls.push_back(expr); }
        expr->type = chosen->returnType;
        expr->tagIndeterminate = chosen->controllingType != nullptr
            && rootType(chosen->returnType) == chosen->controllingType
            && std::none_of(chosen->parameters.begin(), chosen->parameters.end(), [&](Symbol* parameter) {
                return controllingParameterType(parameter->type) == chosen->controllingType
                    && (!parameter->hasDefault || !parameter->defaultExpr->tagIndeterminate);
            });
        return expr->type;
    case SymbolKind::TypeName:
        expr->type = chosen->type;
        return expr->type;
    default:
        expr->type = nullptr;
        return nullptr;
    }
}

Type* Sema::analyzeSelected(SelectedExpr* expr, Scope* scope, Type* expected)
{
    // A selected name is either a qualified entity (Package.Entity) or a record
    // component selection.
    bool ambiguous = false;
    Symbol* prefixSymbol = expandedPrefix(expr->prefix.get(), scope, &ambiguous);
    if (ambiguous) {
        m_diagnostics.error(expr->location, "ambiguous expanded-name prefix");
        return nullptr;
    }
    if (prefixSymbol != nullptr) {
        recordContractName(expr->prefix.get(), prefixSymbol);
        if (expr->prefix->kind == ExprKind::Identifier) {
            static_cast<IdentifierExpr*>(expr->prefix.get())->symbol = prefixSymbol;
        } else {
            static_cast<SelectedExpr*>(expr->prefix.get())->symbol = prefixSymbol;
        }
        std::vector<Symbol*> candidates = contractNames(expr, expandedDeclarations(prefixSymbol, expr->selectorLower, scope));
        if (candidates.empty()) {
            m_diagnostics.error(expr->location, "'" + expr->selector + "' is not declared in '"
                                                    + prefixSymbol->displayName + "'");
            return nullptr;
        }
        Symbol* chosen = resolveBareName(candidates, expected, expr->location, expr->selector);
        if (chosen == nullptr) {
            return nullptr;
        }
        expr->symbol = chosen;
        recordContractName(expr, chosen);
        noteReference(chosen);
        if (chosen->kind == SymbolKind::Subprogram) {
            if (chosen->abstract) { m_abstractCalls.push_back(expr); }
            if (chosen->controllingType != nullptr && chosen->controllingType->abstract
                && std::any_of(chosen->parameters.begin(), chosen->parameters.end(), [&](Symbol* parameter) {
                    return parameter->hasDefault && controllingParameterType(parameter->type) == chosen->controllingType;
                })) { m_abstractDefaultCalls.push_back(expr); }
            expr->type = chosen->returnType;
            expr->tagIndeterminate = chosen->controllingType != nullptr
                && rootType(chosen->returnType) == chosen->controllingType
                && std::none_of(chosen->parameters.begin(), chosen->parameters.end(), [&](Symbol* parameter) {
                    return controllingParameterType(parameter->type) == chosen->controllingType
                        && (!parameter->hasDefault || !parameter->defaultExpr->tagIndeterminate);
                });
        } else {
            expr->type = chosen->type;
            if (chosen->kind == SymbolKind::EnumerationLiteral) {
                expr->isStatic = true;
                expr->staticValue = chosen->enumerationValue;
            } else if (chosen->hasStaticValue && sharedSymbolKind(chosen) == 0) {
                expr->isStatic = true;
                expr->staticValue = chosen->staticValue;
                expr->staticReal = chosen->staticReal;
                expr->exactReal = chosen->exactReal;
            }
        }
        return expr->type;
    }

    for (Symbol* candidate : expressionNames(expr->prefix.get(), scope)) {
        if (candidate->kind == SymbolKind::Statement) {
            m_diagnostics.error(expr->location, "statement identifier '" + candidate->displayName
                + "' does not denote an enclosing named construct");
            return nullptr;
        }
    }

    // A component's expected type can also distinguish overloaded prefixes.
    Type* prefixContext = nullptr;
    bool ambiguousPrefix = false;
    for (Type* type : expressionTypes(expr->prefix.get(), scope)) {
        Type* designated = baseType(type);
        if (designated->kind == TypeKind::Access) {
            if (designated->accessProfile != nullptr) { continue; }
            designated = baseType(completeView(designated->target, scope));
        } else if (expr->isDereference) {
            continue;
        }
        bool matches = expr->isDereference && typesCompatible(expected, designated);
        if (!expr->isDereference && designated != nullptr && designated->kind == TypeKind::Record) {
            for (const FieldInfo& field : designated->fields) {
                bool visible = representationVisible(designated)
                    || (field.isDiscriminant && field.index < designated->privateDiscriminantCount);
                matches = matches || (visible && field.name == expr->selectorLower && typesCompatible(expected, field.type));
            }
        }
        if (matches) {
            if (prefixContext != nullptr && rootType(prefixContext) != rootType(type)) {
                ambiguousPrefix = true;
            }
            prefixContext = type;
        }
    }
    Type* prefixType = analyzeExpr(expr->prefix.get(), scope, ambiguousPrefix ? nullptr : prefixContext);

    if (expr->isDereference) {
        Type* access = baseType(prefixType);
        if (access == nullptr || access->kind != TypeKind::Access || !representationVisible(prefixType)) {
            m_diagnostics.error(expr->location, "only an access value designates an object with '.all'");
            return nullptr;
        }
        expr->type = completeView(access->target, scope);
        return expr->type;
    }

    Type* record = baseType(prefixType);
    if (record != nullptr && record->kind == TypeKind::Access && record->target != nullptr) {
        record = baseType(completeView(record->target, scope));
    }
    if (!checkCompleteView(record, expr->location, "component selection")) { return nullptr; }
    if (record == nullptr || record->kind != TypeKind::Record) {
        m_diagnostics.error(expr->location, "'" + expr->selector + "' is not a component of a record");
        return nullptr;
    }
    if (record->protectedScope != nullptr) {
        if (!checkNotPrivate(record, expr->location, "protected operations require the full view")) {
            return nullptr;
        }
        auto candidates = expressionNames(expr, scope);
        if (candidates.empty()) {
            m_diagnostics.error(expr->location, "protected components and private operations are not externally visible");
            return nullptr;
        }
        Symbol* operation = resolveBareName(candidates, expected, expr->location, expr->selector);
        if (operation != nullptr && operation->protectedField >= 0
            && isProtectedCurrentInstance(expr->prefix.get(), record, scope)) {
            expr->symbol = operation;
            expr->type = operation->type;
            expr->protectedReceiver = protectedReceiver(record);
            noteReference(expr->protectedReceiver);
            return expr->type;
        }
        if (operation == nullptr || operation->kind != SymbolKind::Subprogram) {
            m_diagnostics.error(expr->location, "protected components and private operations are not externally visible");
            return nullptr;
        }
        expr->symbol = operation;
        expr->type = operation->returnType;
        bindProtectedCall(expr, operation, expr->prefix.get(), scope);
        return expr->type;
    }
    bool publicDiscriminant = std::any_of(record->fields.begin(), record->fields.end(), [&](const FieldInfo& field) {
        return field.name == expr->selectorLower && field.isDiscriminant
            && field.index < record->privateDiscriminantCount;
    });
    if (!publicDiscriminant && !checkNotPrivate(record, expr->location, "'.' reaches inside a value")) {
        return nullptr;
    }
    expr->recordType = record;
    for (const FieldInfo& field : record->fields) {
        if (field.name != expr->selectorLower) {
            continue;
        }
        expr->fieldIndex = field.index;
        expr->type = field.type;

        // A component of a variant part is only there when the discriminant
        // says so.  Where the subtype fixed it, the answer is known now; where
        // it did not, the emitter asks at run time.
        if (field.variant >= 0) {
            Type* designated = baseType(prefixType) != nullptr && baseType(prefixType)->kind == TypeKind::Access
                                   ? baseType(prefixType)->target
                                   : prefixType;
            int presence = variantPresence(designated, field.variant);
            if (presence == 0) {
                m_diagnostics.error(expr->location, "'" + expr->selector + "' belongs to another variant of '"
                                                        + record->name + "' than the one this value has");
                return nullptr;
            }
            expr->checkedVariant = presence < 0 ? field.variant : -1;
        }
        return expr->type;
    }
    m_diagnostics.error(expr->location, "'" + expr->selector + "' is not a component of type '" + record->name + "'");
    return nullptr;
}

Type* Sema::classWideType(Type* specific)
{
    if (specific->limitedView) {
        if (specific->classWide == nullptr) {
            Type* wide = m_types.create(TypeKind::Record, specific->name + "'Class");
            *wide = *specific;
            wide->name = specific->name + "'Class";
            wide->base = classWideType(rootType(specific));
            wide->classRoot = rootType(specific);
            wide->classWide = nullptr;
            specific->classWide = wide;
            m_limitedTypes.push_back(wide);
        }
        return specific->classWide;
    }

    specific = rootType(specific);
    if (specific->classWide == nullptr) {
        Type* wide = m_types.create(TypeKind::Record, specific->name + "'Class");
        *wide = *specific;
        wide->base = nullptr;
        wide->name = specific->name + "'Class";
        wide->classRoot = specific;
        wide->abstract = false;
        wide->primitives.clear();
        wide->declarationScope = nullptr;
        specific->classWide = wide;
    }
    return specific->classWide;
}

// Resolution has already selected the specific operation. A surrounding
// dispatch or class-wide assignment supplies the tag without re-resolving names.
void Sema::dispatchIndeterminate(Expr* expr, Type* controlling)
{
    if (expr == nullptr || !expr->tagIndeterminate) {
        return;
    }
    if (expr->implicitCall != nullptr) {
        dispatchIndeterminate(expr->implicitCall.get(), controlling);
    } else if (expr->kind == ExprKind::Qualified) {
        dispatchIndeterminate(static_cast<QualifiedExpr*>(expr)->operand.get(), controlling);
    } else if (expr->kind == ExprKind::Binary) {
        dispatchIndeterminate(static_cast<BinaryExpr*>(expr)->operatorCall.get(), controlling);
    } else if (expr->kind == ExprKind::Unary) {
        dispatchIndeterminate(static_cast<UnaryExpr*>(expr)->operatorCall.get(), controlling);
    } else if (expr->kind == ExprKind::Call) {
        auto* call = static_cast<CallExpr*>(expr);
        if (call->operatorExpression != nullptr) {
            dispatchIndeterminate(call->operatorExpression.get(), controlling);
        }
        if (call->subprogram != nullptr && call->subprogram->controllingType == controlling) {
            call->dispatching = true;
            for (std::size_t i = 0; i < call->resolvedArguments.size(); ++i) {
                if (rootType(call->subprogram->parameters[i]->type) == controlling) {
                    dispatchIndeterminate(call->resolvedArguments[i], controlling);
                }
            }
        }
    }
    if (rootType(expr->type) == controlling) {
        expr->type = classWideType(controlling);
    }
}
