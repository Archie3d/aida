#include "Sema.h"
#include "../../common/CAbiFacts.h"
#include "SemaSupport.h"

Type* Sema::interfacesCTargetType(const std::string& lower, Scope* scope, const SourceLocation& location)
{
    if (scope != m_packageSpecScope || scope->libraryPackage() != "interfaces.c") { return nullptr; }
    if (lower == "aida_c_plain_char") {
        return resolveTypeName(kCAbiIntegers[kCAbiChar].isSigned ? "signed_char" : "unsigned_char", scope, location);
    }
    if (lower == "aida_c_long") {
        switch (kCAbiIntegers[kCAbiLong].bytes) {
        case 4: return resolveTypeName("integer_32", scope, location);
        case 8: return resolveTypeName("integer_64", scope, location);
        }
    }
    if (lower == "aida_c_wchar") {
        switch (kCAbiIntegers[kCAbiWchar].bytes) {
        case 2: return m_types.wideCharacterType();
        case 4: return m_types.wideWideCharacterType();
        }
    }
    return nullptr;
}

bool Sema::declareUnavailableLongDouble(TypeDecl* decl, Scope* scope)
{
    if (decl->lower != "long_double" || scope != m_packageSpecScope || scope->libraryPackage() != "interfaces.c"
        || cAbiHasNativeLongDouble()) {
        return false;
    }
    // Preserve the name and visibility without providing a substitute representation.
    if (auto completion = m_limitedCompletions.find(decl); completion != m_limitedCompletions.end()) {
        m_unavailableLongDouble = completion->second;
        m_unavailableLongDouble->kind = TypeKind::Void;
        m_unavailableLongDouble->isIncomplete = false;
    } else {
        m_unavailableLongDouble = m_types.create(TypeKind::Void, decl->name);
    }
    m_unavailableLongDouble->declarationScope = scope;
    decl->declaredType = m_unavailableLongDouble;
    addTypeTo(scope, m_unavailableLongDouble)->location = decl->location;
    return true;
}

bool Sema::checkAvailableType(const Type* type, const SourceLocation& location)
{
    if (m_unavailableLongDouble == nullptr || type == nullptr
        || (type != m_unavailableLongDouble && (!type->limitedView || type->base != m_unavailableLongDouble))) {
        return true;
    }
    m_diagnostics.error(location, std::string("Interfaces.C.long_double is unsupported on ") + kCAbiTargetName
        + " (C long double format: " + cAbiFloatingFormat(&kCAbiFloats[kCAbiLongDouble]) + ")");
    return false;
}

void Sema::setInterfacesCCharacterConvention(Type* type, const TypeDecl* decl, Scope* scope)
{
    if (scope != m_packageSpecScope || scope->libraryPackage() != "interfaces.c") { return; }
    if (decl->lower == "char") {
        type->cCharacter = kCAbiIntegers[kCAbiChar].isSigned ? CCharacterConvention::Signed : CCharacterConvention::Unsigned;
    } else if (decl->lower == "wchar_t") {
        type->cCharacter = kCAbiIntegers[kCAbiWchar].isSigned ? CCharacterConvention::Signed : CCharacterConvention::Unsigned;
    } else if (decl->lower == "char16_t" || decl->lower == "char32_t") {
        type->cCharacter = CCharacterConvention::Unsigned;
    }
}

void Sema::analyzeInterfacesIntrinsic(PragmaDecl* decl, Scope* scope, Symbol* target)
{
    if (scope != m_packageSpecScope || scope->libraryPackage() != "interfaces"
        || (target->name != "shift_left" && target->name != "shift_right"
            && target->name != "shift_right_arithmetic" && target->name != "rotate_left" && target->name != "rotate_right")) {
        m_diagnostics.error(decl->location, "Intrinsic Import is supported for the predefined Interfaces shifts and rotates");
        return;
    }
    Type* result = target->returnType;
    long long bytes = typeSize(result);
    long long bits = bytes >= 1 && bytes <= 8 ? bytes * 8 : 0;
    bool valid = !target->hasBody && result != nullptr && result->kind == TypeKind::Integer && result->declarationScope == scope
        && (bits == 8 || bits == 16 || bits == 32 || bits == 64)
        && result->modulus == (__int128{1} << bits)
        && qbeClass(result) == (bits <= 16 ? 'w' : 'l')
        && target->parameters.size() == 2
        && SemaSupport::staticallyMatches(target->parameters[0]->type, result)
        && target->parameters[0]->mode == ParameterMode::In
        && SemaSupport::staticallyMatches(target->parameters[1]->type, m_types.naturalType())
        && target->parameters[1]->mode == ParameterMode::In;
    if (!valid) {
        m_diagnostics.error(decl->location, "invalid predefined Interfaces intrinsic profile or repeated Import");
        return;
    }
    target->builtin = BuiltinKind::Runtime;
    target->runtimeSymbol = "$__ada_interfaces_" + target->name + "_" + std::to_string(bits);
    target->intrinsicRenaming = true;
    target->hasBody = true;
}
