#include "QbeEmitter.h"
#include "QbeSupport.h"

#include "Binder.h"

#include <cctype>

using QbeSupport::isUnconstrainedArray;

namespace
{

bool hasControlledObjects(DeclList& declarations);
bool hasControlledObjects(StmtList& statements);

bool hasControlledObjects(std::vector<ExceptionHandler>& handlers)
{
    for (auto& handler : handlers) {
        if (hasControlledObjects(handler.body)) {
            return true;
        }
    }
    return false;
}

// Discover lifetimes before emission so early returns and handlers emitted
// before a later declaration still receive the activation cleanup chain.
bool hasControlledObjects(DeclList& declarations)
{
    for (auto& decl : declarations) {
        if (decl->kind == DeclKind::Object) {
            auto* object = static_cast<ObjectDecl*>(decl.get());
            for (Symbol* symbol : object->symbols) {
                if (!object->m_isRenaming && needsFinalization(symbol->type)) {
                    return true;
                }
            }
        } else if (decl->kind == DeclKind::Type) {
            if (needsCollection(static_cast<TypeDecl*>(decl.get())->declaredType)
                || hasControlledObjects(static_cast<TypeDecl*>(decl.get())->m_taskObjects)) {
                return true;
            }
        } else if (decl->kind == DeclKind::PackageSpecification) {
            auto* package = static_cast<PackageSpecDecl*>(decl.get());
            if (hasControlledObjects(package->publicPart) || hasControlledObjects(package->privatePart)) {
                return true;
            }
        } else if (decl->kind == DeclKind::PackageBody) {
            auto* package = static_cast<PackageBodyDecl*>(decl.get());
            if (package->symbol != nullptr && (hasControlledObjects(package->declarations) || hasControlledObjects(package->body)
                || hasControlledObjects(package->handlers))) {
                return true;
            }
        } else if (decl->kind == DeclKind::GenericInstantiation) {
            if (hasControlledObjects(static_cast<GenericInstantiationDecl*>(decl.get())->expansion)) {
                return true;
            }
        }
    }
    return false;
}

bool hasControlledObjects(StmtList& statements)
{
    for (auto& statement : statements) {
        switch (statement->kind) {
        case StmtKind::Return: {
            auto* result = static_cast<ReturnStmt*>(statement.get());
            if (result->m_object != nullptr || hasControlledObjects(result->m_body)) {
                return true;
            }
            break;
        }
        case StmtKind::Block: {
            auto* block = static_cast<BlockStmt*>(statement.get());
            if (hasControlledObjects(block->declarations) || hasControlledObjects(block->body)
                || hasControlledObjects(block->handlers)) {
                return true;
            }
            break;
        }
        case StmtKind::If: {
            auto* conditional = static_cast<IfStmt*>(statement.get());
            for (auto& branch : conditional->branches) {
                if (hasControlledObjects(branch.body)) {
                    return true;
                }
            }
            if (hasControlledObjects(conditional->elseBody)) {
                return true;
            }
            break;
        }
        case StmtKind::Loop:
            if (hasControlledObjects(static_cast<LoopStmt*>(statement.get())->body)) {
                return true;
            }
            break;
        case StmtKind::Case:
            for (auto& alternative : static_cast<CaseStmt*>(statement.get())->alternatives) {
                if (hasControlledObjects(alternative.body)) {
                    return true;
                }
            }
            break;
        default:
            break;
        }
    }
    return false;
}

}

void QbeEmitter::emitElaboration(const LibraryUnit& unit)
{
    for (CompilationUnit* part : unit.parts) {
        FunctionContext context;
        context.symbol = m_sema.elaborationOwner(part);
        context.hasFrame = context.symbol->needsFrame;
        context.frameTemp = "%.frame";
        context.traceName = unit.key + (part->isSpec ? " (spec elaboration)" : " (body elaboration)");
        context.propagateLabel = newLabel("propagate");
        FunctionContext* saved = m_context;
        m_context = &context;
        if (m_sema.usesFinalization(part) || context.symbol->usesFinalization
            || hasControlledObjects(part->units)) {
            initializeFinalization();
        }
        emitElaborationDeclarations(part->units);
        finishFunction("export function $" + elaborationName(part->partKey) + "()");
        m_context = saved;
        for (SubprogramBody* nested : context.nested) {
            emitSubprogram(nested);
        }
    }
}

void QbeEmitter::emitSubprogramsIn(DeclList& declarations)
{
    for (const DeclPtr& decl : declarations) {
        switch (decl->kind) {
        case DeclKind::SubprogramDeclaration:
            emitSubprogramsIn(static_cast<SubprogramDecl*>(decl.get())->m_renamingExpansion);
            break;
        case DeclKind::Type:
            emitSubprogramsIn(static_cast<TypeDecl*>(decl.get())->m_protectedDeclarations);
            break;
        case DeclKind::SubprogramBody:
            emitSubprogram(static_cast<SubprogramBody*>(decl.get()));
            break;
        case DeclKind::PackageSpecification: {
            auto* package = static_cast<PackageSpecDecl*>(decl.get());
            emitSubprogramsIn(package->publicPart);
            emitSubprogramsIn(package->privatePart);
            break;
        }
        case DeclKind::PackageBody: {
            auto* package = static_cast<PackageBodyDecl*>(decl.get());
            if (package->symbol == nullptr) {
                break;
            }
            emitSubprogramsIn(package->declarations);
            break;
        }
        case DeclKind::GenericInstantiation:
            emitSubprogramsIn(static_cast<GenericInstantiationDecl*>(decl.get())->expansion);
            break;
        default:
            break;
        }
    }
}

void QbeEmitter::emitSubprogram(SubprogramBody* body)
{
    Symbol* symbol = body->symbol;
    if (symbol == nullptr) {
        return;
    }

    // A separately compiled caller can take 'Access without recompiling this
    // body. Export one stable descriptor alongside every library subprogram.
    if (symbol->level == 0) {
        m_data << "export data " << symbol->qbeName << ".elaborated = align 4 { w 0 }\n";
        m_data << "export data " << symbol->qbeName << ".access = align 8 { l " << symbol->qbeName << ", l 0 }\n";
    }

    FunctionContext context;
    context.symbol = symbol;
    bool usesFinalization = symbol->usesFinalization || hasControlledObjects(body->declarations)
        || hasControlledObjects(body->body) || hasControlledObjects(body->handlers);
    // Elide a statically empty boundary: no declarations, collections or
    // owned temporaries. Nested blocks carry their own masters, and allocators
    // depend on their access type's collection. Scalar leaf calls thus keep
    // their existing entry/exit cost. Task-valued results and aggregates remain
    // diagnosed until build-in-place lifetimes support them.
    context.hasMaster = body->m_taskBody || !body->declarations.empty() || usesFinalization;
    context.traceName = symbol->displayName;
    context.sourceLocation = symbol->location;
    context.hasFrame = symbol->needsFrame;
    context.frameTemp = "%.frame";
    context.propagateLabel = newLabel("propagate");
    FunctionContext* saved = m_context;
    m_context = &context;
    if (usesFinalization) {
        initializeFinalization();
    }

    // A subprogram nested in another one is reached through its static link
    // and never by name, so only library level ones leave the object file.
    std::string signature = symbol->level == 0 ? "export function " : "function ";
    if (symbol->returnType != nullptr && !isComposite(symbol->returnType)) {
        signature += std::string(1, qbeClass(symbol->returnType)) + " ";
    }
    signature += symbol->qbeName + "(";
    bool first = true;
    if (isComposite(symbol->returnType)) {
        signature += "l %.result";
        if (needsFinalization(symbol->returnType) || symbol->returnType->tagged) {
            signature += ", l %.resultOwner, l %.resultArena";
            if (symbol->returnType->tagged || hasLimitedControlledParts(symbol->returnType)) {
                signature += ", w %.resultProvided";
            }
        }
        first = false;
    }
    if (symbol->level > 0 || symbol->taskBody || symbol->entryBarrierFunction) {
        signature += (first ? "" : ", ") + std::string("l %.link");
        first = false;
    }
    if (symbol->protectedReceiver != nullptr) {
        signature += (first ? "" : ", ") + std::string("l %.protectedObject");
        first = false;
        Symbol* receiver = symbol->protectedReceiver;
        if (receiver->isUplevel) {
            context.hasFrame = true;
            receiver->frameOffset = reserveFrame(8);
            std::string slot = newTemp();
            context.prologue << "    " << slot << " =l add " << context.frameTemp << ", " << receiver->frameOffset << "\n";
            context.prologue << "    storel %.protectedObject, " << slot << "\n";
        } else {
            context.locals[receiver] = "%.protectedObject";
        }
    }
    for (Symbol* parameter : symbol->parameters) {
        if (!first) {
            signature += ", ";
        }
        first = false;
        char type = parameter->byReference ? 'l' : qbeClass(parameter->type);
        signature += std::string(1, type) + " %p." + parameter->name;
        if (isUnconstrainedArray(parameter->type)) {
            for (int dimension = 0; dimension < parameter->type->arrayRank; ++dimension) {
                std::string suffix = dimension == 0 ? "" : std::to_string(dimension);
                for (const char* field : { "first", "last", "length", "stride" }) {
                    signature += ", l %p." + parameter->name + "." + field + suffix;
                }
            }
        }
    }
    signature += ")";

    for (Symbol* parameter : symbol->parameters) {
        std::string incoming = "%p." + parameter->name;
        bool unconstrained = isUnconstrainedArray(parameter->type);
        Value incomingView { incoming, 'l' };
        if (unconstrained) {
            incomingView.first = incoming + ".first";
            incomingView.last = incoming + ".last";
            for (int dimension = 0; dimension < parameter->type->arrayRank; ++dimension) {
                std::string suffix = dimension == 0 ? "" : std::to_string(dimension);
                if (dimension != 0) {
                    incomingView.innerBounds.push_back({ incoming + ".first" + suffix, incoming + ".last" + suffix });
                }
                incomingView.lengths.push_back(incoming + ".length" + suffix);
                incomingView.strides.push_back(incoming + ".stride" + suffix);
            }
        }
        if (parameter->isUplevel) {
            long long alignment = parameter->byReference ? 8 : typeAlignment(parameter->type);
            long long slotSize = parameter->byReference ? 8 : typeSize(parameter->type);
            if (unconstrained) {
                slotSize = adaArrayLocalSize(parameter->type->arrayRank); // Pointer followed by dimension records.
            }
            parameter->frameOffset = reserveFrame(slotSize, alignment);
            std::string address = newTemp();
            context.prologue << "    " << address << " =l add " << context.frameTemp << ", "
                             << parameter->frameOffset << "\n";
            if (parameter->byReference) {
                context.prologue << "    storel " << incoming << ", " << address << "\n";
            } else if (sharedTypeKind(parameter->type) != 0 || !parameter->type->enumCodes.empty()
                || (isDiscrete(parameter->type) && ((qbeClass(parameter->type) == 'l' && typeSize(parameter->type) < 8)
                    || (qbeClass(parameter->type) == 'w' && typeSize(parameter->type) == 8)))) {
                storeInto(Value { address, 'l' }, Value { incoming, qbeClass(parameter->type) }, parameter->type);
            } else {
                context.prologue << "    " << qbeStoreInstruction(parameter->type) << " " << incoming << ", "
                                 << address << "\n";
            }
            if (unconstrained) {
                storeArrayView(address, incomingView, parameter->type, false);
            }
            continue;
        }
        if (parameter->byReference) {
            context.locals[parameter] = incoming;
            if (unconstrained) {
                context.bounds[parameter] = incomingView;
            }
            continue;
        }
        std::string slot = "%v." + parameter->name + "." + std::to_string(m_tempCounter++);
        long long size = typeSize(parameter->type);
        context.prologue << "    " << slot << " =l " << (size <= 4 ? "alloc4" : "alloc8") << " "
                         << (size < 1 ? 1 : size) << "\n";
        if (sharedTypeKind(parameter->type) != 0 || !parameter->type->enumCodes.empty()
                || (isDiscrete(parameter->type) && ((qbeClass(parameter->type) == 'l' && typeSize(parameter->type) < 8)
                    || (qbeClass(parameter->type) == 'w' && typeSize(parameter->type) == 8)))) {
            storeInto(Value { slot, 'l' }, Value { incoming, qbeClass(parameter->type) }, parameter->type);
        } else {
            context.prologue << "    " << qbeStoreInstruction(parameter->type) << " " << incoming << ", " << slot
                             << "\n";
        }
        context.locals[parameter] = slot;
    }

    // Capture this task before declarations; nested tasks can name this instance
    // through the ordinary lexical frame even while executing on another thread.
    if (symbol->taskIdentity != nullptr) {
        Symbol* identity = symbol->taskIdentity;
        std::string slot = newTemp();
        if (identity->isUplevel) {
            context.hasFrame = true;
            identity->frameOffset = reserveFrame(8);
            context.prologue << "    " << slot << " =l add " << context.frameTemp << ", " << identity->frameOffset << "\n";
        } else {
            context.prologue << "    " << slot << " =l alloc8 8\n";
            context.locals[identity] = slot;
        }
        std::string current = newTemp();
        emitCallInstruction(current + " =l call $__ada_current_task()", CallEffects::DeferredCheck);
        line("storel " + current + ", " + slot);
    }

    // Check at the shared entry so callbacks, dispatch and generated
    // finalization calls obey the same rule as direct source-level calls.
    // This precedes declarations and the subprogram's own handlers.
    emitElaborationCheck(symbol);

    emitLocalDeclarations(body->declarations);
    if (context.hasMaster) {
        emitCallInstruction("call $__ada_master_activate(l %.taskContext, l %.master)", CallEffects::CheckedRuntimeHook);
    }

    if (body->m_taskBody) {
        emitCallInstruction("call $__ada_task_activated(l %.taskContext)", CallEffects::DeferredCheck);
    }
    if (body->handlers.empty()) {
        emitStatements(body->body);
    } else {
        std::string dispatch = newLabel("handler");
        std::string after = newLabel("handled");
        context.handlerStorage[dispatch] = storageCheckpoint();
        pushHandler(dispatch);
        emitStatements(body->body);
        context.handlerLabels.pop_back();
        jump(after);
        emitHandlers(body->handlers, after, dispatch);
        label(after);
    }

    finishFunction(signature);
    std::vector<SubprogramBody*> nested = context.nested;
    m_context = saved;

    for (SubprogramBody* inner : nested) {
        emitSubprogram(inner);
    }
}

void QbeEmitter::finishFunction(const std::string& signature)
{
    FunctionContext& context = *m_context;

    if (!context.terminated) {
        if (context.symbol != nullptr && context.symbol->returnType != nullptr) {
            // A missing result is a failure for every result representation.
            context.sourceLocation = context.symbol->location;
            emitCallInstruction("call $__ada_raise(l $__ada_exc_program_error)", CallEffects::DeferredCheck);
            char type = qbeClass(context.symbol->returnType);
            if (isComposite(context.symbol->returnType)) {
                emitReturn();
            } else if (type == 's' || type == 'd') {
                std::string zero = newTemp();
                line(zero + " =" + std::string(1, type) + " copy " + (type == 's' ? "s_0" : "d_0"));
                emitReturn(zero);
            } else {
                emitReturn("0");
            }
        } else {
            emitReturn();
        }
        context.terminated = true;
    }

    if (context.usesPropagate) {
        label(context.propagateLabel);
        if (context.symbol != nullptr && context.symbol->returnType != nullptr
            && !isComposite(context.symbol->returnType)) {
            char type = qbeClass(context.symbol->returnType);
            emitReturn(type == 's' ? "s_0" : type == 'd' ? "d_0" : "0");
        } else {
            emitReturn();
        }
    }

    std::string text = signature + " {\n@start\n";
    if (context.hasFrame) {
        text += "    " + context.frameTemp + " =l alloc8 " + std::to_string(context.frameSize) + "\n";
        if (context.symbol != nullptr && context.symbol->level > 0) {
            text += "    storel %.link, " + context.frameTemp + "\n";
        } else {
            text += "    storel 0, " + context.frameTemp + "\n";
        }
    }
    text += context.prologue.str();
    text += "    %.trace =l alloc8 24\n";
    text += "    %.traceLocation =l add %.trace, 16\n";
    // Intern in a fixed order: string concatenation does not sequence its
    // operands, and both helpers assign IDs in the shared string pool.
    std::string traceName = stringData(context.traceName);
    std::string traceLocation = sourceLocationData(context.symbol != nullptr ? context.symbol->location : SourceLocation {});
    // Trace entry fetches the bound task context once without an extra call.
    text += "    %.taskContext =l call $__ada_trace_enter(l %.trace, l " + traceName + ", l " + traceLocation + ")\n";
    if (context.hasMaster) {
        text += "    %.master =l alloc8 8\n";
        text += "    call $__ada_master_enter(l %.taskContext, l %.master, w 0)\n";
    }
    // Allocations may occur in a branch emitted after an early return. Once
    // the whole body is known, release the activation's list at every exit.
    std::string body = context.body.str();
    std::string exitActions = functionExitActions(context);
    std::size_t previous = 0;
    for (std::size_t offset : context.returnOffsets) {
        text.append(body, previous, offset - previous);
        text += exitActions;
        previous = offset;
    }
    text.append(body, previous, std::string::npos);
    text += "}\n";
    // Checkpoints are emitted before all branches have been visited. Remove
    // bookkeeping for an arena if no branch ever allocates into it. Arena
    // operands are unique numbered temporaries; match the complete number.
    for (const auto& arena : { std::make_pair(context.arrayArena, context.arrayArenaUsed),
                              std::make_pair(context.temporaryArena, context.temporaryArenaUsed) }) {
        if (arena.first.empty() || arena.second) {
            continue;
        }
        std::istringstream lines(text);
        std::string filtered;
        std::string current;
        while (std::getline(lines, current)) {
            std::size_t position = current.find(arena.first);
            while (position != std::string::npos
                   && position + arena.first.size() < current.size()
                   && std::isdigit(static_cast<unsigned char>(current[position + arena.first.size()]))) {
                position = current.find(arena.first, position + arena.first.size());
            }
            bool referencesArena = position != std::string::npos;
            if (!referencesArena) {
                filtered += current + "\n";
            }
        }
        text = filtered;
    }
    m_functions.push_back(text);
}

// One ordered activation-exit plan for ordinary returns and propagation.
// Lexical master exits have already been emitted by emitReturn/jump.
std::string QbeEmitter::functionExitActions(const FunctionContext& context)
{
    std::string actions;
    if (context.symbol != nullptr && context.symbol->taskBody) {
        actions += "    call $__ada_task_activation_failed(l %.taskContext)\n";
        actions += "    call $__ada_task_complete(l %.taskContext)\n";
    }
    if (context.hasMaster) {
        actions += "    call $__ada_master_await(l %.taskContext, l %.master)\n";
    }
    if (!context.finalizationChain.empty()) {
        actions += "    call $__ada_finalize_to(l " + context.temporaryFinalizationChain + ", l 0)\n";
        actions += "    call $__ada_finalize_to(l " + context.finalizationChain + ", l 0)\n";
    }
    if (!context.arrayArena.empty()) {
        actions += "    call $__ada_array_release(l " + context.arrayArena + ")\n";
    }
    if (!context.temporaryArena.empty()) {
        actions += "    call $__ada_array_release(l " + context.temporaryArena + ")\n";
    }
    if (context.hasMaster) {
        actions += "    call $__ada_master_leave(l %.taskContext, l %.master)\n";
    }
    actions += "    call $__ada_trace_leave_context(l %.taskContext, l %.trace)\n";
    return actions;
}
