#include "Sema.h"

#include <algorithm>
#include <functional>
#include "SemaSupport.h"
#include <cstdint>
#include <limits>

using SemaSupport::isUniversal;
using SemaSupport::adaptUniversal;

static bool checkArrayStorage(Type* type, Diagnostics& diagnostics, const SourceLocation& location)
{
    std::vector<Type*> axes;
    Type* cell = type;
    for (int i = 0; i < type->arrayRank; ++i) { axes.push_back(cell); cell = cell->element; }
    long long size = typeSize(cell);
    for (auto axis = axes.rbegin(); axis != axes.rend(); ++axis) {
        if (!(*axis)->constrained) { return true; }
        __int128 length = (*axis)->indexHigh < (*axis)->indexLow ? 0
            : (__int128)(*axis)->indexHigh - (*axis)->indexLow + 1;
        if (length > INT64_MAX) {
            diagnostics.error(location, "array length exceeds signed 64-bit descriptor capacity");
            return false;
        }
        if (__builtin_mul_overflow(size, (long long)length, &size)) {
            diagnostics.error(location, type->arrayRank > 1 ? "multidimensional array storage size is too large"
                : "array storage size is too large");
            return false;
        }
    }
    return true;
}

void Sema::analyzeTypeDecl(TypeDecl* decl, Scope* scope)
{
    // A type already named here without being described is completed by this
    // declaration rather than shadowed by it, so that the access type which
    // pointed at it goes on pointing at the same thing.
    Type* completing = nullptr;
    for (Symbol* candidate : scope->lookupLocal(decl->lower)) {
        if (candidate->kind == SymbolKind::TypeName && candidate->type != nullptr
            && candidate->type->isIncomplete) {
            completing = candidate->type;
        } else {
            m_diagnostics.error(decl->location, "'" + decl->name + "' has already been declared in this scope");
            return;
        }
    }

    if (decl->definition == nullptr) {
        if (completing != nullptr) {
            m_diagnostics.error(decl->location, "'" + decl->name + "' has already been named here");
            return;
        }
        Type* placeholder = m_types.create(TypeKind::Record, decl->name);
        placeholder->isIncomplete = true;
        placeholder->m_declarationScope = scope;
        decl->declaredType = placeholder;

        Symbol* named = m_symbolTable.createSymbol(SymbolKind::TypeName, decl->lower, decl->name);
        named->type = placeholder;
        named->location = decl->location;
        scope->add(named);
        return;
    }

    // Every branch below builds the type through this, so that a completion
    // fills in the object already handed out instead of making another.  What
    // the visible declaration settled - that the type is private, and to whom -
    // outlives the completion, since that is the whole point of it.
    auto makeType = [&](TypeKind kind) {
        if (completing == nullptr) {
            if (m_namePrefix.size() == 2 && m_namePrefix[0] == "ada" && m_namePrefix[1] == "tags"
                && decl->lower == "tag") {
                return m_tagType;
            }
            if (m_namePrefix.size() == 2
                && m_namePrefix[0] == "ada" && m_namePrefix[1] == "exceptions") {
                if (decl->lower == "exception_occurrence") {
                    return m_exceptionOccurrenceType;
                }
                if (decl->lower == "exception_id") {
                    return m_exceptionIdType;
                }
            }
            if (m_namePrefix.size() == 2 && m_namePrefix[0] == "ada"
                && m_namePrefix[1] == "task_identification" && decl->lower == "task_id") {
                return m_taskIdType;
            }
            return m_types.create(kind, decl->name);
        }
        std::string name = completing->name;
        Symbol* privateTo = completing->privateTo;
        bool isLimited = completing->isLimited;
        bool tagged = completing->m_tagged;
        bool privateViewTagged = completing->m_privateViewTagged;
        bool controlled = completing->m_controlled;
        std::string tagName = completing->m_tagName;
        auto slots = std::move(completing->m_dispatchSlots);
        Type* classWide = completing->m_classWide;
        auto primitives = std::move(completing->m_primitives);
        *completing = Type(kind, name);
        completing->m_primitives = std::move(primitives);
        completing->m_dispatchSlots = std::move(slots);
        completing->m_classWide = classWide;
        completing->privateTo = privateTo;
        completing->isLimited = isLimited;
        completing->m_tagged = tagged;
        completing->m_privateViewTagged = privateViewTagged;
        completing->m_controlled = controlled;
        completing->m_tagName = tagName;
        return completing;
    };

    TypeDefinition* definition = decl->definition.get();
    if (completing != nullptr && completing->privateTo != nullptr && completing->m_tagged
        && !(definition->kind == TypeDefKind::Record && definition->m_tagged)
        && !(definition->kind == TypeDefKind::Derived && definition->m_extension)) {
        m_diagnostics.error(decl->location, "a tagged private type requires a tagged full definition");
        return;
    }
    if (completing != nullptr && completing->privateTo != nullptr
        && completing->m_abstract != definition->m_abstract) {
        m_diagnostics.error(decl->location, "a private type and its completion must agree on abstractness");
        return;
    }
    bool nonlimitedPrivateCompletion = completing != nullptr && completing->privateTo != nullptr
        && !completing->isLimited;
    Type* type = nullptr;

    switch (definition->kind) {
    case TypeDefKind::Protected: {
        type = makeType(TypeKind::Record);
        type->m_protectedScope = m_symbolTable.createScope(scope);
        type->isLimited = true;
        type->needsZeroInit = true;
        std::string name = "$" + mangle(decl->lower) + "__protected";
        std::size_t ordinal = ++m_tagNames[name];
        type->m_tagName = name + (ordinal > 1 ? "__" + std::to_string(ordinal) : "");
        layoutRecord(decl, definition, type, scope);
        for (const FieldInfo& field : type->fields) {
            if (needsFinalization(field.type) || hasTasks(field.type)) {
                m_diagnostics.error(decl->location, "protected private components with controlled, task or protected parts are not yet supported");
            }
        }
        break;
    }
    case TypeDefKind::Task: {
        type = makeType(TypeKind::Record);
        type->byteSize = 8;
        type->isLimited = true;
        type->needsZeroInit = true;
        SubprogramSpec spec;
        spec.name = decl->name;
        spec.lower = "__task_" + (decl->m_taskObjects.empty() ? decl->lower
            : static_cast<ObjectDecl*>(decl->m_taskObjects.front().get())->namesLower.front());
        spec.location = decl->location;
        type->m_taskBody = declareSubprogram(spec, scope, false);
        type->m_taskBody->m_taskBody = true;
        break;
    }
    case TypeDefKind::Enumeration: {
        type = makeType(TypeKind::Enumeration);
        type->low = 0;
        type->high = static_cast<long long>(definition->literals.size()) - 1;
        for (std::size_t i = 0; i < definition->literals.size(); ++i) {
            bool duplicate = definition->literalsLower[i] == decl->lower;
            for (Symbol* existing : scope->lookupLocal(definition->literalsLower[i])) {
                duplicate = duplicate || (existing->kind != SymbolKind::Subprogram
                    && existing->kind != SymbolKind::EnumerationLiteral)
                    || (existing->kind == SymbolKind::EnumerationLiteral && existing->type == type);
            }
            if (duplicate) {
                m_diagnostics.error(definition->location, "'" + definition->literals[i]
                    + "' has already been declared in this scope");
                continue;
            }
            type->literals.push_back(definition->literalsLower[i]);
            Symbol* literal = m_symbolTable.createSymbol(SymbolKind::EnumerationLiteral,
                                                         definition->literalsLower[i], definition->literals[i]);
            literal->location = definition->location;
            literal->type = type;
            literal->enumerationValue = static_cast<long long>(i);
            if (scope == m_packageSpecScope && !m_inVisiblePart && !m_packages.empty()) {
                literal->m_privatePrimitiveTo = m_packages.back();
            }
            scope->add(literal);
            type->m_primitives.push_back(literal);
        }
        break;
    }
    case TypeDefKind::Modular: {
        type = makeType(TypeKind::Integer);
        Type* modulusType = analyzeExpr(definition->rangeHigh.get(), scope, nullptr);
        __int128 modulus = 0;
        StaticFailure failure = StaticFailure::None;
        if (modulusType == nullptr || (modulusType->kind != TypeKind::Integer
            && modulusType->kind != TypeKind::UniversalInteger)
            || !foldWideInteger(definition->rangeHigh.get(), modulus, failure)) {
            m_diagnostics.error(definition->location, "the modulus must be a static integer expression");
            modulus = 1;
        } else if (modulus < 1 || modulus > ((__int128)1 << 64)) {
            m_diagnostics.error(definition->location, "supported moduli are 1 through 2 ** 64");
            modulus = 1;
        }
        type->m_modulus = modulus;
        type->low = 0;
        type->high = modulus - 1;
        break;
    }
    case TypeDefKind::IntegerRange: {
        type = makeType(TypeKind::Integer);
        analyzeExpr(definition->rangeLow.get(), scope, m_types.universalInteger());
        analyzeExpr(definition->rangeHigh.get(), scope, m_types.universalInteger());
        long long low = 0;
        long long high = 0;
        if (!foldStatic(definition->rangeLow.get(), low) || !foldStatic(definition->rangeHigh.get(), high)) {
            m_diagnostics.error(definition->location, "the bounds of an integer type must be static");
        }
        type->low = low;
        type->high = high;
        break;
    }
    case TypeDefKind::FixedDelta: {
        type = makeType(TypeKind::Fixed);
        Type* deltaType = analyzeExpr(definition->m_delta.get(), scope, nullptr);
        ExactReal delta;
        if (!isReal(deltaType) || !exactValue(definition->m_delta.get(), delta)
            || delta.m_numerator <= 0) {
            m_diagnostics.error(definition->location, "fixed-point delta must be a positive exact static real expression");
            delta = ExactReal::make(1);
        }
        if (definition->digits) {
            Type* digitsType = analyzeExpr(definition->digits.get(), scope, m_types.integerType());
            long long digits = 0;
            if (digitsType == nullptr || (digitsType->kind != TypeKind::Integer && digitsType->kind != TypeKind::UniversalInteger)
                || !foldStatic(definition->digits.get(), digits) || digits < 1 || digits > 18) {
                m_diagnostics.error(definition->location, "decimal fixed-point types require 1 to 18 static digits");
                digits = 1;
            }
            int scale = ADA_FIXED_MIN_SCALE;
            for (; scale <= ADA_FIXED_MAX_SCALE; ++scale) {
                ExactReal candidate = scale >= 0 ? ExactReal::make(1, fixedPower10(scale)) : ExactReal::make(fixedPower10(-scale));
                if (candidate.m_numerator == delta.m_numerator && candidate.m_denominator == delta.m_denominator) { break; }
            }
            if (scale > ADA_FIXED_MAX_SCALE) {
                m_diagnostics.error(definition->location, "decimal delta must be a power of ten from 10.0**(-18) through 10.0**18");
                scale = 0;
            }
            type->m_fixedScale = ADA_FIXED_DECIMAL_TAG + scale;
            type->m_fixedAft = std::max(1, scale);
            type->m_fixedRadix = 10;
            type->digits = static_cast<int>(digits);
            type->m_delta = definition->m_delta.get();
            long long limit = static_cast<long long>(fixedPower10(static_cast<int>(digits)) - 1);
            long long lowCounts = -limit, highCounts = limit;
            if (definition->rangeLow && definition->rangeHigh) {
                Type* lowType = analyzeExpr(definition->rangeLow.get(), scope, nullptr);
                Type* highType = analyzeExpr(definition->rangeHigh.get(), scope, nullptr);
                ExactReal low, high;
                if (!isReal(lowType) || !isReal(highType) || !exactValue(definition->rangeLow.get(), low)
                    || !exactValue(definition->rangeHigh.get(), high)
                    || !low.scaled(type->m_fixedScale, lowCounts) || !high.scaled(type->m_fixedScale, highCounts)) {
                    m_diagnostics.error(definition->location, "decimal bounds must be exact static real expressions within 64-bit scaled storage");
                } else {
                    // Check the mathematical bounds before rounding, as required
                    // by the digits definition, including fractional endpoints.
                    ExactReal min = ExactReal::fixed(-limit, type->m_fixedScale);
                    ExactReal max = ExactReal::fixed(limit, type->m_fixedScale);
                    ExactReal below = ExactReal::operation('-', low, min);
                    ExactReal above = ExactReal::operation('-', max, high);
                    if (!below.m_valid || !above.m_valid || below.m_numerator < 0 || above.m_numerator < 0) {
                        m_diagnostics.error(definition->location, "decimal bounds exceed the range permitted by digits");
                    }
                }
            }
            type->low = lowCounts;
            type->high = highCounts;
            if (lowCounts > highCounts) {
                m_diagnostics.error(definition->location, "a decimal fixed-point type requires a non-null range");
            }
            break;
        }
        int bits = ADA_FIXED_MIN_BITS;
        auto belowSmall = [&]() {
            return bits >= 0 ? delta.m_numerator <= (delta.m_denominator - 1) / ((__int128)1 << bits)
                             : delta.m_numerator / delta.m_denominator < ((__int128)1 << -bits);
        };
        while (bits < ADA_FIXED_MAX_BITS && belowSmall()) { ++bits; }
        if (belowSmall() || delta.m_numerator / delta.m_denominator > ((__int128)1 << -ADA_FIXED_MIN_BITS)) {
            m_diagnostics.error(definition->location, "supported fixed-point deltas are 2.0**(-62) through 2.0**62");
        }
        if (decl->m_smallClause != nullptr) {
            type->m_smallClause = decl->m_smallClause;
            // Delta may be referenced in the clause before the final type
            // symbol is published; Small itself is still being defined.
            type->m_delta = definition->m_delta.get();
            Scope* representationScope = m_symbolTable.createScope(scope);
            Symbol* self = m_symbolTable.createSymbol(SymbolKind::TypeName, decl->lower, decl->name);
            self->type = type;
            representationScope->add(self);
            type->m_smallPending = true;
            Type* smallType = analyzeExpr(decl->m_smallClause, representationScope, nullptr);
            type->m_smallPending = false;
            ExactReal small;
            bool valid = isReal(smallType) && exactValue(decl->m_smallClause, small) && small.m_numerator > 0;
            int selected = ADA_FIXED_MIN_BITS;
            for (; valid && selected <= ADA_FIXED_MAX_BITS; ++selected) {
                ExactReal candidate = selected >= 0 ? ExactReal::make(1, (__int128)1 << selected)
                                                    : ExactReal::make((__int128)1 << -selected);
                if (candidate.m_numerator == small.m_numerator && candidate.m_denominator == small.m_denominator) { break; }
            }
            if (!valid || selected > ADA_FIXED_MAX_BITS) {
                m_diagnostics.error(decl->m_smallClause->location, "Small must be a positive static power of two from 2.0**(-62) through 2.0**62");
            } else {
                bits = selected;
                if (belowSmall()) {
                    m_diagnostics.error(decl->m_smallClause->location, "Small must not exceed the type's delta");
                }
            }
        }
        type->m_fixedScale = bits;
        __int128 decimalScale = 10;
        while (type->m_fixedAft < 20
            && delta.m_numerator < (delta.m_denominator - 1) / decimalScale + 1) {
            ++type->m_fixedAft;
            decimalScale *= 10;
        }
        type->m_delta = definition->m_delta.get();
        ExactReal low, high;
        long long lowCounts = 0, highCounts = 0;
        Type* lowType = analyzeExpr(definition->rangeLow.get(), scope, nullptr);
        Type* highType = analyzeExpr(definition->rangeHigh.get(), scope, nullptr);
        if (!isReal(lowType) || !isReal(highType) || !exactValue(definition->rangeLow.get(), low)
            || !exactValue(definition->rangeHigh.get(), high)
            || !low.scaled(bits, lowCounts) || !high.scaled(bits, highCounts)) {
            m_diagnostics.error(definition->location, "fixed-point bounds must be exact static real expressions within 64-bit scaled storage");
        }
        type->low = lowCounts;
        type->high = highCounts;
        if (type->low > type->high) {
            m_diagnostics.error(definition->location, "a fixed-point type requires a non-null range");
        }
        break;
    }
    case TypeDefKind::FloatDigits: {
        type = makeType(TypeKind::Float);
        analyzeExpr(definition->digits.get(), scope, m_types.integerType());
        long long digits = 0;
        if (!foldStatic(definition->digits.get(), digits)) {
            m_diagnostics.error(definition->location, "the accuracy of a floating point type must be static");
            digits = 6;
        }
        if (digits < 1 || digits > 15) {
            m_diagnostics.error(definition->location, "a floating point type supports 1 to 15 digits");
            digits = digits < 1 ? 1 : 15;
        }
        type->digits = static_cast<int>(digits);

        if (definition->rangeLow && definition->rangeHigh) {
            Type* lowType = analyzeExpr(definition->rangeLow.get(), scope, type);
            Type* highType = analyzeExpr(definition->rangeHigh.get(), scope, type);
            double low = 0.0;
            double high = 0.0;
            if (!isReal(lowType) || !isReal(highType)) {
                m_diagnostics.error(definition->location, "the bounds of a floating point type must be real");
            } else if (!foldStaticReal(definition->rangeLow.get(), low)
                       || !foldStaticReal(definition->rangeHigh.get(), high)) {
                m_diagnostics.error(definition->location, "the bounds of a floating point type must be static");
            } else {
                type->hasRealRange = true;
                type->lowReal = low;
                type->highReal = high;
            }
        }
        break;
    }
    case TypeDefKind::Array: {
        type = makeType(TypeKind::Array);
        int rank = static_cast<int>(definition->indexTypes.size());
        Type* cell = resolveSubtypeIndication(definition->elementType.get(), scope);
        type->m_incompleteLayout = hasIncompleteParts(cell);
        if (cell != nullptr && cell->m_abstract) {
            m_diagnostics.error(decl->location, "abstract array components are not supported");
            return;
        }
        if (cell != nullptr && cell->m_classRoot != nullptr) {
            m_diagnostics.error(decl->location, "an array component requires a definite subtype");
            return;
        }
        if (cell != nullptr && cell->kind == TypeKind::Array && !cell->constrained) {
            m_diagnostics.error(definition->elementType->location, cell->m_boundsSymbol != nullptr
                                    ? "runtime-constrained array components are not yet supported"
                                    : "array components require a constrained array subtype");
        }
        bool dynamicBounds = false;
        Type* row = type;
        for (int dimension = 0; dimension < rank; ++dimension) {
            row->arrayRank = rank - dimension;
            row->constrained = !definition->unconstrainedIndexes;
            SubtypeIndication* index = definition->indexTypes[dimension].get();
            Type* indexType = index->name.empty() && index->rangeLow && index->rangeHigh
                ? commonOperandType(index->rangeLow.get(), index->rangeHigh.get(), scope, nullptr)
                : index->name.empty() ? m_types.integerType() : resolveTypeName(index->lower, scope, index->location);
            if (indexType == nullptr || isUniversal(indexType)) { indexType = m_types.integerType(); }
            row->index = indexType;
            if (indexType->high > LLONG_MAX) {
                m_diagnostics.error(index->location, "array index subtypes require signed 64-bit bounds");
            }
            if (!isDiscrete(indexType)) {
                m_diagnostics.error(index->location, "array indices require a discrete type");
            }
            if (index->rangeLow && index->rangeHigh) {
                Type* lowType = analyzeExpr(index->rangeLow.get(), scope, indexType);
                Type* highType = analyzeExpr(index->rangeHigh.get(), scope, indexType);
                if (!typesCompatible(indexType, lowType) || !typesCompatible(indexType, highType)) {
                    m_diagnostics.error(index->location, "array index bounds have an incompatible type");
                }
                if (!foldStatic(index->rangeLow.get(), row->indexLow)
                    || !foldStatic(index->rangeHigh.get(), row->indexHigh)) {
                    if (m_currentSubprogram == nullptr && m_recordContract == nullptr) {
                        m_diagnostics.error(index->location, "runtime array type bounds are supported only inside a subprogram");
                    }
                    dynamicBounds = true;
                }
            } else if (indexType != nullptr && row->constrained) {
                row->indexLow = indexType->low;
                row->indexHigh = indexType->high;
                if (indexType->m_scalarBoundsSymbol != nullptr) {
                    index->rangeLow = scalarBoundExpr(indexType, true, index->location);
                    index->rangeHigh = scalarBoundExpr(indexType, false, index->location);
                }
            }
            if (indexType != nullptr && indexType->m_scalarBoundsSymbol != nullptr && row->constrained) {
                dynamicBounds = true;
            }
            type->m_boundExpressions.push_back({ index->rangeLow.get(), index->rangeHigh.get() });
            if (dimension + 1 == rank) {
                row->element = cell;
            } else {
                row->element = m_types.create(TypeKind::Array, anonymousTypeName());
                row = row->element;
                row->isArrayRow = true;
            }
        }
        if (dynamicBounds) {
            for (Type* axis = type; axis != cell; axis = axis->element) {
                axis->constrained = false;
            }
        } else {
            type->m_boundExpressions.clear();
        }
        if (type->constrained) { checkArrayStorage(type, m_diagnostics, definition->location); }

        break;
    }
    case TypeDefKind::Record: {
        type = makeType(TypeKind::Record);
        type->m_tagged = definition->m_tagged;
        layoutRecord(decl, definition, type, scope);
        break;
    }
    case TypeDefKind::Derived: {
        Type* parent = resolveSubtypeIndication(definition->parent.get(), scope);
        if (parent == nullptr) {
            return;
        }
        if (parent->isIncomplete && (!parent->m_tagged || parent->privateTo == nullptr)) {
            m_diagnostics.error(decl->location, parent->privateTo == nullptr
                ? "a derivation parent must be completely defined"
                : "derivation from an uncompleted private view is not yet supported");
            return;
        }
        if (!decl->discriminants.empty()) {
            m_diagnostics.error(decl->location, "new discriminant parts on derived types are not yet supported");
            return;
        }
        if (hasProtected(parent)) {
            m_diagnostics.error(decl->location, "derivation of types containing protected objects is not yet supported");
            return;
        }
        if (hasTasks(parent)) {
            m_diagnostics.error(decl->location, "derivation of types containing tasks is not yet supported");
            return;
        }
        if (completing != nullptr && parent->m_controlled && parent->isLimited && !completing->isLimited) {
            m_diagnostics.error(decl->location, "a limited controlled completion requires a limited private view");
            return;
        }
        if (parent->m_classRoot != nullptr) {
            m_diagnostics.error(decl->location, "a class-wide type cannot be a derivation parent");
            return;
        }
        if (parent->m_tagged) {
            parent = rootType(parent);
        }
        if (definition->m_extension && (!parent->m_tagged || parent->kind != TypeKind::Record)) {
            m_diagnostics.error(decl->location, "a record extension requires a tagged parent type");
            return;
        }
        if (parent->m_tagged && !definition->m_extension) {
            m_diagnostics.error(decl->location, "derivation from a tagged type requires with record or with null record");
            return;
        }
        bool finalizationRoot = parent->m_controlled && parent->m_parentType == nullptr;
        if (parent->m_tagged && ((!representationVisible(parent) && !finalizationRoot) || parent->isIncomplete)) {
            m_diagnostics.error(decl->location, "extension of a private or incomplete tagged view is not yet supported");
            return;
        }
        Type* built = m_types.makeSubtype(decl->name, parent, parent->low, parent->high);
        built->isSubtype = false;
        built->base = nullptr;
        built->kind = parent->kind;
        built->literals = parent->literals;
        // A derivation severs the subtype chain, so retain inherited constraints.
        for (int i = 0; i < parent->discriminantCount; ++i) {
            long long value = 0;
            if (discriminantValueOf(parent, i, value)) {
                built->discriminantsKnown = true;
                built->discriminantValues.push_back(value);
            }
        }

        // Settled before the literals are made, so that each of them names the
        // type this declaration ends up being.
        type = makeType(parent->kind);
        std::string name = type->name;
        auto primitives = std::move(type->m_primitives);
        Type* classWide = type->m_classWide;
        Symbol* privateTo = type->privateTo;
        bool isLimited = type->isLimited;
        bool privateViewTagged = type->m_privateViewTagged;
        *type = *built;
        if (completing != nullptr) {
            type->privateTo = privateTo;
            type->m_privateViewTagged = privateViewTagged;
            type->isLimited = isLimited;
        }
        if (completing == nullptr && parent->m_controlled) {
            type->privateTo = nullptr;
        }
        type->m_primitives = std::move(primitives);
        type->m_parentType = rootType(parent);
        type->m_parentVisible = completing == nullptr || privateTo == nullptr;
        type->m_classWide = completing != nullptr ? classWide : nullptr;
        type->m_dispatchSlots = parent->m_dispatchSlots;
        for (Symbol* primitive : type->m_primitives) {
            primitive->m_dispatchSlot = -1;
        }
        type->m_dispatchFrozen = false;
        rootType(parent)->m_dispatchFrozen = parent->m_tagged;
        type->name = name;
        if (definition->m_extension) {
            type->byteSize = 0;
            type->m_tagName.clear();
            type->m_parentFieldCount = static_cast<int>(parent->fields.size());
            layoutRecord(decl, definition, type, scope);
        }
        inheritPrimitives(type, parent, scope, decl->location);
        if (type->m_tagged) {
            // A private view can declare operations before its parent is known.
            // Complete inherited slots first, then append its own operations.
            for (Symbol* primitive : type->m_primitives) {
                if (primitive->kind == SymbolKind::Subprogram && primitive->m_dispatchSlot < 0) {
                    primitive->m_controllingType = type;
                    primitive->m_dispatchSlot = static_cast<int>(type->m_dispatchSlots.size());
                    type->m_dispatchSlots.push_back(primitive);
                }
            }
        }

        for (std::size_t i = 0; i < parent->literals.size(); ++i) {
            Symbol* literal = m_symbolTable.createSymbol(SymbolKind::EnumerationLiteral, parent->literals[i],
                                                         parent->literals[i]);
            literal->location = definition->location;
            literal->type = type;
            literal->enumerationValue = static_cast<long long>(i);
            if (scope == m_packageSpecScope && !m_inVisiblePart && !m_packages.empty()) {
                literal->m_privatePrimitiveTo = m_packages.back();
            }
            scope->add(literal);
            type->m_primitives.push_back(literal);
        }
        break;
    }
    case TypeDefKind::Access: {
        type = makeType(TypeKind::Access);
        if (definition->m_accessProfile != nullptr) {
            SubprogramSpec& spec = *definition->m_accessProfile;
            Symbol* profile = m_symbolTable.createSymbol(SymbolKind::Subprogram, "", decl->name);
            profile->returnType = spec.isFunction ? resolveSubtypeIndication(spec.returnType.get(), scope) : nullptr;
            profile->location = decl->location;
            profile->m_pendingControlledResult = profile->returnType != nullptr
                && rootType(profile->returnType)->isIncomplete;
            for (ParameterDecl& declaration : spec.parameters) {
                Symbol* parameter = m_symbolTable.createSymbol(SymbolKind::Parameter, declaration.lower, declaration.name);
                parameter->type = resolveSubtypeIndication(declaration.subtype.get(), scope);
                parameter->mode = declaration.mode;
                parameter->byReference = declaration.mode != ParameterMode::In || isComposite(parameter->type);
                if (declaration.defaultValue != nullptr) {
                    m_diagnostics.error(declaration.location, "access-to-subprogram parameters cannot have defaults");
                }
                if (spec.isFunction && declaration.mode != ParameterMode::In) {
                    m_diagnostics.error(declaration.location, "access-to-function parameters must have mode in");
                }
                if (std::any_of(profile->parameters.begin(), profile->parameters.end(), [&](Symbol* other) {
                        return other->name == parameter->name;
                    })) {
                    m_diagnostics.error(declaration.location, "duplicate access-to-subprogram parameter");
                }
                profile->parameters.push_back(parameter);
            }
            type->m_accessProfile = profile;
            type->m_accessScope = scope;
        } else {
            type->target = resolveSubtypeIndication(definition->parent.get(), scope);
        }
        break;
    }
    case TypeDefKind::Private: {
        if (m_packages.empty()) {
            m_diagnostics.error(decl->location, "a private type belongs in a package specification");
            return;
        }
        type = makeType(TypeKind::Record);
        type->isIncomplete = true;
        type->m_tagged = definition->m_tagged;
        type->m_privateViewTagged = definition->m_tagged;
        type->privateTo = m_packages.back();
        type->isLimited = definition->isLimited;
        break;
    }
    }

    if (type == nullptr) {
        return;
    }
    if (hasProtected(type) && (nonlimitedPrivateCompletion || type->m_tagged || type->variantOn >= 0)) {
        m_diagnostics.error(decl->location, "protected components require an ordinary limited full view without variants");
    }
    if (hasTasks(type) && nonlimitedPrivateCompletion) {
        m_diagnostics.error(decl->location, "a completion containing tasks requires a limited private view");
    }
    if (hasTasks(type) && type->m_tagged) {
        m_diagnostics.error(decl->location, "tagged records containing tasks are not yet supported");
    }
    if (type->variantOn >= 0 && hasTasks(type)) {
        m_diagnostics.error(decl->location, "variant records containing tasks are not yet supported");
    }
    if (type->variantOn >= 0 && needsFinalization(type)) {
        m_diagnostics.error(decl->location, "variant records with controlled components are not yet supported");
    }
    type->m_abstract = definition->m_abstract;
    type->isLimited = type->isLimited || definition->isLimited || hasLimitedControlledParts(type) || hasTasks(type) || hasProtected(type);
    if (type->m_abstract && !type->m_tagged) {
        m_diagnostics.error(decl->location, "an abstract type must be tagged");
    }
    if (m_namePrefix.size() == 2 && m_namePrefix[0] == "ada" && m_namePrefix[1] == "finalization"
        && (decl->lower == "controlled" || decl->lower == "limited_controlled")) {
        type->m_controlled = true;
    }
    if (definition->kind == TypeDefKind::Record && definition->isLimited && !needsFinalization(type) && !hasTasks(type)) {
        m_diagnostics.error(decl->location, "limited record definitions outside Ada.Finalization are not yet supported");
    }
    if (type->m_tagged && type->m_tagName.empty()) {
        std::string name = "$" + mangle(decl->lower) + "__tag";
        std::size_t ordinal = ++m_tagNames[name];
        type->m_tagName = name + (ordinal > 1 ? "__" + std::to_string(ordinal) : "");
    }
    type->m_declarationScope = scope;
    if (type->kind == TypeKind::Access && type->m_accessProfile == nullptr) {
        type->m_collectionOwner = m_currentSubprogram;
        std::string name = "$" + mangle(decl->lower) + "__collection";
        std::size_t ordinal = ++m_tagNames[name];
        type->m_collectionName = name + (ordinal > 1 ? "__" + std::to_string(ordinal) : "");
        type->m_collectionOffset = -1;
        if (m_currentSubprogram != nullptr && type->target != nullptr
            && (needsCollection(type) || type->target->isIncomplete)) {
            m_currentSubprogram->needsFrame = true;
        }
    }
    if (type->m_tagged) {
        type->m_tagOwner = m_currentSubprogram;
        type->m_tagOffset = -1;
        if (m_currentSubprogram != nullptr) {
            m_currentSubprogram->needsFrame = true;
        }
    }
    if (type->kind == TypeKind::Access || type->m_tagged) {
        type->m_accessLevel = type->kind == TypeKind::Access && type->m_parentType != nullptr
            ? type->m_parentType->m_accessLevel : m_accessibilityLevel;
    }
    if (type->m_classWide != nullptr) {
        Type* wide = type->m_classWide;
        *wide = *type;
        wide->base = nullptr;
        wide->name = type->name + "'Class";
        wide->m_classRoot = type;
        wide->m_primitives.clear();
        wide->m_declarationScope = nullptr;
        m_types.refreshRecordSubtypes(wide);
    }
    if (completing != nullptr && type->kind == TypeKind::Record) {
        m_types.refreshRecordSubtypes(type);
    }
    decl->declaredType = type;
    Symbol* symbol = nullptr;
    if (completing != nullptr) {
        // Reuse the name of the incomplete declaration, including its runtime
        // bounds storage when the full declaration supplies a dynamic array.
        for (Symbol* candidate : scope->lookupLocal(decl->lower)) {
            if (candidate->kind == SymbolKind::TypeName && candidate->type == type) {
                symbol = candidate;
                break;
            }
        }
    } else {
        symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, decl->lower, decl->name);
        symbol->type = type;
        symbol->location = decl->location;
        scope->add(symbol);
    }
    if (!type->m_boundExpressions.empty() && (m_currentSubprogram != nullptr || m_recordContract != nullptr)) {
        type->m_boundsSymbol = symbol;
        symbol->owner = m_currentSubprogram;
        if (m_currentSubprogram != nullptr) {
            m_currentSubprogram->needsFrame = true;
        }
    }
}

void Sema::analyzeSubtypeDecl(SubtypeDecl* decl, Scope* scope)
{
    if (!scope->lookupLocal(decl->lower).empty()) {
        m_diagnostics.error(decl->location, "'" + decl->name + "' has already been declared in this scope");
        return;
    }

    // Generic formal objects are unknown while checking the contract. Keep
    // symbolic constraints here; instantiation resolves the actual layout.
    bool symbolicBounds = m_currentSubprogram != nullptr || m_recordContract != nullptr;
    Type* base = resolveSubtypeIndication(decl->subtype.get(), scope, symbolicBounds, symbolicBounds);
    if (base == nullptr) {
        return;
    }
    Type* type = m_types.makeSubtype(decl->name, base, base->low, base->high);
    type->m_decimalConstraint = base->m_decimalConstraint;
    if (base->m_decimalConstraint != nullptr) { type->m_scalarConstraintBase = base->m_scalarConstraintBase; }
    decl->declaredType = type;
    Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, decl->lower, decl->name);
    symbol->type = type;
    symbol->location = decl->location;
    if (decl->subtype->rangeLow != nullptr && base->m_scalarLow != nullptr) {
        type->m_scalarBoundsSymbol = symbol;
        type->m_scalarLow = base->m_scalarLow;
        type->m_scalarHigh = base->m_scalarHigh;
        type->m_scalarConstraintBase = base->m_scalarConstraintBase;
        symbol->owner = m_currentSubprogram;
        if (m_currentSubprogram != nullptr) {
            m_currentSubprogram->needsFrame = true;
        }
    }
    if (type->kind == TypeKind::Array && !type->constrained
        && !decl->subtype->indexLows.empty() && symbolicBounds) {
        type->m_boundsSymbol = symbol;
        symbol->owner = m_currentSubprogram;
        if (m_currentSubprogram != nullptr) {
            m_currentSubprogram->needsFrame = true;
        }
        for (std::size_t i = 0; i < decl->subtype->indexLows.size(); ++i) {
            type->m_boundExpressions.push_back({ decl->subtype->indexLows[i].get(),
                                                decl->subtype->indexHighs[i].get() });
        }
    }
    scope->add(symbol);
}

// Lays out a record: the discriminants first, then the components every value
// has, then the variant part.  The alternatives of a variant part all start at
// the same offset and share their storage, so the record has one size whichever
// discriminant it was made with.
void Sema::layoutRecord(TypeDecl* decl, TypeDefinition* definition, Type* type, Scope* scope)
{
    if (type->m_tagged && (!decl->discriminants.empty() || definition->variant != nullptr)) {
        m_diagnostics.error(decl->location, "discriminated tagged records are not yet supported");
        return;
    }
    long long offset = definition->m_extension ? typeSize(type->m_parentType)
        : type->m_tagged || type->m_protectedScope != nullptr ? 8 : 0;
    int fieldIndex = static_cast<int>(type->fields.size());

    auto place = [&](RecordField& field, int variantIndex, bool isDiscriminant) {
        for (const FieldInfo& seen : type->fields) {
            if (seen.name == field.lower) {
                m_diagnostics.error(field.location, "'" + field.name + "' is named twice in '" + type->name + "'");
                return;
            }
        }

        FieldInfo info;
        info.name = field.lower;
        info.displayName = field.name;
        info.type = resolveSubtypeIndication(field.subtype.get(), scope, m_recordContract != nullptr);
        type->m_incompleteLayout = type->m_incompleteLayout || hasIncompleteParts(info.type);
        if (info.type != nullptr && info.type->m_abstract) {
            m_diagnostics.error(field.location, "abstract record components are not supported");
            return;
        }
        if (info.type != nullptr && info.type->m_classRoot != nullptr) {
            m_diagnostics.error(field.location, "a record component requires a definite subtype");
            return;
        }
        bool symbolicComponent = m_recordContract != nullptr && info.type != nullptr
            && (!field.subtype->indexLows.empty() || info.type->m_boundsSymbol != nullptr);
        if (info.type != nullptr && info.type->kind == TypeKind::Array && !info.type->constrained
            && !symbolicComponent) {
            m_diagnostics.error(field.subtype->location, info.type->m_boundsSymbol != nullptr
                                    ? "runtime-constrained record components are not yet supported"
                                    : "record components require a constrained array subtype");
        }
        info.index = fieldIndex++;
        info.variant = variantIndex;
        info.isDiscriminant = isDiscriminant;
        if (field.defaultValue) {
            if (hasTasks(info.type)) {
                m_diagnostics.error(field.location, "components containing tasks cannot have explicit defaults yet");
            }
            Type* valueType = analyzeExpr(field.defaultValue.get(), scope, info.type);
            if (hasLimitedControlledParts(info.type) && !buildsLimitedResult(field.defaultValue.get())) {
                m_diagnostics.error(field.location, "limited controlled components cannot be copied");
            }
            if (!typesCompatible(info.type, valueType)) {
                m_diagnostics.error(field.defaultValue->location, "the component default has an incompatible type");
            }
            adaptUniversal(field.defaultValue.get(), info.type);
            info.defaultValue = field.defaultValue.get();
        }

        long long alignment = typeAlignment(info.type);
        __int128 aligned = offset;
        if (alignment > 0 && offset % alignment != 0) { aligned += alignment - offset % alignment; }
        __int128 end = aligned + typeSize(info.type);
        long long recordAlignment = std::max(typeAlignment(type), alignment);
        __int128 padded = end;
        if (recordAlignment > 0 && padded % recordAlignment != 0) { padded += recordAlignment - padded % recordAlignment; }
        if (padded > INT64_MAX) {
            m_diagnostics.error(field.location, "record layout exceeds signed 64-bit storage capacity");
            info.offset = 0;
            offset = 0;
        } else {
            info.offset = (long long)aligned;
            offset = (long long)end;
        }
        type->fields.push_back(info);
    };

    for (RecordField& discriminant : decl->discriminants) {
        // A discriminant is fixed when the object is declared and never changes,
        // so it has no value of its own to fall back on.
        if (discriminant.defaultValue) {
            m_diagnostics.error(discriminant.location,
                                "a discriminant is fixed when the object is declared, so it takes no default");
            discriminant.defaultValue.reset();
        }
        place(discriminant, -1, true);
        Type* discriminantType = type->fields.back().type;
        if (!isDiscrete(discriminantType)) {
            m_diagnostics.error(discriminant.location, "a discriminant has to be discrete, and '"
                                                           + (discriminantType != nullptr ? discriminantType->name
                                                                                          : std::string("it"))
                                                           + "' is not");
        }
    }
    if (!definition->m_extension) {
        type->discriminantCount = static_cast<int>(type->fields.size());
    }

    for (RecordField& field : definition->fields) {
        place(field, -1, false);
    }

    if (definition->variant == nullptr) {
        return;
    }

    std::function<void(VariantPart&, int)> layoutVariant = [&](VariantPart& part, int parent) {
        int selectorIndex = -1;
        for (const FieldInfo& field : type->fields) {
            if (field.isDiscriminant && field.name == part.discriminantLower) {
                selectorIndex = field.index;
                part.discriminantIndex = field.index;
            }
        }
        if (selectorIndex < 0) {
            m_diagnostics.error(part.location,
                                "'" + part.discriminant + "' is not a discriminant of '" + type->name + "'");
            return;
        }

        if (parent < 0) {
            type->variantOn = selectorIndex;
        }

        // Every alternative starts where the components common to all of them left
        // off: only one alternative exists in any one value, so they share the
        // storage and the record keeps a single size.
        Type* selector = type->fields[selectorIndex].type;
        long long base = offset;
        std::vector<CaseChoice> covered;
        bool sawOthers = false;

        for (std::size_t v = 0; v < part.variants.size(); ++v) {
            RecordVariant& variant = part.variants[v];
            if (sawOthers) {
                m_diagnostics.error(variant.location, "'others' has to be the last alternative of a variant part");
            }

            VariantInfo info;
            info.parent = parent;
            info.discriminant = selectorIndex;
            info.isOthers = variant.isOthers;
            sawOthers = sawOthers || variant.isOthers;

            for (std::size_t k = 0; k < variant.choiceLows.size(); ++k) {
                CaseChoice choice;
                if (!resolveChoice(variant.choiceLows[k].get(), variant.choiceHighs[k].get(), selector, covered, scope,
                                   choice)) {
                    continue;
                }
                covered.push_back(choice);
                variant.choices.push_back(choice);
                if (choice.low < LLONG_MIN || choice.high > LLONG_MAX) {
                    m_diagnostics.error(definition->location, "variant choices require signed 64-bit values");
                }
                info.choices.push_back(VariantChoice { static_cast<long long>(choice.low), static_cast<long long>(choice.high) });
            }
            int index = static_cast<int>(type->variants.size());
            type->variants.push_back(info);

            offset = base;
            for (RecordField& field : variant.fields) {
                place(field, index, false);
            }
            if (variant.nested != nullptr) {
                layoutVariant(*variant.nested, index);
            }
        }

        if (!sawOthers) {
            reportUncovered(covered, selector, part.location, "variant part");
        }
    };
    layoutVariant(*definition->variant, -1);
}

// A type may be named before it is described, but not left that way: an object
// of it could be declared with nothing to say how wide it is.  The visible part
// of a package is looked at only once its private part has had its say.
void Sema::reportIncompleteTypes(DeclList& declarations)
{
    for (const DeclPtr& decl : declarations) {
        if (decl->kind != DeclKind::Type) {
            continue;
        }
        auto* typeDecl = static_cast<TypeDecl*>(decl.get());
        Type* type = typeDecl->declaredType;
        if (type != nullptr && type->m_tagged && type->m_parentType != nullptr
            && type->fields.size() > static_cast<std::size_t>(type->m_parentFieldCount)) {
            for (Symbol* primitive : type->m_primitives) {
                if (primitive->m_inheritedFrom != nullptr && rootType(primitive->returnType) == type) {
                    m_diagnostics.error(decl->location, "inherited function '" + primitive->displayName
                        + "' returning the extended type must be overridden");
                }
            }
        }
        if (typeDecl->declaredType != nullptr && typeDecl->declaredType->isIncomplete) {
            m_diagnostics.error(decl->location, "'" + typeDecl->name + "' is never described");
        }
    }
}

// A size clause settles how wide a type is laid out, which is the only way to
// say that a stream element occupies one byte rather than the four an integer
// type would otherwise take.
void Sema::analyzeRepresentation(RepresentationDecl* decl, Scope* scope, Type* precedingType)
{
    Symbol* symbol = lookupName(decl->lower, scope);
    auto localSymbols = scope->lookupLocal(decl->lower);
    if (symbol == nullptr || symbol->kind != SymbolKind::TypeName || symbol->type == nullptr
        || std::find(localSymbols.begin(), localSymbols.end(), symbol) == localSymbols.end()) {
        m_diagnostics.error(decl->location, "'" + decl->name + "' is not a type declared here");
        return;
    }
    if (decl->attribute == "enumeration") {
        analyzeEnumerationRepresentation(decl, scope, symbol->type, precedingType);
        return;
    }
    if (decl->attribute == "alignment") {
        Type* type = symbol->type;
        if (type->isSubtype || type->isIncomplete || !representationVisible(type)
            || type->m_tagged || hasTasks(type) || hasProtected(type)) {
            m_diagnostics.error(decl->location, "an Alignment clause requires a complete, visible, untagged first subtype without task or protected parts");
            return;
        }
        if (type->m_alignmentSpecified) {
            m_diagnostics.error(decl->location, "duplicate Alignment clause");
            return;
        }
        if (type != precedingType || type->m_scalarBase != nullptr) {
            m_diagnostics.error(decl->location, "an Alignment clause must be adjacent to its type declaration, before representation use");
            return;
        }
        Type* valueType = analyzeExpr(decl->value.get(), scope, nullptr);
        long long alignment = 0;
        if (valueType == nullptr || (valueType->kind != TypeKind::Integer && valueType->kind != TypeKind::UniversalInteger)
            || !foldStatic(decl->value.get(), alignment)) {
            m_diagnostics.error(decl->location, "an Alignment clause requires a static integer");
            return;
        }
        if (alignment < 0 || alignment > 8 || (alignment != 0 && (alignment & (alignment - 1)) != 0)) {
            m_diagnostics.error(decl->location, "supported Alignment values are 0, 1, 2, 4 and 8 storage elements");
            return;
        }
        if (alignment != 0 && alignment < typeAlignment(type)) {
            m_diagnostics.error(decl->location, "Alignment cannot be smaller than the natural alignment");
            return;
        }
        // Scalar widths also select load/store instructions. Array strides use
        // the element's size, so do not promise alignment requiring extra stride
        // padding. Records already round their size to their alignment.
        if (alignment != 0 && type->kind != TypeKind::Record
            && (type->kind == TypeKind::Array && !type->constrained
                ? typeSize(type->element) % alignment != 0 : typeSize(type) % alignment != 0)) {
            m_diagnostics.error(decl->location, "Alignment requiring scalar or array stride padding is not yet supported");
            return;
        }
        if (alignment > 0 && type->kind == TypeKind::Record && type->m_specifiedSize >= 0
            && typeSize(type) % alignment != 0) {
            m_diagnostics.error(decl->location, "Alignment conflicts with the preceding Size clause");
            return;
        }
        type->m_alignment = alignment;
        type->m_alignmentSpecified = true;
        m_types.refreshStorageRepresentation(type);
        if (type->kind == TypeKind::Enumeration) { m_types.refreshEnumerationRepresentation(type); }
        return;
    }
    if (decl->attribute == "machine_radix") {
        Type* type = symbol->type;
        if (type->kind != TypeKind::Fixed || !fixedIsDecimal(type->m_fixedScale)
            || type->isSubtype || type->m_parentType != nullptr || type != precedingType || type->m_scalarBase != nullptr
            || type->m_fixedRadixSpecified || !representationVisible(type)) {
            m_diagnostics.error(decl->location, "Machine_Radix requires an adjacent decimal fixed-point first subtype");
            return;
        }
        analyzeExpr(decl->value.get(), scope, m_types.integerType());
        long long radix = 0;
        if (!foldStatic(decl->value.get(), radix) || (radix != 2 && radix != 10)) {
            m_diagnostics.error(decl->location, "decimal Machine_Radix must be the static value 2 or 10");
            return;
        }
        type->m_fixedRadix = static_cast<int>(radix);
        type->m_fixedRadixSpecified = true;
        return;
    }
    if (decl->attribute == "small") {
        Type* type = symbol->type;
        if (type->kind != TypeKind::Fixed || fixedIsDecimal(type->m_fixedScale) || type->isSubtype || type->m_parentType != nullptr
            || !representationVisible(type)) {
            m_diagnostics.error(decl->location, "a Small clause requires a nonderived ordinary fixed-point first subtype");
        } else if (type->m_smallClause != decl->value.get()) {
            m_diagnostics.error(decl->location, "a Small clause must occur once, adjacent to its type declaration before representation use");
        }
        return;
    }
    if (decl->attribute != "size") {
        m_diagnostics.error(decl->location, "only a 'Size clause is understood");
        return;
    }

    Type* type = symbol->type;
    if (type->m_tagged) {
        m_diagnostics.error(decl->location, "size clauses for tagged types are not yet supported");
        return;
    }
    if (type->isIncomplete || !representationVisible(type)) {
        m_diagnostics.error(decl->location, "a size clause requires a complete, visible representation");
        return;
    }
    Type* valueType = analyzeExpr(decl->value.get(), scope, nullptr);
    if (valueType == nullptr) {
        return;
    }
    if (valueType->kind != TypeKind::Integer && valueType->kind != TypeKind::UniversalInteger) {
        m_diagnostics.error(decl->location, "a size clause needs an integer number of bits");
        return;
    }
    long long bits = 0;
    if (!foldStatic(decl->value.get(), bits)) {
        m_diagnostics.error(decl->location, "a size clause needs a static number of bits");
        return;
    }
    if (bits <= 0 || bits % 8 != 0) {
        m_diagnostics.error(decl->location, "a size clause requires a positive multiple of 8 bits");
        return;
    }

    long long bytes = bits / 8;
    if (type->kind == TypeKind::Array || type->kind == TypeKind::Record) {
        if (type->m_boundsSymbol != nullptr || (type->kind == TypeKind::Array && !type->constrained)
            || type->m_incompleteLayout || hasIncompleteParts(type)) {
            m_diagnostics.error(decl->location, "a composite Size clause requires a complete static layout");
            return;
        }
        if (bytes < typeLayoutSize(type) || bytes % typeAlignment(type) != 0) {
            m_diagnostics.error(decl->location, "composite Size must cover its components and be a multiple of its alignment");
            return;
        }
        if (bytes != typeSize(type)) {
            if (type != precedingType) {
                m_diagnostics.error(decl->location, "a size-changing clause must be adjacent to its type or subtype declaration, before representation use");
                return;
            }
            if (needsFinalization(type) || hasTasks(type) || hasProtected(type) || type->isLimited) {
                m_diagnostics.error(decl->location, "size-changing clauses for controlled, limited, task or protected composites are not yet supported");
                return;
            }
            type->m_compositeSize = bytes;
            type->m_sizeBounds.clear();
            if (type->kind == TypeKind::Array) {
                Type* axis = type;
                for (int i = 0; i < type->arrayRank; ++i) {
                    type->m_sizeBounds.push_back({axis->indexLow, axis->indexHigh});
                    axis = axis->element;
                }
            }
        }
        type->m_specifiedSize = bytes;
        m_types.refreshStorageRepresentation(type);
        return;
    }
    if (bytes != typeSize(type) && type->isSubtype
        && (type->m_scalarBoundsSymbol != nullptr || type->low > type->high || type->isIncomplete)) {
        m_diagnostics.error(decl->location, "size-changing scalar subtypes require a static nonnull range");
        return;
    }
    if (bits != 8 && bits != 16 && bits != 32 && bits != 64) {
        m_diagnostics.error(decl->location, "scalar size clauses support only 8, 16, 32 or 64 bits");
        return;
    }
    switch (type->kind) {
    case TypeKind::Integer:
    case TypeKind::Enumeration: {
        // Byte and halfword loads can be unsigned. Word arithmetic and loads
        // are signed; types needing the full unsigned word range use 64 bits.
        long long low = 0;
        __int128 high = type->m_modulus != 0 && bits == 64
            ? (__int128)UINT64_MAX : std::numeric_limits<long long>::max();
        if (bits == 64) {
            low = std::numeric_limits<long long>::min();
        } else if (bits == 32 || scalarStorageSigned(type)) {
            low = -(1LL << (bits - 1));
            high = (1LL << (bits - 1)) - 1;
        } else {
            high = (1LL << bits) - 1;
        }
        if (!type->isSubtype && type->m_modulus != 0 && type->m_modulus - 1 > high) {
            m_diagnostics.error(decl->location, "unsupported representation size for this modular type");
            return;
        }
        if (type->kind == TypeKind::Enumeration && bits == 64) {
            m_diagnostics.error(decl->location, "enumeration storage supports only 8, 16 or 32 bits");
            return;
        }
        __int128 storageLow = type->low;
        __int128 storageHigh = type->high;
        if (!type->m_enumCodes.empty() && type->low >= 0 && type->high >= type->low
            && type->high < static_cast<long long>(type->m_enumCodes.size())) {
            storageLow = type->m_enumCodes[static_cast<std::size_t>(type->low)];
            storageHigh = type->m_enumCodes[static_cast<std::size_t>(type->high)];
        }
        if (storageLow < low || storageHigh > high) {
            m_diagnostics.error(decl->location, "the range of '" + decl->name
                + "' does not fit in " + std::to_string(bits) + " bits");
            return;
        }
        break;
    }
    case TypeKind::Float:
        if (bits != (type->digits > 6 ? 64 : 32)) {
            m_diagnostics.error(decl->location,
                "floating-point size must match the representation selected by digits");
            return;
        }
        break;
    case TypeKind::Fixed:
        if (bits != 64) {
            m_diagnostics.error(decl->location, "fixed-point storage currently requires 64 bits");
            return;
        }
        break;
    case TypeKind::Access:
        if (bits != 64) {
            m_diagnostics.error(decl->location, "access storage currently requires 64 bits");
            return;
        }
        break;
    default:
        m_diagnostics.error(decl->location, "size clauses for this type are not yet supported");
        return;
    }
    if (bytes == typeSize(type)) {
        type->m_specifiedSize = bytes;
        return;
    }
    // Subtypes, field offsets and folded attributes can already contain the
    // old width. Until full freezing is implemented, change storage only at
    // the declaration, before another declaration can depend on its layout.
    if (type != precedingType || (!type->isSubtype && type->m_scalarBase != nullptr)) {
        m_diagnostics.error(decl->location,
            "a size-changing clause must be adjacent to its type or subtype declaration, before representation use");
        return;
    }
    if (type->m_alignment > bytes) {
        m_diagnostics.error(decl->location, "Size cannot be smaller than the specified Alignment");
        return;
    }
    type->byteSize = bytes;
    type->m_specifiedSize = bytes;
    m_types.refreshStorageRepresentation(type);
    if (type->kind == TypeKind::Enumeration && !type->isSubtype) { m_types.refreshEnumerationRepresentation(type); }
}

Type* Sema::resolveSubtypeIndication(SubtypeIndication* indication, Scope* scope, bool allowDynamic, bool allowDynamicScalar)
{
    if (indication == nullptr) {
        return nullptr;
    }
    Type* base = resolveTypeName(indication->lower, scope, indication->location);
    if (base == nullptr) {
        return nullptr;
    }
    indication->resolved = base;

    Type* decimalConstraint = nullptr;
    int decimalDigits = 0;
    if (indication->digits) {
        Type* digitsType = analyzeExpr(indication->digits.get(), scope, m_types.integerType());
        long long digits = 0;
        bool decimal = base->kind == TypeKind::Fixed && fixedIsDecimal(base->m_fixedScale);
        if (!isReal(base) && !decimal) {
            m_diagnostics.error(indication->location, "a digits constraint requires a decimal fixed-point or floating point type");
        } else if (digitsType == nullptr || (digitsType->kind != TypeKind::Integer && digitsType->kind != TypeKind::UniversalInteger)
            || !foldStatic(indication->digits.get(), digits)) {
            m_diagnostics.error(indication->location, "an accuracy constraint must be static");
        } else if (digits < 1 || digits > base->digits) {
            m_diagnostics.error(indication->location,
                                "a subtype cannot ask for more digits than " + base->name + " provides");
        } else if (decimal) {
            decimalDigits = static_cast<int>(digits);
            long long limit = static_cast<long long>(fixedPower10(decimalDigits) - 1);
            decimalConstraint = m_types.makeSubtype(anonymousTypeName(), base, -limit, limit);
            decimalConstraint->m_scalarBoundsSymbol = nullptr;
            decimalConstraint->digits = decimalDigits;
            if (!indication->rangeLow) {
                if (!allowDynamicScalar && (base->m_scalarBoundsSymbol != nullptr
                    || -(__int128)limit < base->low || (__int128)limit > base->high)) {
                    m_diagnostics.error(indication->location, "decimal digits constraints requiring an elaboration check need a local subtype declaration");
                }
                Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, -limit, limit);
                subtype->m_scalarBoundsSymbol = nullptr;
                subtype->digits = decimalDigits;
                subtype->m_scalarConstraintBase = base;
                subtype->m_decimalConstraint = decimalConstraint;
                indication->resolved = subtype;
                return subtype;
            }
        }
    }

    if (indication->rangeLow && indication->rangeHigh && isReal(base)) {
        analyzeExpr(indication->rangeLow.get(), scope, base);
        analyzeExpr(indication->rangeHigh.get(), scope, base);
        double low = 0.0;
        double high = 0.0;
        if (!foldStaticReal(indication->rangeLow.get(), low)
            || !foldStaticReal(indication->rangeHigh.get(), high)) {
            m_diagnostics.error(indication->location, "range constraints must be static");
            return base;
        }
        Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
        subtype->hasRealRange = true;
        subtype->lowReal = low;
        subtype->highReal = high;
        indication->resolved = subtype;
        return subtype;
    }

    if (base->kind == TypeKind::Fixed && indication->rangeLow && indication->rangeHigh) {
        Type* lowType = analyzeExpr(indication->rangeLow.get(), scope, base);
        Type* highType = analyzeExpr(indication->rangeHigh.get(), scope, base);
        if (base->m_formalFixed && typesCompatible(base, lowType) && typesCompatible(base, highType)) {
            // The instance rechecks these constraints with its actual scale.
            indication->resolved = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
            return indication->resolved;
        }
        long long low = 0, high = 0;
        if (!typesCompatible(base, lowType) || !typesCompatible(base, highType)) {
            m_diagnostics.error(indication->location, "fixed-point subtype bounds must have the subtype's type");
            return base;
        }
        bool staticLow = foldFixed(indication->rangeLow.get(), low);
        bool staticHigh = foldFixed(indication->rangeHigh.get(), high);
        if (!staticLow || !staticHigh || base->m_scalarBoundsSymbol != nullptr) {
            if (!allowDynamicScalar) {
                m_diagnostics.error(indication->location, "runtime fixed-point constraints require a local subtype declaration");
                return base;
            }
            Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
            if (decimalConstraint != nullptr) {
                subtype->digits = decimalDigits;
                subtype->m_decimalConstraint = decimalConstraint;
            }
            subtype->m_scalarLow = indication->rangeLow.get();
            subtype->m_scalarHigh = indication->rangeHigh.get();
            subtype->m_scalarConstraintBase = base;
            indication->resolved = subtype;
            return subtype;
        }
        Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, low, high);
        if (decimalConstraint != nullptr) {
            if (!allowDynamicScalar && (low < decimalConstraint->low || low > decimalConstraint->high
                || high < decimalConstraint->low || high > decimalConstraint->high
                || (low <= high && (low < base->low || high > base->high)))) {
                m_diagnostics.error(indication->location, "decimal digits constraints requiring an elaboration check need a local subtype declaration");
            }
            subtype->digits = decimalDigits;
            subtype->m_decimalConstraint = decimalConstraint;
            subtype->m_scalarConstraintBase = base;
        }
        indication->resolved = subtype;
        return subtype;
    }

    if (indication->rangeLow && indication->rangeHigh) {
        Type* lowType = analyzeExpr(indication->rangeLow.get(), scope, base);
        Type* highType = analyzeExpr(indication->rangeHigh.get(), scope, base);
        if (!isDiscrete(base) || !typesCompatible(base, lowType) || !typesCompatible(base, highType)) {
            m_diagnostics.error(indication->location, "range bounds must have the subtype's discrete type");
            return base;
        }
        __int128 low = 0, high = 0;
        StaticFailure failure = StaticFailure::None;
        bool staticLow = foldWideInteger(indication->rangeLow.get(), low, failure);
        bool staticHigh = foldWideInteger(indication->rangeHigh.get(), high, failure);
        if (!staticLow || !staticHigh || base->m_scalarBoundsSymbol != nullptr) {
            if (!allowDynamicScalar) {
                m_diagnostics.error(indication->location, "runtime scalar constraints require a local subtype declaration");
                return base;
            }
            Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
            subtype->m_scalarLow = indication->rangeLow.get();
            subtype->m_scalarHigh = indication->rangeHigh.get();
            subtype->m_scalarConstraintBase = base;
            indication->resolved = subtype;
            return subtype;
        }
        Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, low, high);
        indication->resolved = subtype;
        return subtype;
    }

    if (!indication->indexLows.empty()) {
        Type* array = baseType(base);
        if (array != nullptr && array->kind == TypeKind::Record) {
            return constrainDiscriminants(indication, base, scope);
        }
        if (array == nullptr || array->kind != TypeKind::Array) {
            m_diagnostics.error(indication->location, "index constraints require an array type");
            return base;
        }
        if (indication->indexLows.size() != static_cast<std::size_t>(array->arrayRank)) {
            m_diagnostics.error(indication->location, "index constraint count must match the array rank");
            return base;
        }
        if (base->m_boundsSymbol != nullptr) {
            m_diagnostics.error(indication->location, "an index constraint requires an unconstrained array subtype");
            return base;
        }
        if (array->arrayRank > 1) {
            if (base->constrained) {
                m_diagnostics.error(indication->location, "an index constraint requires an unconstrained array subtype");
                return base;
            }
            Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
            Type* row = subtype;
            Type* original = base;
            bool allStatic = true;
            std::vector<Type*> rows;
            for (int dimension = 0; dimension < array->arrayRank; ++dimension) {
                Expr* low = indication->indexLows[dimension].get();
                Expr* high = indication->indexHighs[dimension].get();
                if (high == nullptr) {
                    m_diagnostics.error(indication->location, "array index constraints require ranges");
                    return base;
                }
                Type* lowType = analyzeExpr(low, scope, original->index);
                Type* highType = analyzeExpr(high, scope, original->index);
                if (!typesCompatible(original->index, lowType) || !typesCompatible(original->index, highType)) {
                    m_diagnostics.error(indication->location, "array index bounds have an incompatible type");
                }
                bool staticLow = foldStatic(low, row->indexLow);
                bool staticHigh = foldStatic(high, row->indexHigh);
                allStatic = allStatic && staticLow && staticHigh && original->index->m_scalarBoundsSymbol == nullptr;
                rows.push_back(row);
                if (dimension + 1 < array->arrayRank) {
                    original = original->element;
                    row->element = m_types.makeSubtype(anonymousTypeName(), original, original->low, original->high);
                    row = row->element;
                }
            }
            if (!allStatic && !allowDynamic) {
                m_diagnostics.error(indication->location, "index constraints must be static");
                return base;
            }
            for (Type* axis : rows) { axis->constrained = allStatic; }
            if (allStatic) { checkArrayStorage(subtype, m_diagnostics, indication->location); }
            indication->resolved = subtype;
            return subtype;
        }
        Type* lowType = analyzeExpr(indication->indexLows.front().get(), scope, array->index);
        if (!typesCompatible(array->index, lowType)) {
            m_diagnostics.error(indication->location, "array index bounds have an incompatible type");
        }
        long long low = 0;
        long long high = 0;
        bool ok = foldStatic(indication->indexLows.front().get(), low);
        if (indication->indexHighs.front()) {
            Type* highType = analyzeExpr(indication->indexHighs.front().get(), scope, array->index);
            if (!typesCompatible(array->index, highType)) {
                m_diagnostics.error(indication->location, "array index bounds have an incompatible type");
            }
            ok = ok && foldStatic(indication->indexHighs.front().get(), high);
        }
        ok = ok && array->index->m_scalarBoundsSymbol == nullptr;
        if (!ok && allowDynamic && indication->indexHighs.front() != nullptr) {
            if (!isDiscrete(indication->indexLows.front()->type)
                || !isDiscrete(indication->indexHighs.front()->type)) {
                m_diagnostics.error(indication->location, "array index bounds must be discrete");
                return base;
            }
            Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
            subtype->constrained = false;
            indication->resolved = subtype;
            return subtype;
        }
        if (!ok) {
            m_diagnostics.error(indication->location, "index constraints must be static");
            return base;
        }
        Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
        subtype->constrained = true;
        subtype->indexLow = low;
        subtype->indexHigh = high;
        checkArrayStorage(subtype, m_diagnostics, indication->location);
        indication->resolved = subtype;
        return subtype;
    }

    return base;
}

// 'Shape (Circle)': fixes the discriminants of a record subtype, which settles
// which variant a value of it has and so which components it carries.
Type* Sema::constrainDiscriminants(SubtypeIndication* indication, Type* base, Scope* scope)
{
    Type* record = baseType(base);
    if (record->discriminantCount == 0) {
        m_diagnostics.error(indication->location, "'" + record->name + "' has no discriminants to constrain");
        return base;
    }
    if (hasKnownDiscriminants(base)) {
        m_diagnostics.error(indication->location, "a constrained record subtype cannot be constrained again");
        return base;
    }
    if (static_cast<int>(indication->indexLows.size()) != record->discriminantCount) {
        m_diagnostics.error(indication->location, "'" + record->name + "' has "
                                                      + std::to_string(record->discriminantCount)
                                                      + " discriminants, so that many values are expected");
        return base;
    }

    Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
    subtype->name = record->name;
    subtype->discriminantsKnown = true;

    for (int i = 0; i < record->discriminantCount; ++i) {
        Expr* value = indication->indexLows[static_cast<std::size_t>(i)].get();
        if (indication->indexHighs[static_cast<std::size_t>(i)] != nullptr) {
            m_diagnostics.error(value->location, "a discriminant takes one value, not a range");
        }
        Type* discriminantType = record->fields[static_cast<std::size_t>(i)].type;
        analyzeExpr(value, scope, discriminantType);

        long long fixed = 0;
        if (!foldStatic(value, fixed)) {
            m_diagnostics.error(value->location, "a discriminant has to be fixed with a static value");
            subtype->discriminantsKnown = false;
        } else if (discriminantType != nullptr && (fixed < discriminantType->low || fixed > discriminantType->high)) {
            m_diagnostics.error(value->location, describeValue(discriminantType, fixed) + " is not a value '"
                                                     + record->fields[static_cast<std::size_t>(i)].name
                                                     + "' can take");
        }
        subtype->discriminantValues.push_back(fixed);
    }

    indication->resolved = subtype;
    return subtype;
}

// Whether a constraint somewhere along the subtype chain has fixed the
// discriminants of the record.
bool Sema::hasKnownDiscriminants(Type* type) const
{
    for (Type* walk = type; walk != nullptr; walk = walk->isSubtype ? walk->base : nullptr) {
        if (walk->m_discriminantConstraintSource != nullptr) {
            return hasKnownDiscriminants(walk->m_discriminantConstraintSource);
        }
        if (walk->discriminantsKnown) {
            return true;
        }
    }
    return false;
}

// What a fixed discriminant of the record was fixed to, if anything.
bool Sema::discriminantValue(Type* type, int index, long long& value) const
{
    return discriminantValueOf(type, index, value);
}

bool Sema::typesCompatible(Type* target, Type* source) const
{
    if (target == nullptr || source == nullptr) {
        return true;
    }
    Type* left = rootType(target);
    Type* right = rootType(source);
    if (left->m_classRoot != nullptr) {
        return isTaggedAncestor(left->m_classRoot, right->m_classRoot != nullptr ? right->m_classRoot : right);
    }
    if (left == right) {
        return true;
    }
    if (isUniversal(left) && isUniversal(right)) {
        return left->kind == right->kind;
    }
    if (isUniversal(left) || isUniversal(right)) {
        // Ada keeps the two families apart: a whole number literal never stands
        // for a real value, and the other way round.
        Type* universal = isUniversal(left) ? left : right;
        Type* concrete = isUniversal(left) ? right : left;
        if (universal->kind == TypeKind::UniversalInteger) {
            return concrete->kind == TypeKind::Integer;
        }
        return concrete->kind == TypeKind::Float || concrete->kind == TypeKind::Fixed;
    }
    // Array declarations introduce distinct types, even with identical bounds
    // and components. Subtypes already share identity through the root above.
    return false;
}

// Synthesized bounds retain a subtype reference rather than copying a dynamic
// subtype's placeholder limits into loops, membership tests, or array types.
ExprPtr Sema::scalarBoundExpr(Type* type, bool first, const SourceLocation& location)
{
    if (type->m_scalarBoundsSymbol == nullptr) {
        auto literal = std::make_unique<IntegerLiteralExpr>();
        literal->location = location;
        literal->value = first ? type->low : type->high;
        literal->type = type;
        literal->isStatic = true;
        literal->staticValue = literal->value;
        literal->m_exactReal = ExactReal::make(first ? type->low : type->high);
        return literal;
    }
    auto prefix = std::make_unique<IdentifierExpr>();
    prefix->location = location;
    prefix->type = type;
    prefix->symbol = type->m_scalarBoundsSymbol;
    auto attribute = std::make_unique<AttributeExpr>();
    attribute->location = location;
    attribute->prefix = std::move(prefix);
    attribute->prefixType = type;
    attribute->type = type;
    attribute->name = attribute->lower = first ? "first" : "last";
    return attribute;
}

// Primitive profiles belong to the declaration of a type, not to its layout.
// A subtype shares that identity; a derivation receives new callable views.
void Sema::registerPrimitive(Symbol* symbol, Scope* scope)
{
    std::vector<Type*> types;
    auto consider = [&](Type* type) {
        type = rootType(type);
        if (type != nullptr && type->m_declarationScope == scope
            && std::find(types.begin(), types.end(), type) == types.end()) {
            types.push_back(type);
        }
    };
    consider(symbol->returnType);
    for (Symbol* parameter : symbol->parameters) {
        consider(parameter->type);
    }
    if (scope == m_packageSpecScope && !m_inVisiblePart && !m_packages.empty()) {
        symbol->m_privatePrimitiveTo = m_packages.back();
    }
    for (Type* type : types) {
        if (type->m_tagged && type->m_classRoot == nullptr
            && (scope == m_packageSpecScope || symbol->m_overrides)) {
            if (scope == m_packageSpecScope && !m_inVisiblePart
                && rootType(symbol->returnType) == type && !symbol->m_overrides) {
                for (const auto& declaration : m_symbolTable.symbols()) {
                    if (declaration->kind == SymbolKind::TypeName && declaration->type == type
                        && declaration->m_privatePrimitiveTo == nullptr) {
                        m_diagnostics.error(symbol->location,
                            "a private primitive with a controlling result must override a visible operation");
                        break;
                    }
                }
            }
            if (symbol->m_controllingType != nullptr && symbol->m_controllingType != type) {
                m_diagnostics.error(symbol->location, "a dispatching operation cannot control multiple tagged types");
            }
            for (Symbol* parameter : symbol->parameters) {
                if (rootType(parameter->type) == type && parameter->hasDefault
                    && !parameter->defaultExpr->m_tagIndeterminate) {
                    m_diagnostics.error(parameter->location, "a controlling parameter default must be tag indeterminate");
                }
            }
        }
        bool replaced = false;
        for (Symbol*& primitive : type->m_primitives) {
            if (primitive->name != symbol->name || primitive->m_inheritedFrom == nullptr
                || primitive->parameters.size() != symbol->parameters.size()
                || rootType(primitive->returnType) != rootType(symbol->returnType)) {
                continue;
            }
            bool matches = true;
            for (std::size_t i = 0; i < symbol->parameters.size(); ++i) {
                matches = matches && rootType(primitive->parameters[i]->type) == rootType(symbol->parameters[i]->type);
            }
            if (matches) {
                scope->remove(primitive);
                if (type->m_tagged && primitive->m_dispatchSlot >= 0) {
                    if (type->m_dispatchFrozen) {
                        m_diagnostics.error(symbol->location, "a tagged primitive cannot be overridden after its type is frozen");
                    }
                    symbol->m_dispatchSlot = primitive->m_dispatchSlot;
                    symbol->m_controllingType = type;
                    type->m_dispatchSlots[symbol->m_dispatchSlot] = symbol;
                }
                primitive = symbol;
                replaced = true;
            }
        }
        if (!replaced && (scope == m_packageSpecScope || symbol->m_overrides)) {
            type->m_primitives.push_back(symbol);
            if (type->m_tagged && type->m_classRoot == nullptr) {
                if (type->m_dispatchFrozen) {
                    m_diagnostics.error(symbol->location, "a tagged primitive cannot be declared after its type is frozen");
                }
                symbol->m_controllingType = type;
                symbol->m_dispatchSlot = static_cast<int>(type->m_dispatchSlots.size());
                type->m_dispatchSlots.push_back(symbol);
            }
        }
    }
}

void Sema::inheritPrimitives(Type* type, Type* parent, Scope* scope, const SourceLocation& location)
{
    // Constraints in the parent's profile survive derivation. In particular,
    // deriving from a constrained subtype does not narrow every inherited formal.
    auto corresponding = [&](Type* original) -> Type* {
        if (original == nullptr || rootType(original) != rootType(parent)) {
            return original;
        }
        if (type->m_tagged) {
            // This stage supports only nondiscriminated tagged types, so there
            // are no profile constraints to retain separately from the layout.
            return type;
        }
        Type* subtype = m_types.create(original->kind, type->name);
        *subtype = *original;
        subtype->base = type;
        subtype->isSubtype = true;
        subtype->m_specifiedSize = -1;
        subtype->m_primitives.clear();
        if (original->kind == TypeKind::Record) {
            subtype->m_discriminantConstraintSource = original;
        }
        return subtype;
    };
    for (Symbol* primitive : rootType(parent)->m_primitives) {
        if (primitive->kind != SymbolKind::Subprogram
            || (primitive->m_privatePrimitiveTo != nullptr && !withinPackage(primitive->m_privatePrimitiveTo))) {
            continue;
        }
        if (type->m_tagged && (primitive->name == "=" || primitive->name == "/=")
            && m_types.isBoolean(primitive->returnType) && primitive->parameters.size() == 2
            && rootType(primitive->parameters[0]->type) == rootType(parent)
            && rootType(primitive->parameters[1]->type) == rootType(parent)) {
            continue;
        }
        Symbol* inherited = m_symbolTable.createSymbol(SymbolKind::Subprogram, primitive->name, primitive->displayName);
        inherited->location = location;
        inherited->m_inheritedFrom = primitive;
        inherited->m_dispatchSlot = primitive->m_dispatchSlot;
        inherited->m_controllingType = type;
        if (inherited->m_dispatchSlot >= 0) {
            type->m_dispatchSlots[inherited->m_dispatchSlot] = inherited;
        }
        inherited->m_declarationScope = scope;
        inherited->returnType = corresponding(primitive->returnType);
        inherited->hasBody = true;
        inherited->level = primitive->level;
        inherited->owner = primitive->owner;
        if (scope == m_packageSpecScope && !m_inVisiblePart && !m_packages.empty()) {
            inherited->m_privatePrimitiveTo = m_packages.back();
        }
        for (Symbol* parameter : primitive->parameters) {
            Symbol* copy = m_symbolTable.createSymbol(SymbolKind::Parameter, parameter->name, parameter->displayName);
            *copy = *parameter;
            copy->type = corresponding(parameter->type);
            copy->owner = inherited;
            inherited->parameters.push_back(copy);
        }
        bool overridden = false;
        for (Symbol* declared : type->m_primitives) {
            if (declared->kind != SymbolKind::Subprogram || declared->name != inherited->name
                || rootType(declared->returnType) != rootType(inherited->returnType)
                || declared->parameters.size() != inherited->parameters.size()) {
                continue;
            }
            bool matches = true;
            for (std::size_t i = 0; i < declared->parameters.size(); ++i) {
                matches = matches && rootType(declared->parameters[i]->type) == rootType(inherited->parameters[i]->type);
            }
            if (matches) {
                bool subtypeConformant = declared->returnType == inherited->returnType
                    || SemaSupport::staticallyMatches(declared->returnType, inherited->returnType);
                for (std::size_t i = 0; i < declared->parameters.size(); ++i) {
                    subtypeConformant = subtypeConformant
                        && SemaSupport::staticallyMatches(declared->parameters[i]->type, inherited->parameters[i]->type);
                    if (declared->parameters[i]->mode != inherited->parameters[i]->mode) {
                        m_diagnostics.error(declared->location, "overriding parameters must have the inherited modes");
                    }
                }
                if (type->m_tagged && !subtypeConformant) {
                    m_diagnostics.error(declared->location, "overriding dispatching profile must be subtype conformant with the inherited operation");
                }
                declared->m_overrides = true;
                if (type->m_tagged && inherited->m_dispatchSlot >= 0) {
                    declared->m_controllingType = type;
                    declared->m_dispatchSlot = inherited->m_dispatchSlot;
                    type->m_dispatchSlots[declared->m_dispatchSlot] = declared;
                }
                overridden = true;
            }
        }
        if (!overridden) {
            type->m_primitives.push_back(inherited);
            scope->add(inherited);
        }
    }
}

void Sema::analyzeEnumerationRepresentation(RepresentationDecl* decl, Scope* scope, Type* type, Type* precedingType)
{
    if (type->kind != TypeKind::Enumeration || type->isSubtype || type->isIncomplete
        || !representationVisible(type) || m_types.isBoolean(type)) {
        m_diagnostics.error(decl->location, "an enumeration representation clause requires a complete, visible, nonboolean first subtype");
        return;
    }
    if (m_types.isCharacter(type)) {
        m_diagnostics.error(decl->location, "representation clauses for Character and its descendants are not yet supported");
        return;
    }
    if (type->literals.empty()) {
        m_diagnostics.error(decl->location, "enumeration representation requires a statically known literal list");
        return;
    }
    for (const auto& prior : m_symbolTable.symbols()) {
        if (prior->kind == SymbolKind::TypeName && prior->type != nullptr && prior->type->isSubtype
            && rootType(prior->type) == type && prior->type->kind != TypeKind::Enumeration) {
            m_diagnostics.error(decl->location, "enumeration representation after a subtype of a private view is not yet supported");
            return;
        }
    }
    if (type->m_enumCodesSpecified) {
        m_diagnostics.error(decl->location, "duplicate enumeration representation clause");
        return;
    }
    if (type != precedingType || type->m_scalarBase != nullptr) {
        m_diagnostics.error(decl->location, "an enumeration representation clause must be adjacent to its type declaration, before representation use");
        return;
    }
    if (decl->value == nullptr || decl->value->kind != ExprKind::Aggregate) {
        m_diagnostics.error(decl->location, "an enumeration representation clause requires a one-dimensional array aggregate");
        return;
    }
    auto* aggregate = static_cast<AggregateExpr*>(decl->value.get());
    if (aggregate->m_ancestor != nullptr || aggregate->m_nullRecord) {
        m_diagnostics.error(decl->location, "an enumeration representation clause requires a one-dimensional array aggregate");
        return;
    }
    std::vector<long long> codes(type->literals.size());
    std::vector<bool> assigned(codes.size(), false);
    std::vector<CaseChoice> covered;
    Type* index = m_types.makeSubtype(type->name, type, 0, (long long)codes.size() - 1);
    bool valid = true, named = false, positional = false, others = false;
    std::size_t next = 0;
    auto assign = [&](long long low, long long high, long long code) {
        for (long long i = low; i <= high; ++i) {
            if (assigned[i]) {
                m_diagnostics.error(decl->location, "enumeration representation choices overlap");
                valid = false;
                return;
            }
            assigned[i] = true;
            codes[i] = code;
        }
    };
    for (std::size_t c = 0; c < aggregate->components.size(); ++c) {
        AggregateComponent& component = aggregate->components[c];
        Type* valueType = analyzeExpr(component.value.get(), scope, nullptr);
        long long code = 0;
        if (valueType == nullptr || (valueType->kind != TypeKind::Integer && valueType->kind != TypeKind::UniversalInteger)
            || !foldStatic(component.value.get(), code)) {
            m_diagnostics.error(component.value->location, "enumeration representation codes must be static integers");
            valid = false;
        } else if (code < INT32_MIN || code > INT32_MAX) {
            m_diagnostics.error(component.value->location, "enumeration representation codes must fit signed 32 bits");
            valid = false;
        }
        if (component.isOthers) {
            if (others || c + 1 != aggregate->components.size()) {
                m_diagnostics.error(component.value->location, "others must appear once, as the final representation association");
                valid = false;
            }
            others = true;
            for (std::size_t i = 0; i < codes.size(); ++i) {
                if (!assigned[i]) { assign(i, i, code); }
            }
        } else if (component.choiceLows.empty()) {
            positional = true;
            if (next >= codes.size()) {
                m_diagnostics.error(component.value->location, "too many enumeration representation codes");
                valid = false;
            } else {
                assign(next, next, code);
                ++next;
            }
        } else {
            named = true;
            for (std::size_t i = 0; i < component.choiceLows.size(); ++i) {
                CaseChoice choice;
                if (resolveChoice(component.choiceLows[i].get(), component.choiceHighs[i].get(), index, covered, scope, choice)) {
                    Type* lowType = component.choiceLows[i]->type;
                    Type* highType = component.choiceHighs[i] ? component.choiceHighs[i]->type : lowType;
                    if (lowType == nullptr || highType == nullptr || lowType->kind != TypeKind::Enumeration
                        || highType->kind != TypeKind::Enumeration || rootType(lowType) != rootType(type)
                        || rootType(highType) != rootType(type)) {
                        m_diagnostics.error(component.choiceLows[i]->location, "enumeration representation choices must have the enumeration type");
                        valid = false;
                    } else {
                        covered.push_back(choice);
                        assign((long long)choice.low, (long long)choice.high, code);
                    }
                } else { valid = false; }
            }
        }
    }
    if (named && positional) {
        m_diagnostics.error(decl->location, "positional and named enumeration representation associations cannot be mixed");
        valid = false;
    }
    if (std::find(assigned.begin(), assigned.end(), false) != assigned.end()) {
        m_diagnostics.error(decl->location, "enumeration representation clause must cover every literal");
        valid = false;
    }
    if (!valid) { return; }
    for (std::size_t i = 1; i < codes.size(); ++i) {
        if (codes[i] <= codes[i - 1]) {
            m_diagnostics.error(decl->location, "enumeration representation codes must be distinct and increasing in declaration order");
            return;
        }
    }
    long long bytes = codes.front() >= 0 && codes.back() <= 255 ? 1
        : codes.front() >= -128 && codes.back() <= 127 ? 1
        : codes.front() >= 0 && codes.back() <= 65535 ? 2
        : codes.front() >= -32768 && codes.back() <= 32767 ? 2 : 4;
    if (type->m_specifiedSize >= 0) {
        bytes = type->m_specifiedSize;
        long long low = codes.front() < 0 ? -(1LL << (bytes * 8 - 1)) : 0;
        long long high = bytes == 4 || codes.front() < 0 ? (1LL << (bytes * 8 - 1)) - 1 : (1LL << (bytes * 8)) - 1;
        if (codes.front() < low || codes.back() > high) {
            m_diagnostics.error(decl->location, "enumeration representation codes do not fit the specified Size");
            return;
        }
    }
    if (type->m_alignment > bytes) {
        m_diagnostics.error(decl->location, "enumeration storage cannot be smaller than its specified Alignment");
        return;
    }
    type->m_enumCodes = std::move(codes);
    type->m_enumCodesSpecified = true;
    type->byteSize = bytes;
    m_types.refreshEnumerationRepresentation(type);
}
