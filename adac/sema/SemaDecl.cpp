#include "Sema.h"
#include "Parser.h"
#include "SemaSupport.h"

#include <algorithm>
#include <unordered_set>

using SemaSupport::adaptUniversal;
using SemaSupport::objectView;

void Sema::analyzeDeclarativePart(DeclList& declarations, Scope* scope, bool reportIncomplete)
{
    // Resolve adjacent representation clauses before choosing counts for the
    // type bounds. The expression remains owned by its representation item.
    for (std::size_t i = 0; i < declarations.size(); ++i) {
        if (declarations[i]->kind != DeclKind::Type) { continue; }
        auto* type = static_cast<TypeDecl*>(declarations[i].get());
        type->smallClause = nullptr;
        for (std::size_t j = i + 1; j < declarations.size() && declarations[j]->kind == DeclKind::Representation; ++j) {
            auto* clause = static_cast<RepresentationDecl*>(declarations[j].get());
            if (clause->lower == type->lower && clause->attribute == "small" && type->smallClause == nullptr) {
                type->smallClause = clause->value.get();
            }
        }
    }
    Type* precedingType = nullptr;
    std::unordered_set<const Decl*> settledRecordClauses;
    for (std::size_t i = 0; i < declarations.size(); ++i) {
        const DeclPtr& decl = declarations[i];
        if (decl->kind == DeclKind::Representation) {
            if (settledRecordClauses.contains(decl.get())) { continue; }
            analyzeRepresentation(static_cast<RepresentationDecl*>(decl.get()), scope, precedingType);
            // Adjacent representation items can jointly settle size/alignment.
            continue;
        }
        std::size_t firstSymbol = m_symbolTable.symbols().size();
        analyzeDecl(decl.get(), scope);
        if (decl->kind == DeclKind::Type) {
            auto* type = static_cast<TypeDecl*>(decl.get());
            // Resolve component placement before adjacent Size/Alignment items,
            // including when those items textually precede the record clause.
            for (std::size_t j = i + 1; j < declarations.size() && declarations[j]->kind == DeclKind::Representation; ++j) {
                auto* clause = static_cast<RepresentationDecl*>(declarations[j].get());
                if (clause->attribute == "record" && clause->lower == type->lower) {
                    analyzeRepresentation(clause, scope, type->declaredType);
                    settledRecordClauses.insert(clause);
                }
            }
        }
        if (decl->kind == DeclKind::PackageBody || (decl->kind == DeclKind::SubprogramBody
            && !static_cast<SubprogramBody*>(decl.get())->isNull)) {
            // A body freezes preceding declarations in its declarative part.
            // Register the body's own profile first: it can itself override.
            for (const auto& symbol : m_symbolTable.symbols()) {
                if (symbol->kind == SymbolKind::TypeName && symbol->type != nullptr
                    && symbol->type->declarationScope == scope) {
                    if (symbol->type->isIncomplete && symbol->type->privateTo == nullptr) {
                        m_diagnostics.error(decl->location, "an incomplete type must be completed before an intervening body");
                    }
                    if (symbol->type->tagged) { rootType(symbol->type)->dispatchFrozen = true; }
                }
            }
        }
        if (scope == m_packageSpecScope && !m_inVisiblePart && !m_packages.empty()) {
            for (std::size_t i = firstSymbol; i < m_symbolTable.symbols().size(); ++i) {
                Symbol* symbol = m_symbolTable.symbols()[i].get();
                auto locals = scope->lookupLocal(symbol->name);
                if (std::find(locals.begin(), locals.end(), symbol) != locals.end()) {
                    symbol->privatePrimitiveTo = m_packages.back();
                }
            }
        }
        precedingType = decl->kind == DeclKind::Type
            ? static_cast<TypeDecl*>(decl.get())->declaredType
            : decl->kind == DeclKind::Subtype ? static_cast<SubtypeDecl*>(decl.get())->declaredType : nullptr;
    }
    if (reportIncomplete) {
        reportIncompleteTypes(declarations);
    }
}

// Call only for declarative parts whose bodies must be provided locally.
// Package specifications and generic contracts can be completed later.
void Sema::reportMissingBodies(const DeclList& declarations)
{
    for (const DeclPtr& decl : declarations) {
        if (decl->kind == DeclKind::PackageSpecification) {
            auto* package = static_cast<PackageSpecDecl*>(decl.get());
            reportMissingBodies(package->publicPart);
            reportMissingBodies(package->privatePart);
        }
        if (decl->kind == DeclKind::Type) {
            auto* typeDecl = static_cast<TypeDecl*>(decl.get());
            reportMissingBodies(typeDecl->protectedDeclarations);
            Type* type = typeDecl->declaredType;
            if (type != nullptr && type->taskBody != nullptr && !type->taskBody->hasBody
                && !type->taskBody->missingBodyReported) {
                type->taskBody->missingBodyReported = true;
                m_diagnostics.error(decl->location, "task '" + type->name + "' has no body");
            }
        }
        if (decl->kind != DeclKind::SubprogramDeclaration) {
            continue;
        }
        Symbol* symbol = static_cast<SubprogramDecl*>(decl.get())->symbol;
        if (symbol != nullptr && !symbol->abstract && !symbol->hasBody && !symbol->missingBodyReported) {
            symbol->missingBodyReported = true;
            m_diagnostics.error(decl->location,
                "subprogram '" + symbol->displayName + "' has no body");
        }
    }
}

void Sema::analyzeDecl(Decl* decl, Scope* scope)
{
    bool savedGenericBody = m_genericBodyContext;
    bool savedInstanceBody = m_genericInstanceBody;
    if (m_replayContract != nullptr && (decl->kind == DeclKind::SubprogramBody || decl->kind == DeclKind::PackageBody)) {
        m_genericInstanceBody = true;
    }
    if (m_recordContract != nullptr && (decl->kind == DeclKind::SubprogramBody || decl->kind == DeclKind::PackageBody)) {
        m_genericBodyContext = true;
    }
    Decl* savedConstraintDeclaration = m_scalarConstraintDeclaration;
    m_scalarConstraintDeclaration = decl;
    decl->scalarConstraints.clear();
    decl->discriminantConstraints.clear();
    decl->arrayConstraints.clear();
    decl->constraintOrder.clear();
    switch (decl->kind) {
    case DeclKind::Object:
        analyzeObjectDecl(static_cast<ObjectDecl*>(decl), scope);
        break;
    case DeclKind::Number:
        analyzeNumberDecl(static_cast<NumberDecl*>(decl), scope);
        break;
    case DeclKind::Type: {
        auto* typeDecl = static_cast<TypeDecl*>(decl);
        if (typeDecl->protectedBody) {
            analyzeProtected(typeDecl, scope);
            break;
        }
        analyzeTypeDecl(typeDecl, scope);
        if (typeDecl->declaredType != nullptr && typeDecl->declaredType->protectedScope != nullptr) {
            analyzeProtected(typeDecl, scope);
        }
        analyzeDeclarativePart(typeDecl->taskObjects, scope);
        if (typeDecl->declaredType != nullptr && typeDecl->declaredType->protectedScope != nullptr
            && !typeDecl->taskObjects.empty()) {
            auto* object = static_cast<ObjectDecl*>(typeDecl->taskObjects.front().get());
            if (!object->symbols.empty()) {
                typeDecl->declaredType->protectedObject = object->symbols.front();
            }
        }
        break;
    }
    case DeclKind::Subtype:
        analyzeSubtypeDecl(static_cast<SubtypeDecl*>(decl), scope);
        break;
    case DeclKind::SubprogramDeclaration: {
        auto* subprogram = static_cast<SubprogramDecl*>(decl);
        if (subprogram->renamedName != nullptr) {
            analyzeSubprogramRenaming(subprogram, scope);
        } else {
            subprogram->symbol = declareSubprogram(subprogram->spec, scope, false);
        }
        break;
    }
    case DeclKind::SubprogramBody:
        analyzeSubprogramBody(static_cast<SubprogramBody*>(decl), scope);
        break;
    case DeclKind::PackageSpecification:
        analyzePackageSpec(static_cast<PackageSpecDecl*>(decl), scope);
        break;
    case DeclKind::PackageBody:
        analyzePackageBody(static_cast<PackageBodyDecl*>(decl), scope);
        break;
    case DeclKind::Use:
        analyzeUseClause(*static_cast<UseDecl*>(decl), scope);
        break;
    case DeclKind::GenericDeclaration:
        analyzeGenericDecl(static_cast<GenericDecl*>(decl), scope);
        break;
    case DeclKind::GenericInstantiation:
        analyzeGenericInstantiation(static_cast<GenericInstantiationDecl*>(decl), scope);
        break;
    case DeclKind::Exception:
        analyzeExceptionDecl(static_cast<ExceptionDecl*>(decl), scope);
        break;
    case DeclKind::Pragma:
        analyzePragma(static_cast<PragmaDecl*>(decl), scope);
        break;
    case DeclKind::Representation:
        analyzeRepresentation(static_cast<RepresentationDecl*>(decl), scope);
        break;
    }
    m_scalarConstraintDeclaration = savedConstraintDeclaration;
    m_genericBodyContext = savedGenericBody;
    m_genericInstanceBody = savedInstanceBody;
}

void Sema::analyzeObjectDecl(ObjectDecl* decl, Scope* scope)
{
    Type* type = resolveSubtypeIndication(decl->subtype.get(), scope,
                                          true, !decl->isRenaming);

    checkCompleteView(type, decl->location, "an object declaration");
    if (type != nullptr && !decl->isRenaming && hasIncompleteParts(type)
        && !(decl->isConstant && !decl->initializer)) {
        m_diagnostics.error(decl->location, "an object requires the full type to be completed before its declaration");
        return;
    }
    if (unknownDiscriminantView(type) && !decl->initializer && !decl->isRenaming && !decl->isConstant) {
        m_diagnostics.error(decl->location, "an object of an unknown-discriminant view requires an initializer");
        return;
    }
    bool ownsScalarConstraint = type != nullptr && std::find(decl->scalarConstraints.begin(),
        decl->scalarConstraints.end(), type) != decl->scalarConstraints.end();
    if (ownsScalarConstraint) { type->scalarObjectConstraint = true; }
    bool ownsDiscriminantConstraint = type != nullptr && std::find(decl->discriminantConstraints.begin(),
        decl->discriminantConstraints.end(), type) != decl->discriminantConstraints.end();
    if (ownsDiscriminantConstraint) { type->discriminantObjectConstraint = true; }

    if (hasTasks(type) && !m_packages.empty()) {
        m_packages.back()->containsTasks = true;
    }
    if (type != nullptr && type->tagged && type->classRoot == nullptr) {
        rootType(type)->dispatchFrozen = true;
    }

    if (type != nullptr && !decl->isRenaming) {
        if (abstractView(type)) {
            m_diagnostics.error(decl->location, "an abstract type cannot be used to create an object");
        }
    }

    if (type != nullptr && type->classRoot != nullptr && !decl->isRenaming && !decl->initializer) {
        m_diagnostics.error(decl->location, "a class-wide object requires an initializer");
        return;
    }
    if (decl->isRenaming) {
        Type* targetType = analyzeExpr(decl->initializer.get(), scope, type);
        int view = objectView(decl->initializer.get());
        if (view == 0) {
            m_diagnostics.error(decl->location, "an object renaming requires an object name");
            return;
        }
        if (type == nullptr || targetType == nullptr || (rootType(type) != rootType(targetType)
            && !(type->anonymousAccess && SemaSupport::profileTypeMatches(type, targetType)))) {
            m_diagnostics.error(decl->location, "renamed object is not compatible with the declared subtype");
            return;
        }
        if (decl->subtype->nullExclusion && !targetType->excludesNull) {
            m_diagnostics.error(decl->location, "a null-excluding renaming requires an object that excludes null");
        }
        // A renaming inherits the object's constraints and constant/variable view.
        type = targetType;
        decl->isConstant = view == 1;
    }

    if (type != nullptr && type->kind == TypeKind::Array && !type->constrained) {
        if (decl->subtype->indexLows.empty() && !decl->initializer && type->boundsSymbol == nullptr) {
            m_diagnostics.error(decl->location, "an unconstrained array object needs an initializer or index constraint");
        }
    }

    // Indefinite objects acquire their constraint from an explicit initializer.
    Type* record = baseType(type);
    if (record != nullptr && record->discriminantCount > 0 && !hasKnownDiscriminants(type)
        && !hasDefaultDiscriminants(type) && !decl->initializer && !decl->isRenaming) {
        m_diagnostics.error(decl->location, "an object of '" + record->name + "' has to fix its discriminants, as "
                                                + "in 'X : " + record->name + " (...)'");
    }

    if (decl->initializer && !decl->isRenaming) {
        if (hasIncompleteParts(type)) {
            m_deferredProtectedUses.push_back({type, decl->location});
        }
        if (hasProtected(type)) {
            m_diagnostics.error(decl->location, "objects containing protected parts cannot be copied or explicitly initialized yet");
        }
        if (hasTasks(type)) {
            m_diagnostics.error(decl->location, "objects containing tasks cannot be copied or explicitly initialized yet");
        }
        if (baseType(type) == m_exceptionOccurrenceType && !withinPackage(type->privateTo)) {
            m_diagnostics.error(decl->initializer->location, "an exception occurrence cannot be copied by initialization");
        }
        Type* valueType = analyzeExpr(decl->initializer.get(), scope, type);
        if (!hasProtected(type) && (hasLimitedParts(type) || hasLimitedParts(valueType))
            && !buildsLimitedResult(decl->initializer.get())) {
            m_diagnostics.error(decl->location, (hasLimitedControlledParts(type) ? "limited controlled objects cannot be copied" : "limited objects cannot be copied"));
        }
        if (!typesCompatible(type, valueType)) {
            m_diagnostics.error(decl->initializer->location,
                                "initial value is not compatible with the declared subtype");
        }
        adaptUniversal(decl->initializer.get(), type);
    }

    if (decl->isConstant && !decl->initializer) {
        // A constant with no value is a promise that the private part will give
        // it one, which lets a package name a constant of a type its users are
        // not shown the inside of.
        if (!m_inVisiblePart) {
            m_diagnostics.error(decl->location, "a constant needs a value, unless it is deferred to the private "
                                                "part of a package specification");
            return;
        }
        decl->awaitsValue = true;
    }

    for (std::size_t i = 0; i < decl->names.size(); ++i) {
        // The declaration in the private part gives a value to the constant the
        // visible part named rather than declaring a second one.
        Symbol* deferred = nullptr;
        for (Symbol* candidate : scope->lookupLocal(decl->namesLower[i])) {
            if (candidate->kind == SymbolKind::Object && candidate->awaitsValue) {
                deferred = candidate;
            }
        }
        if (deferred != nullptr && !decl->isRenaming) {
            if (decl->awaitsValue) {
                m_diagnostics.error(decl->location, "'" + decl->names[i] + "' has already been named here");
                continue;
            }
            if (rootType(deferred->type) != rootType(type)) {
                m_diagnostics.error(decl->location, "'" + decl->names[i] + "' was named as a '" + deferred->type->name
                                                        + "' in the visible part");
            } else if (isScalar(type) && !SemaSupport::staticallyMatches(deferred->type, type)) {
                m_diagnostics.error(decl->location,
                    "a deferred constant completion must statically match its declared scalar subtype");
            }
            deferred->awaitsValue = false;
            decl->symbols.push_back(deferred);
            continue;
        }

        if (!scope->lookupLocal(decl->namesLower[i]).empty()) {
            m_diagnostics.error(decl->location, "'" + decl->names[i] + "' has already been declared in this scope");
            continue;
        }

        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Object, decl->namesLower[i], decl->names[i]);
        symbol->type = type;
        if (ownsScalarConstraint && i != 0) {
            Type* distinct = m_types.makeSubtype(anonymousTypeName(), type, type->low, type->high);
            distinct->scalarLow = type->scalarLow;
            distinct->scalarHigh = type->scalarHigh;
            distinct->scalarConstraintBase = type->scalarConstraintBase;
            distinct->decimalConstraint = type->decimalConstraint;
            recordScalarConstraint(distinct, decl->subtype->location, true);
            symbol->type = distinct;
        }
        if (ownsDiscriminantConstraint && i != 0) {
            symbol->type = constrainDiscriminants(decl->subtype.get(), type->base, scope);
            symbol->type->discriminantObjectConstraint = true;
        }
        symbol->objectReference = decl->isRenaming;
        symbol->sharedAlias = decl->isRenaming ? decl->initializer.get() : nullptr;
        symbol->classWideObject = type != nullptr && type->classRoot != nullptr && !decl->isRenaming;
        symbol->awaitsValue = decl->awaitsValue;
        symbol->isConstant = decl->isConstant;
        symbol->constantInitializer = decl->isConstant ? decl->initializer.get() : nullptr;
        symbol->aliased = decl->aliased;
        symbol->location = decl->location;
        symbol->owner = m_currentSubprogram;
        symbol->accessibilityLevel = m_accessibilityLevel;
        symbol->level = m_currentSubprogram != nullptr ? m_currentSubprogram->level : 0;
        symbol->isGlobal = m_currentSubprogram == nullptr;
        if (symbol->isGlobal) {
            symbol->qbeName = "$" + mangle(decl->namesLower[i]);
        }
        if (decl->isConstant && decl->initializer && !decl->isRenaming && type->scalarBoundsSymbol == nullptr
            && sharedTypeKind(type) == 0) {
            long long value = 0;
            double realValue = 0.0;
            if (isReal(type) && foldStaticReal(decl->initializer.get(), realValue)) {
                symbol->hasStaticValue = true;
                symbol->staticReal = realValue;
            } else if (foldStatic(decl->initializer.get(), value)) {
                symbol->hasStaticValue = true;
                symbol->staticValue = value;
            }
        }
        if (decl->initializer && type->kind != TypeKind::Float) {
            symbol->exactReal = decl->initializer->exactReal;
        }
        scope->add(symbol);
        decl->symbols.push_back(symbol);
    }
}

void Sema::analyzeNumberDecl(NumberDecl* decl, Scope* scope)
{
    Type* type = analyzeExpr(decl->value.get(), scope, nullptr);
    if (!isNumeric(type)) {
        m_diagnostics.error(decl->value->location, "the value of a named number must be numeric");
        return;
    }
    // Named numbers retain their universal type and exact value. Storage
    // width is chosen only when a use supplies a specific type.
    long long value = 0;
    double realValue = 0.0;
    bool real = isReal(type) || type->kind == TypeKind::Fixed;
    ExactReal exact;
    bool isStatic = type->kind == TypeKind::Fixed ? exactValue(decl->value.get(), exact)
        : real ? foldStaticReal(decl->value.get(), realValue) : exactValue(decl->value.get(), exact);
    if (type->kind == TypeKind::Fixed && isStatic) {
        realValue = exact.real();
    }
    if (!real && isStatic) {
        exact.integer(value);
    }
    if (!isStatic && decl->value->staticFailure == StaticFailure::None) {
        m_diagnostics.error(decl->value->location, "the value of a named number must be static");
    }

    for (std::size_t i = 0; i < decl->names.size(); ++i) {
        if (!scope->lookupLocal(decl->namesLower[i]).empty()) {
            m_diagnostics.error(decl->location, "'" + decl->names[i] + "' has already been declared in this scope");
            continue;
        }

        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Number, decl->namesLower[i], decl->names[i]);
        symbol->type = real ? m_types.universalReal() : m_types.universalInteger();
        exactValue(decl->value.get(), symbol->exactReal);
        symbol->isConstant = true;
        symbol->hasStaticValue = isStatic;
        symbol->staticValue = value;
        symbol->staticReal = realValue;
        symbol->location = decl->location;
        scope->add(symbol);
        decl->symbols.push_back(symbol);
    }
}

static bool isPrimitive(Symbol* symbol)
{
    auto belongsTo = [&](Type* type) {
        type = rootType(type);
        return type != nullptr && std::any_of(type->primitives.begin(), type->primitives.end(), [&](Symbol* primitive) {
            return primitive == symbol || primitive->renamedSubprogram == symbol;
        });
    };
    if (belongsTo(controllingResultType(symbol->returnType))) {
        return true;
    }
    for (Symbol* parameter : symbol->parameters) {
        if (belongsTo(controllingParameterType(parameter->type))) {
            return true;
        }
    }
    return false;
}

void Sema::checkOverridingIndicator(Symbol* symbol, int indicator, const SourceLocation& location)
{
    if (!symbol->overrides && isPrimitive(symbol) && !operatorSymbol(symbol->name).empty()) {
        Scope* probes = m_symbolTable.createScope(symbol->declarationScope);
        std::vector<ExprPtr> expressions;
        std::vector<Expr*> operands;
        for (std::size_t i = 0; i < symbol->parameters.size(); ++i) {
            std::string name = "$instance_operand_" + std::to_string(i);
            Symbol* probe = m_symbolTable.createSymbol(SymbolKind::Object, name, name);
            probe->type = symbol->parameters[i]->type;
            probes->add(probe);
            auto operand = std::make_unique<IdentifierExpr>();
            operand->name = operand->lower = name;
            operands.push_back(operand.get());
            expressions.push_back(std::move(operand));
        }
        for (const OperatorCandidate& candidate : operatorCandidates(symbol->name, operands, probes, symbol->returnType)) {
            bool matches = candidate.symbol == nullptr && rootType(candidate.result) == rootType(symbol->returnType)
                && candidate.parameters.size() == symbol->parameters.size();
            for (std::size_t i = 0; matches && i < candidate.parameters.size(); ++i) {
                matches = rootType(candidate.parameters[i]) == rootType(symbol->parameters[i]->type)
                    && symbol->parameters[i]->mode == ParameterMode::In;
            }
            symbol->overrides = symbol->overrides || matches;
        }
    }
    if (indicator != 0 && !isPrimitive(symbol)) {
        m_diagnostics.error(location, "an overriding indicator requires a primitive operation");
    }
    if (indicator == 1 && !symbol->overrides) {
        m_diagnostics.error(location, "subprogram marked overriding does not override an inherited operation");
    } else if (indicator == -1 && symbol->overrides) {
        m_diagnostics.error(location, "subprogram marked not overriding overrides an inherited operation");
    }
    if (indicator == -1 && !symbol->overrides) { symbol->pendingOverride = -1; }
}

Symbol* Sema::declareSubprogram(SubprogramSpec& spec, Scope* scope, bool isBody, bool isFormal, Scope* profileScope)
{
    if (profileScope == nullptr) { profileScope = scope; }
    std::vector<Type*> parameterTypes;
    for (ParameterDecl& parameter : spec.parameters) {
        for (const ParameterDecl& preceding : spec.parameters) {
            if (&preceding == &parameter) {
                break;
            }
            if (preceding.lower == parameter.lower) {
                m_diagnostics.error(parameter.location, "duplicate parameter '" + parameter.name + "'");
                break;
            }
        }
        Type* parameterType = resolveSubtypeIndication(parameter.subtype.get(), profileScope);
        if (parameterType != nullptr && parameterType->anonymousAccess && parameterType->target != nullptr
            && parameterType->target->tagged && parameterType->target->classRoot == nullptr
            && rootType(parameterType->target)->declarationScope == scope
            && (scope == m_packageSpecScope || spec.overriding == 1 || (isFormal && spec.abstract))) {
            parameterType->excludesNull = true;
        }
        if (parameterType != nullptr && (!parameterType->tagged || (isBody && !spec.nullProcedure))) {
            checkCompleteView(parameterType, parameter.location, "this subprogram parameter");
        }
        parameterTypes.push_back(parameterType);
    }
    // The synthetic barrier profile always uses Standard.Boolean, independent
    // of source declarations hiding Boolean (or Standard) at this point.
    Type* returnType = spec.entryBarrier ? m_types.booleanType()
        : spec.isFunction ? resolveSubtypeIndication(spec.returnType.get(), profileScope) : nullptr;
    checkCompleteView(returnType, spec.location, "a function result");
    if (hasIncompleteParts(returnType)) {
        m_deferredProtectedUses.push_back({returnType, spec.location});
    }
    if (hasProtected(returnType)) {
        m_diagnostics.error(spec.location, "function results containing protected objects are not yet supported");
    }
    if (hasTasks(returnType)) {
        m_diagnostics.error(spec.location, "function results containing tasks are not yet supported");
    }

    if (!operatorSymbol(spec.lower).empty()) {
        bool unary = spec.lower == "abs" || spec.lower == "not";
        bool either = spec.lower == "+" || spec.lower == "-";
        bool valid = spec.isFunction && (unary ? spec.parameters.size() == 1
            : either ? (spec.parameters.size() == 1 || spec.parameters.size() == 2)
                     : spec.parameters.size() == 2);
        for (const ParameterDecl& parameter : spec.parameters) {
            valid = valid && parameter.mode == ParameterMode::In && !parameter.defaultValue;
        }
        if (!valid) {
            m_diagnostics.error(spec.location, "'" + spec.lower + "' requires "
                + (either ? "one or two" : unary ? "one" : "two") + " in parameters without defaults");
        }
        if (!isFormal && spec.lower == "/=" && m_types.isBoolean(returnType)) {
            m_diagnostics.error(spec.location, "a Boolean '/=' is implicitly declared by '=' and cannot be declared explicitly");
        }
    }
    bool overrides = false;
    for (Symbol* candidate : scope->lookupDeclaration(spec.lower)) {
        if (candidate->kind != SymbolKind::Subprogram || candidate->parameters.size() != parameterTypes.size()
            || !SemaSupport::profileTypeMatches(candidate->returnType, returnType)) {
            continue;
        }
        bool matches = true;
        for (std::size_t i = 0; i < parameterTypes.size(); ++i) {
            // Controlling access parameters exclude null implicitly, including
            // the profile of a completing body before checking its indicator.
            if (!isFormal && candidate->controllingType != nullptr && parameterTypes[i] != nullptr
                && parameterTypes[i]->anonymousAccess
                && controllingParameterType(parameterTypes[i]) == candidate->controllingType) {
                parameterTypes[i]->excludesNull = true;
            }
            matches = matches && (rootType(candidate->parameters[i]->type) == rootType(parameterTypes[i])
                    || SemaSupport::staticallyMatches(candidate->parameters[i]->type, parameterTypes[i]));
        }
        if (!isFormal && matches
            && (candidate->inheritedFrom != nullptr || (isBody && candidate->overrides))) {
            overrides = true;
            bool subtypeConformant = candidate->returnType == returnType
                || SemaSupport::staticallyMatches(candidate->returnType, returnType);
            for (std::size_t i = 0; i < parameterTypes.size(); ++i) {
                subtypeConformant = subtypeConformant
                    && SemaSupport::staticallyMatches(candidate->parameters[i]->type, parameterTypes[i]);
                if (candidate->parameters[i]->mode != spec.parameters[i].mode) {
                    m_diagnostics.error(spec.location, "overriding parameters must have the inherited modes");
                }
            }
            if (candidate->dispatchSlot >= 0 && !subtypeConformant) {
                m_diagnostics.error(spec.location, "overriding dispatching profile must be subtype conformant with the inherited operation");
            }
        }
    }
    if (!isFormal && !overrides && !operatorSymbol(spec.lower).empty() && spec.isFunction) {
        // Predefined operators can only be overridden in the declarative
        // region of their type. An inner homograph merely hides them.
        bool localType = returnType != nullptr && rootType(returnType)->declarationScope == scope;
        for (Type* type : parameterTypes) {
            localType = localType || (type != nullptr && rootType(type)->declarationScope == scope);
        }
        Scope* probes = m_symbolTable.createScope(scope);
        std::vector<ExprPtr> expressions;
        std::vector<Expr*> operands;
        for (std::size_t i = 0; i < parameterTypes.size(); ++i) {
            std::string name = "$primitive_operand_" + std::to_string(i);
            Symbol* probe = m_symbolTable.createSymbol(SymbolKind::Object, name, name);
            probe->type = parameterTypes[i];
            probes->add(probe);
            auto operand = std::make_unique<IdentifierExpr>();
            operand->name = operand->lower = name;
            operands.push_back(operand.get());
            expressions.push_back(std::move(operand));
        }
        for (const OperatorCandidate& candidate : operatorCandidates(spec.lower, operands, probes, returnType)) {
            bool matches = candidate.symbol == nullptr && rootType(candidate.result) == rootType(returnType)
                && candidate.parameters.size() == parameterTypes.size();
            for (std::size_t i = 0; matches && i < parameterTypes.size(); ++i) {
                matches = rootType(candidate.parameters[i]) == rootType(parameterTypes[i])
                    && spec.parameters[i].mode == ParameterMode::In;
            }
            overrides = overrides || (localType && matches);
        }
    }
    auto incompletePrivate = [](Type* type) {
        type = rootType(type);
        return type != nullptr && type->isIncomplete && type->privateTo != nullptr;
    };
    bool pendingOverride = incompletePrivate(returnType);
    for (Type* parameter : parameterTypes) {
        pendingOverride = pendingOverride || incompletePrivate(parameter);
    }
    if (spec.overriding == 1 && !overrides) {
        m_diagnostics.error(spec.location, "subprogram marked overriding does not override an inherited operation");
    } else if (!pendingOverride && spec.overriding == -1 && overrides) {
        m_diagnostics.error(spec.location, "subprogram marked not overriding overrides an inherited operation");
    }
    Symbol* contractDeclaration = nullptr;
    if (m_replayContract != nullptr) {
        auto found = m_replayContract->subprograms.find(contractKey(spec.location));
        if (found != m_replayContract->subprograms.end()) {
            contractDeclaration = instanceSymbol(found->second);
        }
    }
    for (Symbol* candidate : scope->lookupDeclaration(spec.lower)) {
        if (candidate->inheritedFrom != nullptr || candidate->negatedEquality != nullptr) {
            continue;
        }
        bool replayOther = m_replayContract != nullptr && candidate != contractDeclaration;
        // RM 8.3(26): ordinary overloads may collapse in an instance;
        // dispatching operations in its visible part retain the homograph check.
        bool checkVisible = !m_genericInstanceBody && m_inVisiblePart && scope == m_packageSpecScope
            && candidate->declarationScope == scope && candidate->controllingType != nullptr;
        if (replayOther && !checkVisible) { continue; }
        bool matches = candidate->kind == SymbolKind::Subprogram
            && candidate->parameters.size() == parameterTypes.size()
            && SemaSupport::profileTypeMatches(candidate->returnType, returnType);
        for (std::size_t i = 0; matches && i < parameterTypes.size(); ++i) {
            matches = SemaSupport::profileTypeMatches(candidate->parameters[i]->type, parameterTypes[i]);
        }
        bool literalHomograph = candidate->kind == SymbolKind::EnumerationLiteral && parameterTypes.empty()
            && rootType(candidate->type) == rootType(returnType);
        if (literalHomograph || (candidate->kind != SymbolKind::Subprogram && candidate->kind != SymbolKind::EnumerationLiteral)) {
            m_diagnostics.error(spec.location, "'" + spec.name + "' has already been declared in this scope");
            break;
        }
        if (!matches) {
            continue;
        }
        if (replayOther) {
            m_diagnostics.error(spec.location, "instance visible part contains homographic declarations of '" + spec.name + "'");
            continue;
        }
        if (spec.overriding != 0 && !isPrimitive(candidate) && (candidate->protectedType == nullptr || isBody)) {
            m_diagnostics.error(spec.location, "an overriding indicator requires a primitive operation");
        }
        if (candidate->nullProcedure) {
            m_diagnostics.error(spec.location, "a null procedure cannot have a completion");
        } else if (!isBody || candidate->hasBody) {
            m_diagnostics.error(spec.location, "'" + spec.name + "' has already been declared in this scope");
        } else {
            bool conforms = (returnType == nullptr && candidate->returnType == nullptr)
                || SemaSupport::staticallyMatches(candidate->returnType, returnType);
            for (std::size_t i = 0; i < parameterTypes.size(); ++i) {
                Symbol* parameter = candidate->parameters[i];
                if (!isFormal && candidate->controllingType != nullptr && parameterTypes[i] != nullptr
                    && parameterTypes[i]->anonymousAccess
                    && controllingParameterType(parameterTypes[i]) == candidate->controllingType) {
                    parameterTypes[i]->excludesNull = true;
                }
                conforms = conforms && parameter->name == spec.parameters[i].lower
                    && parameter->explicitNullExclusion == spec.parameters[i].subtype->nullExclusion
                    && parameter->mode == spec.parameters[i].mode
                    && SemaSupport::staticallyMatches(parameter->type, parameterTypes[i]);
                if (spec.parameters[i].defaultValue != nullptr) {
                    analyzeExpr(spec.parameters[i].defaultValue.get(), profileScope, parameterTypes[i]);
                    conforms = conforms && SemaSupport::conformingExpressions(parameter->defaultExpr,
                        spec.parameters[i].defaultValue.get());
                    if (candidate->controllingType != nullptr
                        && controllingParameterType(parameter->type) == candidate->controllingType) {
                        dispatchIndeterminate(spec.parameters[i].defaultValue.get(), candidate->controllingType);
                    }
                }
            }
            if (!conforms) {
                m_diagnostics.error(spec.location, "subprogram body does not conform to the previous declaration");
            }
        }
        if (candidate->entry != spec.entry) {
            m_diagnostics.error(spec.location, "entry and procedure declarations cannot complete one another");
        }
        if (isBody && candidate->abstract) {
            m_diagnostics.error(spec.location, "an abstract subprogram cannot have a body");
        }
        if (spec.overriding == -1 && !candidate->overrides) { candidate->pendingOverride = -1; }
        candidate->hasBody = candidate->hasBody || isBody;
        candidate->nullProcedure = candidate->nullProcedure || spec.nullProcedure;
        for (std::size_t i = 0; i < spec.parameters.size(); ++i) {
            spec.parameters[i].symbol = candidate->parameters[i];
        }
        if (m_recordContract != nullptr) {
            m_recordContract->subprograms[contractKey(spec.location)] = candidate;
        }
        return candidate;
    }

    Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Subprogram, spec.lower, spec.name);
    symbol->location = spec.location;
    symbol->returnType = returnType;
    symbol->pendingControlledResult = returnType != nullptr && rootType(returnType)->isIncomplete;
    symbol->hasBody = isBody;
    symbol->abstract = spec.abstract;
    symbol->nullProcedure = spec.nullProcedure;
    symbol->entry = spec.entry;
    symbol->requiresBody = !spec.abstract && !isFormal && m_recordContract == nullptr && m_instanceOwnerContract == nullptr;
    symbol->overrides = overrides;
    symbol->pendingOverride = spec.overriding == -1 && !overrides ? -1 : 0;
    symbol->declarationScope = scope;
    symbol->level = m_currentSubprogram != nullptr ? m_currentSubprogram->level + 1 : 0;
    symbol->owner = m_currentSubprogram;
    symbol->accessibilityLevel = m_accessibilityLevel;
    symbol->needsAccessibility = hasAccessDiscriminants(returnType);
    if (symbol->owner != nullptr) {
        // Every intervening lexical level needs a link, even when it has no
        // captured variables of its own. Defaults can read through that level.
        symbol->owner->needsFrame = true;
    }

    // Block scopes can also declare the same spelling. Reserve emitted names
    // across the compilation, so shadowing never creates duplicate QBE symbols.
    std::string name = "$" + mangle(spec.lower);
    std::size_t ordinal = ++m_subprogramNames[name];
    symbol->explicitNullExclusion = spec.returnType != nullptr && spec.returnType->nullExclusion;
    symbol->qbeName = name;
    if (ordinal > 1) {
        symbol->qbeName += "__" + std::to_string(ordinal);
    }

    for (std::size_t i = 0; i < spec.parameters.size(); ++i) {
        ParameterDecl& declaration = spec.parameters[i];
        Symbol* parameter = m_symbolTable.createSymbol(SymbolKind::Parameter, declaration.lower, declaration.name);
        parameter->type = parameterTypes[i];
        parameter->explicitNullExclusion = declaration.subtype->nullExclusion;
        parameter->mode = declaration.mode;
        parameter->location = declaration.location;
        parameter->owner = symbol;
        parameter->accessibilityLevel = symbol->accessibilityLevel + 1;
        parameter->level = symbol->level;
        parameter->isConstant = declaration.mode == ParameterMode::In;
        parameter->byReference = declaration.mode != ParameterMode::In || isComposite(parameterTypes[i]);
        if (declaration.defaultValue) {
            if (declaration.mode != ParameterMode::In) {
                m_diagnostics.error(declaration.location, "only an in parameter can have a default expression");
            }
            Symbol* savedSubprogram = m_currentSubprogram;
            m_currentSubprogram = symbol;
            Type* defaultType = analyzeExpr(declaration.defaultValue.get(), profileScope, parameterTypes[i]);
            m_currentSubprogram = savedSubprogram;
            if (!typesCompatible(parameterTypes[i], defaultType)) {
                m_diagnostics.error(declaration.defaultValue->location, "the default expression has an incompatible type");
            }
            adaptUniversal(declaration.defaultValue.get(), parameterTypes[i]);
            parameter->hasDefault = true;
            parameter->defaultExpr = declaration.defaultValue.get();
            parameter->defaultTokens = declaration.defaultTokens;
        }
        declaration.symbol = parameter;
        symbol->parameters.push_back(parameter);
    }

    scope->add(symbol);
    if (isFormal) {
        // Hide inherited homographs without replacing the controlling type's
        // primitive or dispatch table: this formal denotes a supplied view.
        for (Symbol* inherited : scope->lookupLocal(symbol->name)) {
            if (inherited->kind == SymbolKind::EnumerationLiteral && inherited->inheritedFrom != nullptr
                && symbol->parameters.empty() && SemaSupport::profileTypeMatches(inherited->type, symbol->returnType)) {
                scope->remove(inherited);
                continue;
            }
            if (inherited == symbol || inherited->kind != SymbolKind::Subprogram
                || (inherited->inheritedFrom == nullptr && inherited->genericFormalOrigin == nullptr)
                || inherited->parameters.size() != symbol->parameters.size()
                || !SemaSupport::profileTypeMatches(inherited->returnType, symbol->returnType)) { continue; }
            bool homograph = true;
            for (std::size_t i = 0; i < symbol->parameters.size(); ++i) {
                homograph = homograph && SemaSupport::profileTypeMatches(inherited->parameters[i]->type, symbol->parameters[i]->type);
            }
            if (homograph) { scope->remove(inherited); }
        }
    }
    if (!isFormal) { registerPrimitive(symbol, scope); }
    if (abstractView(returnType) && !symbol->abstract) {
        m_diagnostics.error(spec.location, "a function returning an abstract type must be abstract");
    }
    if (symbol->abstract && symbol->controllingType != nullptr && !symbol->controllingType->abstract) {
        m_diagnostics.error(spec.location, "an abstract primitive requires an abstract tagged type");
    }
    if (spec.overriding != 0 && !isPrimitive(symbol)) {
        m_diagnostics.error(spec.location, "an overriding indicator requires a primitive operation");
    }
    if (spec.lower == "=" && m_types.isBoolean(returnType)) {
        Symbol* complement = m_symbolTable.createSymbol(SymbolKind::Subprogram, "/=", "/=");
        complement->parameters = symbol->parameters;
        complement->returnType = returnType;
        complement->negatedEquality = symbol;
        complement->abstract = symbol->abstract;
        complement->overrides = symbol->overrides;
        complement->hasBody = true;
        complement->location = symbol->location;
        complement->owner = symbol->owner;
        complement->declarationScope = symbol->declarationScope;
        complement->level = symbol->level;
        scope->add(complement);
        if (!isFormal) { registerPrimitive(complement, scope); }
    }
    if (m_recordContract != nullptr) {
        m_recordContract->subprograms[contractKey(spec.location)] = symbol;
    }
    return symbol;
}

void Sema::analyzeSubprogramBody(SubprogramBody* body, Scope* scope)
{
    if (body->taskBody) {
        Symbol* task = lookupName(body->taskName, scope);
        if (task == nullptr || task->type == nullptr || task->type->taskBody == nullptr
            || (task->kind != SymbolKind::TypeName && task->kind != SymbolKind::Object)
            || task->type->taskBody->name != "__task_" + body->taskName) {
            m_diagnostics.error(body->location, "task body requires a task declaration in the same declarative region");
            return;
        }
        body->spec.lower = task->type->taskBody->name;
        auto declarations = scope->lookupDeclaration(body->spec.lower);
        if (std::find(declarations.begin(), declarations.end(), task->type->taskBody) == declarations.end()) {
            m_diagnostics.error(body->location, "task body must complete a task declared in the same declarative region");
            return;
        }
    }
    // Attach a separately encountered body and check it in the formal environment.
    Symbol* named = lookupName(body->spec.lower, scope);
    if (named != nullptr && named->kind == SymbolKind::Generic && named->generic != nullptr
        && !named->generic->isPackage) {
        if (named->hasBody) {
            m_diagnostics.error(body->location, "generic '" + named->displayName + "' already has a body");
            return;
        }
        if (body->isNull) {
            m_diagnostics.error(body->location, "a null procedure cannot complete a generic subprogram");
            return;
        }
        body->genericBody = named;
        named->hasBody = true;
        std::vector<Token>& tokens = named->generic->tokens;
        Token endOfFile = tokens.back();
        tokens.pop_back();
        tokens.insert(tokens.end(), body->tokens.begin(), body->tokens.end());
        tokens.push_back(endOfFile);
        checkGenericContract(named->generic, scope);
        return;
    }

    Scope* context = subunitScope(body->subunit, scope);
    Symbol* stubSymbol = nullptr;
    if (body->subunit && !body->subunit->stubTokens.empty()) {
        Parser parser(body->subunit->stubTokens, m_diagnostics);
        auto declarations = parser.parseDeclarations();
        if (declarations.size() == 1 && declarations.front()->kind == DeclKind::SubprogramDeclaration) {
            // Keep the stub AST alive: symbols borrow defaults and subtype expressions.
            auto* stub = static_cast<SubprogramDecl*>(declarations.front().get());
            // Generic expansion renames the outer unit while retaining locations.
            stub->spec.name = body->spec.name;
            stub->spec.lower = body->spec.lower;
            stubSymbol = declareSubprogram(stub->spec, scope, true);
            stubSymbol->hasBody = false;
            body->stubDeclaration = std::move(declarations.front());
        }
    }
    Symbol* symbol = declareSubprogram(body->spec, scope, true, false, context);
    if (stubSymbol != nullptr && symbol != stubSymbol) {
        m_diagnostics.error(body->location, "subunit profile does not conform to its body stub");
        return;
    }
    if (scope == m_protectedBodyScope && m_currentProtectedType != nullptr) {
        declareProtectedOperation(symbol, m_currentProtectedType);
    }
    scope->m_publicChildPart = false;
    symbol->hasBody = true;
    body->symbol = symbol;
    symbol->entryBarrierFunction = body->spec.entryBarrier;
    if (body->taskBody && symbol->taskIdentity == nullptr) {
        Symbol* identity = m_symbolTable.createSymbol(SymbolKind::Object, "__task_identity", "__task_identity");
        identity->type = m_taskIdType;
        identity->owner = symbol;
        identity->level = symbol->level;
        identity->isConstant = true;
        symbol->taskIdentity = identity;
    }

    Scope* inner = m_symbolTable.createScope(context);
    symbol->constructScope = inner;
    inner->m_privateRegion = true;
    for (std::size_t i = 0; i < body->spec.parameters.size(); ++i) {
        Symbol* parameter = symbol->parameters[i];
        body->spec.parameters[i].symbol = parameter;
        inner->add(parameter);
    }

    Symbol* savedSubprogram = m_currentSubprogram;
    m_currentSubprogram = symbol;
    int savedAccessibility = m_accessibilityLevel;
    m_accessibilityLevel = symbol->accessibilityLevel + 1;
    int savedHandlerDepth = m_handlerDepth;
    m_handlerDepth = 0;
    m_namePrefix.push_back(symbol->name);

    analyzeDeclarativePart(body->declarations, inner);
    reportMissingBodies(body->declarations);
    analyzeBodyStatements(body->body, body->handlers, inner, body->spec.lower, symbol);

    m_namePrefix.pop_back();
    m_currentSubprogram = savedSubprogram;
    m_accessibilityLevel = savedAccessibility;
    m_handlerDepth = savedHandlerDepth;
}

void Sema::analyzeExceptionDecl(ExceptionDecl* decl, Scope* scope)
{
    // A renaming has to name the very same exception, identity being what a
    // handler matches on.
    std::string renamed;
    if (!decl->renamesLower.empty()) {
        Symbol* target = lookupName(decl->renamesLower, scope);
        if (target == nullptr || target->kind != SymbolKind::Exception) {
            m_diagnostics.error(decl->location, "'" + decl->renames + "' is not an exception");
            return;
        }
        renamed = target->exceptionObject;
    }

    // The run time raises the input output exceptions itself, so those stand
    // for objects it owns rather than ones emitted from this declaration.
    bool predefined = m_namePrefix.size() == 2 && m_namePrefix[0] == "ada" && (m_namePrefix[1] == "io_exceptions" || m_namePrefix[1] == "tags");

    for (std::size_t i = 0; i < decl->names.size(); ++i) {
        if (!scope->lookupLocal(decl->namesLower[i]).empty()) {
            m_diagnostics.error(decl->location, "'" + decl->names[i] + "' has already been declared in this scope");
            continue;
        }

        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Exception, decl->namesLower[i], decl->names[i]);
        if (!renamed.empty()) {
            symbol->exceptionObject = renamed;
        } else if (predefined) {
            symbol->exceptionObject = "$__ada_exc_" + decl->namesLower[i];
        } else {
            symbol->exceptionObject = "$ada_exc." + mangle(decl->namesLower[i]);
            m_unitExceptions[m_currentUnit].push_back(symbol);
        }
        symbol->location = decl->location;
        scope->add(symbol);
        decl->symbols.push_back(symbol);
    }
}

// pragma Import ties the declaration just given to an entry point in the C run
// time.  It applies to the most recent declaration of the name, so that the
// overloads of a subprogram can each name the routine that carries them out.
void Sema::analyzePragma(PragmaDecl* decl, Scope* scope)
{
    std::vector<Symbol*> candidates = scope->lookupLocal(decl->entityLower);
    if (decl->lower == "atomic" || decl->lower == "volatile") {
        Symbol* entity = candidates.size() == 1 ? candidates.front() : nullptr;
        bool object = entity != nullptr && entity->kind == SymbolKind::Object
            && !entity->objectReference && !entity->genericObject && entity->protectedField < 0;
        bool typeName = entity != nullptr && entity->kind == SymbolKind::TypeName
            && entity->type != nullptr && !entity->type->isSubtype;
        if ((!object && !typeName) || entity->type == nullptr) {
            m_diagnostics.error(decl->location, "shared-variable pragma requires an object or full type declared here");
            return;
        }
        Type* type = entity->type;
        long long size = typeSize(type);
        if (type->isIncomplete || !representationVisible(type) || !isScalar(type)
            || (size != 1 && size != 2 && size != 4 && size != 8) || typeAlignment(type) < size) {
            m_diagnostics.error(decl->location, "Atomic and Volatile currently require a complete, naturally aligned 8/16/32/64-bit scalar");
            return;
        }
        if (object && entity->isConstant) {
            m_diagnostics.error(decl->location, "shared-variable pragmas on constants require object Import, which is not yet supported");
            return;
        }
        if (typeName) {
            for (const auto& symbol : m_symbolTable.symbols()) {
                Type* used = rootType(symbol->type);
                while (used != nullptr && used != type) {
                    used = used->parentType;
                }
                if ((symbol->kind == SymbolKind::Object || symbol->kind == SymbolKind::Parameter)
                    && used == type) {
                    m_diagnostics.error(decl->location, "shared type pragma must precede objects and parameter profiles using the type");
                    return;
                }
            }
        }
        int& shared = typeName ? type->shared : entity->shared;
        shared = std::max(shared, decl->lower == "atomic" ? 2 : 1);
        return;
    }
    if (decl->lower == "convention") {
        Symbol* entity = candidates.size() == 1 ? candidates.front() : nullptr;
        Type* type = entity != nullptr ? entity->type : nullptr;
        if (entity == nullptr || entity->kind != SymbolKind::TypeName || type == nullptr
            || type->isSubtype || type->accessProfile == nullptr || type->anonymousAccess) {
            m_diagnostics.error(decl->location, "pragma Convention currently requires a named access-to-subprogram type");
            return;
        }
        if (type->accessConventionFrozen || type->accessConventionSpecified) {
            m_diagnostics.error(decl->location, "callback Convention must be specified once, before uses of the access type");
            return;
        }
        if (!decl->conventionAda) {
            auto supported = [](Type* value) {
                return value == nullptr || (isScalar(value) && !value->anonymousAccess
                    && (value->accessProfile == nullptr || value->accessConventionC));
            };
            bool valid = supported(type->accessProfile->returnType);
            for (Symbol* parameter : type->accessProfile->parameters) { valid &= supported(parameter->type); }
            if (!valid) {
                m_diagnostics.error(decl->location, "C callback profiles currently require scalar or named access parameters and results");
                return;
            }
        }
        type->accessConventionC = !decl->conventionAda;
        type->accessConventionSpecified = true;
        return;
    }
    Symbol* target = nullptr;
    for (Symbol* candidate : candidates) {
        if (candidate->kind == SymbolKind::Subprogram) {
            target = candidate;
        }
    }
    if (target == nullptr) {
        m_diagnostics.error(decl->location,
                            "pragma Import names '" + decl->entity + "', which is not a subprogram declared here");
        return;
    }

    if (target->addressTaken) {
        m_diagnostics.error(decl->location, "pragma Import must precede subprogram Access uses");
        return;
    }
    target->builtin = BuiltinKind::Runtime;
    if (needsFinalization(target->returnType)) {
        m_diagnostics.error(decl->location, "imported controlled function results are not yet supported");
    }
    target->runtimeSymbol = "$" + decl->linkName;
    target->importAda = decl->conventionAda;
    target->canRaise = true;
    target->hasBody = true;
}
