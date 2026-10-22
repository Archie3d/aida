#include "Sema.h"

#include "Lexer.h"
#include "SemaSupport.h"

#include <algorithm>

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
    m_globalScope->m_isLibraryRegistry = true;
    Symbol* standard = m_symbolTable.createSymbol(SymbolKind::Package, "standard", "Standard");
    standard->scope = m_standardScope;
    m_standardScope->add(standard);

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
    addType(m_types.durationType());

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
    system->m_libraryName = "system";
    m_standardScope->add(system);

    m_addressType = m_types.create(TypeKind::Access, "Address");
    addTypeTo(system->scope, m_addressType);
    m_tagType = m_types.create(TypeKind::Access, "Tag");
    m_tagType->privateTo = m_symbolTable.createSymbol(SymbolKind::Package, "ada.tags", "Ada.Tags");

    // Handler bindings exist even without a with clause for Ada.Exceptions.
    // Its source declaration later completes this same canonical type.
    m_exceptionOccurrenceType = m_types.create(TypeKind::Record, "Exception_Occurrence");
    // Runtime ABI: identity pointer, message pointer, signed 32-bit length,
    // 200 inline saved-message bytes, padded to pointer alignment. Keep
    // Ada.Exceptions and adart.h in sync.
    // The saved-message area is followed by the origin, trace count, and
    // 32 pairs of static routine/location pointers (752 bytes in total).
    m_exceptionOccurrenceType->byteSize = 752;
    m_exceptionOccurrenceType->isLimited = true;
    m_exceptionOccurrenceType->privateTo =
        m_symbolTable.createSymbol(SymbolKind::Package, "ada.exceptions", "Ada.Exceptions");
    m_exceptionIdType = m_types.create(TypeKind::Access, "Exception_Id");
    m_exceptionIdType->privateTo = m_exceptionOccurrenceType->privateTo;
    // Task attributes are available without a with clause for the package.
    m_taskIdType = m_types.create(TypeKind::Access, "Task_Id");
    m_taskIdType->privateTo =
        m_symbolTable.createSymbol(SymbolKind::Package, "ada.task_identification", "Ada.Task_Identification");

    Symbol* storageUnit = m_symbolTable.createSymbol(SymbolKind::Number, "storage_unit", "Storage_Unit");
    storageUnit->type = m_types.universalInteger();
    storageUnit->m_exactReal = ExactReal::make(8);
    storageUnit->hasStaticValue = true;
    storageUnit->staticValue = 8;
    system->scope->add(storageUnit);
    for (const auto& limit : { std::pair<const char*, __int128> { "max_binary_modulus", (__int128)1 << 64 },
                              { "max_nonbinary_modulus", ((__int128)1 << 64) - 1 } }) {
        Symbol* number = m_symbolTable.createSymbol(SymbolKind::Number, limit.first, limit.first);
        number->type = m_types.universalInteger();
        number->isConstant = true;
        number->hasStaticValue = true;
        number->m_exactReal = ExactReal::make(limit.second);
        system->scope->add(number);
    }

    Symbol* fineDelta = m_symbolTable.createSymbol(SymbolKind::Number, "fine_delta", "Fine_Delta");
    fineDelta->type = m_types.universalReal();
    fineDelta->hasStaticValue = true;
    fineDelta->m_exactReal = ExactReal::make(1, (__int128)1 << 62);
    fineDelta->staticReal = fineDelta->m_exactReal.real();
    system->scope->add(fineDelta);

    addException(m_standardScope, "Constraint_Error");
    addException(m_standardScope, "Program_Error");
    addException(m_standardScope, "Storage_Error");
    addException(m_standardScope, "Numeric_Error");
    addException(m_standardScope, "Tasking_Error");
}

Symbol* Sema::addException(Scope* scope, const std::string& displayName)
{
    Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Exception, toLower(displayName), displayName);
    symbol->exceptionObject = "$__ada_exc_" + (symbol->name == "numeric_error" ? "constraint_error" : symbol->name);
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

    if (m_namePrefix[1] == "real_time") {
        m_realTimeType = typeIn(package->scope, "time");
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
    // Executable package parts need local ownership without changing the
    // library lifetime of declarations in the package itself.
    m_elaborationOwners[&unit] = m_symbolTable.createSymbol(SymbolKind::Subprogram,
        unit.partKey + ".elaboration", unit.partKey + " elaboration");

    std::string name = unit.partKey.substr(0, unit.partKey.rfind('.'));
    std::replace(name.begin(), name.end(), '-', '.');
    Symbol* existing = lookupName(name, m_globalScope);
    Scope* parent = m_standardScope;
    if (!unit.isSpec && existing != nullptr) {
        if (existing->kind == SymbolKind::Package) {
            parent = existing->scope;
        } else if (m_libraryContexts.contains(name)) {
            parent = m_libraryContexts.at(name);
        }
    } else if (auto dot = name.rfind('.'); dot != std::string::npos) {
        Symbol* enclosing = lookupName(name.substr(0, dot), m_globalScope);
        if (enclosing != nullptr && enclosing->scope != nullptr) {
            parent = enclosing->scope;
        }
    }
    Scope* context = m_symbolTable.createScope(parent);
    m_unitScope = context;
    auto mention = [&](const std::string& fullName) {
        std::size_t end = 0;
        do {
            end = fullName.find('.', end);
            context->addLibraryVisibility(fullName.substr(0, end));
            if (end != std::string::npos) {
                ++end;
            }
        } while (end != std::string::npos);
        std::string root = fullName.substr(0, fullName.find('.'));
        auto symbols = m_globalScope->lookupLocal(root);
        for (Symbol* symbol : symbols) {
            auto local = context->lookupLocal(root);
            if (std::find(local.begin(), local.end(), symbol) == local.end()) {
                context->add(symbol);
            }
        }
    };
    mention(name);
    if (existing != nullptr && existing->kind == SymbolKind::Subprogram && name.find('.') != std::string::npos) {
        context->add(existing);
    }
    for (const WithClause& clause : unit.withClauses) {
        for (std::size_t i = 0; i < clause.names.size(); ++i) {
            Symbol* symbol = lookupName(clause.namesLower[i], m_globalScope);
            if (symbol == nullptr || (symbol->kind != SymbolKind::Package && symbol->kind != SymbolKind::Generic
                && symbol->kind != SymbolKind::Subprogram)) {
                m_diagnostics.error(clause.location, "cannot find the unit '" + clause.names[i] + "'");
            }
            mention(clause.namesLower[i]);
        }
    }
    context->m_contextClause = true;
    for (UseDecl& use : unit.useClauses) {
        analyzeUseClause(use, context);
    }
    context->m_contextClause = false;
    // A child subprogram is declared in its parent's namespace, but its
    // profile and body resolve in this compilation part's context.
    SubprogramSpec* childSpec = nullptr;
    Symbol* childParent = nullptr;
    std::string savedLower;
    std::string savedName;
    if (name.find('.') != std::string::npos && unit.units.size() == 1) {
        Decl* declaration = unit.units.front().get();
        if (declaration->kind == DeclKind::SubprogramDeclaration) {
            childSpec = &static_cast<SubprogramDecl*>(declaration)->spec;
        } else if (declaration->kind == DeclKind::SubprogramBody) {
            childSpec = &static_cast<SubprogramBody*>(declaration)->spec;
        }
        if (childSpec != nullptr) {
            savedLower = childSpec->lower;
            savedName = childSpec->name;
            childSpec->lower = savedLower.substr(savedLower.rfind('.') + 1);
            childSpec->name = savedName.substr(savedName.rfind('.') + 1);
            std::string parentName = name.substr(0, name.rfind('.'));
            childParent = lookupName(parentName, m_globalScope);
            m_namePrefix = SemaSupport::splitDottedName(parentName);
            if (childParent != nullptr) {
                m_packages.push_back(childParent);
            }
            context->m_publicChildPart = unit.isSpec || existing == nullptr;
        }
    }
    analyzeDeclarativePart(unit.units, context);
    if (childSpec != nullptr) {
        context->m_publicChildPart = false;
        auto local = context->lookupLocal(childSpec->lower);
        if (childParent != nullptr && !local.empty() && childParent->scope->lookupLocal(childSpec->lower).empty()) {
            childParent->scope->add(local.back());
        }
        childSpec->lower = savedLower;
        childSpec->name = savedName;
        m_namePrefix.clear();
        if (childParent != nullptr) {
            m_packages.pop_back();
        }
    }

    auto parts = SemaSupport::splitDottedName(name);
    Scope* target = context;
    Symbol* declared = nullptr;
    for (const std::string& part : parts) {
        auto local = target->lookupLocal(part);
        if (local.empty()) {
            declared = nullptr;
            break;
        }
        declared = local.back();
        target = declared->scope;
        if (target == nullptr && part != parts.back()) {
            declared = nullptr;
            break;
        }
    }
    if (declared != nullptr) {
        declared->m_libraryName = name;
        if (parts.size() == 1 && m_globalScope->lookupLocal(name).empty()) {
            m_globalScope->add(declared);
        }
        if (unit.isSpec || !m_libraryContexts.contains(name)) {
            m_libraryContexts[name] = context;
        }
    }

    if (m_main == nullptr) {
        for (const DeclPtr& decl : unit.units) {
            Symbol* candidate = nullptr;
            if (decl->kind == DeclKind::SubprogramBody) {
                candidate = static_cast<SubprogramBody*>(decl.get())->symbol;
            } else if (decl->kind == DeclKind::SubprogramDeclaration) {
                auto* renaming = static_cast<SubprogramDecl*>(decl.get());
                if (!renaming->m_renamingExpansion.empty()) {
                    candidate = renaming->symbol;
                }
            }
            if (candidate != nullptr && candidate->returnType == nullptr && candidate->parameters.empty()) {
                m_main = candidate;
                break;
            }
        }
    }
}

std::string Sema::mangle(const std::string& name) const
{
    auto encode = [](const std::string& part) {
        if (part == "**") {
            return std::string("operator_power");
        }
        if (operatorSymbol(part).empty()) {
            return part;
        }
        std::string encoded = "operator";
        for (unsigned char c : part) {
            encoded += "_" + std::to_string(c);
        }
        return encoded;
    };
    std::string result;
    for (const std::string& part : m_namePrefix) {
        result += encode(part) + "__";
    }
    result += encode(name);
    return result;
}

std::string Sema::anonymousTypeName()
{
    return "anon." + std::to_string(m_anonymousCounter++);
}
