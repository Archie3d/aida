#include "Sema.h"

#include "Parser.h"

#include <algorithm>

namespace
{

// Renames the unit a generic was written as to the name of the instance, so
// that its symbols and the names the emitter gives them are its own.
void renameUnit(Decl* decl, const std::string& name, const std::string& lower, const std::string& fromLower)
{
    switch (decl->kind) {
    case DeclKind::PackageSpecification: {
        auto* spec = static_cast<PackageSpecDecl*>(decl);
        if (spec->lower == fromLower) {
            spec->name = name;
            spec->lower = lower;
        }
        break;
    }
    case DeclKind::PackageBody: {
        auto* body = static_cast<PackageBodyDecl*>(decl);
        if (body->lower == fromLower) {
            body->name = name;
            body->lower = lower;
        }
        break;
    }
    case DeclKind::SubprogramDeclaration: {
        auto* declaration = static_cast<SubprogramDecl*>(decl);
        if (declaration->spec.lower == fromLower) {
            declaration->spec.name = name;
            declaration->spec.lower = lower;
        }
        break;
    }
    case DeclKind::SubprogramBody: {
        auto* body = static_cast<SubprogramBody*>(decl);
        if (body->spec.lower == fromLower) {
            body->spec.name = name;
            body->spec.lower = lower;
        }
        break;
    }
    default:
        break;
    }
}

// An unconstrained array has no width until an object of it exists, so it
// cannot be the element type of a file.
bool isDefinite(Type* type)
{
    Type* base = baseType(type);
    return base == nullptr || base->kind != TypeKind::Array || base->constrained;
}

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

// The symbol a unit declared, which is what makes an instance visible where it
// was written.
Symbol* declaredSymbol(Decl* decl)
{
    switch (decl->kind) {
    case DeclKind::PackageSpecification:
        return static_cast<PackageSpecDecl*>(decl)->symbol;
    case DeclKind::PackageBody:
        return static_cast<PackageBodyDecl*>(decl)->symbol;
    case DeclKind::SubprogramDeclaration:
        return static_cast<SubprogramDecl*>(decl)->symbol;
    case DeclKind::SubprogramBody:
        return static_cast<SubprogramBody*>(decl)->symbol;
    default:
        return nullptr;
    }
}

}

void Sema::analyzeGenericDecl(GenericDecl* decl, Scope* scope)
{
    // A generic unit is not analysed where it stands; only its instances are.
    // One with a dotted name is a child like any other, so it is declared
    // inside its parent rather than under a name with a dot in it.
    Scope* target = scope;
    std::string lower = decl->lower;
    std::string name = decl->name;

    std::size_t dot = decl->lower.find_last_of('.');
    if (dot != std::string::npos) {
        std::size_t pushed = 0;
        Symbol* parent = declarePackagePath(decl->lower.substr(0, dot), decl->name.substr(0, dot), scope,
                                            decl->location, pushed);
        for (std::size_t i = 0; i < pushed; ++i) {
            m_namePrefix.pop_back();
        }
        target = parent->scope;
        lower = decl->lower.substr(dot + 1);
        name = decl->name.substr(dot + 1);
    }

    // Named defaults bind in the declaration environment. Retain the overload
    // set now so declarations encountered later cannot change their meaning.
    std::vector<std::string> preceding;
    for (GenericFormal& formal : decl->formals) {
        if (formal.kind == GenericFormalKind::SubprogramFormal && formal.defaultValue) {
            Expr* value = formal.defaultValue.get();
            std::string defaultLower;
            if (value->kind == ExprKind::Identifier) {
                defaultLower = static_cast<IdentifierExpr*>(value)->lower;
            } else if (value->kind == ExprKind::StringLiteral) {
                defaultLower = operatorSymbol(static_cast<StringLiteralExpr*>(value)->value);
            }
            formal.m_defaultUsesFormal = std::find(preceding.begin(), preceding.end(), defaultLower) != preceding.end();
            formal.m_defaultCandidates = value->kind == ExprKind::StringLiteral
                ? target->lookup(defaultLower) : expressionNames(value, target);
        }
        preceding.push_back(formal.lower);
    }

    Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Generic, lower, name);
    symbol->location = decl->location;
    symbol->generic = decl;
    decl->symbol = symbol;
    target->add(symbol);
}

// A formal type says what kind of type the instantiation may supply, and a
// generic body is only sound for the kinds it was written for.
bool Sema::acceptsFormalType(const GenericFormal& formal, Type* actual, const std::string& genericName,
                             const SourceLocation& location)
{
    Type* base = baseType(actual);
    const char* wanted = nullptr;

    switch (formal.typeClass) {
    case FormalTypeClass::IntegerType:
        if (base->kind != TypeKind::Integer) {
            wanted = "an integer type";
        }
        break;
    case FormalTypeClass::FloatType:
        if (base->kind != TypeKind::Float) {
            wanted = "a floating point type";
        }
        break;
    case FormalTypeClass::Discrete:
        if (!isDiscrete(base)) {
            wanted = "a discrete type";
        }
        break;
    case FormalTypeClass::ArrayType:
        if (base->kind != TypeKind::Array) {
            wanted = "an array type";
        }
        break;
    case FormalTypeClass::Any:
        if (!isDefinite(actual)) {
            wanted = "a type of a fixed size";
        }
        break;
    }

    if (wanted != nullptr) {
        m_diagnostics.error(location, "'" + genericName + "' expects " + wanted + ", and '" + actual->name
                                          + "' is not one");
        return false;
    }
    return true;
}

// Formal arrays retain their shape instead of accepting any definite type.
bool Sema::matchesFormalArray(const GenericFormal& formal, Type* actual, Scope* bindings,
                               const SourceLocation& location)
{
    const TypeDefinition& definition = *formal.m_arrayDefinition;
    bool matches = actual->arrayRank == static_cast<int>(definition.indexTypes.size())
        && actual->constrained != definition.unconstrainedIndexes && actual->m_boundsSymbol == nullptr;
    Type* axis = actual;
    for (const auto& index : definition.indexTypes) {
        Type* expected = resolveSubtypeIndication(index.get(), bindings);
        if (axis == nullptr || axis->kind != TypeKind::Array || expected == nullptr) {
            matches = false;
            break;
        }
        matches = matches && baseType(axis->index) == baseType(expected);
        if (definition.unconstrainedIndexes) {
            matches = matches && staticallyMatches(axis->index, expected);
        } else {
            matches = matches && expected->m_scalarBoundsSymbol == nullptr
                && axis->indexLow == expected->low && axis->indexHigh == expected->high;
        }
        axis = axis->element;
    }
    Type* element = resolveSubtypeIndication(definition.elementType.get(), bindings);
    matches = matches && staticallyMatches(axis, element);
    if (!matches) {
        m_diagnostics.error(location, "actual array does not match generic formal '" + formal.name + "'");
    }
    return matches;
}

bool Sema::bindFormalSubprogram(GenericInstantiationDecl* decl, const GenericFormal& formal,
                                 Expr* actual, Scope* bindings, Scope* actualScope, bool namedDefault)
{
    if (namedDefault) {
        Scope* defaults = m_symbolTable.createScope(nullptr);
        const auto& candidates = formal.m_defaultUsesFormal
            ? bindings->lookupLocal(formal.defaultValue->kind == ExprKind::Identifier
                ? static_cast<IdentifierExpr*>(formal.defaultValue.get())->lower
                : operatorSymbol(static_cast<StringLiteralExpr*>(formal.defaultValue.get())->value))
            : formal.m_defaultCandidates;
        for (Symbol* candidate : candidates) {
            defaults->add(candidate);
        }
        actualScope = defaults;
    }
    IdentifierExpr defaultName;
    defaultName.name = formal.name;
    defaultName.lower = formal.lower;
    defaultName.location = decl->location;
    if (actual == nullptr && formal.m_boxDefault) {
        actual = &defaultName;
    }
    if (actual == nullptr) {
        m_diagnostics.error(decl->location, "no actual supplied for generic formal '" + formal.name + "'");
        return false;
    }

    // Parse a fresh profile for every instance: defaults acquire instance-local
    // type and symbol annotations, and must live as long as the generated body.
    Parser parser(formal.m_subprogramTokens, m_diagnostics);
    DeclList declarations = parser.parseDeclarations();
    if (declarations.size() != 1 || declarations.front()->kind != DeclKind::SubprogramDeclaration) {
        return false;
    }
    auto body = std::make_unique<SubprogramBody>();
    body->location = formal.location;
    body->spec = std::move(static_cast<SubprogramDecl*>(declarations.front().get())->spec);
    int errors = m_diagnostics.errorCount();
    Symbol* wrapper = declareSubprogram(body->spec, bindings, true, true);
    if (m_diagnostics.errorCount() != errors) {
        return false;
    }
    body->symbol = wrapper;

    auto sameType = [](Type* a, Type* b) { return baseType(a) == baseType(b); };
    auto matches = [&](Symbol* candidate) {
        if (candidate->kind != SymbolKind::Subprogram || !sameType(candidate->returnType, wrapper->returnType)
            || candidate->parameters.size() != wrapper->parameters.size()) {
            return false;
        }
        for (std::size_t i = 0; i < wrapper->parameters.size(); ++i) {
            if (!sameType(candidate->parameters[i]->type, wrapper->parameters[i]->type)
                || candidate->parameters[i]->mode != wrapper->parameters[i]->mode) {
                return false;
            }
        }
        return true;
    };

    std::string operation;
    if (actual->kind == ExprKind::StringLiteral) {
        operation = operatorSymbol(static_cast<StringLiteralExpr*>(actual)->value);
    } else if (actual->kind == ExprKind::Identifier) {
        operation = operatorSymbol(static_cast<IdentifierExpr*>(actual)->lower);
    }
    std::vector<Symbol*> candidates;
    const auto& actualCandidates = namedDefault && actual->kind == ExprKind::Selected
        ? formal.m_defaultCandidates : expressionNames(actual, actualScope);
    for (Symbol* candidate : actualCandidates) {
        if (matches(candidate)) {
            candidates.push_back(candidate);
        }
    }

    Scope* operandsScope = m_symbolTable.createScope(nullptr);
    std::vector<ExprPtr> operands;
    for (Symbol* parameter : wrapper->parameters) {
        operandsScope->add(parameter);
        auto operand = std::make_unique<IdentifierExpr>();
        operand->name = parameter->displayName;
        operand->lower = parameter->name;
        operand->location = formal.location;
        operands.push_back(std::move(operand));
    }

    bool intrinsic = false;
    if (!operation.empty()) {
        // Operator lookup uses the expected profile, including its result. Probe
        // values have unique names so they cannot hide declarations at the site.
        Scope* probeScope = m_symbolTable.createScope(actualScope);
        std::vector<ExprPtr> probes;
        std::vector<Expr*> probePointers;
        for (std::size_t i = 0; i < wrapper->parameters.size(); ++i) {
            std::string name = "$generic_operand_" + std::to_string(i);
            Symbol* probe = m_symbolTable.createSymbol(SymbolKind::Object, name, name);
            probe->type = wrapper->parameters[i]->type;
            probeScope->add(probe);
            auto expression = std::make_unique<IdentifierExpr>();
            expression->name = name;
            expression->lower = name;
            probePointers.push_back(expression.get());
            probes.push_back(std::move(expression));
        }
        candidates.clear();
        for (const OperatorCandidate& candidate : operatorCandidates(operation, probePointers,
                 probeScope, wrapper->returnType)) {
            bool conforms = wrapper->returnType != nullptr && sameType(candidate.result, wrapper->returnType);
            for (std::size_t i = 0; i < candidate.parameters.size(); ++i) {
                conforms = conforms && wrapper->parameters[i]->mode == ParameterMode::In
                    && sameType(candidate.parameters[i], wrapper->parameters[i]->type);
            }
            if (conforms) {
                if (candidate.symbol != nullptr) {
                    candidates.push_back(candidate.symbol);
                } else {
                    intrinsic = true;
                }
            }
        }
    }
    std::size_t count = candidates.size() + (intrinsic ? 1 : 0);
    if (count != 1) {
        m_diagnostics.error(actual->location, (count == 0 ? "no matching actual for generic formal '"
            : "ambiguous actual for generic formal '") + formal.name + "'");
        return false;
    }

    Symbol* savedSubprogram = m_currentSubprogram;
    m_currentSubprogram = wrapper;
    ExprPtr implementation;
    if (intrinsic) {
        CallExpr call;
        call.location = formal.location;
        auto callee = std::make_unique<IdentifierExpr>();
        callee->name = operation;
        callee->lower = operation;
        call.callee = std::move(callee);
        for (ExprPtr& operand : operands) {
            Association argument;
            argument.value = std::move(operand);
            call.arguments.push_back(std::move(argument));
        }
        implementation = explicitOperator(&call);
        if (implementation != nullptr) {
            analyzeExpr(implementation.get(), operandsScope, wrapper->returnType);
        }
    } else {
        implementation = bindOperator(candidates.front(), std::move(operands), operandsScope, formal.location);
    }
    m_currentSubprogram = savedSubprogram;
    if (implementation == nullptr) {
        m_diagnostics.error(actual->location, "unsupported actual for generic formal '" + formal.name + "'");
        return false;
    }
    if (wrapper->returnType != nullptr) {
        auto statement = std::make_unique<ReturnStmt>();
        statement->location = formal.location;
        statement->value = std::move(implementation);
        body->body.push_back(std::move(statement));
    } else {
        auto statement = std::make_unique<ProcedureCallStmt>();
        statement->location = formal.location;
        statement->call = std::move(implementation);
        body->body.push_back(std::move(statement));
    }
    decl->expansion.push_back(std::move(body));
    return true;
}

// Matches the actuals to the formals and declares each one in a scope of its
// own, which the copy of the generic is then analysed inside.
bool Sema::bindGenericFormals(GenericInstantiationDecl* decl, Symbol* generic, Scope* bindings, Scope* scope)
{
    const std::vector<GenericFormal>& formals = generic->generic->formals;
    std::vector<Expr*> actuals(formals.size(), nullptr);

    if (decl->arguments.size() > formals.size()) {
        m_diagnostics.error(decl->location, "too many actual parameters for generic '" + decl->genericName + "'");
        return false;
    }
    bool sawNamed = false;
    for (std::size_t i = 0; i < decl->arguments.size(); ++i) {
        std::size_t index = i;
        if (!decl->arguments[i].nameLower.empty()) {
            sawNamed = true;
            index = formals.size();
            for (std::size_t f = 0; f < formals.size(); ++f) {
                if (formals[f].lower == decl->arguments[i].nameLower) {
                    index = f;
                    break;
                }
            }
            if (index == formals.size()) {
                std::string names;
                for (const GenericFormal& candidate : formals) {
                    names += names.empty() ? "'" : "', '";
                    names += candidate.name;
                }
                m_diagnostics.error(decl->location,
                                    "'" + decl->arguments[i].name + "' is not a formal of '" + decl->genericName
                                        + "', which takes " + names + "'");
                return false;
            }
        }
        if (decl->arguments[i].nameLower.empty() && sawNamed) {
            m_diagnostics.error(decl->location, "a positional generic actual cannot follow a named actual");
            return false;
        }
        if (actuals[index] != nullptr) {
            m_diagnostics.error(decl->location, "a generic formal cannot be supplied more than once");
            return false;
        }
        actuals[index] = decl->arguments[i].value.get();
    }

    for (std::size_t i = 0; i < formals.size(); ++i) {
        const GenericFormal& formal = formals[i];
        Expr* actual = actuals[i];
        if (actual == nullptr && formal.defaultValue) {
            actual = formal.defaultValue.get();
        }
        if (formal.kind == GenericFormalKind::SubprogramFormal) {
            if (!bindFormalSubprogram(decl, formal, actual, bindings, scope,
                    actuals[i] == nullptr && formal.defaultValue != nullptr)) {
                return false;
            }
            continue;
        }
        if (actual == nullptr) {
            m_diagnostics.error(decl->location, "no actual supplied for generic formal '" + formal.name + "'");
            return false;
        }

        if (formal.kind == GenericFormalKind::TypeFormal) {
            Type* type = nullptr;
            if (actual->kind == ExprKind::Identifier) {
                type = resolveTypeName(static_cast<IdentifierExpr*>(actual)->lower, scope, actual->location);
            } else if (actual->kind == ExprKind::Attribute
                       && static_cast<AttributeExpr*>(actual)->lower == "base") {
                type = analyzeAttribute(static_cast<AttributeExpr*>(actual), scope);
            } else if (actual->kind == ExprKind::Selected) {
                auto* selected = static_cast<SelectedExpr*>(actual);
                type = analyzeSelected(selected, scope, nullptr);
                if (selected->symbol == nullptr || selected->symbol->kind != SymbolKind::TypeName) {
                    m_diagnostics.error(actual->location, "a generic formal type expects a type name");
                    return false;
                }
            } else {
                m_diagnostics.error(actual->location, "a generic formal type expects a type name");
                return false;
            }
            if (type == nullptr) {
                return false;
            }
            if (!acceptsFormalType(formal, type, decl->genericName, actual->location)) {
                return false;
            }
            if (formal.typeClass == FormalTypeClass::ArrayType
                && !matchesFormalArray(formal, type, bindings, actual->location)) {
                return false;
            }
            // Enumeration_IO cannot yet enumerate Character spellings. This
            // library limitation must not reject Character as a general formal.
            if (generic->generic->lower == "ada.text_io.enumeration_io"
                && baseType(type)->kind == TypeKind::Enumeration && baseType(type)->literals.empty()) {
                m_diagnostics.error(actual->location, "'" + type->name
                    + "' names its literals by their spelling, so use Text_IO.Put on one instead of instantiating '"
                    + decl->genericName + "'");
                return false;
            }
            Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, formal.lower, formal.name);
            symbol->type = type;
            symbol->location = formal.location;
            bindings->add(symbol);
            continue;
        }

        Type* type = resolveSubtypeIndication(formal.subtype.get(), bindings);
        analyzeExpr(actual, scope, type);
        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Number, formal.lower, formal.name);
        symbol->type = type;
        symbol->location = formal.location;
        long long value = 0;
        double real = 0.0;
        if (isReal(type) ? foldStaticReal(actual, real) : foldStatic(actual, value)) {
            symbol->hasStaticValue = true;
            symbol->staticValue = value;
            symbol->staticReal = real;
        } else {
            m_diagnostics.error(actual->location, "a generic formal object expects a static value");
        }
        bindings->add(symbol);
    }

    return true;
}

void Sema::analyzeGenericInstantiation(GenericInstantiationDecl* decl, Scope* scope)
{
    Symbol* generic = lookupName(decl->genericLower, scope);
    if (generic == nullptr || generic->kind != SymbolKind::Generic) {
        m_diagnostics.error(decl->location, "'" + decl->genericName + "' is not a generic unit");
        return;
    }
    if (m_instantiationDepth > 16) {
        m_diagnostics.error(decl->location, "generic instantiation nests too deeply");
        return;
    }
    // An instance with a dotted name belongs inside its parent, but it is
    // analysed against the formals it was bound with.  So it is given the
    // simple name here, and only put in its parent once it is built.
    Scope* home = scope;
    std::size_t pushed = 0;
    std::string instanceName = decl->lower;
    std::string instanceDisplay = decl->name;

    std::size_t dot = decl->lower.find_last_of('.');
    if (dot != std::string::npos) {
        Symbol* parent = declarePackagePath(decl->lower.substr(0, dot), decl->name.substr(0, dot), scope,
                                            decl->location, pushed);
        home = parent->scope;
        instanceName = decl->lower.substr(dot + 1);
        instanceDisplay = decl->name.substr(dot + 1);
    }

    Scope* bindings = m_symbolTable.createScope(scope);
    if (!bindGenericFormals(decl, generic, bindings, scope)) {
        for (std::size_t i = 0; i < pushed; ++i) {
            m_namePrefix.pop_back();
        }
        return;
    }

    Parser parser(generic->generic->tokens, m_diagnostics);
    DeclList expansion = parser.parseDeclarations();
    for (const DeclPtr& unit : expansion) {
        renameUnit(unit.get(), instanceDisplay, instanceName, generic->generic->lower);
    }

    ++m_instantiationDepth;
    analyzeDeclarativePart(expansion, bindings);
    --m_instantiationDepth;

    for (std::size_t i = 0; i < pushed; ++i) {
        m_namePrefix.pop_back();
    }

    // The instance lives in the bindings scope while it is analysed; here it
    // becomes visible where it was written.
    for (const DeclPtr& unit : expansion) {
        Symbol* symbol = declaredSymbol(unit.get());
        if (symbol != nullptr && symbol->name == instanceName) {
            decl->symbol = symbol;
            home->add(symbol);
            break;
        }
    }
    for (DeclPtr& unit : expansion) {
        decl->expansion.push_back(std::move(unit));
    }
    if (decl->symbol == nullptr) {
        m_diagnostics.error(decl->location, "generic '" + decl->genericName + "' produced no instance");
    }
}
