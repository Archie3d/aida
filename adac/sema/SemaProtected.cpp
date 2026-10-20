#include "Sema.h"

void Sema::declareProtectedOperation(Symbol* operation, Type* type)
{
    if (operation->m_protectedReceiver != nullptr) {
        return;
    }
    operation->m_protectedType = type;
    if (operation->returnType != nullptr) {
        for (Symbol* parameter : operation->parameters) {
            if (parameter->mode != ParameterMode::In) {
                m_diagnostics.error(operation->location, "protected functions require in parameters");
            }
        }
    }
    Symbol* receiver = m_symbolTable.createSymbol(SymbolKind::Parameter, "__protected_self", type->name);
    receiver->type = type;
    receiver->owner = operation;
    receiver->level = operation->level;
    receiver->byReference = true;
    receiver->mode = operation->returnType != nullptr ? ParameterMode::In : ParameterMode::InOut;
    receiver->isConstant = receiver->mode == ParameterMode::In;
    operation->m_protectedReceiver = receiver;
}

Symbol* Sema::protectedReceiver(Type* type)
{
    for (Symbol* owner = m_currentSubprogram; owner != nullptr; owner = owner->owner) {
        if (owner->m_protectedType == type) {
            return owner->m_protectedReceiver;
        }
    }
    return nullptr;
}

bool Sema::isProtectedCurrentInstance(Expr* expr, Type* type, Scope* scope)
{
    if (protectedReceiver(type) == nullptr) {
        return false;
    }
    auto names = expressionNames(expr, scope);
    return names.size() == 1 && (names.front() == type->m_protectedObject
        || (names.front()->kind == SymbolKind::TypeName && names.front()->type == type
            && names.front()->name == type->m_protectedName));
}

void Sema::bindProtectedCall(Expr* expr, Symbol* operation, Expr* object, Scope* scope)
{
    if (operation->m_protectedType == nullptr) {
        return;
    }
    if (object != nullptr) {
        Type* type = analyzeExpr(object, scope, nullptr);
        if (type != nullptr && baseType(type)->kind == TypeKind::Access) {
            type = baseType(type)->target;
        }
        if (baseType(type) != operation->m_protectedType) {
            m_diagnostics.error(expr->location, "protected operation requires an object of its protected type");
            return;
        }
        if (isProtectedCurrentInstance(object, operation->m_protectedType, scope)) {
            object = nullptr;
        } else {
            auto names = expressionNames(object, scope);
            if (names.size() == 1 && names.front()->kind == SymbolKind::TypeName) {
                m_diagnostics.error(expr->location, "a protected operation requires an object, not a type name");
                return;
            }
        }
    }
    if (object == nullptr) {
        if (operation->m_entry) {
            m_diagnostics.error(expr->location, "an entry call within a protected action is potentially blocking");
        }
        Symbol* receiver = protectedReceiver(operation->m_protectedType);
        if (receiver == nullptr) {
            m_diagnostics.error(expr->location, "an internal protected call requires the current protected instance");
            return;
        }
        if (operation->returnType == nullptr && receiver->mode == ParameterMode::In) {
            m_diagnostics.error(expr->location, "a protected function cannot call a procedure on its constant current instance");
        }
        noteReference(receiver);
        expr->m_protectedReceiver = receiver;
    } else {
        if (operation->returnType == nullptr) {
            checkAssignable(object, scope, true);
        }
        expr->m_protectedObject = object;
    }
}

void Sema::analyzeProtected(TypeDecl* decl, Scope* scope)
{
    Type* type = decl->declaredType;
    if (decl->m_protectedBody) {
        Symbol* named = lookupName(decl->lower, scope);
        type = named == nullptr ? nullptr : baseType(named->type);
        if (type == nullptr || type->m_protectedScope == nullptr) {
            m_diagnostics.error(decl->location, "protected body requires a protected declaration in the same declarative region");
            return;
        }
        auto declarations = scope->lookupDeclaration(named->name);
        bool local = false;
        for (Symbol* candidate : declarations) {
            local = local || candidate == named;
        }
        bool declaration = (named->kind == SymbolKind::TypeName && named->type == type && !type->isSubtype)
            || named == type->m_protectedObject;
        if (!local || !declaration || type->m_protectedName != decl->lower) {
            m_diagnostics.error(decl->location, "protected body must complete its local protected declaration");
            return;
        }
        if (type->m_protectedBodySeen) {
            m_diagnostics.error(decl->location, "protected unit already has a body");
            return;
        }
        type->m_protectedBodySeen = true;
        decl->declaredType = type;
    }
    if (type == nullptr) {
        return;
    }
    m_namePrefix.push_back(decl->m_protectedBody ? decl->lower
        : decl->m_taskObjects.empty() ? decl->lower
        : static_cast<ObjectDecl*>(decl->m_taskObjects.front().get())->namesLower.front());
    Scope* inner = type->m_protectedScope;
    if (!decl->m_protectedBody) {
        type->m_protectedName = decl->m_taskObjects.empty() ? decl->lower
            : static_cast<ObjectDecl*>(decl->m_taskObjects.front().get())->namesLower.front();
        for (const FieldInfo& field : type->fields) {
            if (field.name == type->m_protectedName) {
                m_diagnostics.error(decl->location, "protected component conflicts with the current instance name");
            }
            Symbol* component = m_symbolTable.createSymbol(SymbolKind::Object, field.name, field.displayName);
            component->type = field.type;
            component->m_protectedType = type;
            component->m_protectedField = field.index;
            component->location = decl->location;
            inner->add(component);
        }
    } else {
        inner = m_symbolTable.createScope(inner);
        inner->setCompletionScope(type->m_protectedScope);
    }
    Type* savedType = m_currentProtectedType;
    Scope* savedScope = m_protectedBodyScope;
    m_currentProtectedType = type;
    m_protectedBodyScope = decl->m_protectedBody ? inner : nullptr;
    for (const auto& operation : decl->m_protectedDeclarations) {
        SubprogramSpec* spec = nullptr;
        if (operation->kind == DeclKind::SubprogramDeclaration) {
            spec = &static_cast<SubprogramDecl*>(operation.get())->spec;
        } else {
            spec = &static_cast<SubprogramBody*>(operation.get())->spec;
        }
        if (spec->lower == type->m_protectedName) {
            m_diagnostics.error(operation->location, "protected operation conflicts with the current instance name");
        }
        if (operation->kind == DeclKind::SubprogramDeclaration) {
            Symbol* symbol = declareSubprogram(*spec, inner, false);
            declareProtectedOperation(symbol, type);
            auto* declaration = static_cast<SubprogramDecl*>(operation.get());
            declaration->symbol = symbol;
            symbol->m_protectedPrivate = declaration->m_protectedPrivate;
        } else {
            auto* body = static_cast<SubprogramBody*>(operation.get());
            analyzeSubprogramBody(body, inner);
            if (body->symbol != nullptr && body->spec.m_entry) {
                Symbol* barrier = body->m_barrierBody->symbol;
                if (barrier != nullptr) {
                    barrier->qbeName = body->symbol->qbeName + ".barrier";
                }
                if (body->symbol->m_declarationScope == inner) {
                    m_diagnostics.error(body->location, "entry body requires a declaration in the protected definition");
                }
            }
        }
    }
    if (decl->m_protectedBody) {
        reportMissingBodies(decl->m_protectedDeclarations);
    }
    m_currentProtectedType = savedType;
    m_protectedBodyScope = savedScope;
    m_namePrefix.pop_back();
}
