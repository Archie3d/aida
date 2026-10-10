#include "QbeEmitter.h"
#include "QbeSupport.h"

#include <cctype>

using QbeSupport::isUnconstrainedArray;

void QbeEmitter::emitTypeTag(Type* type)
{
    if (type == nullptr || !type->m_tagged || m_emittedTags.contains(type)) {
        return;
    }
    m_emittedTags[type] = true;
    std::string name = type->m_tagName.substr(1);
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
    m_data << "export data " << type->m_tagName << " = align 8 { l "
           << (type->m_parentType == nullptr ? "0" : type->m_parentType->m_tagName)
           << ", l " << typeSize(type) << ", l " << typeAlignment(type)
           << ", l " << type->m_tagName << ".slots, l " << type->m_tagName << ".equal, l 0, l "
           << expanded << ", l " << type->m_accessLevel
           << ", l 0, l " << type->m_dispatchSlots.size() << ", l " << type->m_abstract << ", l " << needsFinalization(type) << ", l "
           << (needsFinalization(type) ? type->m_tagName + ".parts" : "0")
           << ", l " << hasLimitedControlledParts(type) << " }\n";
    m_data << "export data " << type->m_tagName << ".slots = align 8 { ";
    bool first = true;
    for (Symbol* slot : type->m_dispatchSlots) {
        while (slot->m_inheritedFrom != nullptr) {
            slot = slot->m_inheritedFrom;
        }
        while (slot->m_renamedSubprogram != nullptr) {
            slot = slot->m_renamedSubprogram;
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
    function.level = type->m_tagOwner == nullptr ? 1 : type->m_tagOwner->level + 1;
    function.owner = type->m_tagOwner;
    function.returnType = m_sema.typeTable().booleanType();
    equality.symbol = &function;
    equality.traceName = name + " equality";
    equality.propagateLabel = newLabel("equalitypropagate");
    FunctionContext* saved = m_context;
    m_context = &equality;
    Value result = comparePrimitiveRecord(Value { "%left", 'l' }, Value { "%right", 'l' }, type);
    line("ret " + result.name);
    equality.terminated = true;
    finishFunction("export function w " + type->m_tagName + ".equal(l %.link, l %left, l %right)");
    m_context = saved;
    if (type->m_controlled) {
        emitFinalizer(type);
    }
    if (needsFinalization(type)) {
        emitControlledParts(type);
    }
}

void QbeEmitter::collectGlobals(DeclList& declarations)
{
    for (const DeclPtr& decl : declarations) {
        switch (decl->kind) {
        case DeclKind::Type: {
            Type* type = static_cast<TypeDecl*>(decl.get())->declaredType;
            emitTypeTag(type);
            if (needsCollection(type) && type->m_collectionOwner == nullptr
                && !m_emittedCollections.contains(type)) {
                m_emittedCollections[type] = true;
                m_data << "export data " << type->m_collectionName << " = align 8 { l 0 }\n";
            }
            break;
        }
        case DeclKind::SubprogramDeclaration:
            collectGlobals(static_cast<SubprogramDecl*>(decl.get())->m_renamingExpansion);
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
                long long size = (symbol->m_objectReference || symbol->m_classWideObject) ? 8 : typeSize(symbol->type);
                long long alignment = (symbol->m_objectReference || symbol->m_classWideObject) ? 8 : typeAlignment(symbol->type);
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
        if (decl->kind == DeclKind::SubprogramDeclaration) {
            auto* renaming = static_cast<SubprogramDecl*>(decl.get());
            emitElaborationDeclarations(renaming->m_renamingExpansion);
            if (renaming->m_callbackBinding != nullptr) {
                checkNotNull(loadFrom(addressOf(renaming->m_callbackBinding), renaming->m_callbackBinding->type));
            }
        } else if (decl->kind == DeclKind::Type) {
            initializeTypeTag(static_cast<TypeDecl*>(decl.get())->declaredType);
            initializeCollection(static_cast<TypeDecl*>(decl.get())->declaredType);
        } else if (decl->kind == DeclKind::Object) {
            auto* object = static_cast<ObjectDecl*>(decl.get());
            if (object->awaitsValue) {
                continue;
            }
            for (Symbol* symbol : object->symbols) {
                if (!symbol->isGlobal) {
                    continue;
                }
                Value address { symbol->qbeName, 'l' };
                if (!symbol->m_objectReference && !symbol->m_classWideObject) {
                    prepareControlledObject(address, symbol->type, true);
                }
                if (symbol->m_classWideObject) {
                    emitClassWideObject(object, symbol);
                } else if (symbol->m_objectReference) {
                    emitObjectReference(object, symbol);
                } else if (object->initializer) {
                    initializeObject(address, symbol, object->initializer.get());
                } else {
                    emitDefaultInit(address, symbol->type);
                }
            }
        } else if (decl->kind == DeclKind::PackageBody) {
            auto* package = static_cast<PackageBodyDecl*>(decl.get());
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
            emitElaborationDeclarations(static_cast<GenericInstantiationDecl*>(decl.get())->expansion);
        }
    }
}

void QbeEmitter::emitLocalDeclarations(DeclList& declarations)
{
    std::string temporaryMark = newTemp();
    line(temporaryMark + " =l loadl " + storageArena(true));
    std::string finalizationMark;
    if (!m_context->m_temporaryFinalizationChain.empty()) {
        finalizationMark = newTemp();
        line(finalizationMark + " =l loadl " + m_context->m_temporaryFinalizationChain);
    }
    for (const DeclPtr& decl : declarations) {
        m_context->sourceLocation = decl->location;
        Symbol* subprogram = nullptr;
        if (decl->kind == DeclKind::SubprogramBody) {
            subprogram = static_cast<SubprogramBody*>(decl.get())->symbol;
        } else if (decl->kind == DeclKind::SubprogramDeclaration) {
            subprogram = static_cast<SubprogramDecl*>(decl.get())->symbol;
        }
        if (subprogram != nullptr && subprogram->m_addressTaken && subprogram->m_renamedAccess == nullptr
            && subprogram->level > 0
            && subprogram->m_descriptorOffset < 0) {
            m_context->frameSize = (m_context->frameSize + 7) / 8 * 8;
            subprogram->m_descriptorOffset = m_context->frameSize;
            m_context->frameSize += 16;
            std::string descriptor = newTemp();
            std::string linkSlot = newTemp();
            line(descriptor + " =l add " + m_context->frameTemp + ", " + std::to_string(subprogram->m_descriptorOffset));
            line("storel " + subprogram->qbeName + ", " + descriptor);
            line(linkSlot + " =l add " + descriptor + ", 8");
            line("storel " + m_context->frameTemp + ", " + linkSlot);
        }
        switch (decl->kind) {
        case DeclKind::SubprogramDeclaration: {
            auto* renaming = static_cast<SubprogramDecl*>(decl.get());
            if (!renaming->m_renamingExpansion.empty()) {
                emitLocalDeclarations(renaming->m_renamingExpansion);
                if (renaming->m_callbackBinding != nullptr) {
                    checkNotNull(loadFrom(addressOf(renaming->m_callbackBinding), renaming->m_callbackBinding->type));
                }
            }
            break;
        }
        case DeclKind::Type: {
            auto* typeDecl = static_cast<TypeDecl*>(decl.get());
            emitTypeTag(typeDecl->declaredType);
            initializeTypeTag(typeDecl->declaredType);
            initializeCollection(typeDecl->declaredType);
            if (typeDecl->definition != nullptr && typeDecl->definition->kind == TypeDefKind::Array) {
                emitTypeBounds(typeDecl->declaredType, decl->location);
            }
            break;
        }
        case DeclKind::Subtype:
            emitScalarSubtype(static_cast<SubtypeDecl*>(decl.get())->declaredType, decl->location);
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
                if (symbol->m_classWideObject) {
                    emitClassWideObject(object, symbol);
                    continue;
                } else if (symbol->m_objectReference) {
                    emitObjectReference(object, symbol);
                    continue;
                }
                if (isUnconstrainedArray(symbol->type)) {
                    emitDynamicArray(object, symbol);
                    continue;
                }
                long long size = (symbol->m_objectReference || symbol->m_classWideObject) ? 8 : typeSize(symbol->type);
                if (size < 1) {
                    size = 1;
                }
                if (symbol->isUplevel) {
                    long long alignment = typeAlignment(symbol->type);
                    m_context->frameSize = (m_context->frameSize + alignment - 1) / alignment * alignment;
                    symbol->frameOffset = m_context->frameSize;
                    m_context->frameSize += size;
                } else {
                    std::string slot = "%v." + symbol->name + "." + std::to_string(m_tempCounter++);
                    m_context->prologue << "    " << slot << " =l " << (size <= 4 ? "alloc4" : "alloc8") << " "
                                        << size << "\n";
                    m_context->locals[symbol] = slot;
                }
                prepareControlledObject(addressOf(symbol), symbol->type);
                if (object->initializer) {
                    initializeObject(addressOf(symbol), symbol, object->initializer.get());
                } else {
                    if (needsZeroInit(symbol->type)) {
                        // A file handle has to read as closed before anything
                        // opens it, so its storage cannot be left as it was
                        // found.
                        Value slot = addressOf(symbol);
                        line("call $memset(l " + slot.name + ", w 0, l " + std::to_string(size) + ")");
                    }
                    emitDefaultInit(addressOf(symbol), symbol->type);
                }
            }
            break;
        }
        case DeclKind::SubprogramBody:
            m_context->nested.push_back(static_cast<SubprogramBody*>(decl.get()));
            break;
        case DeclKind::PackageSpecification: {
            auto* package = static_cast<PackageSpecDecl*>(decl.get());
            emitLocalDeclarations(package->publicPart);
            emitLocalDeclarations(package->privatePart);
            break;
        }
        case DeclKind::PackageBody: {
            auto* package = static_cast<PackageBodyDecl*>(decl.get());
            emitLocalDeclarations(package->declarations);
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
        case DeclKind::GenericInstantiation:
            emitLocalDeclarations(static_cast<GenericInstantiationDecl*>(decl.get())->expansion);
            break;
        default:
            break;
        }
    }
    if (!m_context->terminated) {
        if (!finalizationMark.empty()) {
            line("call $__ada_finalize_to(l " + m_context->m_temporaryFinalizationChain + ", l " + finalizationMark + ")");
        }
        line("call $__ada_array_rewind(l " + storageArena(true) + ", l " + temporaryMark + ")");
        if (!finalizationMark.empty()) {
            emitExceptionCheck();
        }
    }
}

void QbeEmitter::emitScalarSubtype(Type* type, const SourceLocation& location)
{
    if (type == nullptr || type->m_scalarLow == nullptr) {
        return;
    }
    Symbol* symbol = type->m_scalarBoundsSymbol;
    m_context->frameSize = (m_context->frameSize + 7) & ~7LL;
    symbol->frameOffset = m_context->frameSize;
    m_context->frameSize += 16;
    Value low = emitExpr(type->m_scalarLow);
    Value high = emitExpr(type->m_scalarHigh);
    Type* representation = m_sema.typeTable().scalarBaseType(type);
    emitRangeCheck(low, representation, location);
    emitRangeCheck(high, representation, location);
    char width = qbeClass(type);
    std::string nonNull = newTemp();
    line(nonNull + " =w csge" + width + " " + high.name + ", " + low.name);
    std::string check = newLabel("subtypecheck");
    std::string ready = newLabel("subtypeready");
    branch(Value { nonNull, 'w' }, check, ready);
    label(check);
    emitRangeCheck(low, type->m_scalarConstraintBase, location);
    emitRangeCheck(high, type->m_scalarConstraintBase, location);
    jump(ready);
    label(ready);
    std::string lowSlot = newTemp();
    std::string highSlot = newTemp();
    line(lowSlot + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->frameOffset));
    line(highSlot + " =l add " + lowSlot + ", 8");
    line(std::string("store") + width + " " + low.name + ", " + lowSlot);
    line(std::string("store") + width + " " + high.name + ", " + highSlot);
}

Value QbeEmitter::scalarBounds(Type* type)
{
    Value bounds;
    bounds.type = qbeClass(type);
    Symbol* symbol = type->m_scalarBoundsSymbol;
    if (symbol == nullptr) {
        bounds.first = std::to_string(type->low);
        bounds.last = std::to_string(type->high);
        return bounds;
    }
    Value frame = symbol->owner == m_context->symbol ? Value { m_context->frameTemp, 'l' }
                                                     : staticLinkFor(symbol->owner->level);
    std::string lowSlot = newTemp();
    std::string highSlot = newTemp();
    bounds.first = newTemp();
    bounds.last = newTemp();
    line(lowSlot + " =l add " + frame.name + ", " + std::to_string(symbol->frameOffset));
    line(highSlot + " =l add " + lowSlot + ", 8");
    std::string load = bounds.type == 'l' ? " =l loadl " : " =w loadsw ";
    line(bounds.first + load + lowSlot);
    line(bounds.last + load + highSlot);
    return bounds;
}

// Generic in objects are initialized once. Even a statically known shape
// mismatch is an elaboration-time Constraint_Error, before the copy occurs.
void QbeEmitter::initializeObject(const Value& address, Symbol* symbol, Expr* initializer)
{
    Type* type = symbol->type;
    std::string savedOwner = m_context->m_constructionOwner;
    std::string savedArena = m_context->m_constructionArena;
    if (symbol->isGlobal && hasLimitedControlledParts(type)) {
        m_context->m_constructionOwner = newTemp();
        m_context->m_constructionArena = newTemp();
        line(m_context->m_constructionOwner + " =l call $__ada_construction_owner(l 0)");
        line(m_context->m_constructionArena + " =l call $__ada_construction_arena(l 0)");
    }
    if (!needsFinalization(type) && symbol->m_genericObject && type->kind == TypeKind::Array
        && initializer->kind != ExprKind::Aggregate) {
        Value source = emitExpr(initializer);
        Value target = withBounds(address, type, nullptr);
        checkArrayShape(target, type, source, initializer->type);
        copyInto(target, source, type);
    } else {
        assignInto(address, type, initializer, true);
    }
    m_context->m_constructionOwner = savedOwner;
    m_context->m_constructionArena = savedArena;
}

// A reference owns only an address and, for dynamic arrays, saved bounds. No
// array allocation or copy is performed, and the actual is evaluated once.
void QbeEmitter::emitObjectReference(ObjectDecl* object, Symbol* symbol)
{
    Value actual = emitAddress(object->initializer.get());
    bool dynamic = isUnconstrainedArray(symbol->type);
    long long size = dynamic ? 8 + 8 * symbol->type->arrayRank : 8;
    std::string slot;
    if (symbol->isGlobal) {
        slot = symbol->qbeName;
    } else if (symbol->isUplevel) {
        m_context->frameSize = (m_context->frameSize + 7) & ~7LL;
        symbol->frameOffset = m_context->frameSize;
        m_context->frameSize += size;
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
    if (dynamic && symbol->isUplevel) {
        for (int dimension = 0; dimension < symbol->type->arrayRank; ++dimension) {
            const std::string& first = dimension == 0 ? actual.first : actual.innerBounds[dimension - 1].first;
            const std::string& last = dimension == 0 ? actual.last : actual.innerBounds[dimension - 1].second;
            std::string firstSlot = newTemp();
            std::string lastSlot = newTemp();
            line(firstSlot + " =l add " + slot + ", " + std::to_string(8 + dimension * 8));
            line(lastSlot + " =l add " + firstSlot + ", 4");
            line("storew " + first + ", " + firstSlot);
            line("storew " + last + ", " + lastSlot);
        }
    }
}

Value QbeEmitter::typeTag(Type* type)
{
    type = rootType(type);
    if (type->m_classRoot != nullptr) {
        type = type->m_classRoot;
    }
    if (type->m_tagOwner == nullptr) {
        return Value { type->m_tagName, 'l' };
    }
    Value frame = type->m_tagOwner == m_context->symbol
        ? Value { m_context->frameTemp, 'l' } : staticLinkFor(type->m_tagOwner->level);
    std::string slot = newTemp();
    std::string tag = newTemp();
    line(slot + " =l add " + frame.name + ", " + std::to_string(type->m_tagOffset));
    line(tag + " =l loadl " + slot);
    return Value { tag, 'l' };
}

void QbeEmitter::initializeTypeTag(Type* type)
{
    if (type == nullptr || !type->m_tagged) {
        return;
    }
    if (type->m_tagOwner == nullptr) {
        line("call $__ada_tag_register(l " + type->m_tagName + ")");
        emitExceptionCheck();
        return;
    }
    if (type->m_tagOffset >= 0) {
        return; // The visible declaration and private completion share a tag.
    }
    m_context->frameSize = (m_context->frameSize + 7) & ~7LL;
    type->m_tagOffset = m_context->frameSize;
    m_context->frameSize += 8;
    std::string slot = newTemp();
    std::string tag = newTemp();
    std::string parent = type->m_parentType == nullptr ? "0" : typeTag(type->m_parentType).name;
    line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(type->m_tagOffset));
    line(tag + " =l call $__ada_tag_create(l " + type->m_tagName + ", l " + parent
         + ", l " + m_context->frameTemp + ")");
    emitExceptionCheck();
    line("storel " + tag + ", " + slot);
    std::string tableSlot = newTemp();
    std::string table = newTemp();
    line(tableSlot + " =l add " + tag + ", 24");
    line(table + " =l loadl " + tableSlot);
    for (std::size_t i = 0; i < type->m_dispatchSlots.size(); ++i) {
        Symbol* implementation = type->m_dispatchSlots[i];
        while (implementation->m_inheritedFrom != nullptr || implementation->m_renamedSubprogram != nullptr) {
            implementation = implementation->m_inheritedFrom != nullptr
                ? implementation->m_inheritedFrom : implementation->m_renamedSubprogram;
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
    std::string owner = symbol->isGlobal ? "0" : m_context->m_finalizationChain;
    std::string arena = symbol->isGlobal ? "0" : storageArena(false, true);
    if (limited) {
        if (symbol->isGlobal) {
            owner = newTemp();
            arena = newTemp();
            line(owner + " =l call $__ada_construction_owner(l 0)");
            line(arena + " =l call $__ada_construction_arena(l 0)");
        }
        m_context->m_resultTargetOwner = owner;
        m_context->m_resultTargetArena = arena;
    }
    Value source = emitExpr(object->initializer.get());
    line("call $__ada_tag_check_accessibility(l " + source.name + ", w " + std::to_string(symbol->m_accessibilityLevel) + ")");
    emitExceptionCheck();
    std::string pointer = limited ? source.name : newTemp();
    if (!limited) {
        line(pointer + " =l call $__ada_tagged_owned_copy(l " + owner + ", l " + arena + ", l " + source.name + ")");
        emitExceptionCheck();
    }
    std::string slot;
    if (symbol->isGlobal) {
        slot = symbol->qbeName;
    } else if (symbol->isUplevel) {
        m_context->frameSize = (m_context->frameSize + 7) & ~7LL;
        symbol->frameOffset = m_context->frameSize;
        m_context->frameSize += 8;
        slot = newTemp();
        line(slot + " =l add " + m_context->frameTemp + ", " + std::to_string(symbol->frameOffset));
    } else {
        slot = allocScratch(8);
        m_context->locals[symbol] = slot;
    }
    line("storel " + pointer + ", " + slot);
}
