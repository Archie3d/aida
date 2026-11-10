#include "Sema.h"
#include "SemaSupport.h"

void Sema::declareProtectedOperation(Symbol* operation, Type* type)
{
    if (operation->protectedReceiver != nullptr) {
        return;
    }
    operation->protectedType = type;
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
    receiver->aliased = true;
    receiver->mode = operation->returnType != nullptr ? ParameterMode::In : ParameterMode::InOut;
    receiver->isConstant = receiver->mode == ParameterMode::In;
    operation->protectedReceiver = receiver;
}

Symbol* Sema::protectedReceiver(Type* type)
{
    for (Symbol* owner = m_currentSubprogram; owner != nullptr; owner = owner->owner) {
        if (owner->protectedType == type) {
            return owner->protectedReceiver;
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
    return names.size() == 1 && (names.front() == type->protectedObject
        || (names.front()->kind == SymbolKind::TypeName && names.front()->type == type
            && names.front()->name == type->protectedName));
}

void Sema::bindProtectedCall(Expr* expr, Symbol* operation, Expr* object, Scope* scope, bool requeue)
{
    if (operation->protectedType == nullptr) {
        return;
    }
    if (object != nullptr) {
        Type* receiver = operation->protectedType;
        for (Type* type : expressionTypes(object, scope)) {
            if (baseType(type)->kind == TypeKind::Access && baseType(baseType(type)->target) == operation->protectedType) {
                receiver = type;
            }
        }
        Type* type = analyzeExpr(object, scope, receiver);
        if (type != nullptr && baseType(type)->kind == TypeKind::Access) {
            type = baseType(type)->target;
        }
        if (baseType(type) != operation->protectedType) {
            m_diagnostics.error(expr->location, "protected operation requires an object of its protected type");
            return;
        }
        if (isProtectedCurrentInstance(object, operation->protectedType, scope)) {
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
        if (operation->entry && !requeue) {
            m_diagnostics.error(expr->location, "an entry call within a protected action is potentially blocking");
        }
        Symbol* receiver = protectedReceiver(operation->protectedType);
        if (receiver == nullptr) {
            m_diagnostics.error(expr->location, "an internal protected call requires the current protected instance");
            return;
        }
        if (operation->returnType == nullptr && receiver->mode == ParameterMode::In) {
            m_diagnostics.error(expr->location, "a protected function cannot call a procedure on its constant current instance");
        }
        noteReference(receiver);
        expr->protectedReceiver = receiver;
    } else {
        if (operation->returnType == nullptr) {
            checkAssignable(object, scope, true);
        }
        expr->protectedObject = object;
    }
}

void Sema::analyzeProtected(TypeDecl* decl, Scope* scope)
{
    Type* type = decl->declaredType;
    if (decl->protectedBody) {
        Symbol* named = lookupName(decl->lower, scope);
        type = named == nullptr ? nullptr : baseType(named->type);
        if (type == nullptr || type->protectedScope == nullptr) {
            m_diagnostics.error(decl->location, "protected body requires a protected declaration in the same declarative region");
            return;
        }
        auto declarations = scope->lookupDeclaration(named->name);
        bool local = false;
        for (Symbol* candidate : declarations) {
            local = local || candidate == named;
        }
        bool declaration = (named->kind == SymbolKind::TypeName && named->type == type && !type->isSubtype)
            || named == type->protectedObject;
        if (!local || !declaration || type->protectedName != decl->lower) {
            m_diagnostics.error(decl->location, "protected body must complete its local protected declaration");
            return;
        }
        if (type->protectedBodySeen) {
            m_diagnostics.error(decl->location, "protected unit already has a body");
            return;
        }
        type->protectedBodySeen = true;
        decl->declaredType = type;
    }
    if (type == nullptr) {
        return;
    }
    m_namePrefix.push_back(decl->protectedBody ? decl->lower
        : decl->taskObjects.empty() ? decl->lower
        : static_cast<ObjectDecl*>(decl->taskObjects.front().get())->namesLower.front());
    Scope* inner = type->protectedScope;
    if (!decl->protectedBody) {
        Scope* pragmaScope = m_symbolTable.createScope(scope);
        for (const FieldInfo& field : type->fields) {
            if (!field.isDiscriminant) { continue; }
            Symbol* discriminant = m_symbolTable.createSymbol(SymbolKind::Object, field.name, field.displayName);
            discriminant->type = field.type;
            discriminant->isConstant = true;
            discriminant->discriminantRecord = type;
            discriminant->discriminantIndex = field.index;
            pragmaScope->add(discriminant);
        }
        analyzeConcurrentPragmas(decl, pragmaScope);
        type->protectedName = decl->taskObjects.empty() ? decl->lower
            : static_cast<ObjectDecl*>(decl->taskObjects.front().get())->namesLower.front();
        for (const FieldInfo& field : type->fields) {
            if (field.name == type->protectedName) {
                m_diagnostics.error(decl->location, "protected component conflicts with the current instance name");
            }
            Symbol* component = m_symbolTable.createSymbol(SymbolKind::Object, field.name, field.displayName);
            component->type = field.type;
            component->protectedType = type;
            component->protectedField = field.index;
            component->location = decl->location;
            component->isConstant = field.isDiscriminant;
            inner->add(component);
        }
    } else {
        // The operation declarations remain in the protected region, while
        // names from the enclosing body/context must also be visible here.
        Scope* bodyScope = m_symbolTable.createScope(scope);
        for (const auto& [name, declarations] : inner->declarations()) {
            for (Symbol* declaration : declarations) { bodyScope->add(declaration); }
        }
        inner = bodyScope;
        inner->setCompletionScope(type->protectedScope);
    }
    Type* savedType = m_currentProtectedType;
    Scope* savedScope = m_protectedBodyScope;
    m_currentProtectedType = type;
    m_protectedBodyScope = decl->protectedBody ? inner : nullptr;
    for (const auto& operation : decl->protectedDeclarations) {
        SubprogramSpec* spec = nullptr;
        if (operation->kind == DeclKind::SubprogramDeclaration) {
            spec = &static_cast<SubprogramDecl*>(operation.get())->spec;
        } else {
            spec = &static_cast<SubprogramBody*>(operation.get())->spec;
        }
        if (spec->lower == type->protectedName) {
            m_diagnostics.error(operation->location, "protected operation conflicts with the current instance name");
        }
        if (operation->kind == DeclKind::SubprogramDeclaration) {
            OverridingIndicator indicator = spec->overriding;
            spec->overriding = OverridingIndicator::Unspecified;
            Symbol* symbol = declareSubprogram(*spec, inner, false);
            spec->overriding = indicator;
            declareProtectedOperation(symbol, type);
            bool implements = implementConcurrentOperation(type, symbol);
            if (indicator == OverridingIndicator::Overriding && !implements) {
                m_diagnostics.error(spec->location, "protected operation marked overriding implements no inherited operation");
            } else if (indicator == OverridingIndicator::NotOverriding && implements) {
                m_diagnostics.error(spec->location, "protected operation marked not overriding implements an inherited operation");
            }
            auto* declaration = static_cast<SubprogramDecl*>(operation.get());
            declaration->symbol = symbol;
            symbol->protectedPrivate = declaration->protectedPrivate;
        } else {
            auto* body = static_cast<SubprogramBody*>(operation.get());
            analyzeSubprogramBody(body, inner);
            if (body->symbol != nullptr && body->spec.entry) {
                Symbol* barrier = body->barrierBody->symbol;
                if (barrier != nullptr) {
                    barrier->qbeName = body->symbol->qbeName + ".barrier";
                }
                if (body->symbol->declarationScope == inner) {
                    m_diagnostics.error(body->location, "entry body requires a declaration in the protected definition");
                }
            }
        }
    }
    if (decl->protectedBody) {
        reportMissingBodies(decl->protectedDeclarations);
    }
    m_currentProtectedType = savedType;
    m_protectedBodyScope = savedScope;
    m_namePrefix.pop_back();
}

bool Sema::isTaskCurrentInstance(Expr* expr, Type* type, Scope* scope)
{
    auto names = expressionNames(expr, scope);
    if (names.size() != 1 || names.front()->type == nullptr || names.front()->type->isSubtype
        || (names.front()->kind != SymbolKind::TypeName && names.front()->kind != SymbolKind::Object)
        || names.front()->type->taskBody != type->taskBody
        || type->taskBody->name != "__task_" + names.front()->name) { return false; }
    for (Symbol* owner = m_currentSubprogram; owner != nullptr; owner = owner->owner) {
        if (owner == type->taskBody) { return true; }
    }
    return false;
}

bool Sema::implementConcurrentOperation(Type* type, Symbol* operation)
{
    bool implemented = false;
    for (Symbol* primitive : type->primitives) {
        if (primitive->name != operation->name || primitive->parameters.size() != operation->parameters.size() + 1
            || primitive->returnType != operation->returnType || primitive->parameters.empty()) { continue; }
        if (controllingParameterType(primitive->parameters.front()->type) != type) { continue; }
        bool matches = true;
        for (std::size_t i = 0; i < operation->parameters.size(); ++i) {
            Symbol* formal = primitive->parameters[i + 1];
            Symbol* actual = operation->parameters[i];
            matches = matches && SemaSupport::profileTypeMatches(formal->type, actual->type) && formal->mode == actual->mode;
        }
        if (!matches) { continue; }
        if (primitive->concurrentImplementation != nullptr) {
            m_diagnostics.error(operation->location, "more than one concurrent operation implements the inherited primitive");
            continue;
        }
        if (operation->entryIndexType != nullptr) { continue; }
        for (std::size_t i = 0; i < operation->parameters.size(); ++i) {
            if (!SemaSupport::staticallyMatches(primitive->parameters[i + 1]->type, operation->parameters[i]->type)) {
                m_diagnostics.error(operation->location, "a concurrent implementation must be subtype conformant with its inherited profile");
            }
        }
        Symbol* first = primitive->parameters.front();
        if (type->protectedScope != nullptr && operation->returnType == nullptr
            && first->mode == ParameterMode::In
            && (first->type->kind != TypeKind::Access || first->type->accessConstant)) {
            m_diagnostics.error(operation->location, "a protected procedure or entry requires a variable controlling parameter");
        }
        primitive->concurrentImplementation = operation;
        primitive->abstract = false;
        primitive->nullProcedure = false;
        implemented = true;
    }
    return implemented;
}
