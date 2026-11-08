#include "Sema.h"
#include "SemaSupport.h"

#include <unordered_set>

Symbol* Sema::bindPrefixedView(SelectedExpr* name, const std::vector<Type*>& types,
    const std::vector<ParameterMode>& modes, Type* result, Scope* scope,
    DeclList& expansion, const std::string& qbeName)
{
    int errors = m_diagnostics.errorCount();
    Symbol* chosen = nullptr;
    for (Symbol* candidate : prefixedOperations(name, scope)) {
        bool matches = SemaSupport::profileTypeMatches(candidate->returnType, result)
            && candidate->parameters.size() == types.size() + 1;
        for (std::size_t i = 0; matches && i < types.size(); ++i) {
            matches = SemaSupport::profileTypeMatches(candidate->parameters[i + 1]->type, types[i])
                && candidate->parameters[i + 1]->mode == modes[i];
        }
        if (!matches) { continue; }
        if (chosen != nullptr) {
            m_diagnostics.error(name->location, "ambiguous prefixed view profile");
            return nullptr;
        }
        chosen = candidate;
    }
    if (chosen == nullptr) {
        m_diagnostics.error(name->location, "no subprogram matches the reduced prefixed view profile");
        return nullptr;
    }
    Symbol* view = m_symbolTable.createSymbol(SymbolKind::Subprogram, qbeName, chosen->displayName);
    view->qbeName = qbeName;
    view->location = name->location;
    view->returnType = chosen->returnType;
    view->owner = m_currentSubprogram;
    view->level = view->owner != nullptr ? view->owner->level + 1 : 0;
    view->accessibilityLevel = m_accessibilityLevel;
    view->declarationScope = scope;
    view->hasBody = true;
    view->noReturn = chosen->noReturn;
    view->intrinsicRenaming = true;
    view->usesFinalization = true;
    Scope* operandsScope = m_symbolTable.createScope(scope);
    auto call = std::make_unique<CallExpr>();
    call->location = name->location;
    auto selected = std::make_unique<SelectedExpr>();
    selected->location = name->location;
    selected->selector = name->selector;
    selected->selectorLower = name->selectorLower;
    selected->prefix = std::move(name->prefix);
    call->callee = std::move(selected);
    for (std::size_t i = 1; i < chosen->parameters.size(); ++i) {
        Symbol* parameter = m_symbolTable.createSymbol(SymbolKind::Parameter,
            chosen->parameters[i]->name, chosen->parameters[i]->displayName);
        *parameter = *chosen->parameters[i];
        // Synthetic operands must not hide names in the receiver expression.
        parameter->name = "$prefixed_operand_" + std::to_string(i);
        parameter->owner = view;
        parameter->level = view->level;
        parameter->isUplevel = false;
        parameter->frameOffset = -1;
        view->parameters.push_back(parameter);
        operandsScope->add(parameter);
        auto operand = std::make_unique<IdentifierExpr>();
        operand->location = name->location;
        operand->name = parameter->displayName;
        operand->lower = parameter->name;
        Association argument;
        argument.value = std::move(operand);
        call->arguments.push_back(std::move(argument));
    }
    Symbol* saved = m_currentSubprogram;
    m_currentSubprogram = view;
    analyzeCall(call.get(), operandsScope, result);
    m_currentSubprogram = saved;
    if (m_diagnostics.errorCount() != errors) { return nullptr; }

    ExprPtr receiver = std::move(call->arguments.front().value);
    ExprPtr* value = &receiver;
    for (;;) {
        if ((*value)->kind == ExprKind::Qualified) {
            value = &static_cast<QualifiedExpr*>(value->get())->operand;
        } else if ((*value)->kind == ExprKind::Selected && static_cast<SelectedExpr*>(value->get())->symbol == nullptr
            && !static_cast<SelectedExpr*>(value->get())->isDereference) {
            value = &static_cast<SelectedExpr*>(value->get())->prefix;
        } else if ((*value)->kind == ExprKind::Call && static_cast<CallExpr*>(value->get())->form == CallForm::Indexing) {
            value = &static_cast<CallExpr*>(value->get())->callee;
        } else {
            break;
        }
    }
    Type* receiverLayout = (*value)->type;
    if ((*value)->kind != ExprKind::Aggregate && isComposite(receiverLayout) && !receiverLayout->tagged
        && !needsFinalization(receiverLayout) && SemaSupport::objectView(value->get()) == 0) {
        m_diagnostics.error(name->location, "binding this temporary receiver layout is not yet supported");
        return nullptr;
    }
    if ((*value)->kind == ExprKind::Aggregate) {
        // Materialize an aggregate value as an ordinary constant object. Its
        // controlled components must outlive declaration-temporary cleanup.
        Type* type = (*value)->type;
        if (type->kind != TypeKind::Record || hasRuntimeLayout(type)) {
            m_diagnostics.error(name->location, "binding this aggregate receiver layout is not yet supported");
            return nullptr;
        }
        std::string valueName = qbeName + ".value";
        Symbol* storage = m_symbolTable.createSymbol(SymbolKind::Object, valueName, valueName);
        storage->type = type;
        storage->location = name->location;
        storage->owner = saved;
        storage->level = saved != nullptr ? saved->level : 0;
        storage->isGlobal = saved == nullptr;
        storage->isConstant = true;
        storage->qbeName = valueName;
        scope->add(storage);
        auto object = std::make_unique<ObjectDecl>();
        object->location = name->location;
        object->isConstant = true;
        object->symbols.push_back(storage);
        object->initializer = std::move(*value);
        auto reference = std::make_unique<IdentifierExpr>();
        reference->location = name->location;
        reference->name = reference->lower = valueName;
        reference->symbol = storage;
        reference->type = type;
        *value = std::move(reference);
        expansion.push_back(std::move(object));
    }
    std::string bindingName = qbeName + ".receiver";
    Symbol* binding = m_symbolTable.createSymbol(SymbolKind::Object, bindingName, bindingName);
    binding->location = name->location;
    binding->type = receiver->type;
    binding->objectReference = binding->type->kind != TypeKind::Access;
    binding->boundReceiver = true;
    binding->isConstant = binding->objectReference ? SemaSupport::objectView(receiver.get()) != 2 : true;
    binding->aliased = binding->objectReference;
    binding->sharedAlias = binding->objectReference ? receiver.get() : nullptr;
    binding->owner = saved;
    binding->level = saved != nullptr ? saved->level : 0;
    binding->isGlobal = saved == nullptr;
    binding->isUplevel = !binding->isGlobal;
    binding->qbeName = bindingName;
    if (saved != nullptr) { saved->needsFrame = true; saved->usesFinalization = true; }
    else { m_finalizingUnits.insert(m_currentUnit); }
    scope->add(binding);
    auto identifier = [&]() {
        auto expression = std::make_unique<IdentifierExpr>();
        expression->location = name->location;
        expression->name = expression->lower = bindingName;
        expression->symbol = binding;
        expression->type = binding->type;
        return expression;
    };
    call->arguments.front().value = identifier();
    call->resolvedArguments.front() = call->arguments.front().value.get();
    name->prefix = identifier();
    name->symbol = view;
    auto object = std::make_unique<ObjectDecl>();
    object->location = name->location;
    object->isRenaming = binding->objectReference;
    object->isConstant = binding->isConstant;
    object->symbols.push_back(binding);
    object->initializer = std::move(receiver);
    expansion.push_back(std::move(object));
    auto body = std::make_unique<SubprogramBody>();
    body->location = name->location;
    body->symbol = view;
    if (result != nullptr) {
        auto statement = std::make_unique<ReturnStmt>();
        statement->location = name->location;
        statement->value = std::move(call);
        body->body.push_back(std::move(statement));
    } else {
        auto statement = std::make_unique<ProcedureCallStmt>();
        statement->location = name->location;
        statement->call = std::move(call);
        body->body.push_back(std::move(statement));
    }
    expansion.push_back(std::move(body));
    return view;
}

void Sema::analyzeSubprogramRenaming(SubprogramDecl* decl, Scope* scope)
{
    SubprogramSpec& spec = decl->spec;
    int errors = m_diagnostics.errorCount();
    for (std::size_t i = 0; i < decl->renamedTokens.size(); ++i) {
        const Token& token = decl->renamedTokens[i];
        bool selector = i > 0 && (decl->renamedTokens[i - 1].kind == TokenKind::Dot
            || decl->renamedTokens[i - 1].kind == TokenKind::Tick);
        bool association = i + 1 < decl->renamedTokens.size()
            && decl->renamedTokens[i + 1].kind == TokenKind::Arrow;
        if (token.kind == TokenKind::Identifier && !selector && !association) {
            for (const ParameterDecl& parameter : spec.parameters) {
                if (parameter.lower == token.lower) {
                    m_diagnostics.error(token.location, "a renaming target cannot reference a formal parameter");
                    return;
                }
            }
        }
    }
    std::vector<Type*> types;
    std::unordered_set<std::string> parameterNames;
    for (ParameterDecl& parameter : spec.parameters) {
        if (!parameterNames.insert(parameter.lower).second) {
            m_diagnostics.error(parameter.location, "duplicate parameter in subprogram renaming");
        }
        types.push_back(resolveSubtypeIndication(parameter.subtype.get(), scope));
    }
    Type* result = spec.isFunction ? resolveSubtypeIndication(spec.returnType.get(), scope) : nullptr;
    if (m_diagnostics.errorCount() != errors) {
        return;
    }
    auto matches = [&](Symbol* candidate) {
        if (candidate->kind != SymbolKind::Subprogram || !SemaSupport::profileTypeMatches(candidate->returnType, result)
            || candidate->parameters.size() != types.size()) {
            return false;
        }
        for (std::size_t i = 0; i < types.size(); ++i) {
            if (!SemaSupport::profileTypeMatches(candidate->parameters[i]->type, types[i])
                || candidate->parameters[i]->mode != spec.parameters[i].mode) {
                return false;
            }
        }
        return true;
    };

    // Resolve the target before adding the new name, so it cannot hide its target.
    Expr* name = decl->renamedName.get();
    Symbol* prefixedView = nullptr;
    if (name->kind == ExprKind::Selected
        && !prefixedOperations(static_cast<SelectedExpr*>(name), scope).empty()) {
        std::vector<ParameterMode> modes;
        for (const ParameterDecl& parameter : spec.parameters) { modes.push_back(parameter.mode); }
        prefixedView = bindPrefixedView(static_cast<SelectedExpr*>(name), types, modes, result, scope,
            decl->renamingExpansion, "$" + mangle(spec.lower) + ".prefixed."
                + std::to_string(decl->location.line) + "." + std::to_string(decl->location.column));
        if (prefixedView == nullptr) { return; }
    }
    std::vector<Symbol*> candidates = prefixedView != nullptr ? std::vector<Symbol*> { prefixedView } : expressionNames(name, scope);
    std::string operation;
    Scope* operatorScope = nullptr;
    if (name->kind == ExprKind::StringLiteral) {
        operation = operatorSymbol(Unicode::utf8(static_cast<StringLiteralExpr*>(name)->value));
        candidates = contractNames(name, scope->lookup(operation));
    }
    if (name->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(name);
        Symbol* prefix = expandedPrefix(selected->prefix.get(), scope);
        if (prefix != nullptr) {
            operation = operatorSymbol(selected->selectorLower);
            operatorScope = expandedScope(prefix, scope);
        }
    }
    Symbol* chosen = nullptr;
    for (Symbol* candidate : candidates) {
        bool enumeration = candidate->kind == SymbolKind::EnumerationLiteral && types.empty()
            && spec.isFunction && baseType(candidate->type) == baseType(result);
        if (matches(candidate) || enumeration) {
            if (chosen != nullptr) {
                m_diagnostics.error(name->location, "ambiguous renamed subprogram");
                return;
            }
            chosen = candidate;
        }
    }

    if (chosen != nullptr && chosen->protectedType != nullptr) {
        m_diagnostics.error(decl->location, "renaming protected operations is not yet supported");
        return;
    }
    Type* access = nullptr;
    if (name->kind == ExprKind::Selected && static_cast<SelectedExpr*>(name)->isDereference) {
        auto* selected = static_cast<SelectedExpr*>(name);
        for (Type* type : expressionTypes(selected->prefix.get(), scope)) {
            if (type->accessProfile != nullptr && matches(type->accessProfile)) {
                if (access != nullptr) {
                    m_diagnostics.error(name->location, "ambiguous renamed subprogram");
                    return;
                }
                access = type;
                chosen = type->accessProfile;
            }
        }
    }

    // Probe predefined operators with exactly the declared operand types.
    bool intrinsic = false;
    std::vector<Type*> intrinsicParameters;
    Type* intrinsicResult = nullptr;
    if (!operation.empty() && chosen == nullptr) {
        Scope* probes = m_symbolTable.createScope(scope);
        std::vector<ExprPtr> expressions;
        std::vector<Expr*> operands;
        for (std::size_t i = 0; i < types.size(); ++i) {
            std::string probeName = "$rename_operand_" + std::to_string(i);
            Symbol* probe = m_symbolTable.createSymbol(SymbolKind::Object, probeName, probeName);
            probe->type = types[i];
            probes->add(probe);
            auto operand = std::make_unique<IdentifierExpr>();
            operand->name = probeName;
            operand->lower = probeName;
            operands.push_back(operand.get());
            expressions.push_back(std::move(operand));
        }
        for (const OperatorCandidate& candidate : operatorCandidates(operation, operands, probes, result, false, {}, operatorScope)) {
            bool conforms = spec.isFunction && candidate.parameters.size() == types.size()
                && baseType(candidate.result) == baseType(result);
            for (std::size_t i = 0; conforms && i < types.size(); ++i) {
                conforms = baseType(candidate.parameters[i]) == baseType(types[i])
                    && spec.parameters[i].mode == ParameterMode::In;
            }
            if (conforms && candidate.symbol == nullptr) {
                intrinsic = true;
                intrinsicParameters = candidate.parameters;
                intrinsicResult = candidate.result;
            }
        }
    }
    Type* attributeType = nullptr;
    Symbol* attributeProfile = nullptr;
    if (chosen == nullptr && name->kind == ExprKind::Attribute) {
        auto* attribute = static_cast<AttributeExpr*>(name);
        Symbol* profile = attributeSubprogram(attribute, scope);
        if (profile != nullptr && matches(profile)) {
            attributeType = profile->intrinsicPrefix;
            attributeProfile = profile;
        }
    }
    if (chosen == nullptr && !intrinsic && attributeType == nullptr) {
        m_diagnostics.error(name->location, "no subprogram matches the renaming profile");
        return;
    }

    if (chosen != nullptr) {
        if (spec.returnType != nullptr && spec.returnType->nullExclusion && !chosen->returnType->excludesNull) {
            m_diagnostics.error(spec.location, "a null-excluding renaming result requires a result that excludes null");
        }
        for (std::size_t i = 0; i < types.size(); ++i) {
            if (spec.parameters[i].subtype->nullExclusion && !chosen->parameters[i]->type->excludesNull) {
                m_diagnostics.error(spec.parameters[i].location, "a null-excluding renaming parameter requires a parameter that excludes null");
            }
        }
    }
    Symbol* completion = nullptr;
    for (Symbol* existing : scope->lookupDeclaration(spec.lower)) {
        if (existing->inheritedFrom != nullptr) {
            continue;
        }
        bool sameTypes = existing->kind == SymbolKind::Subprogram
            && SemaSupport::profileTypeMatches(existing->returnType, result) && existing->parameters.size() == types.size();
        for (std::size_t i = 0; sameTypes && i < types.size(); ++i) {
            sameTypes = SemaSupport::profileTypeMatches(existing->parameters[i]->type, types[i]);
        }
        if (existing->kind != SymbolKind::Subprogram || sameTypes) {
            if (existing->kind != SymbolKind::Subprogram || existing->hasBody) {
                m_diagnostics.error(decl->location, "'" + spec.name + "' has already been declared in this scope");
                return;
            }
            completion = existing;
        }
    }
    if (completion != nullptr) {
        bool conforms = SemaSupport::staticallyMatches(completion->returnType, result)
            || (completion->returnType == nullptr && result == nullptr);
        for (std::size_t i = 0; i < types.size(); ++i) {
            Symbol* parameter = completion->parameters[i];
            if (completion->controllingType != nullptr && types[i]->anonymousAccess
                && controllingParameterType(types[i]) == completion->controllingType) {
                types[i]->excludesNull = true;
            }
            conforms = conforms && parameter->explicitNullExclusion == spec.parameters[i].subtype->nullExclusion
                && parameter->name == spec.parameters[i].lower
                && parameter->mode == spec.parameters[i].mode
                && SemaSupport::staticallyMatches(parameter->type, types[i]);
            if (spec.parameters[i].defaultValue != nullptr) {
                analyzeExpr(spec.parameters[i].defaultValue.get(), scope, types[i]);
                conforms = conforms && SemaSupport::conformingExpressions(parameter->defaultExpr,
                    spec.parameters[i].defaultValue.get());
            }
        }
        if (!conforms) {
            m_diagnostics.error(decl->location, "renaming-as-body does not conform to the previous declaration");
            return;
        }
        std::unordered_set<Symbol*> seen;
        for (Symbol* target = chosen; target != nullptr; target = target->renamedSubprogram) {
            if (target == completion || !seen.insert(target).second) {
                m_diagnostics.error(decl->location, "a subprogram cannot rename itself");
                return;
            }
        }
    }

    if (completion != nullptr) { checkOverridingIndicator(completion, spec.overriding, spec.location); }
    spec.abstract = chosen != nullptr && chosen->abstract;
    if (completion != nullptr && spec.abstract) {
        m_diagnostics.error(decl->location, "a renaming-as-body cannot rename an abstract subprogram");
    }
    Symbol* alias = completion != nullptr ? completion : declareSubprogram(spec, scope, false);
    if (completion != nullptr) {
        for (std::size_t i = 0; i < spec.parameters.size(); ++i) {
            spec.parameters[i].symbol = alias->parameters[i];
        }
        if (m_recordContract != nullptr) {
            m_recordContract->subprograms[contractKey(spec.location)] = alias;
        }
    }
    decl->symbol = alias;
    if (completion != nullptr) { m_noReturnRenamings.emplace_back(alias, chosen); }
    if (chosen != nullptr && chosen->noReturn) { alias->noReturn = true; }
    if (completion != nullptr && completion->abstract) {
        m_diagnostics.error(decl->location, "an abstract subprogram cannot have a completion");
    }
    alias->abstract = chosen != nullptr && chosen->abstract;
    if (chosen != nullptr && alias->controllingType != nullptr) {
        for (std::size_t i = 0; i < alias->parameters.size(); ++i) {
            Type* parameter = alias->parameters[i]->type;
            if (parameter->accessParameter && controllingParameterType(parameter) == alias->controllingType
                && !chosen->parameters[i]->type->excludesNull) {
                m_diagnostics.error(decl->location, "a renamed controlling access parameter must exclude null");
            }
        }
    }
    alias->hasBody = !alias->abstract;
    if (chosen != nullptr && (chosen->classWideWrapper || chosen->classWideResultRenaming)) {
        bool controllingParameters = false;
        for (Symbol* parameter : chosen->parameters) {
            controllingParameters = controllingParameters
                || controllingParameterType(parameter->type) == chosen->controllingType;
        }
        alias->classWideResultRenaming = chosen->classWideResultRenaming
            || (!controllingParameters && controllingResultType(chosen->returnType) == chosen->controllingType);
    }
    alias->requiresBody = false;
    if (m_diagnostics.errorCount() != errors) {
        return;
    }
    if (chosen != nullptr && access == nullptr) {
        recordContractName(name, chosen);
    }
    bool enumeration = chosen != nullptr && chosen->kind == SymbolKind::EnumerationLiteral;
    if (completion == nullptr && chosen != nullptr && !enumeration) {
        // Only the names and defaults come from the new specification.
        alias->returnType = chosen->returnType;
        for (std::size_t i = 0; i < types.size(); ++i) {
            alias->parameters[i]->type = chosen->parameters[i]->type;
            alias->parameters[i]->byReference = chosen->parameters[i]->byReference;
        }
    }

    if (completion == nullptr && intrinsic) {
        alias->returnType = isDiscrete(intrinsicResult) || isReal(intrinsicResult)
            ? m_types.scalarBaseType(intrinsicResult) : intrinsicResult;
        for (std::size_t i = 0; i < types.size(); ++i) {
            Type* type = intrinsicParameters[i];
            alias->parameters[i]->type = isDiscrete(type) || isReal(type) ? m_types.scalarBaseType(type) : type;
            alias->parameters[i]->byReference = isComposite(intrinsicParameters[i]);
        }
    }
    if (completion == nullptr && attributeType != nullptr) {
        alias->returnType = attributeProfile->returnType;
        for (std::size_t i = 0; i < types.size(); ++i) {
            alias->parameters[i]->type = attributeProfile->parameters[i]->type;
            alias->parameters[i]->byReference = attributeProfile->parameters[i]->byReference;
        }
    }
    if (completion == nullptr && enumeration) {
        alias->returnType = chosen->type;
    }

    Symbol* binding = nullptr;
    if (access != nullptr) {
        auto* selected = static_cast<SelectedExpr*>(name);
        ExprPtr prefix = std::move(selected->prefix);
        analyzeExpr(prefix.get(), scope, access);
        std::string bindingName = "$renamed_callback_" + std::to_string(m_symbolTable.symbols().size());
        binding = m_symbolTable.createSymbol(SymbolKind::Object, bindingName, bindingName);
        binding->type = access;
        binding->location = decl->location;
        binding->owner = m_currentSubprogram;
        binding->level = m_currentSubprogram != nullptr ? m_currentSubprogram->level : 0;
        binding->isGlobal = m_currentSubprogram == nullptr;
        binding->qbeName = alias->qbeName + ".binding";
        binding->isUplevel = !binding->isGlobal;
        if (binding->owner != nullptr) {
            binding->owner->needsFrame = true;
        }
        auto object = std::make_unique<ObjectDecl>();
        object->location = decl->location;
        object->symbols.push_back(binding);
        object->initializer = std::move(prefix);
        decl->renamingExpansion.push_back(std::move(object));
        decl->callbackBinding = binding;
    }
    if (completion == nullptr && !intrinsic && !enumeration && attributeType == nullptr) {
        alias->renamedSubprogram = access == nullptr ? chosen : nullptr;
        alias->renamedAccess = binding;
        if (scope != m_unitScope || alias->abstract) {
            return;
        }
    }

    // A completion owns a callable body and keeps the original declaration's
    // profile. Intrinsics and library renamings also need an emitted entry point.
    auto body = std::make_unique<SubprogramBody>();
    body->location = decl->location;
    body->symbol = alias;
    Scope* operandsScope = m_symbolTable.createScope(nullptr);
    std::vector<ExprPtr> operands;
    for (Symbol* parameter : alias->parameters) {
        operandsScope->add(parameter);
        auto operand = std::make_unique<IdentifierExpr>();
        operand->location = decl->location;
        operand->name = parameter->displayName;
        operand->lower = parameter->name;
        operands.push_back(std::move(operand));
    }
    Symbol* saved = m_currentSubprogram;
    m_currentSubprogram = alias;
    ExprPtr implementation;
    std::string attributeIntrinsic;
    if (attributeType != nullptr) {
        auto* attribute = static_cast<AttributeExpr*>(name);
        attributeIntrinsic = "attribute:" + attribute->lower;
        for (ExprPtr& operand : operands) {
            analyzeExpr(operand.get(), operandsScope);
            attribute->arguments.push_back(std::move(operand));
        }
        // The prefix resolves in the declaration's scope; parameter operands
        // have already been bound to the generated body's own parameters.
        Scope* attributeScope = m_symbolTable.createScope(scope);
        for (Symbol* parameter : alias->parameters) {
            attributeScope->add(parameter);
        }
        analyzeExpr(attribute, attributeScope, alias->returnType);
        implementation = std::move(decl->renamedName);
    } else if (intrinsic) {
        CallExpr call;
        call.location = decl->location;
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
            // The declaration's lookup already selected this intrinsic.
            if (implementation->kind == ExprKind::Unary) {
                static_cast<UnaryExpr*>(implementation.get())->predefinedOnly = true;
            } else if (implementation->kind == ExprKind::Binary) {
                static_cast<BinaryExpr*>(implementation.get())->predefinedOnly = true;
            }
            analyzeExpr(implementation.get(), operandsScope, alias->returnType);
        }
    } else if (enumeration) {
        auto value = std::make_unique<IntegerLiteralExpr>();
        value->type = chosen->type;
        value->isStatic = true;
        value->value = chosen->enumerationValue;
        value->staticValue = chosen->enumerationValue;
        implementation = std::move(value);
    } else {
        implementation = bindOperator(chosen, std::move(operands), operandsScope, decl->location);
        if (binding != nullptr) {
            auto* call = static_cast<CallExpr*>(implementation.get());
            auto callee = std::make_unique<IdentifierExpr>();
            callee->symbol = binding;
            callee->type = binding->type;
            callee->location = decl->location;
            call->callee = std::move(callee);
            call->indirect = true;
        }
    }
    m_currentSubprogram = saved;
    alias->intrinsicRenaming = intrinsic || enumeration || attributeType != nullptr || prefixedView != nullptr;
    for (Symbol* complement : scope->lookupLocal("/=")) {
        if (complement->negatedEquality == alias) {
            complement->intrinsicRenaming = alias->intrinsicRenaming;
        }
    }
    if (completion == nullptr) {
        alias->genericActualEntity = enumeration ? chosen : nullptr;
        alias->genericIntrinsic = intrinsic ? operation : attributeType != nullptr
            ? attributeIntrinsic : std::string();
        alias->intrinsicPrefix = attributeType != nullptr ? baseType(attributeType) : nullptr;
    }
    if (implementation == nullptr) {
        m_diagnostics.error(name->location, "unsupported renamed subprogram");
        return;
    }
    if (alias->returnType != nullptr) {
        auto statement = std::make_unique<ReturnStmt>();
        statement->location = decl->location;
        statement->value = std::move(implementation);
        body->body.push_back(std::move(statement));
    } else {
        auto statement = std::make_unique<ProcedureCallStmt>();
        statement->location = decl->location;
        statement->call = std::move(implementation);
        body->body.push_back(std::move(statement));
    }
    decl->renamingExpansion.push_back(std::move(body));
}

// A renamed unit shares the original declarations and generic contract. Only
// its defining name and visibility belong to this new view.
Symbol* Sema::declareUnitRenaming(const std::string& name, const std::string& lower,
                                 const std::string& renamed, const std::string& renamedLower,
                                 SourceLocation location, Scope* scope, bool isGeneric,
                                 bool isPackage, bool isFunction)
{
    std::string key = contractKey(location) + ":" + renamedLower;
    Symbol* original = nullptr;
    if (m_replayContract != nullptr) {
        auto found = m_replayContract->unitNames.find(key);
        if (found != m_replayContract->unitNames.end()) {
            original = instanceSymbol(found->second);
            if (original == nullptr) {
                m_diagnostics.error(location, "cannot map a renamed unit to its generic instance");
                return nullptr;
            }
        }
    }
    if (original == nullptr) {
        original = lookupName(renamedLower, scope);
    }
    if (m_recordContract != nullptr && original != nullptr) {
        m_recordContract->unitNames[key] = original;
    }
    Symbol* renamedView = original;
    if (original != nullptr && original->renamedUnit != nullptr) {
        original = original->renamedUnit;
    }
    bool matches = original != nullptr && (isGeneric
        ? original->kind == SymbolKind::Generic && original->generic != nullptr
            && original->generic->isPackage == isPackage && original->generic->isFunction == isFunction
        : original->kind == SymbolKind::Package);
    if (!matches) {
        std::string kind = isGeneric ? isPackage ? "a generic package" : isFunction ? "a generic function" : "a generic procedure"
            : "a package";
        m_diagnostics.error(location, "'" + renamed + "' does not denote " + kind);
        return nullptr;
    }
    if (original->kind == SymbolKind::Generic && original->parentPackage != nullptr
        && original->parentPackage->kind == SymbolKind::Generic
        && scope->immediateDepth(original->parentPackage->scope) < 0) {
        m_diagnostics.error(location, "a generic child must be selected through a parent instance");
        return nullptr;
    }
    if (scope == m_unitScope && renamedView->libraryName.empty()) {
        m_diagnostics.error(location, "a library unit renaming must rename a library unit");
        return nullptr;
    }
    Scope* home = scope;
    std::string simple = lower;
    std::string display = name;
    auto dot = lower.rfind('.');
    if (dot != std::string::npos) {
        std::size_t pushed = 0;
        Symbol* parent = declarePackagePath(lower.substr(0, dot), name.substr(0, dot), scope, location, pushed, true);
        for (std::size_t i = 0; i < pushed; ++i) {
            m_namePrefix.pop_back();
        }
        home = parent->scope;
        simple = lower.substr(dot + 1);
        display = name.substr(dot + 1);
    }
    if (!home->lookupDeclaration(simple).empty()) {
        m_diagnostics.error(location, "'" + name + "' has already been declared in this scope");
        return nullptr;
    }
    Symbol* alias = m_symbolTable.createSymbol(original->kind, simple, display);
    alias->location = location;
    alias->renamedUnit = original;
    alias->declarationScope = home;
    alias->scope = original->scope;
    alias->generic = original->generic;
    alias->hasSpecification = true;
    alias->hasBody = true;
    home->add(alias);
    return alias;
}
