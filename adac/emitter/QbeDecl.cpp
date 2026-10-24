#include "QbeEmitter.h"
#include "QbeSupport.h"

#include <cctype>
#include <cfloat>

using QbeSupport::isUnconstrainedArray;

void QbeEmitter::emitTypeTag(Type* type)
{
    if (type != nullptr && type->dependentStorage && !m_emittedTags.contains(rootType(type))) {
        type = rootType(type);
        m_emittedTags[type] = true;
        FunctionContext context;
        context.traceName = type->name + " component storage";
        context.propagateLabel = newLabel("storagepropagate");
        FunctionContext* saved = m_context;
        m_context = &context;
        emitCallInstruction("call $__ada_array_release(l %object)", CallEffects::DeferredCheck);
        finishFunction("export function " + type->tagName + ".finalize(l %object)");
        FunctionContext adjustment;
        adjustment.traceName = type->name + " component copy";
        adjustment.propagateLabel = newLabel("storagecopypropagate");
        m_context = &adjustment;
        line("storel 0, %object");
        std::string failed = newLabel("storagecopyfailed"), done = newLabel("storagecopydone");
        pushHandler(failed);
        cloneDependentStorage(Value {"%object", 'l'}, type);
        m_context->handlerLabels.pop_back();
        jump(done);
        label(failed);
        emitCallInstruction("call $__ada_array_release(l %object)", CallEffects::DeferredCheck);
        m_context->usesPropagate = true;
        jump(m_context->propagateLabel);
        label(done);
        finishFunction("export function " + type->tagName + ".adjust(l %object)");
        m_context = saved;
        return;
    }
    if (type != nullptr && type->protectedScope != nullptr && !m_emittedTags.contains(rootType(type))) {
        type = rootType(type);
        m_emittedTags[type] = true;
        FunctionContext context;
        context.traceName = type->name + " protected finalization";
        FunctionContext* saved = m_context;
        m_context = &context;
        emitCallInstruction("call $__ada_protected_destroy(l %object)", CallEffects::DeferredCheck);
        finishFunction("export function " + type->tagName + ".finalize(l %object)");
        m_context = saved;
        return;
    }
    if (type == nullptr || !type->tagged || m_emittedTags.contains(type)) {
        return;
    }
    m_emittedTags[type] = true;
    std::string name = type->tagName.substr(1);
    if (name.ends_with("__tag")) {
        name.resize(name.size() - 5);
    }
    for (std::size_t at = 0; (at = name.find("__", at)) != std::string::npos; ++at) {
        name.replace(at, 2, ".");
    }
    for (char& character : name) {
        character = static_cast<char>(std::toupper(static_cast<unsigned char>(character)));
    }
    std::string expanded = stringData(name);
    m_data << "export data " << type->tagName << " = align 8 { l "
           << (type->parentType == nullptr ? "0" : type->parentType->tagName)
           << ", l " << typeSize(type) << ", l " << typeAlignment(type)
           << ", l " << type->tagName << ".slots, l " << type->tagName << ".equal, l 0, l "
           << expanded << ", l " << type->accessLevel
           << ", l 0, l " << type->dispatchSlots.size() << ", l " << type->abstract << ", l " << needsFinalization(type) << ", l "
           << (needsFinalization(type) ? type->tagName + ".parts" : "0")
           << ", l " << hasLimitedControlledParts(type) << " }\n";
    m_data << "export data " << type->tagName << ".slots = align 8 { ";
    bool first = true;
    for (Symbol* slot : type->dispatchSlots) {
        while (slot->inheritedFrom != nullptr) {
            slot = slot->inheritedFrom;
        }
        while (slot->renamedSubprogram != nullptr) {
            slot = slot->renamedSubprogram;
        }
        if (!first) {
            m_data << ", ";
        }
        first = false;
        m_data << "l " << (slot->qbeName.empty() ? "0" : slot->qbeName) << ", l 0";
    }
    if (first) {
        m_data << "l 0";
    }
    m_data << " }\n";

    // Every type owns a uniform equality entry, including predefined equality.
    FunctionContext equality;
    Symbol function;
    function.level = type->tagOwner == nullptr ? 1 : type->tagOwner->level + 1;
    function.owner = type->tagOwner;
    function.returnType = m_sema.typeTable().booleanType();
    equality.symbol = &function;
    equality.traceName = name + " equality";
    equality.propagateLabel = newLabel("equalitypropagate");
    FunctionContext* saved = m_context;
    m_context = &equality;
    Value result = comparePrimitiveRecord(Value { "%left", 'l' }, Value { "%right", 'l' }, type);
    emitReturn(result.name);
    equality.terminated = true;
    finishFunction("export function w " + type->tagName + ".equal(l %.link, l %left, l %right)");
    m_context = saved;
    if (type->controlled) {
        emitFinalizer(type);
    }
    if (needsFinalization(type)) {
        emitControlledParts(type);
    }
}

void QbeEmitter::collectGlobals(DeclList& declarations)
{
    for (const DeclPtr& decl : declarations) {
        for (Type* access : decl->anonymousAccessTypes) {
            if (access->collectionOwner == nullptr && !access->accessParameter && !access->accessDiscriminant
                && !m_emittedCollections.contains(access)) {
                m_emittedCollections[access] = true;
                m_data << "export data " << access->collectionName << " = align 8 { l 0 }\n";
            }
        }
        for (Type* constraint : decl->arrayConstraints) {
            Symbol* bounds = constraint->boundsSymbol;
            if (bounds->isGlobal) {
                m_data << "export data " << bounds->qbeName << " = align 8 { z "
                    << adaArrayLocalSize(constraint->arrayRank) << " }\n";
            }
        }
        for (Type* constraint : decl->discriminantConstraints) {
            Symbol* bounds = constraint->discriminantSymbol;
            if (bounds->isGlobal) {
                m_data << "export data " << bounds->qbeName << " = align 8 { z "
                       << constraint->discriminantCount * 32 << " }\n";
            }
        }
        for (Type* constraint : decl->scalarConstraints) {
            Symbol* bounds = constraint->scalarBoundsSymbol;
            if (bounds != nullptr && bounds->isGlobal) {
                m_data << "export data " << bounds->qbeName << " = align 8 { z 16 }\n";
            }
        }
        switch (decl->kind) {
        case DeclKind::Type: {
            auto* taskDecl = static_cast<TypeDecl*>(decl.get());
            collectGlobals(taskDecl->taskObjects);
            collectGlobals(taskDecl->protectedDeclarations);
            Type* type = taskDecl->declaredType;
            emitTypeTag(type);
            if (!taskDecl->protectedBody && taskDecl->definition != nullptr && taskDecl->definition->kind != TypeDefKind::Private
                && type->layoutSymbol != nullptr && type->layoutSymbol->type == type && type->layoutSymbol->isGlobal) {
                m_data << "export data " << type->layoutSymbol->qbeName << " = align 8 { z "
                    << (type->fields.size() + 1) * 8 << " }\n";
            }
            if (taskDecl->definition != nullptr && taskDecl->definition->kind == TypeDefKind::Array
                && !type->boundExpressions.empty() && type->boundsSymbol->isGlobal) {
                m_data << "export data " << type->boundsSymbol->qbeName << " = align 8 { z "
                       << adaArrayLocalSize(type->arrayRank) << " }\n";
            }
            if (needsCollection(type) && type->parentType == nullptr && type->collectionOwner == nullptr
                && !m_emittedCollections.contains(type)) {
                m_emittedCollections[type] = true;
                m_data << "export data " << type->collectionName << " = align 8 { l 0 }\n";
            }
            break;
        }
        case DeclKind::Subtype: {
            Type* type = static_cast<SubtypeDecl*>(decl.get())->declaredType;
            if (type != nullptr && !type->boundExpressions.empty() && type->boundsSymbol->isGlobal) {
                m_data << "export data " << type->boundsSymbol->qbeName << " = align 8 { z "
                       << adaArrayLocalSize(type->arrayRank) << " }\n";
            }
            break;
        }
        case DeclKind::GenericDeclaration: {
            Symbol* symbol = static_cast<GenericDecl*>(decl.get())->symbol;
            if (symbol != nullptr && symbol->hasBody && symbol->level == 0) {
                m_data << "export data " << symbol->qbeName << ".elaborated = align 4 { w 0 }\n";
            }
            break;
        }
        case DeclKind::SubprogramDeclaration:
            collectGlobals(static_cast<SubprogramDecl*>(decl.get())->renamingExpansion);
            break;
        case DeclKind::Object: {
            auto* object = static_cast<ObjectDecl*>(decl.get());
            if (object->awaitsValue) {
                // The declaration in the private part is the one that counts.
                break;
            }
            for (Symbol* symbol : object->symbols) {
                if (!symbol->isGlobal) {
                    continue;
                }
                long long size = (symbol->objectReference || symbol->classWideObject) ? 8 : typeSize(symbol->type);
                if (symbol->type->kind == TypeKind::Array && hasRuntimeLayout(symbol->type)) {
                    size = adaArrayLocalSize(symbol->type->arrayRank);
                }
                else if (hasRuntimeLayout(symbol->type)) { size = 8; }
                long long alignment = (symbol->objectReference || symbol->classWideObject || hasRuntimeLayout(symbol->type))
                    ? 8 : typeAlignment(symbol->type);
                m_data << "export data " << symbol->qbeName << " = align " << (alignment < 1 ? 1 : alignment)
                       << " { z " << (size < 1 ? 1 : size) << " }\n";
            }
            break;
        }
        case DeclKind::PackageSpecification: {
            auto* package = static_cast<PackageSpecDecl*>(decl.get());
            collectGlobals(package->publicPart);
            collectGlobals(package->privatePart);
            break;
        }
        case DeclKind::PackageBody: {
            auto* package = static_cast<PackageBodyDecl*>(decl.get());
            if (package->symbol == nullptr) {
                break; // Generic template bodies are emitted only in instances.
            }
            collectGlobals(package->declarations);
            break;
        }
        case DeclKind::GenericInstantiation:
            collectGlobals(static_cast<GenericInstantiationDecl*>(decl.get())->expansion);
            break;
        default:
            break;
        }
    }
}

void QbeEmitter::emitElaborationDeclarations(DeclList& declarations)
{
    for (const DeclPtr& decl : declarations) {
        m_context->sourceLocation = decl->location;
        for (Type* access : decl->anonymousAccessTypes) { initializeCollection(access); }
        for (Type* constraint : decl->constraintOrder) {
            if (constraint->kind == TypeKind::Array) { emitTypeBounds(constraint, decl->location); }
            else if (constraint->kind == TypeKind::Record) {
                if (!constraint->discriminantObjectConstraint) { emitDiscriminantConstraint(constraint); }
            } else if (!constraint->scalarObjectConstraint) { emitScalarSubtype(constraint, decl->location); }
        }
        if (decl->kind == DeclKind::GenericDeclaration) {
            auto* generic = static_cast<GenericDecl*>(decl.get());
            if (generic->hasBody) {
                line("storew 1, " + elaborationFlag(generic->symbol).name);
            }
        } else if (decl->kind == DeclKind::SubprogramBody) {
            auto* body = static_cast<SubprogramBody*>(decl.get());
            Symbol* symbol = body->symbol != nullptr ? body->symbol : body->genericBody;
            if (symbol != nullptr) {
                line("storew 1, " + elaborationFlag(symbol).name);
            }
        } else if (decl->kind == DeclKind::SubprogramDeclaration) {
            auto* renaming = static_cast<SubprogramDecl*>(decl.get());
            emitElaborationDeclarations(renaming->renamingExpansion);
            if (renaming->callbackBinding != nullptr) {
                checkNotNull(loadFrom(addressOf(renaming->callbackBinding), renaming->callbackBinding->type));
            }
        } else if (decl->kind == DeclKind::Subtype) {
            emitTypeBounds(static_cast<SubtypeDecl*>(decl.get())->declaredType, decl->location);
        } else if (decl->kind == DeclKind::Type) {
            auto* typeDecl = static_cast<TypeDecl*>(decl.get());
            if (typeDecl->definition != nullptr && typeDecl->definition->kind == TypeDefKind::Array) {
                emitTypeBounds(typeDecl->declaredType, decl->location);
            }
            if (!typeDecl->protectedBody && typeDecl->definition != nullptr && typeDecl->definition->kind != TypeDefKind::Private) {
                emitRecordLayout(typeDecl->declaredType);
            }
            initializeTypeTag(static_cast<TypeDecl*>(decl.get())->declaredType);
            initializeCollection(static_cast<TypeDecl*>(decl.get())->declaredType);
            emitElaborationDeclarations(static_cast<TypeDecl*>(decl.get())->taskObjects);
            emitElaborationDeclarations(static_cast<TypeDecl*>(decl.get())->protectedDeclarations);
        } else if (decl->kind == DeclKind::Object) {
            auto* object = static_cast<ObjectDecl*>(decl.get());
            if (object->awaitsValue) {
                continue;
            }
            for (Symbol* symbol : object->symbols) {
                if (!symbol->isGlobal) {
                    continue;
                }
                if (symbol->type->discriminantObjectConstraint) { emitDiscriminantConstraint(symbol->type); }
                if (symbol->type->scalarObjectConstraint) { emitScalarSubtype(symbol->type, decl->location); }
                Value address { symbol->qbeName, 'l' };
                address.accessLevel = "0";
                address.shared = sharedSymbolKind(symbol) != 0;
                if (hasRuntimeLayout(symbol->type) && symbol->type->kind == TypeKind::Array && !symbol->objectReference) {
                    emitDynamicArray(object, symbol);
                    continue;
                }
                std::string savedOwner = m_context->constructionOwner;
                std::string savedArena = m_context->constructionArena;
                if (hasRuntimeLayout(symbol->type) && symbol->type->kind == TypeKind::Record && !symbol->objectReference) {
                    std::string arena = newTemp(), pointer = newTemp();
                    emitCallInstruction(arena + " =l call $__ada_construction_arena(l 0)", CallEffects::CheckException);
                    std::string size = runtimeSize(symbol->type);
                    emitCallInstruction(pointer + " =l call $__ada_array_local(l " + arena + ", l 1, l 1, l " + size + ")", CallEffects::CheckException);
                    line("storel " + pointer + ", " + symbol->qbeName);
                    address.name = pointer;
                    if (hasLimitedControlledParts(symbol->type)) {
                        m_context->constructionOwner = newTemp();
                        m_context->constructionArena = arena;
                        emitCallInstruction(m_context->constructionOwner + " =l call $__ada_construction_owner(l 0)", CallEffects::CheckException);
                    }
                }
                if (!symbol->objectReference && !symbol->classWideObject) {
                    prepareControlledObject(address, symbol->type, true);
                }
                if (symbol->classWideObject) {
                    emitClassWideObject(object, symbol);
                } else if (symbol->objectReference) {
                    emitObjectReference(object, symbol);
                } else if (object->initializer) {
                    initializeObject(address, symbol, object->initializer.get());
                } else {
                    emitDefaultInit(address, symbol->type);
                }
                m_context->constructionOwner = savedOwner;
                m_context->constructionArena = savedArena;
            }
        } else if (decl->kind == DeclKind::PackageBody) {
            auto* package = static_cast<PackageBodyDecl*>(decl.get());
            if (package->symbol == nullptr) {
                if (package->genericBody != nullptr) {
                    line("storew 1, " + elaborationFlag(package->genericBody).name);
                }
                continue;
            }
            // Declaration failures bypass this package's handlers, but may
            // reach a surrounding handled sequence containing the package.
            emitElaborationDeclarations(package->declarations);
            if (package->handlers.empty()) {
                emitStatements(package->body);
            } else {
                std::string dispatch = newLabel("packagehandler");
                std::string after = newLabel("packagehandled");
                m_context->handlerStorage[dispatch] = storageCheckpoint();
                pushHandler(dispatch);
                emitStatements(package->body);
                m_context->handlerLabels.pop_back();
                jump(after);
                emitHandlers(package->handlers, after, dispatch);
                label(after);
            }
        } else if (decl->kind == DeclKind::PackageSpecification) {
            auto* package = static_cast<PackageSpecDecl*>(decl.get());
            emitElaborationDeclarations(package->publicPart);
            emitElaborationDeclarations(package->privatePart);
        } else if (decl->kind == DeclKind::GenericInstantiation) {
            auto* instance = static_cast<GenericInstantiationDecl*>(decl.get());
            if (instance->generic != nullptr && instance->generic->hasBody) {
                emitElaborationCheck(instance->generic);
            }
            emitElaborationDeclarations(instance->expansion);
        }
    }
}

// Reserve all flags before evaluating any initializer, including bodies in
// later package declarations. Each activation owns its flags in its frame.
void QbeEmitter::prepareElaborationFlags(DeclList& declarations)
{
    for (const auto& decl : declarations) {
        if (decl->kind == DeclKind::SubprogramBody || decl->kind == DeclKind::GenericDeclaration) {
            Symbol* symbol = decl->kind == DeclKind::SubprogramBody
                ? static_cast<SubprogramBody*>(decl.get())->symbol
                : static_cast<GenericDecl*>(decl.get())->symbol;
            if (symbol != nullptr && symbol->level > 0 && symbol->elaborationOffset < 0) {
                m_context->hasFrame = true;
                symbol->elaborationOffset = reserveFrame(4, 4);
                Value flag = elaborationFlag(symbol);
                line("storew 0, " + flag.name);
            }
        } else if (decl->kind == DeclKind::Type) {
            prepareElaborationFlags(static_cast<TypeDecl*>(decl.get())->protectedDeclarations);
        } else if (decl->kind == DeclKind::PackageSpecification) {
            auto* package = static_cast<PackageSpecDecl*>(decl.get());
            prepareElaborationFlags(package->publicPart);
            prepareElaborationFlags(package->privatePart);
        } else if (decl->kind == DeclKind::PackageBody) {
            prepareElaborationFlags(static_cast<PackageBodyDecl*>(decl.get())->declarations);
        } else if (decl->kind == DeclKind::GenericInstantiation) {
            prepareElaborationFlags(static_cast<GenericInstantiationDecl*>(decl.get())->expansion);
        } else if (decl->kind == DeclKind::SubprogramDeclaration) {
            prepareElaborationFlags(static_cast<SubprogramDecl*>(decl.get())->renamingExpansion);
        }
    }
}

void QbeEmitter::emitElaborationCheck(Symbol* symbol)
{
    Value flag = elaborationFlag(symbol);
    std::string ready = newTemp();
    line(ready + " =w loadw " + flag.name);
    std::string elaborated = newLabel("elaborated");
    std::string unelaborated = newLabel("unelaborated");
    branch(Value { ready, 'w' }, elaborated, unelaborated);
    label(unelaborated);
    emitCallInstruction("call $__ada_raise(l $__ada_exc_program_error)", CallEffects::CheckException);
    jump(elaborated);
    label(elaborated);
}

Value QbeEmitter::elaborationFlag(Symbol* symbol)
{
    if (symbol->level == 0) {
        return Value { symbol->qbeName + ".elaborated", 'l' };
    }
    Value frame = staticLinkFor(symbol->level - 1);
    Value flag { newTemp(), 'l' };
    line(flag.name + " =l add " + frame.name + ", " + std::to_string(symbol->elaborationOffset));
    return flag;
}

void QbeEmitter::emitLocalDeclarations(DeclList& declarations)
{
    prepareElaborationFlags(declarations);
    std::string temporaryMark = newTemp();
    line(temporaryMark + " =l loadl " + storageArena(true));
    std::string finalizationMark;
    if (!m_context->temporaryFinalizationChain.empty()) {
        finalizationMark = newTemp();
        line(finalizationMark + " =l loadl " + m_context->temporaryFinalizationChain);
    }
    for (const DeclPtr& decl : declarations) {
        m_context->sourceLocation = decl->location;
        for (Type* access : decl->anonymousAccessTypes) { initializeCollection(access); }
        for (Type* constraint : decl->constraintOrder) {
            if (constraint->kind == TypeKind::Array) { emitTypeBounds(constraint, decl->location); }
            else if (constraint->kind == TypeKind::Record) {
                if (!constraint->discriminantObjectConstraint) { emitDiscriminantConstraint(constraint); }
            } else if (!constraint->scalarObjectConstraint) { emitScalarSubtype(constraint, decl->location); }
        }
        Symbol* subprogram = nullptr;
        if (decl->kind == DeclKind::SubprogramBody) {
            subprogram = static_cast<SubprogramBody*>(decl.get())->symbol;
        } else if (decl->kind == DeclKind::SubprogramDeclaration) {
            subprogram = static_cast<SubprogramDecl*>(decl.get())->symbol;
        }
        if (subprogram != nullptr) { captureAccessLevel(subprogram); }
        initializeAccessDescriptor(subprogram);
        switch (decl->kind) {
        case DeclKind::SubprogramDeclaration: {
            auto* renaming = static_cast<SubprogramDecl*>(decl.get());
            if (!renaming->renamingExpansion.empty()) {
                emitLocalDeclarations(renaming->renamingExpansion);
                if (renaming->callbackBinding != nullptr) {
                    checkNotNull(loadFrom(addressOf(renaming->callbackBinding), renaming->callbackBinding->type));
                }
            }
            break;
        }
        case DeclKind::Type: {
            auto* typeDecl = static_cast<TypeDecl*>(decl.get());
            emitTypeTag(typeDecl->declaredType);
            if (!typeDecl->protectedBody && typeDecl->definition != nullptr && typeDecl->definition->kind != TypeDefKind::Private) {
                emitRecordLayout(typeDecl->declaredType);
            }
            initializeTypeTag(typeDecl->declaredType);
            if (typeDecl->declaredType != nullptr) {
                for (Symbol* primitive : typeDecl->declaredType->primitives) {
                    if (primitive->inheritedFrom != nullptr) { initializeAccessDescriptor(primitive); }
                }
                emitInheritedAccessEntries(typeDecl->declaredType);
            }
            initializeCollection(typeDecl->declaredType);
            if (!typeDecl->taskObjects.empty()) {
                emitLocalDeclarations(typeDecl->taskObjects);
            }
            if (!typeDecl->protectedDeclarations.empty()) {
                emitLocalDeclarations(typeDecl->protectedDeclarations);
            }
            if (typeDecl->definition != nullptr && typeDecl->definition->kind == TypeDefKind::Array) {
                emitTypeBounds(typeDecl->declaredType, decl->location);
            }
            break;
        }
        case DeclKind::Subtype:
            emitTypeBounds(static_cast<SubtypeDecl*>(decl.get())->declaredType, decl->location);
            break;
        case DeclKind::Object: {
            auto* object = static_cast<ObjectDecl*>(decl.get());
            if (object->awaitsValue) {
                break;
            }
            for (Symbol* symbol : object->symbols) {
                if (symbol->isGlobal) {
                    continue;
                }
                captureAccessLevel(symbol);
                if (symbol->type->discriminantObjectConstraint) { emitDiscriminantConstraint(symbol->type); }
                if (symbol->type->scalarObjectConstraint) { emitScalarSubtype(symbol->type, decl->location); }
                if (symbol->classWideObject) {
                    emitClassWideObject(object, symbol);
                    continue;
                } else if (symbol->objectReference) {
                    emitObjectReference(object, symbol);
                    continue;
                }
                if ((hasRuntimeLayout(symbol->type) && symbol->type->kind == TypeKind::Array)
                    || (symbol->type->kind == TypeKind::Array && typeSize(symbol->type) > kArrayStackLimit)) {
                    emitDynamicArray(object, symbol);
                    continue;
                }
                if (hasRuntimeLayout(symbol->type)) {
                    std::string size = runtimeSize(symbol->type), pointer = newTemp();
                    emitCallInstruction(pointer + " =l call $__ada_array_local(l " + storageArena(false, true)
                        + ", l 1, l 1, l " + size + ")", CallEffects::CheckException);
                    if (symbol->isUplevel) {
                        symbol->frameOffset = reserveFrame(8);
                        std::string slot = newTemp();
                        line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->frameOffset));
                        line("storel " + pointer + ", " + slot);
                    } else { m_context->locals[symbol] = pointer; }
                    prepareControlledObject(Value {pointer, 'l'}, symbol->type);
                    if (object->initializer) { initializeObject(Value {pointer, 'l'}, symbol, object->initializer.get()); }
                    else { emitDefaultInit(Value {pointer, 'l'}, symbol->type); }
                    continue;
                }
                long long size = (symbol->objectReference || symbol->classWideObject) ? 8 : typeSize(symbol->type);
                if (size < 1) {
                    size = 1;
                }
                if (symbol->isUplevel) {
                    long long alignment = typeAlignment(symbol->type);
                    symbol->frameOffset = reserveFrame(size, alignment);
                } else {
                    std::string slot = "%v." + symbol->name + "." + std::to_string(m_tempCounter++);
                    if (size > kArrayStackLimit) {
                        emitCallInstruction(slot + " =l call $__ada_array_local(l " + storageArena(false, true)
                             + ", l 1, l 1, l " + std::to_string(size) + ")", CallEffects::CheckException);
                    } else {
                        m_context->prologue << "    " << slot << " =l " << (size <= 4 && typeAlignment(symbol->type) <= 4 ? "alloc4" : "alloc8") << " "
                                            << size << "\n";
                    }
                    m_context->locals[symbol] = slot;
                }
                prepareControlledObject(addressOf(symbol), symbol->type);
                if (object->initializer) {
                    initializeObject(addressOf(symbol), symbol, object->initializer.get());
                } else {
                    if (needsZeroInit(symbol->type) && !(isScalar(symbol->type) && sharedSymbolKind(symbol) != 0)) {
                        // A file handle has to read as closed before anything
                        // opens it, so its storage cannot be left as it was
                        // found.
                        Value slot = addressOf(symbol);
                        emitCallInstruction("call $memset(l " + slot.name + ", w 0, l " + std::to_string(size) + ")", CallEffects::DeferredCheck);
                    }
                    emitDefaultInit(addressOf(symbol), symbol->type);
                }
            }
            break;
        }
        case DeclKind::GenericDeclaration: {
            auto* generic = static_cast<GenericDecl*>(decl.get());
            if (generic->hasBody) {
                line("storew 1, " + elaborationFlag(generic->symbol).name);
            }
            break;
        }
        case DeclKind::SubprogramBody: {
            auto* body = static_cast<SubprogramBody*>(decl.get());
            if (body->genericBody != nullptr) {
                line("storew 1, " + elaborationFlag(body->genericBody).name);
            }
            if (body->symbol != nullptr) {
                line("storew 1, " + elaborationFlag(body->symbol).name);
                m_context->nested.push_back(body);
            }
            break;
        }
        case DeclKind::PackageSpecification: {
            auto* package = static_cast<PackageSpecDecl*>(decl.get());
            std::string savedGroup = m_context->taskGroup;
            if (package->symbol->containsTasks) {
                m_context->taskGroup = allocScratch(1);
                m_context->taskGroups[package->symbol] = m_context->taskGroup;
            }
            emitLocalDeclarations(package->publicPart);
            emitLocalDeclarations(package->privatePart);
            if (package->symbol->containsTasks && !package->symbol->hasBody) {
                emitCallInstruction("call $__ada_task_activate_group(l %.taskContext, l 0, l " + m_context->taskGroup + ")", CallEffects::CheckException);
            }
            m_context->taskGroup = savedGroup;
            break;
        }
        case DeclKind::PackageBody: {
            auto* package = static_cast<PackageBodyDecl*>(decl.get());
            if (package->genericBody != nullptr) {
                line("storew 1, " + elaborationFlag(package->genericBody).name);
                break;
            }
            std::string savedGroup = m_context->taskGroup;
            if (package->symbol->containsTasks) {
                m_context->taskGroup = m_context->taskGroups.at(package->symbol);
            }
            emitLocalDeclarations(package->declarations);
            if (package->symbol->containsTasks) {
                emitCallInstruction("call $__ada_task_activate_group(l %.taskContext, l 0, l " + m_context->taskGroup + ")", CallEffects::CheckException);
            }
            m_context->taskGroup = savedGroup;
            if (package->handlers.empty()) {
                emitStatements(package->body);
            } else {
                std::string dispatch = newLabel("packagehandler");
                std::string after = newLabel("packagehandled");
                m_context->handlerStorage[dispatch] = storageCheckpoint();
                pushHandler(dispatch);
                emitStatements(package->body);
                m_context->handlerLabels.pop_back();
                jump(after);
                emitHandlers(package->handlers, after, dispatch);
                label(after);
            }
            break;
        }
        case DeclKind::GenericInstantiation: {
            auto* instance = static_cast<GenericInstantiationDecl*>(decl.get());
            if (instance->generic != nullptr && instance->generic->hasBody) {
                emitElaborationCheck(instance->generic);
            }
            emitLocalDeclarations(instance->expansion);
            break;
        }
        default:
            break;
        }
    }
    if (!m_context->terminated) {
        if (!finalizationMark.empty()) {
            emitCallInstruction("call $__ada_finalize_to(l " + m_context->temporaryFinalizationChain + ", l " + finalizationMark + ")", CallEffects::DeferredCheck);
        }
        emitCallInstruction("call $__ada_array_rewind(l " + storageArena(true) + ", l " + temporaryMark + ")", CallEffects::RuntimeHook);
        if (!finalizationMark.empty()) {
            emitExceptionCheck();
        }
    }
}

void QbeEmitter::emitScalarSubtype(Type* type, const SourceLocation& location)
{
    if (type == nullptr) { return; }
    if (type->scalarLow == nullptr) {
        if (isReal(type) && type->scalarConstraintBase != nullptr) {
            Value bounds = scalarBounds(type);
            Value low {bounds.first, bounds.type}, high {bounds.last, bounds.type};
            emitRangeCheck(low, m_sema.typeTable().scalarBaseType(type), location);
            emitRangeCheck(high, m_sema.typeTable().scalarBaseType(type), location);
            if (type->lowReal <= type->highReal) {
                emitRangeCheck(low, type->scalarConstraintBase, location);
                emitRangeCheck(high, type->scalarConstraintBase, location);
            }
        }
        if (type->decimalConstraint != nullptr) {
            Value low { scalarText(type->low), 'l' }, high { scalarText(type->high), 'l' };
            emitRangeCheck(low, type->decimalConstraint, location);
            emitRangeCheck(high, type->decimalConstraint, location);
            if (type->low <= type->high) {
                emitRangeCheck(low, type->scalarConstraintBase, location);
                emitRangeCheck(high, type->scalarConstraintBase, location);
            }
        }
        return;
    }
    Symbol* symbol = type->scalarBoundsSymbol;
    if (!symbol->isGlobal && symbol->frameOffset < 0) { symbol->frameOffset = reserveFrame(16); }
    Value low = emitExpr(type->scalarLow);
    Value high = emitExpr(type->scalarHigh);
    Type* representation = m_sema.typeTable().scalarBaseType(type);
    emitRangeCheck(low, representation, location);
    emitRangeCheck(high, representation, location);
    if (type->decimalConstraint != nullptr) {
        emitRangeCheck(low, type->decimalConstraint, location);
        emitRangeCheck(high, type->decimalConstraint, location);
    }
    char width = qbeClass(type);
    std::string nonNull = newTemp();
    line(nonNull + " =w " + QbeSupport::comparisonInstruction(BinaryOp::GreaterEqual, width, type->modulus != 0)
        + " " + high.name + ", " + low.name);
    std::string check = newLabel("subtypecheck");
    std::string ready = newLabel("subtypeready");
    branch(Value { nonNull, 'w' }, check, ready);
    label(check);
    emitRangeCheck(low, type->scalarConstraintBase, location);
    emitRangeCheck(high, type->scalarConstraintBase, location);
    jump(ready);
    label(ready);
    Value slot = scalarBoundsAddress(type);
    std::string lowSlot = slot.name;
    std::string highSlot = newTemp();
    line(highSlot + " =l add " + lowSlot + ", 8");
    line(std::string("store") + width + " " + low.name + ", " + lowSlot);
    line(std::string("store") + width + " " + high.name + ", " + highSlot);
}

Value QbeEmitter::scalarBoundsAddress(Type* type)
{
    Symbol* symbol = type->scalarBoundsSymbol;
    if (symbol->isGlobal) { return Value {symbol->qbeName, 'l'}; }
    Value frame = symbol->owner == m_context->symbol ? Value {m_context->frameTemp, 'l'}
        : staticLinkFor(symbol->owner->level);
    Value slot {newTemp(), 'l'};
    line(slot.name + " =l add " + frame.name + ", " + std::to_string(symbol->frameOffset));
    return slot;
}

Value QbeEmitter::scalarBounds(Type* type)
{
    Value bounds;
    bounds.type = qbeClass(type);
    Symbol* symbol = type->scalarBoundsSymbol;
    if (symbol == nullptr) {
        bounds.first = isReal(type) ? QbeSupport::realLiteral(type->hasRealRange ? type->lowReal
            : (typeSize(type) == 4 ? -static_cast<double>(FLT_MAX) : -DBL_MAX), bounds.type) : scalarText(type->low);
        bounds.last = isReal(type) ? QbeSupport::realLiteral(type->hasRealRange ? type->highReal
            : (typeSize(type) == 4 ? static_cast<double>(FLT_MAX) : DBL_MAX), bounds.type) : scalarText(type->high);
        return bounds;
    }
    std::string lowSlot = scalarBoundsAddress(type).name;
    std::string highSlot = newTemp();
    bounds.first = newTemp();
    bounds.last = newTemp();
    line(highSlot + " =l add " + lowSlot + ", 8");
    std::string load = bounds.type == 'l' ? " =l loadl " : bounds.type == 'w' ? " =w loadsw "
        : " =" + std::string(1, bounds.type) + " load" + bounds.type + " ";
    line(bounds.first + load + lowSlot);
    line(bounds.last + load + highSlot);
    return bounds;
}

// Generic in objects are initialized once. Even a statically known shape
// mismatch is an elaboration-time Constraint_Error, before the copy occurs.
void QbeEmitter::initializeObject(const Value& address, Symbol* symbol, Expr* initializer)
{
    Type* type = symbol->type;
    std::string savedOwner = m_context->constructionOwner;
    std::string savedArena = m_context->constructionArena;
    if (symbol->isGlobal && hasLimitedControlledParts(type)) {
        m_context->constructionOwner = newTemp();
        m_context->constructionArena = newTemp();
        emitCallInstruction(m_context->constructionOwner + " =l call $__ada_construction_owner(l 0)", CallEffects::DeferredCheck);
        emitCallInstruction(m_context->constructionArena + " =l call $__ada_construction_arena(l 0)", CallEffects::DeferredCheck);
    }
    if (!needsFinalization(type) && symbol->genericObject && type->kind == TypeKind::Array
        && initializer->kind != ExprKind::Aggregate) {
        Value source = emitExpr(initializer);
        Value target = withBounds(address, type, nullptr);
        checkArrayShape(target, type, source, initializer->type);
        copyInto(target, source, type);
    } else {
        assignInto(address, type, initializer, true);
    }
    m_context->constructionOwner = savedOwner;
    m_context->constructionArena = savedArena;
}

// A reference owns only an address and, for dynamic arrays, saved bounds. No
// array allocation or copy is performed, and the actual is evaluated once.
void QbeEmitter::emitObjectReference(ObjectDecl* object, Symbol* symbol)
{
    Value actual = emitAddress(object->initializer.get());
    if (symbol->needsAccessibility && symbol->owner != nullptr && !symbol->isGlobal) {
        if (actual.accessLevel.empty()) {
            Expr* source = object->initializer.get();
            while (source->kind == ExprKind::Selected && static_cast<SelectedExpr*>(source)->symbol == nullptr) {
                source = static_cast<SelectedExpr*>(source)->prefix.get();
            }
            while (source->kind == ExprKind::Call && static_cast<CallExpr*>(source)->form == CallForm::Indexing) {
                source = static_cast<CallExpr*>(source)->callee.get();
            }
            Symbol* binding = source->kind == ExprKind::Identifier ? static_cast<IdentifierExpr*>(source)->symbol
                : source->kind == ExprKind::Selected ? static_cast<SelectedExpr*>(source)->symbol : nullptr;
            actual.accessLevel = binding != nullptr ? accessLevelFor(binding) : currentAccessLevel();
        }
        std::string levelSlot = newTemp();
        line(levelSlot + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->accessibilityOffset));
        line("storew " + actual.accessLevel + ", " + levelSlot);
    }
    bool dynamic = isUnconstrainedArray(symbol->type);
    long long size = dynamic ? adaArrayLocalSize(symbol->type->arrayRank) : 8;
    std::string slot;
    if (symbol->isGlobal) {
        slot = symbol->qbeName;
    } else if (symbol->isUplevel) {
        symbol->frameOffset = reserveFrame(size);
        slot = newTemp();
        line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->frameOffset));
    } else {
        slot = allocScratch(size);
        m_context->locals[symbol] = slot;
        if (dynamic) {
            m_context->bounds[symbol] = actual;
        }
    }
    line("storel " + actual.name + ", " + slot);
    if (dynamic && (symbol->isGlobal || symbol->isUplevel)) {
        storeArrayView(slot, actual, symbol->type, false);
    }
}

Value QbeEmitter::typeTag(Type* type)
{
    type = rootType(type);
    if (type->classRoot != nullptr) {
        type = type->classRoot;
    }
    if (type->tagOwner == nullptr) {
        return Value { type->tagName, 'l' };
    }
    Value frame = type->tagOwner == m_context->symbol
        ? Value { m_context->frameTemp, 'l' } : staticLinkFor(type->tagOwner->level);
    std::string slot = newTemp();
    std::string tag = newTemp();
    line(slot + " =l add " + frame.name + ", " + std::to_string(type->tagOffset));
    line(tag + " =l loadl " + slot);
    return Value { tag, 'l' };
}

void QbeEmitter::initializeTypeTag(Type* type)
{
    if (type == nullptr || !type->tagged) {
        return;
    }
    if (type->tagOwner == nullptr) {
        emitCallInstruction("call $__ada_tag_register(l " + type->tagName + ")", CallEffects::CheckException);
        return;
    }
    if (type->tagOffset >= 0) {
        return; // The visible declaration and private completion share a tag.
    }
    type->tagOffset = reserveFrame(8);
    std::string slot = newTemp();
    std::string tag = newTemp();
    std::string parent = type->parentType == nullptr ? "0" : typeTag(type->parentType).name;
    line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(type->tagOffset));
    emitCallInstruction(tag + " =l call $__ada_tag_create(l " + type->tagName + ", l " + parent
         + ", l " + m_context->frameTemp + ")", CallEffects::CheckException);
    line("storel " + tag + ", " + slot);
    std::string tableSlot = newTemp();
    std::string table = newTemp();
    line(tableSlot + " =l add " + tag + ", 24");
    line(table + " =l loadl " + tableSlot);
    for (std::size_t i = 0; i < type->dispatchSlots.size(); ++i) {
        Symbol* implementation = type->dispatchSlots[i];
        while (implementation->inheritedFrom != nullptr || implementation->renamedSubprogram != nullptr) {
            implementation = implementation->inheritedFrom != nullptr
                ? implementation->inheritedFrom : implementation->renamedSubprogram;
        }
        if (implementation->level > 0) {
            std::string linkSlot = newTemp();
            Value link = staticLinkFor(implementation->level - 1);
            line(linkSlot + " =l add " + table + ", " + std::to_string(i * 16 + 8));
            line("storel " + link.name + ", " + linkSlot);
        }
    }
}

void QbeEmitter::emitClassWideObject(ObjectDecl* object, Symbol* symbol)
{
    bool limited = hasLimitedControlledParts(symbol->type) || hasLimitedControlledParts(object->initializer->type);
    std::string owner = symbol->isGlobal ? "0" : m_context->finalizationChain;
    std::string arena = symbol->isGlobal ? "0" : storageArena(false, true);
    if (limited) {
        if (symbol->isGlobal) {
            owner = newTemp();
            arena = newTemp();
            emitCallInstruction(owner + " =l call $__ada_construction_owner(l 0)", CallEffects::DeferredCheck);
            emitCallInstruction(arena + " =l call $__ada_construction_arena(l 0)", CallEffects::DeferredCheck);
        }
        m_context->resultTargetOwner = owner;
        m_context->resultTargetArena = arena;
    }
    Value source = emitExpr(object->initializer.get());
    emitCallInstruction("call $__ada_tag_check_accessibility(l " + source.name + ", w " + std::to_string(symbol->accessibilityLevel) + ")", CallEffects::CheckException);
    std::string pointer = limited ? source.name : newTemp();
    if (!limited) {
        emitCallInstruction(pointer + " =l call $__ada_tagged_owned_copy(l " + owner + ", l " + arena + ", l " + source.name + ")", CallEffects::CheckException);
    }
    std::string slot;
    if (symbol->isGlobal) {
        slot = symbol->qbeName;
    } else if (symbol->isUplevel) {
        symbol->frameOffset = reserveFrame(8);
        slot = newTemp();
        line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->frameOffset));
    } else {
        slot = allocScratch(8);
        m_context->locals[symbol] = slot;
    }
    line("storel " + pointer + ", " + slot);
}

void QbeEmitter::initializeAccessDescriptor(Symbol* symbol)
{
    if (symbol == nullptr || !symbol->addressTaken || symbol->renamedAccess != nullptr
        || symbol->builtin != BuiltinKind::None || symbol->level == 0 || symbol->descriptorOffset >= 0) { return; }
    symbol->descriptorOffset = reserveFrame(16);
    std::string descriptor = newTemp(), linkSlot = newTemp();
    line(descriptor + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->descriptorOffset));
    line("storel " + symbol->qbeName + ", " + descriptor);
    line(linkSlot + " =l add " + descriptor + ", 8");
    line("storel " + m_context->frameTemp + ", " + linkSlot);
}
