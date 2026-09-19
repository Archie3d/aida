#include "Sema.h"

#include "Lexer.h"

namespace
{

Type* typeIn(Scope* scope, const std::string& name)
{
    for (Symbol* candidate : scope->lookupLocal(name)) {
        if (candidate->kind == SymbolKind::TypeName) {
            return candidate->type;
        }
    }
    return nullptr;
}

}

Sema::Sema(Diagnostics& diagnostics)
    : m_diagnostics(diagnostics)
{
    setupStandardScope();
}

void Sema::setupStandardScope()
{
    m_standardScope = m_symbolTable.createScope(nullptr);
    m_globalScope = m_symbolTable.createScope(m_standardScope);

    auto addType = [&](Type* type) {
        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, toLower(type->name), type->name);
        symbol->type = type;
        m_standardScope->add(symbol);
    };

    addType(m_types.integerType());
    addType(m_types.longIntegerType());
    addType(m_types.naturalType());
    addType(m_types.positiveType());
    addType(m_types.booleanType());
    addType(m_types.characterType());
    addType(m_types.floatType());
    addType(m_types.longFloatType());
    addType(m_types.stringType());

    const char* booleanLiterals[] = { "False", "True" };
    for (int i = 0; i < 2; ++i) {
        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::EnumerationLiteral, toLower(booleanLiterals[i]),
                                                    booleanLiterals[i]);
        symbol->type = m_types.booleanType();
        symbol->enumerationValue = i;
        m_standardScope->add(symbol);
    }

    // System describes the machine rather than the language.  Address is what
    // 'Address yields, so the compiler has to know the type whether or not a
    // program ever names the package.
    Symbol* system = m_symbolTable.createSymbol(SymbolKind::Package, "system", "System");
    system->scope = m_symbolTable.createScope(nullptr);
    m_standardScope->add(system);

    m_addressType = m_types.create(TypeKind::Access, "Address");
    addTypeTo(system->scope, m_addressType);

    // Handler bindings exist even without a with clause for Ada.Exceptions.
    // Its source declaration later completes this same canonical type.
    m_exceptionOccurrenceType = m_types.create(TypeKind::Record, "Exception_Occurrence");
    // Runtime ABI: identity pointer, message pointer, signed 32-bit length,
    // 200 inline saved-message bytes, padded to pointer alignment. Keep
    // Ada.Exceptions and adart.h in sync.
    m_exceptionOccurrenceType->byteSize = 224;
    m_exceptionOccurrenceType->isLimited = true;
    m_exceptionOccurrenceType->privateTo =
        m_symbolTable.createSymbol(SymbolKind::Package, "ada.exceptions", "Ada.Exceptions");
    m_exceptionIdType = m_types.create(TypeKind::Access, "Exception_Id");
    m_exceptionIdType->privateTo = m_exceptionOccurrenceType->privateTo;

    Symbol* storageUnit = m_symbolTable.createSymbol(SymbolKind::Number, "storage_unit", "Storage_Unit");
    storageUnit->type = m_types.integerType();
    storageUnit->hasStaticValue = true;
    storageUnit->staticValue = 8;
    system->scope->add(storageUnit);

    addException(m_standardScope, "Constraint_Error");
    addException(m_standardScope, "Program_Error");
    addException(m_standardScope, "Storage_Error");
    addException(m_standardScope, "Numeric_Error");
    addException(m_standardScope, "Tasking_Error");
}

Symbol* Sema::addException(Scope* scope, const std::string& displayName)
{
    Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Exception, toLower(displayName), displayName);
    symbol->exceptionObject = "$__ada_exc_" + symbol->name;
    scope->add(symbol);
    return symbol;
}

const std::vector<Symbol*>& Sema::exceptionsIn(const CompilationUnit* unit) const
{
    static const std::vector<Symbol*> none;
    auto found = m_unitExceptions.find(unit);
    return found == m_unitExceptions.end() ? none : found->second;
}

Symbol* Sema::addTypeTo(Scope* scope, Type* type)
{
    Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, toLower(type->name), type->name);
    symbol->type = type;
    scope->add(symbol);
    return symbol;
}

// One unit of the predefined environment holds something the compiler itself
// has to lay hands on: the types the Text_IO generics are written in terms of.
// They are picked up as the Ada source declaring them is analysed.
void Sema::adoptLibraryUnit(PackageSpecDecl* decl, Symbol* package)
{
    if (m_namePrefix.size() != 2 || m_namePrefix[0] != "ada" || package->scope == nullptr) {
        return;
    }

    if (m_namePrefix[1] == "text_io") {
        m_textFileType = typeIn(package->scope, "file_type");
        m_fieldType = typeIn(package->scope, "field");
        m_numberBaseType = typeIn(package->scope, "number_base");
        m_typeSetType = typeIn(package->scope, "type_set");
    }
}

void Sema::analyze(CompilationUnit& unit)
{
    m_currentUnit = &unit;

    // The loader has already read whatever it could find, so a name still
    // standing for nothing is one no unit answers to.
    for (const WithClause& clause : unit.withClauses) {
        for (std::size_t i = 0; i < clause.names.size(); ++i) {
            Symbol* symbol = lookupName(clause.namesLower[i], m_globalScope);
            if (symbol == nullptr || (symbol->kind != SymbolKind::Package && symbol->kind != SymbolKind::Generic)) {
                m_diagnostics.error(clause.location, "cannot find the unit '" + clause.names[i] + "'");
            }
        }
    }

    for (UseDecl& use : unit.useClauses) {
        analyzeUseClause(use, m_globalScope);
    }
    analyzeDeclarativePart(unit.units, m_globalScope);

    if (m_main == nullptr) {
        for (const DeclPtr& decl : unit.units) {
            if (decl->kind != DeclKind::SubprogramBody) {
                continue;
            }
            auto* body = static_cast<SubprogramBody*>(decl.get());
            if (body->symbol != nullptr && !body->spec.isFunction && body->spec.parameters.empty()) {
                m_main = body->symbol;
                break;
            }
        }
    }
}

std::string Sema::mangle(const std::string& name) const
{
    std::string result;
    for (const std::string& part : m_namePrefix) {
        result += part + "__";
    }
    result += name == "**" ? "operator_power" : name;
    return result;
}

std::string Sema::anonymousTypeName()
{
    return "anon." + std::to_string(m_anonymousCounter++);
}
