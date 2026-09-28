#include "Sema.h"
#include "SemaSupport.h"

#include <unordered_set>

namespace
{

bool sameDefault(const std::vector<Token>& left, const std::vector<Token>& right)
{
    if (left.size() != right.size()) {
        return false;
    }
    for (std::size_t i = 0; i < left.size(); ++i) {
        if (left[i].kind != right[i].kind) {
            return false;
        }
        if (left[i].kind == TokenKind::IntegerLiteral) {
            if (left[i].intValue != right[i].intValue) {
                return false;
            }
        } else if (left[i].kind == TokenKind::RealLiteral) {
            if (left[i].realValue != right[i].realValue) {
                return false;
            }
        } else if (left[i].kind == TokenKind::Identifier ? left[i].lower != right[i].lower : left[i].text != right[i].text) {
            return false;
        }
    }
    return true;
}

}

void Sema::analyzeSubprogramRenaming(SubprogramDecl* decl, Scope* scope)
{
    SubprogramSpec& spec = decl->spec;
    int errors = m_diagnostics.errorCount();
    for (std::size_t i = 0; i < decl->m_renamedTokens.size(); ++i) {
        const Token& token = decl->m_renamedTokens[i];
        bool selector = i > 0 && (decl->m_renamedTokens[i - 1].kind == TokenKind::Dot
            || decl->m_renamedTokens[i - 1].kind == TokenKind::Tick);
        bool association = i + 1 < decl->m_renamedTokens.size()
            && decl->m_renamedTokens[i + 1].kind == TokenKind::Arrow;
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
        if (candidate->kind != SymbolKind::Subprogram || baseType(candidate->returnType) != baseType(result)
            || candidate->parameters.size() != types.size()) {
            return false;
        }
        for (std::size_t i = 0; i < types.size(); ++i) {
            if (baseType(candidate->parameters[i]->type) != baseType(types[i])
                || candidate->parameters[i]->mode != spec.parameters[i].mode) {
                return false;
            }
        }
        return true;
    };

    // Resolve the target before adding the new name, so it cannot hide its target.
    Expr* name = decl->m_renamedName.get();
    std::vector<Symbol*> candidates = expressionNames(name, scope);
    std::string operation;
    if (name->kind == ExprKind::StringLiteral) {
        operation = operatorSymbol(static_cast<StringLiteralExpr*>(name)->value);
        candidates = contractNames(name, scope->lookup(operation));
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

    Type* access = nullptr;
    if (name->kind == ExprKind::Selected && static_cast<SelectedExpr*>(name)->isDereference) {
        auto* selected = static_cast<SelectedExpr*>(name);
        for (Type* type : expressionTypes(selected->prefix.get(), scope)) {
            if (type->m_accessProfile != nullptr && matches(type->m_accessProfile)) {
                if (access != nullptr) {
                    m_diagnostics.error(name->location, "ambiguous renamed subprogram");
                    return;
                }
                access = type;
                chosen = type->m_accessProfile;
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
        for (const OperatorCandidate& candidate : operatorCandidates(operation, operands, probes, result)) {
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
    if (chosen == nullptr && name->kind == ExprKind::Attribute) {
        auto* attribute = static_cast<AttributeExpr*>(name);
        for (Symbol* prefix : expressionNames(attribute->prefix.get(), scope)) {
            if (prefix->kind == SymbolKind::TypeName) {
                attributeType = prefix->type;
                break;
            }
        }
        bool conforms = attributeType != nullptr && attribute->arguments.empty() && spec.isFunction
            && types.size() == 1 && spec.parameters.front().mode == ParameterMode::In;
        if (conforms) {
            const std::string& operation = attribute->lower;
            if (operation == "succ" || operation == "pred") {
                conforms = isDiscrete(attributeType) && baseType(types[0]) == baseType(attributeType)
                    && baseType(result) == baseType(attributeType);
            } else if (operation == "image") {
                conforms = (isDiscrete(attributeType) || isReal(attributeType)) && baseType(types[0]) == baseType(attributeType)
                    && baseType(result) == baseType(m_types.stringType());
            } else if (operation == "value") {
                conforms = (isDiscrete(attributeType) || isReal(attributeType)) && baseType(types[0]) == baseType(m_types.stringType())
                    && baseType(result) == baseType(attributeType);
            } else {
                conforms = false;
            }
        }
        if (!conforms) {
            attributeType = nullptr;
        }
    }
    if (chosen == nullptr && !intrinsic && attributeType == nullptr) {
        m_diagnostics.error(name->location, "no subprogram matches the renaming profile");
        return;
    }

    Symbol* completion = nullptr;
    for (Symbol* existing : scope->lookupLocal(spec.lower)) {
        if (existing->m_inheritedFrom != nullptr) {
            continue;
        }
        bool sameTypes = existing->kind == SymbolKind::Subprogram
            && baseType(existing->returnType) == baseType(result) && existing->parameters.size() == types.size();
        for (std::size_t i = 0; sameTypes && i < types.size(); ++i) {
            sameTypes = baseType(existing->parameters[i]->type) == baseType(types[i]);
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
            conforms = conforms && parameter->name == spec.parameters[i].lower
                && parameter->mode == spec.parameters[i].mode
                && SemaSupport::staticallyMatches(parameter->type, types[i]);
            if (spec.parameters[i].defaultValue != nullptr) {
                conforms = conforms && sameDefault(parameter->m_defaultTokens, spec.parameters[i].m_defaultTokens);
            }
        }
        if (!conforms) {
            m_diagnostics.error(decl->location, "renaming-as-body does not conform to the previous declaration");
            return;
        }
        std::unordered_set<Symbol*> seen;
        for (Symbol* target = chosen; target != nullptr; target = target->m_renamedSubprogram) {
            if (target == completion || !seen.insert(target).second) {
                m_diagnostics.error(decl->location, "a subprogram cannot rename itself");
                return;
            }
        }
    }

    Symbol* alias = completion != nullptr ? completion : declareSubprogram(spec, scope, false);
    if (completion != nullptr) {
        for (std::size_t i = 0; i < spec.parameters.size(); ++i) {
            spec.parameters[i].symbol = alias->parameters[i];
        }
        if (m_recordContract != nullptr) {
            m_recordContract->m_subprograms[contractKey(spec.location)] = alias;
        }
    }
    decl->symbol = alias;
    alias->hasBody = true;
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
        const std::string& attribute = static_cast<AttributeExpr*>(name)->lower;
        Type* scalar = m_types.scalarBaseType(attributeType);
        alias->parameters.front()->type = attribute == "value" ? m_types.stringType() : scalar;
        alias->parameters.front()->byReference = attribute == "value";
        alias->returnType = attribute == "image" ? m_types.stringType() : scalar;
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
        decl->m_renamingExpansion.push_back(std::move(object));
        decl->m_callbackBinding = binding;
    }
    if (completion == nullptr && !intrinsic && !enumeration && attributeType == nullptr) {
        alias->m_renamedSubprogram = access == nullptr ? chosen : nullptr;
        alias->m_renamedAccess = binding;
        if (scope != m_globalScope) {
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
    if (attributeType != nullptr) {
        auto* attribute = static_cast<AttributeExpr*>(name);
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
        implementation = std::move(decl->m_renamedName);
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
        implementation = explicitOperator(&call);
        if (implementation != nullptr) {
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
            call->m_indirect = true;
        }
    }
    m_currentSubprogram = saved;
    alias->m_intrinsicRenaming = intrinsic || enumeration || attributeType != nullptr;
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
    decl->m_renamingExpansion.push_back(std::move(body));
}
