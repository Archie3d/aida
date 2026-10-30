#include "Sema.h"
#include "Categorization.h"

#include "Parser.h"
#include "SemaSupport.h"

#include <algorithm>
#include <limits>

using SemaSupport::staticallyMatches;

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

// Definiteness belongs to the actual subtype, not its unconstrained base.
bool isDefiniteArray(Type* type)
{
    return type == nullptr || type->kind != TypeKind::Array || type->constrained || type->boundsSymbol != nullptr;
}

bool matchesAccessFormal(Type* actual, Type* formal)
{
    if (actual->excludesNull != formal->excludesNull) { return false; }
    if (actual->accessProfile == nullptr && formal->accessProfile == nullptr) {
        return actual->accessConstant == formal->accessConstant
            && (!formal->generalAccess || actual->generalAccess)
            && staticallyMatches(actual->target, formal->target);
    }
    Symbol* actualProfile = actual->accessProfile;
    Symbol* formalProfile = formal->accessProfile;
    if (actualProfile == nullptr || formalProfile == nullptr || actual->accessProtected != formal->accessProtected
        || actualProfile->parameters.size() != formalProfile->parameters.size()
        || !SemaSupport::profileTypeMatches(actualProfile->returnType, formalProfile->returnType)) {
        return false;
    }
    // RM 12.5.4 requires mode conformance, which permits different subtype
    // constraints and parameter names in a designated profile.
    for (std::size_t i = 0; i < actualProfile->parameters.size(); ++i) {
        if (actualProfile->parameters[i]->mode != formalProfile->parameters[i]->mode
            || !SemaSupport::profileTypeMatches(actualProfile->parameters[i]->type, formalProfile->parameters[i]->type)) {
            return false;
        }
    }
    return true;
}

bool matchesFormalProfile(Symbol* actual, Symbol* formal)
{
    if (actual->kind == SymbolKind::EnumerationLiteral) {
        return formal->parameters.empty() && formal->returnType != nullptr
            && SemaSupport::profileTypeMatches(actual->type, formal->returnType);
    }
    if (actual->kind != SymbolKind::Subprogram || actual->parameters.size() != formal->parameters.size()
        || !SemaSupport::profileTypeMatches(actual->returnType, formal->returnType)) { return false; }
    if (formal->explicitNullExclusion && !actual->returnType->excludesNull) { return false; }
    for (std::size_t i = 0; i < formal->parameters.size(); ++i) {
        if (actual->parameters[i]->mode != formal->parameters[i]->mode
            || !SemaSupport::profileTypeMatches(actual->parameters[i]->type, formal->parameters[i]->type)
            || (formal->parameters[i]->explicitNullExclusion && !actual->parameters[i]->type->excludesNull)) { return false; }
    }
    return true;
}

Symbol* dispatchingOperation(Symbol* symbol)
{
    while (symbol != nullptr) {
        if (symbol->controllingType != nullptr && !symbol->classWideWrapper
            && (symbol->dispatchSlot >= 0 || (symbol->genericFormalSubprogram && symbol->abstract))) { return symbol; }
        symbol = symbol->renamedSubprogram;
    }
    return nullptr;
}

bool matchesAbstractActual(Symbol* candidate, Type* controlling, bool classwideReplay)
{
    Symbol* operation = dispatchingOperation(candidate);
    return operation != nullptr && (operation->controllingType == controlling
        || (classwideReplay && candidate->classWideWrapper && candidate->controllingType == controlling));
}

Symbol* formalOperationImplementation(Symbol* operation, Type* actual)
{
    Type* specific = actual->classRoot != nullptr ? actual->classRoot : actual;
    const auto& slots = rootType(specific)->dispatchSlots;
    if (operation->interfaceOrigins.empty()) {
        return operation->dispatchSlot >= 0 && static_cast<std::size_t>(operation->dispatchSlot) < slots.size()
            ? slots[operation->dispatchSlot] : nullptr;
    }
    auto originOf = [](Symbol* symbol) {
        while (symbol->renamedSubprogram != nullptr || symbol->inheritedFrom != nullptr) {
            symbol = symbol->renamedSubprogram != nullptr ? symbol->renamedSubprogram : symbol->inheritedFrom;
        }
        return symbol;
    };
    Symbol* implementation = nullptr;
    for (Symbol* origin : operation->interfaceOrigins) {
        Symbol* found = nullptr;
        for (Symbol* candidate : slots) {
            bool matches = candidate == origin || originOf(candidate) == originOf(origin);
            for (Symbol* candidateOrigin : candidate->interfaceOrigins) {
                matches = matches || candidateOrigin == origin || originOf(candidateOrigin) == originOf(origin);
            }
            if (matches) { found = candidate; break; }
        }
        if (found == nullptr || (implementation != nullptr && implementation != found)) { return nullptr; }
        implementation = found;
    }
    return implementation;
}

// Restrict reference actuals to object names; calls can also denote indexed
// components, whose prefix must itself be a variable.
bool isGenericVariable(Expr* expr)
{
    if (expr->kind == ExprKind::Identifier) {
        Symbol* symbol = static_cast<IdentifierExpr*>(expr)->symbol;
        return symbol != nullptr && ((symbol->kind == SymbolKind::Object && !symbol->isConstant)
            || (symbol->kind == SymbolKind::Parameter && symbol->mode != ParameterMode::In));
    }
    if (expr->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(expr);
        if (selected->symbol != nullptr) {
            return selected->symbol->kind == SymbolKind::Object && !selected->symbol->isConstant;
        }
        return (baseType(selected->prefix->type) != nullptr
            && baseType(selected->prefix->type)->kind == TypeKind::Access)
            || isGenericVariable(selected->prefix.get());
    }
    if (expr->kind == ExprKind::Call) {
        auto* call = static_cast<CallExpr*>(expr);
        return (call->form == CallForm::Indexing || call->form == CallForm::Slice)
            && isGenericVariable(call->callee.get());
    }
    return false;
}

bool requiresGenericBody(const DeclList& declarations)
{
    for (const auto& declaration : declarations) {
        if (declaration->kind == DeclKind::SubprogramDeclaration) {
            Symbol* symbol = static_cast<SubprogramDecl*>(declaration.get())->symbol;
            if (symbol != nullptr && !symbol->hasBody) {
                return true;
            }
        } else if (declaration->kind == DeclKind::Type) {
            auto* type = static_cast<TypeDecl*>(declaration.get());
            if (requiresGenericBody(type->protectedDeclarations)) {
                return true;
            }
        } else if (declaration->kind == DeclKind::PackageSpecification) {
            auto* package = static_cast<PackageSpecDecl*>(declaration.get());
            if (requiresGenericBody(package->publicPart) || requiresGenericBody(package->privatePart)) {
                return true;
            }
        } else if (declaration->kind == DeclKind::GenericDeclaration) {
            auto* generic = static_cast<GenericDecl*>(declaration.get());
            if (!generic->isPackage && generic->symbol != nullptr && !generic->symbol->hasBody) {
                return true;
            }
        }
    }
    return false;
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
    if (!decl->renamesLower.empty()) {
        decl->symbol = declareUnitRenaming(decl->name, decl->lower, decl->renames, decl->renamesLower,
            decl->location, scope, true, decl->isPackage, decl->isFunction);
        return;
    }
    // Register the generic before checking its formal contract.
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

    if (!target->lookupLocal(lower).empty()) {
        m_diagnostics.error(decl->location, "'" + name + "' has already been declared in this scope");
        return;
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
                defaultLower = operatorSymbol(Unicode::utf8(static_cast<StringLiteralExpr*>(value)->value));
            }
            formal.defaultUsesFormal = std::find(preceding.begin(), preceding.end(), defaultLower) != preceding.end();
            formal.defaultCandidates = value->kind == ExprKind::StringLiteral
                ? target->lookup(defaultLower) : expressionNames(value, target);
        }
        preceding.push_back(formal.lower);
    }

    Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Generic, lower, name);
    symbol->location = decl->location;
    symbol->generic = decl;
    symbol->hasBody = decl->hasBody;
    symbol->requiresBody = m_recordContract == nullptr && m_instanceOwnerContract == nullptr;
    symbol->owner = m_currentSubprogram;
    symbol->level = m_currentSubprogram != nullptr ? m_currentSubprogram->level + 1 : 0;
    symbol->qbeName = "$" + mangle(lower) + "__generic";
    std::size_t ordinal = ++m_subprogramNames[symbol->qbeName];
    if (ordinal > 1) {
        symbol->qbeName += "__" + std::to_string(ordinal);
    }
    if (symbol->owner != nullptr) {
        symbol->owner->needsFrame = true;
    }
    decl->symbol = symbol;
    target->add(symbol);
    checkGenericContract(decl, scope == m_unitScope ? scope : target);
}

std::string Sema::contractKey(const SourceLocation& location)
{
    return std::to_string(location.file) + ":" + std::to_string(location.line) + ":" + std::to_string(location.column);
}

std::string Sema::nameContractKey(Expr* expr)
{
    std::string name;
    if (expr->kind == ExprKind::Identifier) {
        name = static_cast<IdentifierExpr*>(expr)->lower;
    } else if (expr->kind == ExprKind::Selected) {
        name = static_cast<SelectedExpr*>(expr)->selectorLower;
    } else if (expr->kind == ExprKind::CharacterLiteral) {
        name = "'" + Unicode::encode(static_cast<CharacterLiteralExpr*>(expr)->value) + "'";
    }
    return contractKey(expr->location) + ":" + std::to_string(static_cast<int>(expr->kind)) + ":" + name;
}

std::string Sema::operatorContractKey(const std::string& name, const std::vector<Expr*>& operands)
{
    std::string key = name;
    for (Expr* operand : operands) {
        key += ":" + contractKey(operand->location);
    }
    return key;
}

Symbol* Sema::instanceSymbol(Symbol* symbol)
{
    if (symbol == nullptr || m_replayContract == nullptr) {
        return symbol;
    }
    auto copied = m_instanceCopies.find(symbol);
    if (copied != m_instanceCopies.end()) {
        return copied->second;
    }
    const auto& symbols = m_symbolTable.symbols();
    if (!m_replayContract->localSymbols.contains(symbol)) {
        return symbol;
    }
    // Implicit inherited overloads share the derived declaration's location.
    // Their originating operation distinguishes them during contract replay.
    Symbol* declarationOrigin = symbol->genericFormalOrigin != nullptr ? symbol->genericFormalOrigin : symbol->inheritedFrom;
    Symbol* origin = declarationOrigin != nullptr ? instanceSymbol(declarationOrigin) : nullptr;
    for (std::size_t i = symbols.size(); i > m_instanceFirstSymbol; --i) {
        Symbol* candidate = symbols[i - 1].get();
        Symbol* candidateOrigin = candidate->genericFormalOrigin != nullptr
            ? candidate->genericFormalOrigin : candidate->inheritedFrom;
        if (candidate->kind == symbol->kind && candidateOrigin == origin
            && (candidate->negatedEquality == nullptr) == (symbol->negatedEquality == nullptr)
            && (candidate->name == symbol->name || (symbol->name == m_replayContract->unitName
                && (symbol->kind == SymbolKind::Subprogram || symbol->kind == SymbolKind::Package)))
            && contractKey(candidate->location) == contractKey(symbol->location)) {
            return candidate;
        }
    }
    return nullptr;
}

std::vector<Symbol*> Sema::contractNames(Expr* expr, std::vector<Symbol*> candidates)
{
    if (m_replayContract != nullptr) {
        auto found = m_replayContract->names.find(nameContractKey(expr));
        if (found != m_replayContract->names.end()) {
            if (Symbol* symbol = instanceSymbol(found->second)) {
                return { symbol };
            }
            m_diagnostics.error(expr->location, "cannot map a resolved generic name to its instance");
            return {};
        }
    }
    return candidates;
}

void Sema::recordContractName(Expr* expr, Symbol* symbol)
{
    if (m_recordContract != nullptr && symbol != nullptr) {
        m_recordContract->names[nameContractKey(expr)] = symbol;
    }
}

void Sema::checkGenericContract(GenericDecl* decl, Scope* scope, GenericInstantiationDecl* partial,
                                Symbol* packageTemplate, Scope* actualScope)
{
    auto& owned = m_genericContracts[decl];
    bool continuation = owned != nullptr;
    if (!continuation) {
        owned = std::make_unique<GenericContract>();
        owned->environment = scope;
        owned->bindings = m_symbolTable.createScope(scope);
        owned->bindings->m_genericBindings = true;
        owned->unitName = decl->lower.substr(decl->lower.find_last_of('.') + 1);
        owned->valid = true;
        owned->checkMask = m_suppressedChecks;
        // A nested declaration is rechecked in each enclosing instance. Keep
        // its source-level discriminant identities and attribute legality even
        // when the enclosing formal has already been replaced by its actual.
        if (m_replayContract != nullptr) {
            for (Symbol* original : m_replayContract->localSymbols) {
                if (original->kind != SymbolKind::Generic || original->generic == nullptr
                    || contractKey(original->generic->location) != contractKey(decl->location)) {
                    continue;
                }
                auto source = m_genericContracts.find(original->generic);
                if (source != m_genericContracts.end()) {
                    owned->source = source->second.get();
                    owned->discriminantSelections = source->second->discriminantSelections;
                    owned->discriminantConstraints = source->second->discriminantConstraints;
                    owned->definiteAttributes = source->second->definiteAttributes;
                    owned->classAttributes = source->second->classAttributes;
                    owned->abstractFormals = source->second->abstractFormals;
                    break;
                }
            }
        }
    }
    GenericContract* contract = owned.get();
    ScopedCheckPolicy contractChecks(m_suppressedChecks, m_suppressedChecks | contract->checkMask);
    std::size_t firstSymbol = m_symbolTable.symbols().size();
    if (!continuation) { contract->firstSymbol = firstSymbol; }
    Scope* bindings = contract->bindings;
    if (continuation && scope != bindings) {
        contract->environment = scope;
        bindings->setParent(scope);
    }
    auto* savedRecord = m_recordContract;
    auto* savedReplay = m_replayContract;
    auto savedNames = m_subprogramNames;
    auto savedTagNames = m_tagNames;
    auto savedExceptions = m_unitExceptions;
    auto savedCopies = m_instanceCopies;
    std::size_t savedFirst = m_instanceFirstSymbol;
    if (contract->source != nullptr && savedReplay != nullptr) {
        // References outside the nested template still denote declarations
        // in the enclosing template. Map those before switching contracts.
        for (Symbol* original : savedReplay->localSymbols) {
            if (Symbol* copy = instanceSymbol(original)) { m_instanceCopies[original] = copy; }
        }
    }
    m_recordContract = contract;
    m_replayContract = contract->source;
    m_instanceFirstSymbol = contract->firstSymbol;
    int errors = m_diagnostics.errorCount();
    std::vector<Expr*> packageActuals;
    if (partial != nullptr && !associateGenericActuals(partial, packageTemplate, packageActuals)) {
        contract->valid = false;
    }
    std::size_t formalIndex = 0;
    for (GenericFormal& formal : decl->formals) {
        if (continuation || (partial != nullptr && !contract->valid)) {
            break;
        }
        applyGenericUses(decl, formalIndex++, bindings, contract->profiles);
        if (partial != nullptr && contract->valid && partial->packageMatches[formalIndex - 1]) {
            // Reuse ordinary binding for an explicit actual or a required
            // default. Boxed declarations are checked in the usual formal path.
            auto savedCopies = m_instanceCopies;
            for (const auto& [original, copy] : partial->packageCopies) { m_instanceCopies[original] = copy; }
            bool bound = bindGenericFormal(partial, packageTemplate, packageTemplate->generic->formals[formalIndex - 1],
                                            packageActuals[formalIndex - 1], bindings, actualScope, firstSymbol);
            m_instanceCopies = std::move(savedCopies);
            if (!bound) { contract->valid = false; }
            continue;
        }
        if (formal.kind == GenericFormalKind::PackageFormal) {
            formal.packageView = declareFormalPackage(formal, bindings, contract->profiles);
            continue;
        }
        if (formal.kind == GenericFormalKind::SubprogramFormal) {
            Parser parser(formal.subprogramTokens, m_diagnostics);
            DeclList profile = parser.parseDeclarations();
            if (profile.size() == 1) {
                auto* declaration = static_cast<SubprogramDecl*>(profile.front().get());
                // A default name is resolved before the declaration becomes
                // visible, so a same-named formal cannot capture its default.
                auto defaults = formal.defaultValue != nullptr
                    ? expressionNames(formal.defaultValue.get(), bindings) : std::vector<Symbol*> {};
                Symbol* attributeDefault = formal.defaultValue != nullptr && formal.defaultValue->kind == ExprKind::Attribute
                    ? attributeSubprogram(static_cast<AttributeExpr*>(formal.defaultValue.get()), bindings) : nullptr;
                declaration->spec.abstract = formal.abstract;
                declaration->symbol = declareSubprogram(declaration->spec, bindings, false, true);
                declaration->symbol->genericFormalSubprogram = true;
                if (!formal.abstract && std::any_of(defaults.begin(), defaults.end(), [](Symbol* candidate) {
                        return candidate->kind == SymbolKind::EnumerationLiteral;
                    })) {
                    std::vector<Symbol*> matching;
                    for (Symbol* candidate : defaults) {
                        if (matchesFormalProfile(candidate, declaration->symbol)) { matching.push_back(candidate); }
                    }
                    if (matching.size() != 1) {
                        m_diagnostics.error(formal.location, "enumeration-literal default does not uniquely match the generic formal function profile");
                    } else if (matching.front()->kind == SymbolKind::EnumerationLiteral) {
                        formal.literalDefault = matching.front();
                        recordContractName(formal.defaultValue.get(), formal.literalDefault);
                    }
                }
                if (attributeDefault != nullptr && !matchesFormalProfile(attributeDefault, declaration->symbol)) {
                    m_diagnostics.error(formal.location, "attribute default does not match the generic formal subprogram profile");
                }
                if (formal.abstract) {
                    analyzeAbstractFormalSubprogram(declaration->symbol,
                        contract->abstractFormals.contains(contractKey(declaration->symbol->location)));
                    contract->abstractFormals.insert(contractKey(declaration->symbol->location));
                    if (formal.defaultValue != nullptr) {
                        for (Symbol* candidate : defaults) {
                            if (matchesFormalProfile(candidate, declaration->symbol)
                                && matchesAbstractActual(candidate, declaration->symbol->controllingType,
                                    contract->source != nullptr)) {
                                if (formal.abstractDefault != nullptr) {
                                    m_diagnostics.error(formal.location, "ambiguous default for abstract formal subprogram");
                                    break;
                                }
                                formal.abstractDefault = candidate;
                            }
                        }
                        if (formal.abstractDefault == nullptr) {
                            m_diagnostics.error(formal.location, "abstract formal default must be a mode-conformant dispatching operation of its controlling type");
                        }
                    }
                }
                contract->profiles.push_back(std::move(profile.front()));
            }
            continue;
        }
        if (formal.kind == GenericFormalKind::TypeFormal) {
            Type* type = nullptr;
            if (!formal.discriminants.empty()) {
                Parser parser(formal.typeTokens, m_diagnostics);
                DeclList declarations = parser.parseDeclarations();
                if (declarations.size() == 1 && declarations.front()->kind == DeclKind::Type) {
                    type = analyzeFormalPrivateType(static_cast<TypeDecl*>(declarations.front().get()), formal, bindings, decl->symbol);
                    if (type != nullptr) { type->preelaborableInitialization = formal.preelaborableInitialization; }
                    Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, formal.lower, formal.name);
                    symbol->type = type;
                    symbol->location = formal.location;
                    bindings->add(symbol);
                    contract->profiles.push_back(std::move(declarations.front()));
                }
                continue;
            }
            if (formal.typeClass == FormalTypeClass::AccessType || formal.typeClass == FormalTypeClass::DerivedType
                || formal.typeClass == FormalTypeClass::InterfaceType) {
                Parser parser(formal.typeTokens, m_diagnostics);
                DeclList declarations = parser.parseDeclarations();
                if (declarations.size() == 1 && declarations.front()->kind == DeclKind::Type) {
                    auto* typeDecl = static_cast<TypeDecl*>(declarations.front().get());
                    if (formal.typeClass == FormalTypeClass::DerivedType) {
                        analyzeFormalDerivedType(typeDecl, formal, bindings, decl->symbol);
                    } else {
                        analyzeTypeDecl(typeDecl, bindings);
                        if (formal.typeClass == FormalTypeClass::InterfaceType && typeDecl->declaredType != nullptr) {
                            typeDecl->declaredType->formalDerived = true;
                        }
                    }
                    if (typeDecl->declaredType != nullptr) {
                        typeDecl->declaredType->genericFormal = true;
                        typeDecl->declaredType->preelaborableInitialization = formal.preelaborableInitialization;
                    }
                    contract->profiles.push_back(std::move(declarations.front()));
                }
                continue;
            }
            if (formal.typeClass == FormalTypeClass::ArrayType) {
                TypeDecl array;
                array.name = formal.name;
                array.lower = formal.lower;
                array.location = formal.location;
                array.definition = std::move(formal.arrayDefinition);
                analyzeTypeDecl(&array, bindings);
                if (array.declaredType != nullptr) { array.declaredType->genericFormal = true; }
                formal.arrayDefinition = std::move(array.definition);
                continue;
            }
            TypeKind kind = (formal.typeClass == FormalTypeClass::IntegerType || formal.typeClass == FormalTypeClass::ModularType) ? TypeKind::Integer
                : formal.typeClass == FormalTypeClass::FloatType ? TypeKind::Float
                : (formal.typeClass == FormalTypeClass::FixedType || formal.typeClass == FormalTypeClass::DecimalType) ? TypeKind::Fixed
                : formal.typeClass == FormalTypeClass::Discrete ? TypeKind::Enumeration : TypeKind::Record;
            type = m_types.create(kind, formal.name);
            type->genericFormal = true;
            type->preelaborableInitialization = formal.preelaborableInitialization;
            type->low = std::numeric_limits<int>::min();
            type->high = std::numeric_limits<int>::max();
            if (formal.typeClass == FormalTypeClass::ModularType) {
                type->modulus = (__int128)1 << 64;
                type->low = 0;
                type->high = type->modulus - 1;
            }
            type->digits = 6;
            if (formal.typeClass == FormalTypeClass::DecimalType) {
                type->fixedScale = ADA_FIXED_DECIMAL_TAG;
                type->digits = 18;
            }
            type->formalFixed = kind == TypeKind::Fixed;
            if (formal.typeClass == FormalTypeClass::Any) {
                type->privateTo = decl->symbol;
                type->isLimited = formal.limited;
                type->unknownDiscriminants = formal.unknownDiscriminants;
                type->tagged = type->privateViewTagged = formal.tagged;
                type->abstract = type->privateViewAbstract = formal.abstract;
            }
            Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, formal.lower, formal.name);
            symbol->type = type;
            symbol->location = formal.location;
            bindings->add(symbol);
        } else {
            Type* type = resolveSubtypeIndication(formal.subtype.get(), bindings);
            if (formal.mode == ParameterMode::In && abstractView(type)) {
                m_diagnostics.error(formal.location, "a generic formal in object cannot have an abstract type");
            }
            Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Object, formal.lower, formal.name);
            symbol->type = type;
            symbol->location = formal.location;
            symbol->isConstant = formal.mode == ParameterMode::In;
            symbol->genericObject = true;
            symbol->explicitNullExclusion = formal.subtype->nullExclusion;
            symbol->accessibilityLevel = m_accessibilityLevel;
            // Formal values are unknown while checking the contract. Symbolic
            // layouts are retained here and resolved when instantiating.
            if (formal.defaultValue) {
                Type* value = analyzeExpr(formal.defaultValue.get(), bindings, type);
                if (!typesCompatible(type, value)) {
                    m_diagnostics.error(formal.location, "generic formal object default has an incompatible type");
                } else {
                    checkGenericObjectActual(formal, formal.defaultValue.get(), type, value);
                }
            }
            bindings->add(symbol);
        }
    }
    if (partial != nullptr && !contract->valid) {
        m_recordContract = savedRecord;
        m_replayContract = savedReplay;
        m_instanceCopies = std::move(savedCopies);
        m_instanceFirstSymbol = savedFirst;
        m_subprogramNames = std::move(savedNames);
        m_tagNames = std::move(savedTagNames);
        m_unitExceptions = std::move(savedExceptions);
        return;
    }
    if (!continuation) { applyGenericUses(decl, decl->formals.size(), bindings, contract->profiles); }
    std::vector<Token> tokens(decl->tokens.begin() + static_cast<std::ptrdiff_t>(contract->tokenCount),
                              decl->tokens.end());
    Parser parser(tokens, m_diagnostics);
    DeclList declarations = parser.parseDeclarations();
    for (const DeclPtr& unit : declarations) {
        renameUnit(unit.get(), contract->unitName, contract->unitName, decl->lower);
    }
    for (const DeclPtr& unit : declarations) {
        // A template is checked under its own category. Ordinary instances do
        // not inherit a library-unit pragma (RM 10.1.5(7.1)).
        if (unit->kind == DeclKind::PackageSpecification) { unit->category = decl->category; }
    }
    analyzeDeclarativePart(declarations, bindings);
    for (DeclPtr& unit : declarations) {
        contract->declarations.push_back(std::move(unit));
    }
    const auto& symbols = m_symbolTable.symbols();
    for (std::size_t i = firstSymbol; i < symbols.size(); ++i) {
        contract->localSymbols.insert(symbols[i].get());
    }
    contract->tokenCount = decl->tokens.size() - 1;
    contract->valid = contract->valid && m_diagnostics.errorCount() == errors;
    m_recordContract = savedRecord;
    m_replayContract = savedReplay;
    m_instanceCopies = std::move(savedCopies);
    m_instanceFirstSymbol = savedFirst;
    m_subprogramNames = std::move(savedNames);
    m_tagNames = std::move(savedTagNames);
    m_unitExceptions = std::move(savedExceptions);
}

bool Sema::isDefiniteSubtype(Type* type) const
{
    return type != nullptr && type->classRoot == nullptr && !unknownDiscriminantView(type)
        && isDefiniteArray(type)
        && (type->kind != TypeKind::Record || type->discriminantCount == 0
            || hasDiscriminantConstraint(type) || hasDefaultDiscriminants(type));
}

Type* Sema::analyzeFormalPrivateType(TypeDecl* decl, const GenericFormal& formal, Scope* scope, Symbol* owner)
{
    Type* type = m_types.create(TypeKind::Record, formal.name);
    decl->declaredType = type;
    type->privateTo = owner;
    type->genericFormal = true;
    type->isLimited = formal.limited;
    type->tagged = type->privateViewTagged = formal.tagged;
    type->abstract = type->privateViewAbstract = formal.abstract;
    type->declarationScope = scope;
    layoutRecord(decl, decl->definition.get(), type, scope);
    type->privateDiscriminantCount = type->discriminantCount;
    return type;
}

bool Sema::matchesFormalDiscriminants(GenericInstantiationDecl* decl, Symbol* generic, const GenericFormal& formal,
                                      Type* actual, Scope* bindings, std::size_t firstSymbol, const SourceLocation& location)
{
    Parser parser(formal.typeTokens, m_diagnostics);
    DeclList declarations = parser.parseDeclarations();
    if (declarations.size() != 1 || declarations.front()->kind != DeclKind::Type) { return false; }
    auto* savedReplay = m_replayContract;
    auto* savedRecord = m_recordContract;
    std::size_t savedFirst = m_instanceFirstSymbol;
    m_replayContract = m_genericContracts.at(generic->generic).get();
    m_recordContract = nullptr;
    m_instanceFirstSymbol = firstSymbol;
    int errors = m_diagnostics.errorCount();
    Type* expected = analyzeFormalPrivateType(static_cast<TypeDecl*>(declarations.front().get()), formal,
        m_symbolTable.createScope(bindings), generic);
    m_replayContract = savedReplay;
    m_recordContract = savedRecord;
    m_instanceFirstSymbol = savedFirst;
    Type* record = baseType(actual);
    bool matches = errors == m_diagnostics.errorCount() && record->kind == TypeKind::Record
        && actual->classRoot == nullptr && !unknownDiscriminantView(actual) && !hasDiscriminantConstraint(actual)
        && record->discriminantCount == expected->discriminantCount;
    for (int i = 0; matches && i < expected->discriminantCount; ++i) {
        const FieldInfo& field = discriminantField(record, i);
        matches = fieldVisible(record, field) && staticallyMatches(field.type, discriminantField(expected, i).type);
    }
    decl->formalTypes.push_back(std::move(declarations.front()));
    if (!matches) {
        m_diagnostics.error(location, "actual subtype does not match discriminants of generic formal '" + formal.name + "'");
    }
    return matches;
}

void Sema::analyzeFormalDerivedType(TypeDecl* decl, const GenericFormal& formal, Scope* scope, Symbol* owner)
{
    TypeDefinition* definition = decl->definition.get();
    Type* parent = resolveSubtypeIndication(definition->parent.get(), scope, false, true);
    if (parent == nullptr) { return; }
    if (formal.unknownDiscriminants && parent->kind != TypeKind::Record) {
        m_diagnostics.error(decl->location, "a formal derived discriminant part requires a record ancestor");
        return;
    }
    bool requiresSynchronized = parent->interfaceKind >= InterfaceKind::Synchronized;
    bool allLimited = limitedView(parent);
    for (const auto& progenitor : definition->progenitors) {
        Type* type = resolveTypeName(progenitor->lower, scope, progenitor->location);
        if (type == nullptr) { return; }
        requiresSynchronized = requiresSynchronized || type->interfaceKind >= InterfaceKind::Synchronized;
        allLimited = allLimited && limitedView(type);
    }
    if (formal.synchronized && !parent->tagged) {
        m_diagnostics.error(decl->location, "a synchronized formal derived type requires a tagged ancestor");
        return;
    }
    if (formal.limited && !allLimited) {
        m_diagnostics.error(decl->location, "limited or synchronized formal derivation requires limited ancestors and progenitors");
        return;
    }
    if (requiresSynchronized && !formal.synchronized) {
        m_diagnostics.error(decl->location, "a synchronized interface ancestor or progenitor requires synchronized formal derivation");
        return;
    }
    definition->formalDerived = true;
    definition->formalSynchronized = formal.synchronized;
    if (parent->tagged != definition->privateExtension) {
        m_diagnostics.error(decl->location, "with private is required exactly for a tagged formal derived ancestor");
        return;
    }
    if (formal.abstract && !parent->tagged) {
        m_diagnostics.error(decl->location, "an abstract formal derived type requires a tagged ancestor");
        return;
    }
    // Build the inherited contract through ordinary derivation. Its private
    // extension is a complete formal view, with no package completion to await.
    bool privateExtension = definition->privateExtension;
    definition->privateExtension = false;
    analyzeTypeDecl(decl, scope);
    definition->privateExtension = privateExtension;
    Type* type = decl->declaredType;
    if (type == nullptr) { return; }
    type->formalDerived = true;
    type->unknownDiscriminants = formal.unknownDiscriminants;
    if (parent->tagged) {
        type->privateTo = owner;
        type->privateViewTagged = true;
        type->privateViewAbstract = formal.abstract;
        type->privateExtension = true;
        type->privateAncestor = rootType(parent);
        type->publicParentFieldCount = static_cast<int>(parent->fields.size());
        type->privateDiscriminantCount = parent->discriminantCount;
    } else if (formal.unknownDiscriminants) {
        type->unknownDiscriminantAncestor = true;
    }
    for (Symbol* operation : type->primitives) {
        if (operation->kind != SymbolKind::Subprogram) { continue; }
        // RM 6.3.1(8): unknown-discriminant tagged formal operations are
        // intrinsic even before an instance supplies a class-wide actual.
        operation->intrinsicRenaming = parent->tagged && formal.unknownDiscriminants;
        // A nonabstract formal relies on the actual's required overrides.
        // An abstract private extension has no known null-extension full view.
        operation->abstract = formal.abstract && (operation->abstract
            || controllingResultType(operation->returnType) == type);
    }
}

bool Sema::bindFormalDerivedType(GenericInstantiationDecl* decl, Symbol* generic, const GenericFormal& formal,
                                 Type*& actual, Scope* bindings, std::size_t firstSymbol, const SourceLocation& location)
{
    Parser parser(formal.typeTokens, m_diagnostics);
    DeclList declarations = parser.parseDeclarations();
    if (declarations.size() != 1 || declarations.front()->kind != DeclKind::Type) { return false; }
    auto* definition = static_cast<TypeDecl*>(declarations.front().get());
    Scope* matching = m_symbolTable.createScope(bindings);
    auto* savedReplay = m_replayContract;
    auto* savedRecord = m_recordContract;
    std::size_t savedFirst = m_instanceFirstSymbol;
    m_replayContract = m_genericContracts.at(generic->generic).get();
    m_recordContract = nullptr;
    m_instanceFirstSymbol = firstSymbol;
    int errors = m_diagnostics.errorCount();
    if (formal.typeClass == FormalTypeClass::InterfaceType) {
        analyzeTypeDecl(definition, matching);
    } else {
        analyzeFormalDerivedType(definition, formal, matching, generic);
    }
    m_replayContract = savedReplay;
    m_recordContract = savedRecord;
    m_instanceFirstSymbol = savedFirst;
    Type* view = definition->declaredType;
    if (view == nullptr || errors != m_diagnostics.errorCount()) { return false; }
    Type* parent = view->parentSubtype;
    Type* specificParent = parent != nullptr && parent->classRoot != nullptr ? parent->classRoot : parent;
    Type* specificActual = actual->classRoot != nullptr ? actual->classRoot : actual;
    bool matches = false;
    if (formal.typeClass == FormalTypeClass::InterfaceType) {
        matches = actual->interfaceKind == formal.interfaceKind && representationVisible(actual);
    } else if (parent->interfaceKind != InterfaceKind::None) {
        matches = taggedViewVisible(actual) && isTaggedAncestor(specificParent, specificActual);
    } else {
        Type* ancestor = baseType(specificActual);
        while (ancestor != nullptr && ancestor != baseType(specificParent)) {
            if (!ancestor->parentVisible && !representationVisible(ancestor)) {
                ancestor = ancestor->privateAncestor;
            } else {
                ancestor = ancestor->parentType;
            }
        }
        matches = ancestor != nullptr && (!parent->tagged || taggedViewVisible(actual));
    }
    if (formal.typeClass == FormalTypeClass::DerivedType) {
        matches = matches && (view->isLimited || !limitedView(actual));
        if (formal.synchronized) {
            matches = matches && (actual->synchronizedFormal || actual->interfaceKind >= InterfaceKind::Synchronized);
        }
    }
    for (Type* progenitor : view->interfaces) {
        matches = matches && isTaggedAncestor(progenitor, specificActual);
    }
    if (matches && parent != nullptr && !formal.unknownDiscriminants) {
        matches = !isDefiniteSubtype(parent) || isDefiniteSubtype(actual);
        if (parent->kind == TypeKind::Array) {
            Type normalized = *actual;
            normalized.base = baseType(parent);
            normalized.isSubtype = true;
            matches = matches && staticallyMatches(&normalized, parent);
        } else if (parent->kind == TypeKind::Record) {
            bool parentConstrained = parent->discriminantCount == 0 || hasDiscriminantConstraint(parent);
            bool actualConstrained = actual->discriminantCount == 0 || hasDiscriminantConstraint(actual);
            matches = matches && parentConstrained == actualConstrained;
            if (parentConstrained && parent->discriminantCount > 0) {
                Type normalized = *actual;
                normalized.base = baseType(parent);
                normalized.isSubtype = true;
                normalized.discriminantConstraintSource = actual;
                matches = matches && staticallyMatches(&normalized, parent);
            } else if (!parentConstrained) {
                matches = matches && actual->discriminantCount == parent->discriminantCount;
                for (int i = 0; matches && i < parent->discriminantCount; ++i) {
                    matches = staticallyMatches(discriminantField(actual, i).type, discriminantField(parent, i).type);
                }
            }
        } else if (parent->kind == TypeKind::Access) {
            matches = matches && actual->excludesNull == parent->excludesNull;
        } else if (parent->scalarBoundsSymbol != nullptr || actual->scalarBoundsSymbol != nullptr) {
            matches = matches && parent->scalarBoundsSymbol == actual->scalarBoundsSymbol;
        } else if (parent->kind == TypeKind::Float) {
            matches = matches && (!parent->hasRealRange || (actual->hasRealRange
                && (actual->lowReal > actual->highReal
                    || (actual->lowReal >= parent->lowReal && actual->highReal <= parent->highReal))));
        } else {
            matches = matches && (actual->low > actual->high || (actual->low >= parent->low && actual->high <= parent->high));
        }
    }
    if (!matches) {
        m_diagnostics.error(location, "actual subtype does not match generic formal "
            + std::string(formal.typeClass == FormalTypeClass::InterfaceType ? "interface" : "derived") + " type '" + formal.name + "'");
        return false;
    }
    for (Symbol* operation : view->primitives) {
        if (actual->tagged && operation->kind == SymbolKind::Subprogram) {
            Symbol* implementation = formalOperationImplementation(operation, actual);
            if (implementation == nullptr) {
                m_diagnostics.error(location, "actual type has no corresponding primitive for generic formal operation '" + operation->displayName + "'");
                return false;
            }
            if (actual->classRoot == nullptr && !operation->abstract && implementation->abstract) {
                m_diagnostics.error(location, "actual primitive '" + operation->displayName + "' must be nonabstract for this formal "
                    + std::string(formal.typeClass == FormalTypeClass::InterfaceType ? "interface" : "derived") + " type");
                return false;
            }
        }
    }
    // Keep the formal operation profiles, but give their corresponding type
    // the actual identity, layout, tag and ownership rather than a new type.
    auto operations = std::move(view->primitives);
    *view = *actual;
    view->base = actual;
    view->isSubtype = true;
    view->primitives = operations;
    view->formalDerived = true;
    for (Symbol* operation : operations) {
        if (operation->kind != SymbolKind::Subprogram) {
            bindings->add(operation);
            continue;
        }
        if (actual->tagged) {
            Symbol* implementation = formalOperationImplementation(operation, actual);
            Symbol* origin = operation->inheritedFrom;
            auto parameters = std::move(operation->parameters);
            Type* result = operation->returnType;
            Symbol* privateTo = operation->privatePrimitiveTo;
            SourceLocation operationLocation = operation->location;
            bool intrinsic = operation->intrinsicRenaming;
            *operation = *implementation;
            operation->parameters = std::move(parameters);
            operation->returnType = result;
            operation->privatePrimitiveTo = privateTo;
            operation->location = operationLocation;
            operation->intrinsicRenaming = intrinsic;
            operation->genericFormalOrigin = origin;
            operation->inheritedFrom = nullptr;
            operation->renamedSubprogram = implementation;
            operation->classWideWrapper = actual->classRoot != nullptr;
            if (operation->classWideWrapper) {
                operation->abstract = false;
                operation->controllingType = rootType(actual);
            }
        }
        bindings->add(operation);
    }
    decl->formalTypes.push_back(std::move(declarations.front()));
    actual = view;
    return true;
}

// A formal type says what kind of type the instantiation may supply, and a
// generic body is only sound for the kinds it was written for.
bool Sema::acceptsFormalType(const GenericFormal& formal, Type* actual, const std::string& genericName,
                             const SourceLocation& location)
{
    Type* base = baseType(actual);
    if (unknownDiscriminantView(actual) && !formal.unknownDiscriminants
        && formal.typeClass != FormalTypeClass::DerivedType) {
        m_diagnostics.error(location, "this generic formal requires a definite actual subtype view");
        return false;
    }
    if (abstractView(actual) && !formal.abstract) {
        m_diagnostics.error(location, "an abstract actual type requires an abstract generic formal type");
        return false;
    }
    const char* wanted = nullptr;

    switch (formal.typeClass) {
    case FormalTypeClass::IntegerType:
        if (base->kind != TypeKind::Integer || base->modulus != 0) {
            wanted = "a signed integer type";
        }
        break;
    case FormalTypeClass::ModularType:
        if (base->modulus == 0) {
            wanted = "a modular type";
        }
        break;
    case FormalTypeClass::FloatType:
        if (base->kind != TypeKind::Float) {
            wanted = "a floating point type";
        }
        break;
    case FormalTypeClass::DecimalType:
        if (base->kind != TypeKind::Fixed || !fixedIsDecimal(base->fixedScale)) {
            wanted = "a decimal fixed-point type";
        }
        break;
    case FormalTypeClass::FixedType:
        if (base->kind != TypeKind::Fixed || fixedIsDecimal(base->fixedScale)) {
            wanted = "an ordinary fixed-point type";
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
    case FormalTypeClass::AccessType:
        if (base->kind != TypeKind::Access) {
            wanted = "an access type";
        }
        break;
    case FormalTypeClass::DerivedType:
        // The ancestor and its inherited constraints use preceding bindings.
        break;
    case FormalTypeClass::InterfaceType:
        if (base->interfaceKind == InterfaceKind::None || actual->classRoot != nullptr || !representationVisible(actual)) {
            wanted = "an interface type";
        }
        break;
    case FormalTypeClass::Any:
        if (formal.tagged && !taggedViewVisible(actual)) {
            wanted = "a tagged type";
        } else if (!formal.unknownDiscriminants && formal.discriminants.empty() && !isDefiniteSubtype(actual)) {
            wanted = "a type of a fixed size";
        } else if (!formal.limited && base->isLimited) {
            wanted = "a nonlimited type";
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
    const TypeDefinition& definition = *formal.arrayDefinition;
    bool matches = (!definition.aliasedElements || actual->aliasedElements)
        && actual->arrayRank == static_cast<int>(definition.indexTypes.size())
        && (actual->constrained || actual->boundsSymbol != nullptr) != definition.unconstrainedIndexes;
    Type* axis = actual;
    for (const auto& index : definition.indexTypes) {
        // Constrained formal indexes are subtype marks (RM 12.5.3). Ignore
        // generated bound expressions left by contract analysis of that mark.
        Type* expected = definition.unconstrainedIndexes ? resolveSubtypeIndication(index.get(), bindings)
            : resolveTypeName(index->lower, bindings, index->location);
        if (axis == nullptr || axis->kind != TypeKind::Array || expected == nullptr) {
            matches = false;
            break;
        }
        matches = matches && baseType(axis->index) == baseType(expected);
        if (definition.unconstrainedIndexes) {
            matches = matches && staticallyMatches(axis->index, expected);
        } else {
            matches = matches && (axis->indexConstraint != nullptr
                ? staticallyMatches(axis->indexConstraint, expected)
                : axis->indexRangeStatic && expected->scalarBoundsSymbol == nullptr
                    && axis->indexLow == expected->low && axis->indexHigh == expected->high);
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

void Sema::analyzeAbstractFormalSubprogram(Symbol* symbol, bool classwideReplay)
{
    Type* controlling = nullptr;
    bool multiple = false;
    auto consider = [&](Type* type) {
        if (type != nullptr && type->anonymousAccess) { type = type->target; }
        type = rootType(type);
        if (type == nullptr || !type->tagged || (type->classRoot != nullptr && !classwideReplay)) { return; }
        multiple = multiple || (controlling != nullptr && controlling != type);
        controlling = type;
    };
    consider(symbol->returnType);
    for (Symbol* parameter : symbol->parameters) { consider(parameter->type); }
    if (controlling == nullptr || multiple) {
        m_diagnostics.error(symbol->location, "an abstract formal subprogram must have exactly one specific tagged controlling type");
        return;
    }
    symbol->controllingType = controlling;
    symbol->abstract = true;
    symbol->genericFormalSubprogram = true;
    for (Symbol* complement : symbol->declarationScope->lookupLocal("/=")) {
        if (complement->negatedEquality == symbol) {
            complement->abstract = true;
            complement->genericFormalSubprogram = true;
            complement->controllingType = controlling;
        }
    }
    for (Symbol* parameter : symbol->parameters) {
        if (controllingParameterType(parameter->type) != controlling) { continue; }
        if (parameter->type->anonymousAccess) { parameter->type->excludesNull = true; }
        if (parameter->hasDefault) {
            if (!parameter->defaultExpr->tagIndeterminate) {
                m_diagnostics.error(parameter->location, "a controlling parameter default must be tag indeterminate");
            } else {
                dispatchIndeterminate(parameter->defaultExpr, controlling);
            }
        }
    }
}

bool Sema::bindFormalSubprogram(GenericInstantiationDecl* decl, const GenericFormal& formal,
                                 Expr* actual, Scope* bindings, Scope* actualScope, bool namedDefault,
                                 GenericContract* contract, std::size_t firstSymbol)
{
    if (namedDefault) {
        Scope* defaults = m_symbolTable.createScope(nullptr);
        const auto& candidates = formal.defaultUsesFormal
            ? bindings->lookupLocal(formal.defaultValue->kind == ExprKind::Identifier
                ? static_cast<IdentifierExpr*>(formal.defaultValue.get())->lower
                : operatorSymbol(Unicode::utf8(static_cast<StringLiteralExpr*>(formal.defaultValue.get())->value)))
            : formal.defaultCandidates;
        for (Symbol* candidate : candidates) {
            defaults->add(candidate);
        }
        actualScope = defaults;
    }
    IdentifierExpr defaultName;
    defaultName.name = formal.name;
    defaultName.lower = formal.lower;
    defaultName.location = decl->location;
    if (actual == nullptr && formal.boxDefault) {
        actual = &defaultName;
    }
    bool nullDefault = actual == nullptr && formal.nullDefault;
    if (actual == nullptr && !nullDefault) {
        m_diagnostics.error(decl->location, "no actual supplied for generic formal '" + formal.name + "'");
        return false;
    }

    // Parse a fresh profile for every instance: defaults acquire instance-local
    // type and symbol annotations, and must live as long as the generated body.
    Parser parser(formal.subprogramTokens, m_diagnostics);
    DeclList declarations = parser.parseDeclarations();
    if (declarations.size() != 1 || declarations.front()->kind != DeclKind::SubprogramDeclaration) {
        return false;
    }
    auto body = std::make_unique<SubprogramBody>();
    body->location = formal.location;
    body->spec = std::move(static_cast<SubprogramDecl*>(declarations.front().get())->spec);
    body->spec.abstract = formal.abstract;
    body->spec.nullProcedure = nullDefault;
    int errors = m_diagnostics.errorCount();
    auto* savedReplay = m_replayContract;
    auto* savedRecord = m_recordContract;
    std::size_t savedFirst = m_instanceFirstSymbol;
    m_replayContract = contract;
    m_recordContract = nullptr;
    m_instanceFirstSymbol = firstSymbol;
    Symbol* wrapper = declareSubprogram(body->spec, bindings, true, true);
    bool checkedInstance = (savedReplay != nullptr
        && savedReplay->instances.contains(contractKey(decl->location)))
        || (contract->source != nullptr && contract->abstractFormals.contains(contractKey(wrapper->location)));
    if (formal.abstract) { analyzeAbstractFormalSubprogram(wrapper, checkedInstance); }
    Symbol* abstractDefault = formal.abstractDefault != nullptr ? instanceSymbol(formal.abstractDefault) : nullptr;
    Symbol* literalDefault = formal.literalDefault != nullptr ? instanceSymbol(formal.literalDefault) : nullptr;
    m_replayContract = savedReplay;
    m_recordContract = savedRecord;
    m_instanceFirstSymbol = savedFirst;
    if (m_diagnostics.errorCount() != errors) {
        return false;
    }
    body->symbol = wrapper;
    if (nullDefault) {
        body->isNull = true;
        wrapper->intrinsicRenaming = true;
        auto defaults = contract->bindings->lookupLocal(formal.lower);
        if (!defaults.empty()) { wrapper->genericActualEntity = defaults.front(); }
        decl->expansion.push_back(std::move(body));
        return true;
    }

    auto sameType = [](Type* a, Type* b) { return SemaSupport::profileTypeMatches(a, b); };
    auto matches = [&](Symbol* candidate) {
        if (candidate->kind == SymbolKind::EnumerationLiteral) { return matchesFormalProfile(candidate, wrapper); }
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
        operation = operatorSymbol(Unicode::utf8(static_cast<StringLiteralExpr*>(actual)->value));
    } else if (actual->kind == ExprKind::Identifier) {
        operation = operatorSymbol(static_cast<IdentifierExpr*>(actual)->lower);
    }
    if (actual->kind == ExprKind::Selected
        && !prefixedOperations(static_cast<SelectedExpr*>(actual), actualScope).empty()) {
        m_diagnostics.error(actual->location, "prefixed views as generic actuals are not yet supported");
        return false;
    }
    std::vector<Symbol*> candidates;
    Symbol* attributeProfile = nullptr;
    if (actual->kind == ExprKind::Attribute) {
        auto* previousReplay = m_replayContract;
        std::size_t previousFirst = m_instanceFirstSymbol;
        if (namedDefault) {
            m_replayContract = contract;
            m_instanceFirstSymbol = firstSymbol;
        }
        attributeProfile = attributeSubprogram(static_cast<AttributeExpr*>(actual), namedDefault ? bindings : actualScope);
        m_replayContract = previousReplay;
        m_instanceFirstSymbol = previousFirst;
        if (attributeProfile == nullptr) { return false; }
    }
    const auto actualCandidates = namedDefault && literalDefault != nullptr ? std::vector<Symbol*> { literalDefault }
        : attributeProfile != nullptr ? std::vector<Symbol*> { attributeProfile }
        : namedDefault && formal.abstract
        ? (abstractDefault != nullptr ? std::vector<Symbol*> { abstractDefault } : std::vector<Symbol*> {})
        : namedDefault && actual->kind == ExprKind::Selected
            ? formal.defaultCandidates : expressionNames(actual, actualScope);
    for (Symbol* candidate : actualCandidates) {
        if (matches(candidate) && candidate->protectedType != nullptr) {
            m_diagnostics.error(actual->location, "protected operation generic actuals are not yet supported");
            return false;
        }
        if (matches(candidate)) {
            if (candidate->kind == SymbolKind::EnumerationLiteral) {
                candidates.push_back(candidate);
                continue;
            }
            if (body->spec.returnType != nullptr && body->spec.returnType->nullExclusion
                && !((candidate->genericFormalSubprogram && m_genericBodyContext) ? candidate->explicitNullExclusion : candidate->returnType->excludesNull)) {
                m_diagnostics.error(actual->location, "generic actual result must exclude null");
                return false;
            }
            for (std::size_t i = 0; i < wrapper->parameters.size(); ++i) {
                if (body->spec.parameters[i].subtype->nullExclusion
                    && !((candidate->genericFormalSubprogram && m_genericBodyContext) ? candidate->parameters[i]->explicitNullExclusion
                        : candidate->parameters[i]->type->excludesNull)) {
                    m_diagnostics.error(actual->location, "generic actual parameter must exclude null");
                    return false;
                }
            }
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

    if (!formal.abstract && !intrinsic && candidates.front()->abstract) {
        m_diagnostics.error(actual->location, "an abstract subprogram cannot be an actual for a nonabstract formal subprogram");
        return false;
    }
    // The formal's calling convention comes from its actual, including an
    // intrinsic null default forwarded through another generic or renaming.
    bool enumeration = !intrinsic && candidates.front()->kind == SymbolKind::EnumerationLiteral;
    wrapper->genericActualExpression = actual == &defaultName ? nullptr : actual;
    wrapper->genericActualEntity = intrinsic ? nullptr : candidates.front();
    wrapper->genericIntrinsic = intrinsic ? operation : std::string();
    wrapper->intrinsicRenaming = intrinsic || enumeration;
    if (!intrinsic) {
        for (Symbol* target = candidates.front(); target != nullptr; target = target->renamedSubprogram) {
            wrapper->intrinsicRenaming = wrapper->intrinsicRenaming || target->intrinsicRenaming;
        }
    }
    if (formal.abstract) {
        Symbol* operation = intrinsic ? nullptr : dispatchingOperation(candidates.front());
        bool classwide = checkedInstance && !intrinsic && candidates.front()->classWideWrapper
            && candidates.front()->controllingType == wrapper->controllingType;
        if (operation == nullptr || (!classwide && operation->controllingType != wrapper->controllingType)) {
            m_diagnostics.error(actual->location, "actual for abstract formal subprogram must be a dispatching operation of its controlling type");
            return false;
        }
        wrapper->classWideWrapper = classwide;
        wrapper->dispatchSlot = operation->dispatchSlot;
        wrapper->renamedSubprogram = candidates.front();
        wrapper->interfaceOrigins = operation->interfaceOrigins;
        wrapper->hasBody = false;
        wrapper->requiresBody = false;
        // A formal abstract declaration is a dispatching view, not a new
        // primitive slot or a statically callable forwarding body.
        for (std::size_t i = 0; i < wrapper->parameters.size(); ++i) {
            wrapper->parameters[i]->type = candidates.front()->parameters[i]->type;
            wrapper->parameters[i]->byReference = candidates.front()->parameters[i]->byReference;
        }
        wrapper->returnType = candidates.front()->returnType;
        auto declaration = std::make_unique<SubprogramDecl>();
        declaration->spec = std::move(body->spec);
        declaration->symbol = wrapper;
        declaration->location = formal.location;
        decl->formalTypes.push_back(std::move(declaration));
        return true;
    }
    // An instance's formal subprogram takes the actual's subtypes and ABI,
    // while retaining its own parameter names and default expressions.
    if (!intrinsic) {
        // Delay the inherited literal's base result subtype until callable use:
        // creating it at type declaration would freeze representation clauses.
        wrapper->returnType = enumeration ? m_types.scalarBaseType(candidates.front()->type) : candidates.front()->returnType;
        for (std::size_t i = 0; i < wrapper->parameters.size(); ++i) {
            wrapper->parameters[i]->type = candidates.front()->parameters[i]->type;
            wrapper->parameters[i]->byReference = candidates.front()->parameters[i]->byReference;
            operands[i]->type = wrapper->parameters[i]->type;
        }
    }

    Symbol* savedSubprogram = m_currentSubprogram;
    m_currentSubprogram = wrapper;
    ExprPtr implementation;
    if (enumeration) {
        auto value = std::make_unique<IntegerLiteralExpr>();
        value->location = actual->location;
        value->type = wrapper->returnType;
        value->isStatic = true;
        value->value = value->staticValue = candidates.front()->enumerationValue;
        implementation = std::move(value);
    } else if (attributeProfile != nullptr) {
        auto* source = static_cast<AttributeExpr*>(actual);
        auto attribute = std::make_unique<AttributeExpr>();
        attribute->name = source->name;
        attribute->lower = source->lower;
        attribute->location = source->location;
        // A bound internal type mark avoids both mutating a shared default AST
        // and capture by a formal parameter named like the source prefix.
        std::string prefixName = "$attribute_type_" + std::to_string(m_symbolTable.symbols().size());
        Symbol* prefix = m_symbolTable.createSymbol(SymbolKind::TypeName, prefixName, prefixName);
        prefix->type = source->prefixType;
        operandsScope->add(prefix);
        auto mark = std::make_unique<IdentifierExpr>();
        mark->name = mark->lower = prefixName;
        mark->location = source->prefix->location;
        attribute->prefix = std::move(mark);
        attribute->arguments = std::move(operands);
        analyzeExpr(attribute.get(), operandsScope, wrapper->returnType);
        implementation = std::move(attribute);
    } else if (intrinsic) {
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
        implementation = explicitOperator(&call, operandsScope);
        if (implementation != nullptr) {
            // Lookup at the instantiation site already selected this intrinsic.
            if (implementation->kind == ExprKind::Unary) {
                static_cast<UnaryExpr*>(implementation.get())->predefinedOnly = true;
            } else if (implementation->kind == ExprKind::Binary) {
                static_cast<BinaryExpr*>(implementation.get())->predefinedOnly = true;
            }
            analyzeExpr(implementation.get(), operandsScope, wrapper->returnType);
        }
    } else {
        implementation = bindOperator(candidates.front(), std::move(operands), operandsScope, formal.location);
        // bindOperator builds an already-resolved call without passing its
        // result through analyzeExpr. The synthetic wrapper still owns the
        // returned temporary, including when a private formal becomes a
        // controlled or tagged actual at instantiation.
        if (implementation != nullptr) {
            Type* result = implementation->type;
            if (needsFinalization(result) || needsCollection(result) || (result != nullptr && result->tagged)) {
                wrapper->usesFinalization = true;
            }
        }
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

bool Sema::associateGenericActuals(GenericInstantiationDecl* decl, Symbol* generic, std::vector<Expr*>& actuals)
{
    const auto& formals = generic->generic->formals;
    actuals.assign(formals.size(), nullptr);
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
                    if (decl->formalPackageActualPart && std::count_if(formals.begin(), formals.end(), [&](const GenericFormal& formal) {
                            return formal.lower == decl->arguments[i].nameLower;
                        }) != 1) {
                        m_diagnostics.error(decl->location, "a named formal-package association cannot select overloaded formals");
                        return false;
                    }
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
    if (decl->formalPackageActualPart) {
        bool partial = decl->packageOthers || std::any_of(actuals.begin(), actuals.end(), [](Expr* actual) {
            return actual != nullptr && actual->kind == ExprKind::Box;
        });
        bool explicitActual = std::any_of(actuals.begin(), actuals.end(), [](Expr* actual) {
            return actual != nullptr && actual->kind != ExprKind::Box;
        });
        decl->packageMatches.assign(formals.size(), !partial);
        for (std::size_t i = 0; i < formals.size(); ++i) {
            if (partial) {
                decl->packageMatches[i] = actuals[i] != nullptr ? actuals[i]->kind != ExprKind::Box
                    : !decl->packageOthers && !explicitActual;
            }
            if (!decl->packageOthers && actuals[i] == nullptr && formals[i].defaultValue == nullptr
                && !formals[i].boxDefault && !formals[i].nullDefault) {
                m_diagnostics.error(decl->location, "no actual supplied for generic formal '" + formals[i].name + "'");
                return false;
            }
        }
    }
    return true;
}

bool Sema::checkGenericObjectActual(const GenericFormal& formal, Expr* actual, Type* type, Type* actualType)
{
    if (type == nullptr || actualType == nullptr) { return false; }
    bool reference = formal.mode == ParameterMode::InOut;
    if (!reference) {
        if (hasTasks(type) || hasProtected(type)) {
            m_diagnostics.error(actual->location, "generic in objects containing task or protected parts are not yet supported");
            return false;
        }
        if ((limitedView(type) || limitedView(actualType)) && !buildsLimitedResult(actual)) {
            m_diagnostics.error(actual->location, "a limited generic in object requires an aggregate or function call");
            return false;
        }
    }
    if (type->kind == TypeKind::Access) {
        // RM 12.4: anonymous formal objects require exact designated subtype
        // and constant-view matching, unlike ordinary access initialization.
        if (type->anonymousAccess && actual->kind != ExprKind::Null) {
            bool matches = actualType->kind == TypeKind::Access
                && (!reference || actualType->anonymousAccess)
                && type->accessConstant == actualType->accessConstant
                && type->accessProtected == actualType->accessProtected
                && type->accessConventionC == actualType->accessConventionC;
            if (matches) {
                matches = type->accessProfile != nullptr
                    ? actualType->accessProfile != nullptr && matchesAccessProfile(actualType->accessProfile, type)
                    : actualType->accessProfile == nullptr && staticallyMatches(actualType->target, type->target);
            }
            if (!matches) {
                m_diagnostics.error(actual->location, "actual access type does not match anonymous generic formal object '" + formal.name + "'");
                return false;
            }
        }
        if (reference && formal.subtype->nullExclusion) {
            Symbol* entity = actual->kind == ExprKind::Identifier ? static_cast<IdentifierExpr*>(actual)->symbol
                : actual->kind == ExprKind::Selected ? static_cast<SelectedExpr*>(actual)->symbol : nullptr;
            bool excludesNull = entity != nullptr && entity->genericObject && m_genericBodyContext
                ? entity->explicitNullExclusion : actualType->excludesNull;
            if (!excludesNull) {
                m_diagnostics.error(actual->location, "a null-excluding generic in out object requires an actual that excludes null");
                return false;
            }
        }
    }
    return true;
}

bool Sema::bindGenericFormal(GenericInstantiationDecl* decl, Symbol* generic, const GenericFormal& formal,
                              Expr* supplied, Scope* bindings, Scope* scope, std::size_t firstSymbol)
{
    Expr* actual = supplied;
    if (actual == nullptr && formal.defaultValue) {
        actual = formal.defaultValue.get();
    }
    if (formal.kind == GenericFormalKind::SubprogramFormal) {
        if (!bindFormalSubprogram(decl, formal, actual, bindings, scope,
                supplied == nullptr && formal.defaultValue != nullptr,
                m_genericContracts.at(generic->generic).get(), firstSymbol)) {
            return false;
        }
        return true;
    }
    if (actual == nullptr) {
        m_diagnostics.error(decl->location, "no actual supplied for generic formal '" + formal.name + "'");
        return false;
    }

    if (formal.kind == GenericFormalKind::PackageFormal) {
        if (!bindFormalPackage(decl, formal, actual, bindings, scope,
                m_genericContracts.at(generic->generic).get(), firstSymbol)) { return false; }
        return true;
    }

    if (formal.kind == GenericFormalKind::TypeFormal) {
        Type* type = nullptr;
        if (actual->kind == ExprKind::Identifier) {
            type = resolveTypeName(static_cast<IdentifierExpr*>(actual)->lower, scope, actual->location);
        } else if (actual->kind == ExprKind::Attribute
                   && (static_cast<AttributeExpr*>(actual)->lower == "base"
                       || static_cast<AttributeExpr*>(actual)->lower == "class")) {
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
        if (!checkCompleteView(type, actual->location, "a generic actual type")) { return false; }
        Symbol* owner = type->privateTo != nullptr && scope->privateVisible(type->privateTo) ? type->privateTo : nullptr;
        if (formal.preelaborableInitialization && !hasPreelaborableInitialization(type, owner, nullptr, scope)) {
            m_diagnostics.error(actual->location, "generic actual type must have preelaborable initialization");
            return false;
        }
        bool indefiniteDeallocation = generic->generic->lower == "ada.unchecked_deallocation"
            && formal.lower == "object";
        if (!indefiniteDeallocation && !acceptsFormalType(formal, type, decl->genericName, actual->location)) {
            return false;
        }
        if (!formal.discriminants.empty()
            && !matchesFormalDiscriminants(decl, generic, formal, type, bindings, firstSymbol, actual->location)) {
            return false;
        }
        if (formal.typeClass == FormalTypeClass::ArrayType
            && !matchesFormalArray(formal, type, bindings, actual->location)) {
            return false;
        }
        if ((formal.typeClass == FormalTypeClass::DerivedType || formal.typeClass == FormalTypeClass::InterfaceType)
            && !bindFormalDerivedType(decl, generic, formal, type, bindings, firstSymbol, actual->location)) {
            return false;
        }
        if (formal.typeClass == FormalTypeClass::AccessType) {
            // Resolve a fresh definition with preceding formals bound to
            // their actuals, while retaining declaration-site name binding.
            Parser parser(formal.typeTokens, m_diagnostics);
            DeclList declarations = parser.parseDeclarations();
            if (declarations.size() != 1 || declarations.front()->kind != DeclKind::Type) { return false; }
            auto* definition = static_cast<TypeDecl*>(declarations.front().get());
            auto* savedReplay = m_replayContract;
            auto* savedRecord = m_recordContract;
            std::size_t savedFirst = m_instanceFirstSymbol;
            m_replayContract = m_genericContracts.at(generic->generic).get();
            m_recordContract = nullptr;
            m_instanceFirstSymbol = firstSymbol;
            int errors = m_diagnostics.errorCount();
            analyzeTypeDecl(definition, m_symbolTable.createScope(bindings));
            m_replayContract = savedReplay;
            m_recordContract = savedRecord;
            m_instanceFirstSymbol = savedFirst;
            Type* expected = definition->declaredType;
            bool matches = errors == m_diagnostics.errorCount() && expected != nullptr
                && matchesAccessFormal(type, expected);
            decl->formalTypes.push_back(std::move(declarations.front()));
            if (!matches) {
                m_diagnostics.error(actual->location, "actual access type does not match generic formal '" + formal.name + "'");
                return false;
            }
        }
        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, formal.lower, formal.name);
        symbol->type = type;
        symbol->location = formal.location;
        bindings->add(symbol);
        return true;
    }

    // Reparse the subtype/default so its semantic annotations belong to
    // this instance. Declaration-site names replay the checked contract.
    Parser objectParser(formal.objectTokens, m_diagnostics);
    DeclList objects = objectParser.parseDeclarations();
    if (objects.size() != 1 || objects.front()->kind != DeclKind::Object) {
        return false;
    }
    auto* object = static_cast<ObjectDecl*>(objects.front().get());
    auto* savedReplay = m_replayContract;
    auto* savedRecord = m_recordContract;
    std::size_t savedFirst = m_instanceFirstSymbol;
    m_replayContract = m_genericContracts.at(generic->generic).get();
    m_recordContract = nullptr;
    m_instanceFirstSymbol = firstSymbol;
    Type* type = resolveSubtypeIndication(object->subtype.get(), bindings);
    bool useDefault = supplied == nullptr;
    Type* actualType = nullptr;
    if (useDefault) {
        actual = object->initializer.get();
        actualType = analyzeExpr(actual, bindings, type);
    }
    m_replayContract = savedReplay;
    m_recordContract = savedRecord;
    m_instanceFirstSymbol = savedFirst;
    if (!useDefault) {
        actualType = analyzeExpr(actual, scope, type);
    }
    if (!typesCompatible(type, actualType)) {
        m_diagnostics.error(actual->location, "generic formal object actual has an incompatible type");
        return false;
    }
    if (type == nullptr || actualType == nullptr) {
        return false;
    }
    bool reference = formal.mode == ParameterMode::InOut;
    if (!checkGenericObjectActual(formal, actual, type, actualType)) { return false; }
    if (reference) {
        m_sharedGenericActuals.push_back({actual, type});
        int errors = m_diagnostics.errorCount();
        if (!isGenericVariable(actual)) {
            m_diagnostics.error(actual->location, "an in out generic formal object requires a variable actual");
        } else {
            checkAssignable(actual, scope, true);
        }
        if (errors != m_diagnostics.errorCount()) {
            return false;
        }
        // A formal in out is a view of the actual, including its bounds.
        type = actualType;
    } else {
        SemaSupport::adaptUniversal(actual, type);
        // An unconstrained constant acquires its bounds/discriminants from
        // its initializer. A dynamic array keeps its per-object descriptor.
        if ((type->kind == TypeKind::Array && !type->constrained && type->boundsSymbol == nullptr)
            || (type->kind == TypeKind::Record && type->classRoot == nullptr && !hasKnownDiscriminants(type))) {
            type = actualType;
        }
    }
    Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Object, formal.lower, formal.name);
    symbol->type = type;
    symbol->location = formal.location;
    symbol->isConstant = !reference;
    symbol->objectReference = reference;
    symbol->classWideObject = !reference && type->classRoot != nullptr;
    symbol->genericObject = true;
    symbol->explicitNullExclusion = object->subtype->nullExclusion;
    symbol->accessibilityLevel = m_accessibilityLevel;
    symbol->genericActualExpression = actual;
    symbol->sharedAlias = reference ? actual : nullptr;
    symbol->owner = m_currentSubprogram;
    symbol->level = m_currentSubprogram != nullptr ? m_currentSubprogram->level : 0;
    symbol->isGlobal = m_currentSubprogram == nullptr;
    if (symbol->isGlobal) {
        symbol->qbeName = "$" + mangle(decl->lower + "__formal__" + formal.lower);
    }
    if (!reference && !isComposite(type) && sharedTypeKind(type) == 0) {
        long long value = 0;
        double real = 0.0;
        if (isReal(type) ? foldStaticReal(actual, real) : foldStatic(actual, value)) {
            symbol->hasStaticValue = true;
            symbol->staticValue = value;
            symbol->staticReal = real;
        }
    }
    bindings->add(symbol);
    object->isConstant = !reference;
    object->symbols.push_back(symbol);
    if (!useDefault) {
        for (Association& argument : decl->arguments) {
            if (argument.value.get() == actual) {
                object->initializer = std::move(argument.value);
                break;
            }
        }
    }
    decl->expansion.push_back(std::move(objects.front()));

    return true;
}

// Matches the actuals to the formals and declares each one in a scope of its
// own, which the copy of the generic is then analysed inside.
bool Sema::bindGenericFormals(GenericInstantiationDecl* decl, Symbol* generic, Scope* bindings, Scope* scope)
{
    const std::vector<GenericFormal>& formals = generic->generic->formals;
    std::size_t firstSymbol = m_symbolTable.symbols().size();
    std::vector<Expr*> actuals(formals.size(), nullptr);

    if (!associateGenericActuals(decl, generic, actuals)) { return false; }

    for (std::size_t i = 0; i < formals.size(); ++i) {
        applyGenericUses(generic->generic, i, bindings, decl->formalTypes);
        auto savedCopies = m_instanceCopies;
        for (const auto& [original, copy] : decl->packageCopies) { m_instanceCopies[original] = copy; }
        bool bound = bindGenericFormal(decl, generic, formals[i], actuals[i], bindings, scope, firstSymbol);
        m_instanceCopies = std::move(savedCopies);
        if (!bound) { return false; }
    }

    applyGenericUses(generic->generic, formals.size(), bindings, decl->formalTypes);
    // The predefined deallocator uses a private formal in its source until
    // general formal access types are implemented. Preserve its actual access
    // contract rather than accepting arbitrary private types as pointers.
    if (generic->generic->lower == "ada.unchecked_deallocation") {
        Symbol* object = lookupName("object", bindings);
        Symbol* name = lookupName("name", bindings);
        Type* access = name != nullptr ? baseType(name->type) : nullptr;
        if (object == nullptr || access == nullptr || access->kind != TypeKind::Access
            || access->accessProfile != nullptr || access->accessConstant
            || !SemaSupport::staticallyMatches(access->target, object->type)) {
            m_diagnostics.error(decl->location,
                "Ada.Unchecked_Deallocation requires Name to be an access-to-object type designating Object");
            return false;
        }
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
    if (generic->renamedUnit != nullptr) {
        generic = generic->renamedUnit;
    }
    if (generic->generic->isPackage != decl->isPackage || generic->generic->isFunction != decl->isFunction) {
        m_diagnostics.error(decl->location, "generic unit kind does not match this instantiation");
        return;
    }
    decl->generic = generic;
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

    auto foundContract = m_genericContracts.find(generic->generic);
    if (foundContract == m_genericContracts.end() || !foundContract->second->valid) {
        for (std::size_t i = 0; i < pushed; ++i) {
            m_namePrefix.pop_back();
        }
        return;
    }
    GenericContract* contract = foundContract->second.get();
    ScopedCheckPolicy instanceChecks(m_suppressedChecks, m_suppressedChecks | contract->checkMask);
    std::size_t firstSymbol = m_symbolTable.symbols().size();
    Scope* bindings = m_symbolTable.createScope(contract->environment);
    bindings->m_genericBindings = true;
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
        if (unit->kind == DeclKind::PackageSpecification) { unit->category = decl->category; }
    }

    auto* savedReplay = m_replayContract;
    auto* savedRecord = m_recordContract;
    auto* savedOwnerContract = m_instanceOwnerContract;
    if (savedRecord != nullptr) {
        m_instanceOwnerContract = savedRecord;
    }
    std::size_t savedFirst = m_instanceFirstSymbol;
    m_replayContract = contract;
    m_recordContract = nullptr;
    m_instanceFirstSymbol = firstSymbol;
    auto savedCopies = std::move(m_instanceCopies);
    m_instanceCopies = decl->packageCopies;
    ++m_instantiationDepth;
    analyzeDeclarativePart(expansion, bindings);
    if (!generic->hasBody) {
        PendingGenericInstance pending;
        pending.instance = decl;
        pending.bindings = bindings;
        pending.contract = contract;
        pending.enclosingContract = m_instanceOwnerContract;
        pending.tokenCount = generic->generic->tokens.size() - 1;
        pending.copies = m_instanceCopies;
        for (Symbol* original : contract->localSymbols) {
            if (Symbol* copy = instanceSymbol(original)) {
                pending.copies[original] = copy;
            }
        }
        pending.owner = m_currentSubprogram;
        pending.unit = m_currentUnit;
        pending.packages = m_packages;
        pending.prefix = m_namePrefix;
        pending.accessibilityLevel = m_accessibilityLevel;
        pending.depth = m_instantiationDepth;
        pending.bodyContext = m_genericInstanceBody;
        m_pendingGenericInstances.push_back(std::move(pending));
    }
    --m_instantiationDepth;
    m_replayContract = savedReplay;
    m_recordContract = savedRecord;
    m_instanceFirstSymbol = savedFirst;
    m_instanceOwnerContract = savedOwnerContract;
    m_instanceCopies = std::move(savedCopies);

    // Distinguish multiple copies of the same nested template. Their source
    // locations coincide, but their enclosing instantiations do not.
    const auto& symbols = m_symbolTable.symbols();
    std::string instanceKey = contractKey(decl->location);
    if (m_recordContract != nullptr) {
        auto& copies = m_recordContract->instances[instanceKey];
        for (std::size_t i = firstSymbol; i < symbols.size(); ++i) {
            copies.push_back(symbols[i].get());
        }
    }
    if (m_replayContract != nullptr) {
        auto found = m_replayContract->instances.find(instanceKey);
        if (found != m_replayContract->instances.end()) {
            std::size_t next = firstSymbol;
            for (Symbol* original : found->second) {
                for (std::size_t i = next; i < symbols.size(); ++i) {
                    Symbol* copy = symbols[i].get();
                    if (copy->kind == original->kind && copy->name == original->name
                        && contractKey(copy->location) == contractKey(original->location)) {
                        m_instanceCopies[original] = copy;
                        next = i + 1;
                        break;
                    }
                }
            }
        }
    }

    for (std::size_t i = 0; i < pushed; ++i) {
        m_namePrefix.pop_back();
    }

    // The instance lives in the bindings scope while it is analysed; here it
    // becomes visible where it was written.
    for (const DeclPtr& unit : expansion) {
        Symbol* symbol = declaredSymbol(unit.get());
        if (symbol != nullptr && symbol->name == instanceName) {
            decl->symbol = symbol;
            symbol->genericInstance = true;
            if (symbol->kind == SymbolKind::Package) {
                symbol->instanceTemplate = generic;
                symbol->instanceBindings = bindings;
                symbol->instanceSpecification = symbol;
            }
            if (symbol->kind == SymbolKind::Subprogram) {
                symbol->declarationScope = home;
                for (Symbol* inherited : home->lookupDeclaration(symbol->name)) {
                    bool matches = inherited->kind == SymbolKind::Subprogram
                        && inherited->parameters.size() == symbol->parameters.size()
                        && SemaSupport::profileTypeMatches(inherited->returnType, symbol->returnType);
                    for (std::size_t i = 0; matches && i < symbol->parameters.size(); ++i) {
                        matches = SemaSupport::profileTypeMatches(inherited->parameters[i]->type, symbol->parameters[i]->type);
                    }
                    if (inherited->kind != SymbolKind::Subprogram || (matches && inherited->inheritedFrom == nullptr)) {
                        m_diagnostics.error(decl->location, "'" + decl->name + "' has already been declared in this scope");
                    }
                    if (matches && inherited->inheritedFrom != nullptr) {
                        symbol->overrides = true;
                        bool conforms = inherited->returnType == symbol->returnType
                            || SemaSupport::staticallyMatches(inherited->returnType, symbol->returnType);
                        for (std::size_t i = 0; i < symbol->parameters.size(); ++i) {
                            conforms = conforms && inherited->parameters[i]->mode == symbol->parameters[i]->mode
                                && SemaSupport::staticallyMatches(inherited->parameters[i]->type, symbol->parameters[i]->type);
                        }
                        if (inherited->dispatchSlot >= 0 && !conforms) {
                            m_diagnostics.error(decl->location, "overriding dispatching profile must be subtype conformant with the inherited operation");
                        }
                    }
                }
                for (Symbol* parameter : symbol->parameters) {
                    Type* type = parameter->type;
                    if (type->anonymousAccess && type->accessParameter && type->target != nullptr
                        && rootType(type->target)->declarationScope == home && rootType(type->target)->tagged
                        && !type->excludesNull) {
                        m_diagnostics.error(decl->location, "a controlling access parameter of a generic instance must exclude null");
                    }
                }
                registerPrimitive(symbol, home);
                checkOverridingIndicator(symbol, decl->overriding, decl->location);
                if (symbol->name == "=" && m_types.isBoolean(symbol->returnType)) {
                    for (Symbol* complement : bindings->lookupLocal("/=")) {
                        if (complement->negatedEquality != symbol) { continue; }
                        complement->declarationScope = home;
                        complement->overrides = symbol->overrides;
                        registerPrimitive(complement, home);
                        home->add(complement);
                    }
                }
            }
            home->add(symbol);
            break;
        }
    }
    for (DeclPtr& unit : expansion) {
        decl->expansion.push_back(std::move(unit));
    }
    if (decl->category != 0) {
        m_categorizedRegions.push_back({ &decl->expansion, nullptr, decl->category, nullptr });
    }
    if (decl->symbol == nullptr) {
        m_diagnostics.error(decl->location, "generic '" + decl->genericName + "' produced no instance");
    }
}

void Sema::completeGenericInstance(PendingGenericInstance pending)
{
    auto* instance = pending.instance;
    Symbol* generic = instance->generic;
    auto* contract = pending.contract;
    if (!contract->valid) {
        return;
    }
    const auto& source = generic->generic->tokens;
    std::vector<Token> tokens(source.begin() + static_cast<std::ptrdiff_t>(pending.tokenCount), source.end());
    Parser parser(tokens, m_diagnostics);
    DeclList bodies = parser.parseDeclarations();
    for (const DeclPtr& body : bodies) {
        renameUnit(body.get(), instance->symbol->displayName, instance->symbol->name, generic->generic->lower);
    }

    // This runs after source analysis, so no caller's lexical or replay state
    // is active. Body names were resolved at the template body's original
    // position by checkGenericContract; replay maps them to this instance.
    m_currentSubprogram = pending.owner;
    m_currentUnit = pending.unit;
    m_assertionsEnabled = pending.unit->assertionsEnabled;
    m_suppressedChecks = pending.instance->suppressedChecks | contract->checkMask;
    m_packages = std::move(pending.packages);
    m_namePrefix = std::move(pending.prefix);
    m_accessibilityLevel = pending.accessibilityLevel;
    m_instantiationDepth = pending.depth;
    m_genericInstanceBody = pending.bodyContext;
    m_recordContract = nullptr;
    m_replayContract = contract;
    m_instanceOwnerContract = pending.enclosingContract;
    m_instanceCopies = std::move(pending.copies);
    m_instanceFirstSymbol = m_symbolTable.symbols().size();
    std::size_t firstSymbol = m_instanceFirstSymbol;
    int errors = m_diagnostics.errorCount();
    auto savedNames = m_subprogramNames;
    auto savedTagNames = m_tagNames;
    auto savedExceptions = m_unitExceptions;
    pending.bindings->setParent(contract->environment);
    analyzeDeclarativePart(bodies, pending.bindings);
    for (DeclPtr& body : bodies) {
        instance->expansion.push_back(std::move(body));
    }
    if (pending.enclosingContract != nullptr) {
        // A deferred instance inside a template is still contract-only code.
        // Keep its symbols available for later replay without consuming emitted
        // names or exporting its exception objects.
        auto* enclosing = pending.enclosingContract;
        enclosing->valid = enclosing->valid && m_diagnostics.errorCount() == errors;
        auto& copies = enclosing->instances[contractKey(instance->location)];
        const auto& symbols = m_symbolTable.symbols();
        for (std::size_t i = firstSymbol; i < symbols.size(); ++i) {
            enclosing->localSymbols.insert(symbols[i].get());
            copies.push_back(symbols[i].get());
        }
        m_subprogramNames = std::move(savedNames);
        m_tagNames = std::move(savedTagNames);
        m_unitExceptions = std::move(savedExceptions);
    }
    m_currentSubprogram = nullptr;
    m_currentUnit = nullptr;
    m_packages.clear();
    m_namePrefix.clear();
    m_accessibilityLevel = 0;
    m_instantiationDepth = 0;
    m_replayContract = nullptr;
    m_genericInstanceBody = false;
    m_instanceOwnerContract = nullptr;
    m_instanceCopies.clear();
}

void Sema::finishAnalysis(bool requireLibraryBodies)
{
    if (m_diagnostics.hasErrors()) {
        return;
    }
    // Completing one body can reveal another nested template body. Drain all
    // ready instances before deciding whether any required bodies are missing.
    bool progress = true;
    while (progress && !m_diagnostics.hasErrors()) {
        progress = false;
        for (std::size_t i = 0; i < m_pendingGenericInstances.size() && !m_diagnostics.hasErrors();) {
            if (!m_pendingGenericInstances[i].instance->generic->hasBody) {
                ++i;
                continue;
            }
            PendingGenericInstance pending = std::move(m_pendingGenericInstances[i]);
            m_pendingGenericInstances.erase(m_pendingGenericInstances.begin() + static_cast<std::ptrdiff_t>(i));
            completeGenericInstance(std::move(pending));
            progress = true;
        }
    }
    if (m_diagnostics.hasErrors()) {
        return;
    }
    for (const CategorizedRegion& region : m_categorizedRegions) {
        if (region.declarations != nullptr) {
            checkCategorizedDeclarations(*region.declarations, region.category, m_diagnostics, region.owner);
        }
        if (region.statements != nullptr) { checkCategorizedStatements(*region.statements, m_diagnostics, region.owner); }
    }
    for (const auto& owned : m_symbolTable.symbols()) {
        Symbol* symbol = owned.get();
        if (symbol->noReturn && symbol->inheritedFrom == nullptr) {
            for (const SourceLocation& location : symbol->returnStatements) {
                m_diagnostics.error(location, "a return statement cannot apply to a No_Return procedure");
            }
        }
    }
    for (const auto& [symbol, location] : m_noReturnAccesses) {
        Symbol* implementation = symbol;
        bool noReturn = symbol->noReturn;
        while (implementation->renamedSubprogram != nullptr || implementation->inheritedFrom != nullptr) {
            implementation = implementation->renamedSubprogram != nullptr
                ? implementation->renamedSubprogram : implementation->inheritedFrom;
            noReturn = noReturn || implementation->noReturn;
        }
        if (implementation->builtin == BuiltinKind::Runtime && noReturn) {
            m_diagnostics.error(location, "'Access of imported No_Return procedures is not yet supported");
        } else if (noReturn && !implementation->noReturn) {
            m_diagnostics.error(location, "'Access of a No_Return renaming without a No_Return target is not yet supported");
        }
    }
    for (const auto& [declared, renamed] : m_noReturnRenamings) {
        if (declared->noReturn && (renamed == nullptr || !renamed->noReturn)) {
            m_diagnostics.error(declared->location, "a No_Return renaming-as-body requires a No_Return target");
        }
    }
    for (const auto& [overriding, parent] : m_noReturnOverrides) {
        if ((parent->noReturn || m_noReturnRequired.contains(overriding)) && !overriding->noReturn) {
            m_diagnostics.error(overriding->location, "an override of a No_Return procedure must also be No_Return");
        }
    }
    finishLimitedViews();
    if (m_diagnostics.hasErrors()) { return; }
    // A result's dispatching tag can come from its enclosing call or assignment.
    auto dispatchingCall = [](Expr* call) {
        return call->kind == ExprKind::Call ? static_cast<CallExpr*>(call)->dispatching
            : call->tagIndeterminate && call->type != nullptr && designatedViewType(call->type)->classRoot != nullptr;
    };
    std::unordered_set<Expr*> abstractCalls;
    for (Expr* call : m_abstractCalls) {
        bool dispatching = dispatchingCall(call);
        if (!dispatching && abstractCalls.insert(call).second) {
            m_diagnostics.error(call->location, "a call to an abstract subprogram must be dispatching");
        }
    }
    for (Expr* call : m_abstractDefaultCalls) {
        if (!dispatchingCall(call) && abstractCalls.insert(call).second) {
            m_diagnostics.error(call->location, "a defaulted abstract controlling operand requires a dispatching call");
        }
    }
    // Private completions can reveal noncopying parts after an earlier use was
    // analyzed. Reject unsupported delayed layouts and value operations before
    // emitting code, rather than treating a partial view as an empty record.
    for (Expr* escape : m_sharedEscapes) {
        if (sharedObjectKind(escape) != 0) {
            m_diagnostics.error(escape->location, "address escape and streaming of shared scalars are not yet supported");
        }
    }
    for (const auto& actual : m_sharedGenericActuals) {
        if (sharedObjectKind(actual.first) > sharedTypeKind(actual.second)) {
            m_diagnostics.error(actual.first->location, "a shared generic in out actual requires a correspondingly shared formal type");
        }
    }
    std::unordered_set<Type*> checked;
    for (const auto& symbol : m_symbolTable.symbols()) {
        Type* type = symbol->type;
        if (isScalar(type) && (sharedSymbolKind(symbol.get()) != 0)) {
            long long size = typeSize(type);
            if ((size != 1 && size != 2 && size != 4 && size != 8) || typeAlignment(type) < size) {
                m_diagnostics.error(symbol->location, "shared scalar storage requires a naturally aligned 8/16/32/64-bit representation");
            }
        }
        if (symbol->builtin == BuiltinKind::Runtime) {
            for (Symbol* parameter : symbol->parameters) {
                Type* formal = baseType(parameter->type);
                if (hasSharedAccessParts(formal) && symbol->runtimeSymbol != "$__ada_deallocate") {
                    m_diagnostics.error(symbol->location, "imported parameters containing access to shared objects are not yet supported");
                }
            }
        }
        if (symbol->kind == SymbolKind::TypeName && isComposite(type) && hasSharedParts(type)) {
            m_diagnostics.error(symbol->location, "composites containing atomic or volatile parts are not yet supported");
        }
        if (symbol->kind == SymbolKind::TypeName && type != nullptr && checked.insert(type).second) {
            if (hasProtected(type) && (type->deferredLayout || type->tagged || type->variantOn >= 0)) {
                m_diagnostics.error(symbol->location,
                    "protected components require a complete ordinary record or array layout at declaration");
            }
        }
    }
    for (const auto& use : m_deferredProtectedUses) {
        if (hasProtected(use.first)) {
            m_diagnostics.error(use.second, "this value operation cannot copy, convert or stream a protected private completion");
        }
    }
    for (const auto& pending : m_pendingGenericInstances) {
        Symbol* generic = pending.instance->generic;
        if (!generic->generic->isPackage || requiresGenericBody(pending.instance->expansion)) {
            m_diagnostics.error(pending.instance->location,
                std::string(generic->generic->isPackage ? "generic package '" : "generic subprogram '")
                + generic->displayName + "' has no body");
        }
    }
    if (requireLibraryBodies && !m_diagnostics.hasErrors()) {
        for (const auto& symbol : m_symbolTable.symbols()) {
            if (symbol->kind == SymbolKind::TypeName && symbol->type != nullptr
                && symbol->type->isIncomplete && symbol->type->incompleteInPrivatePart) {
                m_diagnostics.error(symbol->location, "an incomplete declaration in a private part has no completion");
            }
            if (symbol->requiresBody && !symbol->hasBody && !symbol->missingBodyReported) {
                if (symbol->kind == SymbolKind::Generic && symbol->generic->isPackage) {
                    auto contract = m_genericContracts.find(symbol->generic);
                    if (contract == m_genericContracts.end() || !requiresGenericBody(contract->second->declarations)) {
                        continue;
                    }
                }
                std::string kind = symbol->kind == SymbolKind::Generic
                    ? (symbol->generic->isPackage ? "generic package" : "generic subprogram") : symbol->taskBody ? "task" : "subprogram";
                m_diagnostics.error(symbol->location, kind + " '" + symbol->displayName + "' has no body");
            }
        }
    }

}
