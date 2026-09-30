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
                if (!object->m_isRenaming && symbol->type->m_controlled) {
                    return true;
                }
            }
        } else if (decl->kind == DeclKind::PackageSpecification) {
            auto* package = static_cast<PackageSpecDecl*>(decl.get());
            if (hasControlledObjects(package->publicPart) || hasControlledObjects(package->privatePart)) {
                return true;
            }
        } else if (decl->kind == DeclKind::PackageBody) {
            auto* package = static_cast<PackageBodyDecl*>(decl.get());
            if (hasControlledObjects(package->declarations) || hasControlledObjects(package->body)
                || hasControlledObjects(package->handlers)) {
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
        context.traceName = unit.key + (part->isSpec ? " (spec elaboration)" : " (body elaboration)");
        context.propagateLabel = newLabel("propagate");
        FunctionContext* saved = m_context;
        m_context = &context;
        emitElaborationDeclarations(part->units);
        finishFunction("export function $" + elaborationName(unit.key + (part->isSpec ? ".spec" : ".body")) + "()");
        m_context = saved;
    }
}

void QbeEmitter::emitSubprogramsIn(DeclList& declarations)
{
    for (const DeclPtr& decl : declarations) {
        switch (decl->kind) {
        case DeclKind::SubprogramDeclaration:
            emitSubprogramsIn(static_cast<SubprogramDecl*>(decl.get())->m_renamingExpansion);
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
        m_data << "export data " << symbol->qbeName << ".access = align 8 { l " << symbol->qbeName << ", l 0 }\n";
    }

    FunctionContext context;
    context.symbol = symbol;
    context.traceName = symbol->displayName;
    context.sourceLocation = symbol->location;
    context.hasFrame = symbol->needsFrame;
    context.frameTemp = "%.frame";
    context.propagateLabel = newLabel("propagate");
    FunctionContext* saved = m_context;
    m_context = &context;
    if (hasControlledObjects(body->declarations) || hasControlledObjects(body->body)
        || hasControlledObjects(body->handlers)) {
        context.m_finalizationChain = allocScratch(8);
        context.prologue << "    storel 0, " << context.m_finalizationChain << "\n";
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
        first = false;
    }
    if (symbol->level > 0) {
        signature += (first ? "" : ", ") + std::string("l %.link");
        first = false;
    }
    for (Symbol* parameter : symbol->parameters) {
        if (!first) {
            signature += ", ";
        }
        first = false;
        char type = parameter->byReference ? 'l' : qbeClass(parameter->type);
        signature += std::string(1, type) + " %p." + parameter->name;
        if (isUnconstrainedArray(parameter->type)) {
            signature += ", w %p." + parameter->name + ".first, w %p." + parameter->name + ".last";
            for (int dimension = 1; dimension < parameter->type->arrayRank; ++dimension) {
                std::string suffix = std::to_string(dimension);
                signature += ", w %p." + parameter->name + ".first" + suffix
                    + ", w %p." + parameter->name + ".last" + suffix;
            }
        }
    }
    signature += ")";

    for (Symbol* parameter : symbol->parameters) {
        std::string incoming = "%p." + parameter->name;
        bool unconstrained = isUnconstrainedArray(parameter->type);
        if (parameter->isUplevel) {
            parameter->frameOffset = context.frameSize;
            long long slotSize = parameter->byReference ? 8 : typeSize(parameter->type);
            if (unconstrained) {
                slotSize = 8 + 8 * parameter->type->arrayRank; // Pointer followed by each pair of bounds.
            }
            context.frameSize += slotSize;
            std::string address = newTemp();
            context.prologue << "    " << address << " =l add " << context.frameTemp << ", "
                             << parameter->frameOffset << "\n";
            if (parameter->byReference) {
                context.prologue << "    storel " << incoming << ", " << address << "\n";
            } else {
                context.prologue << "    " << qbeStoreInstruction(parameter->type) << " " << incoming << ", "
                                 << address << "\n";
            }
            if (unconstrained) {
                std::string firstAddress = newTemp();
                std::string lastAddress = newTemp();
                context.prologue << "    " << firstAddress << " =l add " << context.frameTemp << ", "
                                 << parameter->frameOffset + 8 << "\n";
                context.prologue << "    storew " << incoming << ".first, " << firstAddress << "\n";
                context.prologue << "    " << lastAddress << " =l add " << context.frameTemp << ", "
                                 << parameter->frameOffset + 12 << "\n";
                context.prologue << "    storew " << incoming << ".last, " << lastAddress << "\n";
                for (int dimension = 1; dimension < parameter->type->arrayRank; ++dimension) {
                    std::string firstSlot = newTemp();
                    std::string lastSlot = newTemp();
                    context.prologue << "    " << firstSlot << " =l add " << address << ", " << 8 + dimension * 8 << "\n";
                    context.prologue << "    " << lastSlot << " =l add " << firstSlot << ", 4\n";
                    context.prologue << "    storew " << incoming << ".first" << dimension << ", " << firstSlot << "\n";
                    context.prologue << "    storew " << incoming << ".last" << dimension << ", " << lastSlot << "\n";
                }
            }
            continue;
        }
        if (parameter->byReference) {
            context.locals[parameter] = incoming;
            if (unconstrained) {
                Value bounds { incoming, 'l', incoming + ".first", incoming + ".last" };
                for (int dimension = 1; dimension < parameter->type->arrayRank; ++dimension) {
                    bounds.innerBounds.push_back({ incoming + ".first" + std::to_string(dimension),
                                                   incoming + ".last" + std::to_string(dimension) });
                }
                context.bounds[parameter] = bounds;
            }
            continue;
        }
        std::string slot = "%v." + parameter->name + "." + std::to_string(m_tempCounter++);
        long long size = typeSize(parameter->type);
        context.prologue << "    " << slot << " =l " << (size <= 4 ? "alloc4" : "alloc8") << " "
                         << (size < 1 ? 1 : size) << "\n";
        context.prologue << "    " << qbeStoreInstruction(parameter->type) << " " << incoming << ", " << slot
                         << "\n";
        context.locals[parameter] = slot;
    }

    emitLocalDeclarations(body->declarations);

    if (body->handlers.empty()) {
        emitStatements(body->body);
    } else {
        std::string dispatch = newLabel("handler");
        std::string after = newLabel("handled");
        context.handlerStorage[dispatch] = storageCheckpoint();
        context.handlerLabels.push_back(dispatch);
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
            line("call $__ada_raise(l $__ada_exc_program_error)");
            char type = qbeClass(context.symbol->returnType);
            if (isComposite(context.symbol->returnType)) {
                line("ret");
            } else if (type == 's' || type == 'd') {
                std::string zero = newTemp();
                line(zero + " =" + std::string(1, type) + " copy " + (type == 's' ? "s_0" : "d_0"));
                line("ret " + zero);
            } else {
                line("ret 0");
            }
        } else {
            line("ret");
        }
        context.terminated = true;
    }

    if (context.usesPropagate) {
        context.body << context.propagateLabel << "\n";
        if (context.symbol != nullptr && context.symbol->returnType != nullptr) {
            char type = qbeClass(context.symbol->returnType);
            if (isComposite(context.symbol->returnType)) {
                context.body << "    ret\n";
            } else if (type == 's' || type == 'd') {
                context.body << "    ret " << (type == 's' ? "s_0" : "d_0") << "\n";
            } else {
                context.body << "    ret 0\n";
            }
        } else {
            context.body << "    ret\n";
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
    // Intern in a fixed order: string concatenation does not sequence its
    // operands, and both helpers assign IDs in the shared string pool.
    std::string traceName = stringData(context.traceName);
    std::string traceLocation = sourceLocationData(context.symbol != nullptr ? context.symbol->location : SourceLocation {});
    text += "    call $__ada_trace_enter(l %.trace, l " + traceName + ", l " + traceLocation + ")\n";
    // Allocations may occur in a branch emitted after an early return. Once
    // the whole body is known, release the activation's list at every exit.
    std::istringstream bodyLines(context.body.str());
    std::string bodyLine;
    while (std::getline(bodyLines, bodyLine)) {
        if (!context.m_finalizationChain.empty() && bodyLine.compare(0, 7, "    ret") == 0) {
            text += "    call $__ada_finalize_to(l " + context.m_finalizationChain + ", l 0)\n";
        }
        if (!context.arrayArena.empty() && bodyLine.compare(0, 7, "    ret") == 0) {
            text += "    call $__ada_array_release(l " + context.arrayArena + ")\n";
        }
        if (!context.temporaryArena.empty() && bodyLine.compare(0, 7, "    ret") == 0) {
            text += "    call $__ada_array_release(l " + context.temporaryArena + ")\n";
        }
        if (bodyLine.compare(0, 7, "    ret") == 0) {
            text += "    call $__ada_trace_leave(l %.trace)\n";
        }
        text += bodyLine + "\n";
    }
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
