#include "QbeEmitter.h"

#include "Binder.h"

namespace
{

// Encodes an Ada string as a QBE data body, keeping printable runs quoted.
std::string encodeString(const std::string& text)
{
    std::string result;
    bool inQuotes = false;
    auto closeQuotes = [&]() {
        if (inQuotes) {
            result += "\"";
            inQuotes = false;
        }
    };

    for (unsigned char c : text) {
        bool printable = c >= 0x20 && c < 0x7f && c != '"' && c != '\\';
        if (printable) {
            if (!inQuotes) {
                if (!result.empty()) {
                    result += " ";
                }
                result += "\"";
                inQuotes = true;
            }
            result.push_back(static_cast<char>(c));
        } else {
            closeQuotes();
            if (!result.empty()) {
                result += " ";
            }
            result += std::to_string(static_cast<int>(c));
        }
    }
    closeQuotes();
    if (!result.empty()) {
        result += " ";
    }
    result += "0";
    return result;
}

}

QbeEmitter::QbeEmitter(Sema& sema, Diagnostics& diagnostics)
    : m_sema(sema)
    , m_diagnostics(diagnostics)
{
}

// Each unit is emitted on its own, so nothing survives from the one before it.
void QbeEmitter::beginUnit(const std::string& key)
{
    m_data.str(std::string());
    m_data.clear();
    m_functions.clear();
    m_stringPool.clear();
    m_enumTables.clear();
    m_pendingSubprograms.clear();
    m_unitTag = unitTag(key);
    m_tempCounter = 0;
    m_labelCounter = 0;
    m_dataCounter = 0;
}

void QbeEmitter::writeUnit(std::ostream& out)
{
    out << m_data.str() << "\n";
    for (const std::string& function : m_functions) {
        out << function << "\n";
    }
}

void QbeEmitter::emitUnit(const LibraryUnit& unit, std::ostream& out)
{
    beginUnit(unit.key);

    for (CompilationUnit* part : unit.parts) {
        collectGlobals(part->units);
        emitExceptionObjects(m_sema.exceptionsIn(part));
    }

    emitElaboration(unit);

    for (CompilationUnit* part : unit.parts) {
        emitSubprogramsIn(part->units);
    }

    writeUnit(out);
}

std::string QbeEmitter::newTemp()
{
    return "%.t" + std::to_string(m_tempCounter++);
}

std::string QbeEmitter::newLabel(const char* prefix)
{
    return "@" + std::string(prefix) + "." + std::to_string(m_labelCounter++);
}

void QbeEmitter::line(const std::string& text)
{
    if (m_context->terminated) {
        m_context->body << newLabel("unreachable") << "\n";
        m_context->terminated = false;
    }
    // All potentially raising calls use the active AST location. Centralizing
    // this also covers imported routines and compiler-generated check helpers.
    bool call = text.starts_with("call ") || text.find(" call ") != std::string::npos;
    bool cleanup = text.find("$__ada_array_rewind(") != std::string::npos
        || text.find("$__ada_array_release(") != std::string::npos;
    if (call && !cleanup && m_context->sourceLocation.line > 0) {
        m_context->body << "    call $__ada_trace_location(l "
                        << sourceLocationData(m_context->sourceLocation) << ")\n";
    }
    m_context->body << "    " << text << "\n";
}

std::string QbeEmitter::sourceLocationData(const SourceLocation& location)
{
    if (location.line <= 0) {
        return stringData("<unknown>");
    }
    std::string file = m_diagnostics.fileName(location.file);
    std::size_t slash = file.find_last_of("/\\");
    if (slash != std::string::npos) {
        file.erase(0, slash + 1);
    }
    return stringData(file + ":" + std::to_string(location.line) + ":" + std::to_string(location.column));
}

void QbeEmitter::label(const std::string& name)
{
    if (!m_context->terminated) {
        m_context->body << "    jmp " << name << "\n";
    }
    m_context->body << name << "\n";
    m_context->terminated = false;
}

void QbeEmitter::jump(const std::string& target)
{
    if (!m_context->terminated) {
        m_context->body << "    jmp " << target << "\n";
        m_context->terminated = true;
    }
}

void QbeEmitter::branch(const Value& condition, const std::string& ifTrue, const std::string& ifFalse)
{
    line("jnz " + condition.name + ", " + ifTrue + ", " + ifFalse);
    m_context->terminated = true;
}

std::string QbeEmitter::stringData(const std::string& text)
{
    auto it = m_stringPool.find(text);
    if (it != m_stringPool.end()) {
        return it->second;
    }
    std::string name = "$." + m_unitTag + ".str." + std::to_string(m_dataCounter++);
    m_data << "data " << name << " = { b " << encodeString(text) << " }\n";
    m_stringPool.emplace(text, name);
    return name;
}

// The literal names of an enumeration type, laid down once as an array of C
// strings so that the run time can write one out or read one back.
std::string QbeEmitter::enumTableFor(const Type* type)
{
    const Type* base = type;
    while (base->base != nullptr) {
        base = base->base;
    }

    auto it = m_enumTables.find(base);
    if (it != m_enumTables.end()) {
        return it->second;
    }

    std::string table = "$." + m_unitTag + ".enum." + std::to_string(m_dataCounter++);
    std::vector<std::string> names;
    for (std::size_t i = 0; i < base->literals.size(); ++i) {
        std::string name = table + "." + std::to_string(i);
        m_data << "data " << name << " = { b " << encodeString(base->literals[i]) << ", b 0 }\n";
        names.push_back(name);
    }

    m_data << "data " << table << " = {";
    for (std::size_t i = 0; i < names.size(); ++i) {
        m_data << (i > 0 ? ", l " : " l ") << names[i];
    }
    m_data << " }\n";

    m_enumTables.emplace(base, table);
    return table;
}
