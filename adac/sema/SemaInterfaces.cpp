#include "Sema.h"
#include "../../common/CAbiFacts.h"

#include <algorithm>

namespace {

bool nativeCScalar(const Type* type)
{
    if (type->kind == TypeKind::Float) {
        return typeSize(type) == 4 || typeSize(type) == 8;
    }
    if (type->kind != TypeKind::Integer && type->kind != TypeKind::Enumeration) {
        return false;
    }
    long long size = typeSize(type);
    return size == 1 || size == 2 || size == 4 || size == 8;
}

bool nativeCElement(const Type* type)
{
    return type != nullptr && !type->isIncomplete && nativeCScalar(type) && typeAlignment(type) == typeSize(type)
        && (type->kind != TypeKind::Enumeration || typeSize(type) == 4
            || type->cCharacter != CCharacterConvention::None);
}

bool nativeCObjectTarget(const Type* type)
{
    return nativeCElement(type) && sharedTypeKind(type) == 0;
}

bool nativeCArray(const Type* type)
{
    return type->arrayRank == 1 && type->packedComponentBits == 0
        && nativeCObjectTarget(type->element) && typeAlignment(type) >= typeAlignment(type->element);
}

bool nativeCRecord(Type* type)
{
    Type* layout = baseType(type);
    return !layout->tagged && !layout->isLimited && layout->discriminantCount == 0
        && !layout->fields.empty() && !hasRuntimeLayout(type) && !needsFinalization(type)
        && std::all_of(layout->fields.begin(), layout->fields.end(), [&](const FieldInfo& field) {
            return field.bitWidth == 0 && nativeCObjectTarget(field.type)
                && typeAlignment(type) >= typeAlignment(field.type)
                && field.offset % typeAlignment(field.type) == 0;
        });
}

} // namespace

bool Sema::supportsNativeCResult(Type* type)
{
    if (type == nullptr || rootType(type) == m_addressType) { return true; }
    if (type->isIncomplete) { return false; }
    if (type->kind != TypeKind::Access) { return nativeCScalar(type); }
    if (type->anonymousAccess) { return false; }
    if (type->accessProfile != nullptr) { return type->accessConventionC; }
    return nativeCObjectTarget(type->target);
}

bool Sema::supportsNativeCParameter(const Symbol* parameter)
{
    Type* type = parameter->type;
    if (type == nullptr || type->isIncomplete) { return false; }
    if (type->kind == TypeKind::Array) { return nativeCArray(type); }
    if (type->kind == TypeKind::Record) { return nativeCRecord(type); }
    if (type->kind == TypeKind::Access && type->anonymousAccess) {
        return parameter->mode == ParameterMode::In && type->accessParameter
            && type->accessProfile == nullptr && nativeCObjectTarget(type->target);
    }
    return supportsNativeCResult(type);
}

bool Sema::supportsNativeCStorage(Type* type)
{
    return !type->isIncomplete && (type->kind == TypeKind::Array ? nativeCArray(type)
        : type->kind == TypeKind::Access && type->accessProfile == nullptr && supportsNativeCResult(type));
}
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
