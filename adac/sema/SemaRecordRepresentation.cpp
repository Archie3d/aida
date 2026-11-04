#include "Sema.h"

#include <algorithm>
#include <cstdint>

namespace {

bool repackableFields(const Type* type)
{
    if (type->kind == TypeKind::Array) { return repackableFields(type->element); }
    return std::all_of(type->fields.begin(), type->fields.end(), [](const FieldInfo& field) {
        return !field.aliased && !field.dependentBounds && !hasPrivateOrTaggedParts(field.type)
            && repackableFields(field.type);
    });
}

} // namespace

void Sema::analyzeRecordRepresentation(RepresentationDecl* decl, Scope* scope, Type* type, Type* precedingType)
{
    if (type->kind != TypeKind::Record || type->isSubtype || type->isIncomplete
        || !representationVisible(type) || type->tagged || hasTasks(type) || hasProtected(type)) {
        m_diagnostics.error(decl->location,
            "a record representation clause requires a complete, visible, untagged record first subtype without task or protected parts");
        return;
    }
    if (type->recordRepresentationSpecified) {
        m_diagnostics.error(decl->location, "duplicate record representation clause");
        return;
    }
    if (type != precedingType) {
        m_diagnostics.error(decl->location, "a record representation clause must be adjacent to its type declaration, before representation use");
        return;
    }
    if (type->layoutSymbol != nullptr) {
        m_diagnostics.error(decl->location, "record representation clauses with declaration-sized runtime components are not yet supported");
        return;
    }
    bool valid = true;
    auto error = [&](const SourceLocation& location, const std::string& message) {
        m_diagnostics.error(location, message);
        valid = false;
    };
    auto integer = [&](Expr* expression, long long& value) {
        Type* actual = analyzeExpr(expression, scope, nullptr);
        if (actual == nullptr || (actual->kind != TypeKind::Integer && actual->kind != TypeKind::UniversalInteger)
            || !foldStatic(expression, value)) {
            error(expression->location, "a record representation expression must be a static integer");
            return false;
        }
        return true;
    };
    Type proposed = *type;
    proposed.dependentStorage = false;
    if (decl->modulus != nullptr) {
        long long modulus = 0;
        if (!integer(decl->modulus.get(), modulus)) { return; }
        if (modulus <= 0 || modulus > 8 || (modulus & (modulus - 1)) != 0) {
            error(decl->location, "a record mod clause supports alignments 1, 2, 4 and 8 no smaller than natural alignment");
        } else if (type->alignmentSpecified && type->alignment != modulus) {
            error(decl->location, "record mod clause conflicts with the preceding Alignment clause");
        } else {
            proposed.alignment = modulus;
            proposed.alignmentSpecified = true;
        }
    }
    // A represented dependent array occupies its maximum inline extent. Bounds
    // still come from each enclosing object, never from a hidden descriptor.
    for (FieldInfo& field : proposed.fields) {
        if (!field.dependentBounds) { continue; }
        Type* cell = field.type;
        for (int dimension = 0; dimension < field.type->arrayRank; ++dimension) { cell = cell->element; }
        __int128 size = typeSize(cell);
        bool bounded = true;
        auto endpoint = [&](Expr* expression, bool upper, long long& value) {
            if (expression->kind == ExprKind::Identifier) {
                Symbol* symbol = static_cast<IdentifierExpr*>(expression)->symbol;
                if (symbol != nullptr && symbol->discriminantRecord != nullptr) {
                    Type* boundType = symbol->type;
                    __int128 bound = upper ? boundType->high : boundType->low;
                    if (boundType->scalarBoundsSymbol == nullptr && bound >= INT64_MIN && bound <= INT64_MAX) {
                        value = static_cast<long long>(bound);
                        return true;
                    }
                    return false;
                }
            }
            return foldStatic(expression, value);
        };
        for (const auto& bounds : field.type->boundExpressions) {
            long long low = 0, high = 0;
            if (!endpoint(bounds.first, false, low) || !endpoint(bounds.second, true, high)) {
                bounded = false;
                break;
            }
            size *= std::max(static_cast<__int128>(0), static_cast<__int128>(high) - low + 1);
            if (size > INT64_MAX) { bounded = false; break; }
        }
        if (!bounded || field.type->boundExpressions.size() != static_cast<std::size_t>(field.type->arrayRank)) {
            error(decl->location, "a represented discriminant-dependent array requires statically bounded maximum storage");
            continue;
        }
        field.inlineDependent = true;
        field.representationSize = static_cast<long long>(size);
    }
    std::vector<bool> specified(proposed.fields.size(), false);
    long long extent = 0;
    for (ComponentClause& clause : decl->components) {
        auto found = std::find_if(proposed.fields.begin(), proposed.fields.end(), [&](const FieldInfo& field) {
            return field.name == clause.lower;
        });
        if (found == proposed.fields.end()) {
            error(clause.location, "'" + clause.name + "' is not a component of '" + type->name + "'");
            continue;
        }
        FieldInfo& field = *found;
        if (specified[field.index]) {
            error(clause.location, "duplicate component clause for '" + clause.name + "'");
            continue;
        }
        specified[field.index] = true;
        long long position = 0, first = 0, last = 0;
        bool constants = integer(clause.position.get(), position);
        constants = integer(clause.firstBit.get(), first) && constants;
        constants = integer(clause.lastBit.get(), last) && constants;
        if (!constants) { continue; }
        if (position < 0 || first < 0 || static_cast<__int128>(last) < static_cast<__int128>(first) - 1) {
            error(clause.location, "component positions and first bits must be nonnegative, and last bit must be at least first bit minus one");
            continue;
        }
        __int128 width = static_cast<__int128>(last) - first + 1;
        __int128 offset = static_cast<__int128>(position) + first / 8;
        int bitOffset = static_cast<int>(first % 8);
        __int128 bytes = width == 0 ? 0 : (bitOffset + width + 7) / 8;
        __int128 end = offset + bytes;
        if (end > INT64_MAX || offset > INT64_MAX) {
            error(clause.location, "record component storage exceeds signed 64-bit capacity");
            continue;
        }
        field.bitOffset = 0;
        field.bitWidth = 0;
        long long required = field.inlineDependent ? fieldStorageSize(field) : typeSize(field.type);
        bool wholeBytes = bitOffset == 0 && width % 8 == 0;
        bool staticDiscrete = isDiscrete(field.type) && field.type->scalarBoundsSymbol == nullptr;
        __int128 low = field.type->low, high = field.type->high;
        if (!field.type->enumCodes.empty() && low >= 0 && high >= low
            && high < static_cast<long long>(field.type->enumCodes.size())) {
            low = field.type->enumCodes[static_cast<std::size_t>(low)];
            high = field.type->enumCodes[static_cast<std::size_t>(high)];
        }
        auto fits = [&](bool signedStorage) {
            __int128 storageLow = signedStorage ? -(static_cast<__int128>(1) << (width - 1)) : 0;
            __int128 storageHigh = (static_cast<__int128>(1) << (width - signedStorage)) - 1;
            return low <= high && low >= storageLow && high <= storageHigh;
        };
        if (wholeBytes && bytes < required && staticDiscrete
            && (bytes == 1 || bytes == 2 || bytes == 4 || bytes == 8)
            && !(field.type->kind == TypeKind::Enumeration && bytes == 8)
            && field.type->alignment <= bytes && offset % bytes == 0
            && fits(low < 0 || width == 32)) {
            Type* storage = m_types.makeSubtype(anonymousTypeName(), field.type, field.type->low, field.type->high);
            storage->byteSize = static_cast<long long>(bytes);
            storage->specifiedSize = storage->byteSize;
            field.type = storage;
            required = storage->byteSize;
        }
        if (!wholeBytes || offset % typeAlignment(field.type) != 0 || (bytes < required && staticDiscrete)) {
            if (!staticDiscrete || field.aliased || sharedTypeKind(field.type) != 0 || width < 1 || width > 64) {
                error(clause.location, "packed or unaligned components require nonaliased, nonshared discrete subtypes with static bounds and 1 to 64 bits");
                continue;
            }
            if (!fits(low < 0)) {
                error(clause.location, "component storage is smaller than its required representation");
                continue;
            }
            field.bitOffset = bitOffset;
            field.bitWidth = static_cast<int>(width);
        } else if (bytes < required) {
            error(clause.location, "component storage is smaller than its required representation");
            continue;
        }
        field.offset = static_cast<long long>(offset);
        field.representationSize = static_cast<long long>(bytes);
        extent = std::max(extent, static_cast<long long>(end));
    }
    bool changed = type->parentType != nullptr
        && !std::equal(proposed.fields.begin(), proposed.fields.end(), type->fields.begin(), sameFieldPlacement);
    if (changed) {
        if (hasLimitedParts(type) || needsFinalization(type) || hasSharedParts(type) || !repackableFields(type)) {
            error(decl->location, "changing inherited record layouts requires nonlimited, unmanaged fields without aliases, shared storage or dependent bounds");
        }
        auto designatesRecord = [&](const Type* profileType) {
            return profileType != nullptr && profileType->kind == TypeKind::Access && profileType->anonymousAccess
                && rootType(profileType->target) == type;
        };
        for (const Symbol* primitive : type->primitives) {
            if (primitive->inheritedFrom == nullptr) { continue; }
            if (designatesRecord(primitive->returnType)
                || std::any_of(primitive->parameters.begin(), primitive->parameters.end(), [&](const Symbol* parameter) {
                    return designatesRecord(parameter->type);
                })) {
                error(decl->location, "changing inherited record layouts with inherited anonymous access profiles is not supported");
                break;
            }
        }
    }
    // Unspecified components follow all explicit slots. This also keeps common
    // components clear of every variant arm without guessing an active variant.
    for (FieldInfo& field : proposed.fields) {
        if (specified[field.index]) { continue; }
        if (type->parentType == nullptr || changed) {
            long long fieldAlignment = field.bitWidth != 0 ? 1 : typeAlignment(field.type);
            __int128 offset = (static_cast<__int128>(extent) + fieldAlignment - 1) / fieldAlignment * fieldAlignment;
            __int128 end = offset + fieldStorageSize(field);
            if (end > INT64_MAX) {
                error(decl->location, "record component storage exceeds signed 64-bit capacity");
                continue;
            }
            field.offset = static_cast<long long>(offset);
            extent = static_cast<long long>(end);
        }
    }
    auto firstBit = [](const FieldInfo& field) {
        return static_cast<__int128>(field.offset) * 8 + field.bitOffset;
    };
    auto bitWidth = [](const FieldInfo& field) -> __int128 {
        return field.bitWidth != 0 ? field.bitWidth : static_cast<__int128>(fieldStorageSize(field)) * 8;
    };
    for (std::size_t i = 0; i < proposed.fields.size(); ++i) {
        const FieldInfo& first = proposed.fields[i];
        for (std::size_t j = 0; j < i; ++j) {
            const FieldInfo& second = proposed.fields[j];
            if (!variantsExclusive(type, first.variant, second.variant)
                && bitWidth(first) > 0 && bitWidth(second) > 0
                && firstBit(first) < firstBit(second) + bitWidth(second)
                && firstBit(second) < firstBit(first) + bitWidth(first)) {
                error(decl->location, "component storage overlaps for '" + first.displayName + "' and '" + second.displayName + "'");
            }
        }
    }
    Type natural = proposed;
    natural.alignment = 0;
    if (decl->modulus != nullptr && proposed.alignment > 0 && proposed.alignment < typeAlignment(&natural)) {
        error(decl->location, "a record mod clause cannot weaken component alignment");
    }
    long long alignment = typeAlignment(&proposed);
    proposed.compositeSize = 0;
    long long size = typeLayoutSize(&proposed);
    if (size == INT64_MAX) { error(decl->location, "record layout exceeds signed 64-bit storage capacity"); }
    if (type->specifiedSize >= 0 && (type->specifiedSize < size || type->specifiedSize % alignment != 0)) {
        error(decl->location, "record representation conflicts with the preceding Size clause");
    }
    if (!valid) { return; }
    type->fields = std::move(proposed.fields);
    type->dependentStorage = false;
    type->alignment = proposed.alignment;
    type->alignmentSpecified = proposed.alignmentSpecified;
    type->recordRepresentationSpecified = true;
    type->changedRecordPlacement = type->changedRecordPlacement || changed;
    m_types.refreshRecordSubtypes(type);
    m_types.refreshStorageRepresentation(type);
}
