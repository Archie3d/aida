#include "Sema.h"
#include "../../common/Array.h"

#include <algorithm>
#include <functional>
#include "SemaSupport.h"
#include <cstdint>
#include <limits>
#include <filesystem>
#include <cctype>

using SemaSupport::isUniversal;
using SemaSupport::adaptUniversal;

static Type* discriminantDependency(Expr* expression)
{
    if (expression == nullptr) { return nullptr; }
    switch (expression->kind) {
    case ExprKind::Identifier:
        if (Symbol* symbol = static_cast<IdentifierExpr*>(expression)->symbol) {
            return symbol->discriminantRecord != nullptr ? symbol->discriminantRecord : symbol->currentInstance;
        }
        return nullptr;
    case ExprKind::Binary: {
        auto* binary = static_cast<BinaryExpr*>(expression);
        Type* left = discriminantDependency(binary->left.get());
        return left != nullptr ? left : discriminantDependency(binary->right.get());
    }
    case ExprKind::Unary: return discriminantDependency(static_cast<UnaryExpr*>(expression)->operand.get());
    case ExprKind::Qualified: return discriminantDependency(static_cast<QualifiedExpr*>(expression)->operand.get());
    case ExprKind::Attribute: {
        auto* attribute = static_cast<AttributeExpr*>(expression);
        Type* prefix = discriminantDependency(attribute->prefix.get());
        if (prefix != nullptr) { return prefix; }
        for (const auto& argument : attribute->arguments) {
            if (Type* found = discriminantDependency(argument.get())) { return found; }
        }
        return nullptr;
    }
    case ExprKind::Call:
        for (const auto& argument : static_cast<CallExpr*>(expression)->arguments) {
            if (Type* found = discriminantDependency(argument.value.get())) { return found; }
        }
        return nullptr;
    default: return nullptr;
    }
}

static bool hasLanguageFinalization(const Type* type)
{
    if (type == nullptr) { return false; }
    if (type->controlled || type->protectedScope != nullptr || type->classRoot != nullptr) { return true; }
    if (type->kind == TypeKind::Array) { return hasLanguageFinalization(type->element); }
    if (type->kind == TypeKind::Record) {
        for (const FieldInfo& field : type->fields) {
            if (hasLanguageFinalization(field.type)) { return true; }
        }
    }
    return false;
}

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

    if (completing == nullptr) {
        for (Symbol* candidate : scope->lookupDeclaration(decl->lower)) {
            if (candidate->kind == SymbolKind::TypeName && candidate->type != nullptr
                && candidate->type->isIncomplete && candidate->type->incompleteInPrivatePart) {
                completing = candidate->type;
            }
        }
    }
    bool predeclared = false;
    if (completing == nullptr) {
        auto pending = m_limitedCompletions.find(decl);
        if (pending != m_limitedCompletions.end()) {
            completing = pending->second;
            predeclared = true;
        }
    }
    TypeDecl* partialDeclaration = completing != nullptr ? completing->incompleteDeclaration : nullptr;
    Type* declaredAncestorSubtype = completing != nullptr && completing->privateExtension ? completing->parentSubtype : nullptr;
    if (decl->definition == nullptr) {
        if (decl->taggedIncomplete) {
            for (const RecordField& discriminant : decl->discriminants) {
                if (discriminant.defaultValue != nullptr) {
                    m_diagnostics.error(discriminant.location, "a tagged type cannot have defaulted discriminants");
                }
            }
        }
        if (completing != nullptr && !predeclared) {
            m_diagnostics.error(decl->location, "'" + decl->name + "' has already been named here");
            return;
        }
        Type* placeholder = completing != nullptr ? completing : m_types.create(TypeKind::Record, decl->name);
        placeholder->tagged = decl->taggedIncomplete;
        placeholder->privateViewTagged = decl->taggedIncomplete;
        placeholder->isIncomplete = true;
        placeholder->incompleteInPrivatePart = scope == m_packageSpecScope && !m_inVisiblePart;
        placeholder->incompleteDeclaration = decl;
        placeholder->constrained = decl->discriminants.empty() && !decl->unknownDiscriminants;
        placeholder->declarationScope = scope;
        placeholder->discriminantCount = static_cast<int>(decl->discriminants.size());
        for (RecordField& discriminant : decl->discriminants) {
            FieldInfo field;
            field.name = discriminant.lower;
            field.displayName = discriminant.name;
            field.index = static_cast<int>(placeholder->fields.size());
            field.type = resolveSubtypeIndication(discriminant.subtype.get(), scope);
            field.isDiscriminant = true;
            field.defaultValue = discriminant.defaultValue.get();
            if (discriminant.defaultValue != nullptr) { analyzeExpr(discriminant.defaultValue.get(), scope, field.type); }
            placeholder->fields.push_back(field);
        }
        decl->declaredType = placeholder;

        Symbol* named = m_symbolTable.createSymbol(SymbolKind::TypeName, decl->lower, decl->name);
        named->type = placeholder;
        named->location = decl->location;
        scope->add(named);
        return;
    }

    if (completing != nullptr && completing->incompleteDeclaration != nullptr
        && !completing->incompleteDeclaration->discriminants.empty()) {
        const auto& previous = completing->incompleteDeclaration->discriminants;
        bool conforms = previous.size() == decl->discriminants.size();
        for (std::size_t i = 0; conforms && i < previous.size(); ++i) {
            const auto& before = previous[i];
            const auto& after = decl->discriminants[i];
            Type* beforeType = discriminantField(completing, static_cast<int>(i)).type;
            Type* afterType = resolveSubtypeIndication(after.subtype.get(), scope);
            if (after.defaultValue != nullptr) { analyzeExpr(after.defaultValue.get(), scope, afterType); }
            conforms = before.lower == after.lower && SemaSupport::staticallyMatches(beforeType, afterType)
                && before.subtype->nullExclusion == after.subtype->nullExclusion
                && SemaSupport::conformingExpressions(before.defaultValue.get(), after.defaultValue.get());
        }
        if (!conforms) {
            m_diagnostics.error(decl->location, "completion discriminants do not conform to the incomplete declaration");
            return;
        }
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
        int privateDiscriminantCount = completing->privateDiscriminantCount;
        bool isLimited = completing->isLimited;
        bool tagged = completing->tagged;
        bool privateViewTagged = completing->privateViewTagged;
        bool privateViewAbstract = completing->privateViewAbstract;
        bool unknownDiscriminants = completing->unknownDiscriminants;
        bool privateExtension = completing->privateExtension;
        Type* privateAncestor = completing->privateAncestor;
        int publicParentFieldCount = completing->publicParentFieldCount;
        bool controlled = completing->controlled;
        std::string tagName = completing->tagName;
        auto slots = std::move(completing->dispatchSlots);
        Type* classWide = completing->classWide;
        auto primitives = std::move(completing->primitives);
        *completing = Type(kind, name);
        completing->primitives = std::move(primitives);
        completing->dispatchSlots = std::move(slots);
        completing->classWide = classWide;
        completing->privateTo = privateTo;
        completing->privateDiscriminantCount = privateDiscriminantCount;
        completing->isLimited = isLimited;
        completing->tagged = tagged;
        completing->privateViewTagged = privateViewTagged;
        completing->privateViewAbstract = privateViewAbstract;
        completing->unknownDiscriminants = unknownDiscriminants;
        completing->privateExtension = privateExtension;
        completing->privateAncestor = privateAncestor;
        completing->publicParentFieldCount = publicParentFieldCount;
        completing->controlled = controlled;
        completing->tagName = tagName;
        return completing;
    };

    TypeDefinition* definition = decl->definition.get();
    bool taggedDefinition = (definition->kind == TypeDefKind::Record && definition->tagged)
        || (definition->kind == TypeDefKind::Derived && definition->extension);
    if (!taggedDefinition && definition->kind == TypeDefKind::Derived && definition->parent != nullptr) {
        Symbol* parent = lookupName(definition->parent->lower, scope);
        taggedDefinition = parent != nullptr && parent->kind == SymbolKind::TypeName && taggedViewVisible(parent->type);
    }
    if (completing != nullptr && !predeclared && completing->tagged && !taggedDefinition) {
        m_diagnostics.error(decl->location, completing->privateTo != nullptr
            ? "a tagged private type requires a tagged full definition"
            : "a tagged incomplete type requires a tagged full definition");
        return;
    }
    if (completing != nullptr && completing->privateTo != nullptr
        && !completing->abstract && definition->abstract) {
        m_diagnostics.error(decl->location, "a nonabstract private view cannot have an abstract full view");
        return;
    }
    if (completing != nullptr && completing->privateTo != nullptr && completing->tagged
        && completing->isLimited && definition->kind == TypeDefKind::Record && !definition->isLimited) {
        m_diagnostics.error(decl->location, "a limited tagged private view requires a limited full view");
    }
    if (completing != nullptr && completing->privateExtension && partialDeclaration != nullptr) {
        if (definition->kind == TypeDefKind::Derived
            && partialDeclaration->definition->isLimited != definition->isLimited) {
            m_diagnostics.error(decl->location, "a private extension and its completion must agree on the limited keyword");
        }
        if (!completing->unknownDiscriminants && partialDeclaration->discriminants.empty()
            && completing->discriminantCount > 0 && !decl->discriminants.empty()) {
            m_diagnostics.error(decl->location, "a private extension with inherited discriminants must inherit them in its full view");
            return;
        }
    }
    bool nonlimitedPrivateCompletion = completing != nullptr && completing->privateTo != nullptr
        && !completing->isLimited;
    Type* type = nullptr;

    switch (definition->kind) {
    case TypeDefKind::Protected: {
        type = makeType(TypeKind::Record);
        type->protectedScope = m_symbolTable.createScope(scope);
        type->isLimited = true;
        type->needsZeroInit = true;
        std::string name = "$" + mangle(decl->lower) + "__protected";
        std::size_t ordinal = ++m_tagNames[name];
        type->tagName = name + (ordinal > 1 ? "__" + std::to_string(ordinal) : "");
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
        spec.lower = "__task_" + (decl->taskObjects.empty() ? decl->lower
            : static_cast<ObjectDecl*>(decl->taskObjects.front().get())->namesLower.front());
        spec.location = decl->location;
        type->taskBody = declareSubprogram(spec, scope, false);
        type->taskBody->taskBody = true;
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
                literal->privatePrimitiveTo = m_packages.back();
            }
            scope->add(literal);
            type->primitives.push_back(literal);
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
        type->modulus = modulus;
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
        Type* deltaType = analyzeExpr(definition->delta.get(), scope, nullptr);
        ExactReal delta;
        if (!isReal(deltaType) || !exactValue(definition->delta.get(), delta)
            || delta.numerator <= 0) {
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
                if (candidate.numerator == delta.numerator && candidate.denominator == delta.denominator) { break; }
            }
            if (scale > ADA_FIXED_MAX_SCALE) {
                m_diagnostics.error(definition->location, "decimal delta must be a power of ten from 10.0**(-18) through 10.0**18");
                scale = 0;
            }
            type->fixedScale = ADA_FIXED_DECIMAL_TAG + scale;
            type->fixedAft = std::max(1, scale);
            type->fixedRadix = 10;
            type->digits = static_cast<int>(digits);
            type->delta = definition->delta.get();
            long long limit = static_cast<long long>(fixedPower10(static_cast<int>(digits)) - 1);
            long long lowCounts = -limit, highCounts = limit;
            if (definition->rangeLow && definition->rangeHigh) {
                Type* lowType = analyzeExpr(definition->rangeLow.get(), scope, nullptr);
                Type* highType = analyzeExpr(definition->rangeHigh.get(), scope, nullptr);
                ExactReal low, high;
                if (!isReal(lowType) || !isReal(highType) || !exactValue(definition->rangeLow.get(), low)
                    || !exactValue(definition->rangeHigh.get(), high)
                    || !low.scaled(type->fixedScale, lowCounts) || !high.scaled(type->fixedScale, highCounts)) {
                    m_diagnostics.error(definition->location, "decimal bounds must be exact static real expressions within 64-bit scaled storage");
                } else {
                    // Check the mathematical bounds before rounding, as required
                    // by the digits definition, including fractional endpoints.
                    ExactReal min = ExactReal::fixed(-limit, type->fixedScale);
                    ExactReal max = ExactReal::fixed(limit, type->fixedScale);
                    ExactReal below = ExactReal::operation('-', low, min);
                    ExactReal above = ExactReal::operation('-', max, high);
                    if (!below.valid || !above.valid || below.numerator < 0 || above.numerator < 0) {
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
            return bits >= 0 ? delta.numerator <= (delta.denominator - 1) / ((__int128)1 << bits)
                             : delta.numerator / delta.denominator < ((__int128)1 << -bits);
        };
        while (bits < ADA_FIXED_MAX_BITS && belowSmall()) { ++bits; }
        if (belowSmall() || delta.numerator / delta.denominator > ((__int128)1 << -ADA_FIXED_MIN_BITS)) {
            m_diagnostics.error(definition->location, "supported fixed-point deltas are 2.0**(-62) through 2.0**62");
        }
        if (decl->smallClause != nullptr) {
            type->smallClause = decl->smallClause;
            // Delta may be referenced in the clause before the final type
            // symbol is published; Small itself is still being defined.
            type->delta = definition->delta.get();
            Scope* representationScope = m_symbolTable.createScope(scope);
            Symbol* self = m_symbolTable.createSymbol(SymbolKind::TypeName, decl->lower, decl->name);
            self->type = type;
            representationScope->add(self);
            type->smallPending = true;
            Type* smallType = analyzeExpr(decl->smallClause, representationScope, nullptr);
            type->smallPending = false;
            ExactReal small;
            bool valid = isReal(smallType) && exactValue(decl->smallClause, small) && small.numerator > 0;
            int selected = ADA_FIXED_MIN_BITS;
            for (; valid && selected <= ADA_FIXED_MAX_BITS; ++selected) {
                ExactReal candidate = selected >= 0 ? ExactReal::make(1, (__int128)1 << selected)
                                                    : ExactReal::make((__int128)1 << -selected);
                if (candidate.numerator == small.numerator && candidate.denominator == small.denominator) { break; }
            }
            if (!valid || selected > ADA_FIXED_MAX_BITS) {
                m_diagnostics.error(decl->smallClause->location, "Small must be a positive static power of two from 2.0**(-62) through 2.0**62");
            } else {
                bits = selected;
                if (belowSmall()) {
                    m_diagnostics.error(decl->smallClause->location, "Small must not exceed the type's delta");
                }
            }
        }
        type->fixedScale = bits;
        __int128 decimalScale = 10;
        while (type->fixedAft < 20
            && delta.numerator < (delta.denominator - 1) / decimalScale + 1) {
            ++type->fixedAft;
            decimalScale *= 10;
        }
        type->delta = definition->delta.get();
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
        type->aliasedElements = definition->aliasedElements;
        Type* cell = resolveSubtypeIndication(definition->elementType.get(), scope, true, true);
        checkCompleteView(cell, decl->location, "an array component");
        if (unknownDiscriminantView(cell)) {
            m_diagnostics.error(decl->location, "an array component requires a definite subtype view");
        }
        if (!unknownDiscriminantView(cell) && baseType(cell) != nullptr && baseType(cell)->discriminantCount > 0
            && !hasKnownDiscriminants(cell) && !hasDefaultDiscriminants(cell)) {
            m_diagnostics.error(decl->location, "an array component requires a definite discriminated subtype");
        }
        type->incompleteLayout = hasIncompleteParts(cell);
        if (abstractView(cell)) {
            m_diagnostics.error(decl->location, "an array component cannot have an abstract type");
            return;
        }
        if (cell != nullptr && cell->classRoot != nullptr) {
            m_diagnostics.error(decl->location, "an array component requires a definite subtype");
            return;
        }
        if (cell != nullptr && cell->kind == TypeKind::Array && !cell->constrained) {
            recordArrayConstraint(cell, definition->elementType.get());
            if (cell->boundsSymbol == nullptr) {
                m_diagnostics.error(definition->elementType->location, "array components require a constrained array subtype");
            }
        }
        bool dynamicBounds = false;
        Type* row = type;
        for (int dimension = 0; dimension < rank; ++dimension) {
            row->aliasedElements = definition->aliasedElements;
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
                    dynamicBounds = true;
                }
            } else if (indexType != nullptr && row->constrained) {
                row->indexLow = indexType->low;
                row->indexHigh = indexType->high;
                if (indexType->scalarBoundsSymbol != nullptr) {
                    index->rangeLow = scalarBoundExpr(indexType, true, index->location);
                    index->rangeHigh = scalarBoundExpr(indexType, false, index->location);
                }
            }
            if (indexType != nullptr && indexType->scalarBoundsSymbol != nullptr && row->constrained) {
                dynamicBounds = true;
            }
            type->boundExpressions.push_back({ index->rangeLow.get(), index->rangeHigh.get() });
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
            type->boundExpressions.clear();
        }
        if (type->constrained) { checkArrayStorage(type, m_diagnostics, definition->location); }

        break;
    }
    case TypeDefKind::Record: {
        type = makeType(TypeKind::Record);
        type->tagged = definition->tagged;
        layoutRecord(decl, definition, type, scope);
        break;
    }
    case TypeDefKind::Derived: {
        Type* extensionType = nullptr;
        Scope* parentScope = scope;
        std::vector<Symbol*> newDiscriminants;
        if (!decl->discriminants.empty()) {
            extensionType = makeType(TypeKind::Record);
            parentScope = m_symbolTable.createScope(scope);
            for (RecordField& discriminant : decl->discriminants) {
                Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Object, discriminant.lower, discriminant.name);
                symbol->type = resolveSubtypeIndication(discriminant.subtype.get(), scope);
                symbol->isConstant = true;
                symbol->discriminantRecord = extensionType;
                parentScope->add(symbol);
                newDiscriminants.push_back(symbol);
            }
        }
        Type* parent = resolveSubtypeIndication(definition->parent.get(), parentScope, false, true);
        if (!checkCompleteView(parent, decl->location, "type derivation")) { return; }
        if (parent == nullptr) {
            return;
        }
        if (parent->isIncomplete && (!parent->tagged || parent->privateTo == nullptr)) {
            m_diagnostics.error(decl->location, parent->privateTo == nullptr
                ? "a derivation parent must be completely defined"
                : "derivation from an uncompleted private view is not yet supported");
            return;
        }
        if (!decl->discriminants.empty() && unknownDiscriminantView(parent)) {
            m_diagnostics.error(decl->location, "a new discriminant part requires a known parent subtype view");
            return;
        }
        if (!definition->privateExtension && !decl->discriminants.empty() && parent->discriminantCount > 0
            && !hasDiscriminantConstraint(parent)) {
            m_diagnostics.error(decl->location, "a derived type with new discriminants requires a constrained parent subtype");
            return;
        }
        if (definition->isLimited && !parent->isLimited) {
            m_diagnostics.error(decl->location, "a limited extension requires a limited ancestor type");
        }
        if (!decl->discriminants.empty() && !parent->tagged) {
            m_diagnostics.error(decl->location, "new discriminants on an untagged derived type are not yet supported");
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
        if (completing != nullptr && parent->controlled && parent->isLimited && !completing->isLimited) {
            m_diagnostics.error(decl->location, "a limited controlled completion requires a limited private view");
            return;
        }
        if (parent->classRoot != nullptr) {
            m_diagnostics.error(decl->location, "a class-wide type cannot be a derivation parent");
            return;
        }
        if (definition->extension && (!parent->tagged || parent->kind != TypeKind::Record)) {
            m_diagnostics.error(decl->location, "a record extension requires a tagged parent type");
            return;
        }
        if (parent->tagged && !definition->extension) {
            m_diagnostics.error(decl->location, "derivation from a tagged type requires with record or with null record");
            return;
        }
        if (!definition->privateExtension && parent->incompleteLayout) {
            m_diagnostics.error(decl->location, "a derivation parent requires complete component types");
            return;
        }
        if (declaredAncestorSubtype != nullptr && hasDiscriminantConstraint(declaredAncestorSubtype)) {
            Type* ancestorView = parent;
            while (ancestorView != nullptr && rootType(ancestorView) != rootType(declaredAncestorSubtype)) {
                ancestorView = rootType(ancestorView)->parentSubtype;
            }
            if (ancestorView == nullptr || !SemaSupport::staticallyMatches(ancestorView, declaredAncestorSubtype)) {
                m_diagnostics.error(decl->location, "a private extension completion must preserve its ancestor discriminant constraint");
            }
        }
        if (parent->tagged && parent->isIncomplete && !definition->privateExtension) {
            m_diagnostics.error(decl->location, "extension of a private or incomplete tagged view is not yet supported");
            return;
        }
        Type* built = m_types.makeSubtype(decl->name, parent, parent->low, parent->high);
        built->isSubtype = false;
        built->base = nullptr;
        built->kind = parent->kind;
        built->literals = parent->literals;
        if (hasKnownDiscriminants(parent)) { built->discriminantConstraintSource = parent; }
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
        type = extensionType != nullptr ? extensionType : makeType(parent->kind);
        for (std::size_t i = 0; i < newDiscriminants.size(); ++i) {
            newDiscriminants[i]->discriminantIndex = static_cast<int>(parent->fields.size() + i);
        }
        std::string name = type->name;
        auto primitives = std::move(type->primitives);
        Type* classWide = type->classWide;
        Symbol* privateTo = type->privateTo;
        int privateDiscriminantCount = type->privateDiscriminantCount;
        bool isLimited = type->isLimited;
        bool privateViewTagged = type->privateViewTagged;
        bool privateViewAbstract = type->privateViewAbstract;
        bool unknownDiscriminants = type->unknownDiscriminants;
        bool privateExtension = type->privateExtension;
        Type* privateAncestor = type->privateAncestor;
        int publicParentFieldCount = type->publicParentFieldCount;
        *type = *built;
        type->layoutSymbol = nullptr;
        type->discriminantRecord = nullptr;
        type->recordRepresentationSpecified = false;
        if (completing != nullptr) {
            type->privateTo = privateTo;
            type->privateDiscriminantCount = privateDiscriminantCount;
            type->privateViewTagged = privateViewTagged;
            type->privateViewAbstract = privateViewAbstract;
            type->unknownDiscriminants = unknownDiscriminants;
            type->privateExtension = privateExtension;
            type->privateAncestor = privateAncestor;
            type->publicParentFieldCount = publicParentFieldCount;
            type->isLimited = isLimited;
        }
        if (completing == nullptr) {
            type->privateTo = nullptr;
            type->privateExtension = false;
            type->privateAncestor = nullptr;
            type->publicParentFieldCount = 0;
            type->unknownDiscriminants = false;
        }
        if (!representationVisible(parent)) {
            for (FieldInfo& field : type->fields) {
                bool visible = fieldVisible(rootType(parent), field);
                if (!visible) { field.name = "$hidden_" + std::to_string(field.index); }
            }
        }
        if (completing != nullptr && privateExtension && !isTaggedAncestor(privateAncestor, parent)) {
            m_diagnostics.error(decl->location, "a private extension completion must descend from its declared ancestor");
        }
        type->primitives = std::move(primitives);
        type->parentType = rootType(parent);
        type->parentSubtype = parent;
        type->unknownDiscriminantAncestor = unknownDiscriminantView(parent);
        type->parentVisible = completing == nullptr || privateTo == nullptr;
        type->classWide = completing != nullptr ? classWide : nullptr;
        type->dispatchSlots = rootType(parent)->dispatchSlots;
        for (Symbol* primitive : type->primitives) {
            primitive->dispatchSlot = -1;
        }
        type->dispatchFrozen = false;
        if (!definition->privateExtension) { rootType(parent)->dispatchFrozen = parent->tagged; }
        type->name = name;
        if (definition->extension) {
            type->byteSize = 0;
            type->compositeSize = 0;
            type->sizeBounds.clear();
            type->tagName.clear();
            type->parentFieldCount = static_cast<int>(parent->fields.size());
            if (!decl->discriminants.empty()) {
                for (FieldInfo& field : type->fields) {
                    if (field.isDiscriminant) { field.name = "$ancestor_" + std::to_string(field.index); }
                }
                type->discriminantStart = static_cast<int>(type->fields.size());
                type->discriminantCount = static_cast<int>(decl->discriminants.size());
                type->discriminantConstraintSource = nullptr;
                type->discriminantsKnown = false;
                type->discriminantValues.clear();
            }
            layoutRecord(decl, definition, type, scope);
        }
        if (definition->privateExtension) {
            if (scope != m_packageSpecScope || !m_inVisiblePart || m_packages.empty()) {
                m_diagnostics.error(decl->location, "a private extension belongs in the visible part of a package");
            } else {
                type->privateTo = m_packages.back();
                type->privateViewTagged = true;
                type->privateViewAbstract = definition->abstract;
                type->privateExtension = true;
                type->privateAncestor = rootType(parent);
                type->publicParentFieldCount = static_cast<int>(parent->fields.size());
                type->privateDiscriminantCount = static_cast<int>(decl->discriminants.size());
                type->unknownDiscriminants = decl->unknownDiscriminants;
                type->isIncomplete = true;
                type->incompleteDeclaration = decl;
            }
        }
        type->abstract = definition->abstract;
        inheritPrimitives(type, rootType(parent), scope, decl->location);
        if (type->tagged) {
            // A private view can declare operations before its parent is known.
            // Complete inherited slots first, then append its own operations.
            for (Symbol* primitive : type->primitives) {
                if (primitive->kind == SymbolKind::Subprogram && primitive->dispatchSlot < 0) {
                    primitive->controllingType = type;
                    primitive->dispatchSlot = static_cast<int>(type->dispatchSlots.size());
                    type->dispatchSlots.push_back(primitive);
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
                literal->privatePrimitiveTo = m_packages.back();
            }
            scope->add(literal);
            type->primitives.push_back(literal);
        }
        break;
    }
    case TypeDefKind::Access: {
        type = makeType(TypeKind::Access);
        type->excludesNull = definition->nullExclusion;
        type->generalAccess = definition->generalAccess;
        type->accessConstant = definition->accessConstant;
        type->accessScope = scope;
        if (definition->accessProfile != nullptr) {
            SubprogramSpec& spec = *definition->accessProfile;
            Symbol* profile = m_symbolTable.createSymbol(SymbolKind::Subprogram, "", decl->name);
            profile->returnType = spec.isFunction ? resolveSubtypeIndication(spec.returnType.get(), scope) : nullptr;
            if (profile->returnType != nullptr && profile->returnType->limitedView) {
                checkCompleteView(profile->returnType, decl->location, "an access-to-subprogram result");
            }
            profile->location = decl->location;
            profile->pendingControlledResult = profile->returnType != nullptr
                && rootType(profile->returnType)->isIncomplete;
            for (ParameterDecl& declaration : spec.parameters) {
                Symbol* parameter = m_symbolTable.createSymbol(SymbolKind::Parameter, declaration.lower, declaration.name);
                parameter->type = resolveSubtypeIndication(declaration.subtype.get(), scope);
                if (parameter->type != nullptr && parameter->type->limitedView && !parameter->type->tagged) {
                    checkCompleteView(parameter->type, declaration.location, "an access-to-subprogram parameter");
                }
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
            type->accessProfile = profile;
            type->accessScope = scope;
        } else {
            type->target = resolveSubtypeIndication(definition->parent.get(), scope, true, true);
            recordArrayConstraint(type->target, definition->parent.get());
        }
        break;
    }
    case TypeDefKind::Private: {
        if (m_packages.empty() || scope != m_packageSpecScope || !m_inVisiblePart) {
            m_diagnostics.error(decl->location, "a private type belongs in the visible part of a package specification");
            return;
        }
        type = makeType(TypeKind::Record);
        type->isIncomplete = true;
        type->tagged = definition->tagged;
        type->privateViewTagged = definition->tagged;
        type->privateViewAbstract = definition->abstract;
        type->unknownDiscriminants = decl->unknownDiscriminants;
        type->incompleteDeclaration = decl;
        type->privateTo = m_packages.back();
        type->privateDiscriminantCount = static_cast<int>(decl->discriminants.size());
        type->isLimited = definition->isLimited;
        for (RecordField& discriminant : decl->discriminants) {
            FieldInfo field;
            field.name = discriminant.lower;
            field.displayName = discriminant.name;
            field.index = static_cast<int>(type->fields.size());
            field.type = resolveSubtypeIndication(discriminant.subtype.get(), scope);
            field.isDiscriminant = true;
            field.defaultValue = discriminant.defaultValue.get();
            if (discriminant.defaultValue) {
                if (definition->tagged) { m_diagnostics.error(discriminant.location, "a tagged type cannot have defaulted discriminants"); }
                analyzeExpr(discriminant.defaultValue.get(), scope, field.type);
            }
            type->fields.push_back(field);
        }
        type->discriminantCount = static_cast<int>(decl->discriminants.size());
        break;
    }
    }

    if (type == nullptr) {
        return;
    }
    if (hasProtected(type) && (nonlimitedPrivateCompletion || type->tagged || type->variantOn >= 0)) {
        m_diagnostics.error(decl->location, "protected components require an ordinary limited full view without variants");
    }
    if (hasTasks(type) && nonlimitedPrivateCompletion) {
        m_diagnostics.error(decl->location, "a completion containing tasks requires a limited private view");
    }
    if (hasTasks(type) && type->tagged) {
        m_diagnostics.error(decl->location, "tagged records containing tasks are not yet supported");
    }
    if (type->variantOn >= 0 && hasTasks(type)) {
        m_diagnostics.error(decl->location, "variant records containing tasks are not yet supported");
    }
    if (nonlimitedPrivateCompletion && hasLimitedParts(type)) {
        m_diagnostics.error(decl->location, "a nonlimited private view requires a nonlimited full view");
    }
    if (completing != nullptr && type->privateTo != nullptr && !type->unknownDiscriminants
        && type->privateDiscriminantCount == 0 && !type->privateExtension
        && ((type->discriminantCount > 0 && !hasKnownDiscriminants(type) && !hasDefaultDiscriminants(type))
            || (type->kind == TypeKind::Array && !type->constrained && type->boundsSymbol == nullptr
                && type->boundExpressions.empty()))) {
        m_diagnostics.error(decl->location, "a private view without discriminants requires a definite full view");
    }
    type->abstract = definition->abstract;
    type->isLimited = type->isLimited || definition->isLimited || hasLimitedParts(type) || hasTasks(type) || hasProtected(type);
    if (type->abstract && !type->tagged) {
        m_diagnostics.error(decl->location, "an abstract type must be tagged");
    }
    if (m_namePrefix.size() == 2 && m_namePrefix[0] == "ada" && m_namePrefix[1] == "finalization"
        && (decl->lower == "controlled" || decl->lower == "limited_controlled")) {
        type->controlled = true;
    }
    if (definition->kind == TypeDefKind::Record && definition->tagged
        && !definition->isLimited && hasLimitedParts(type)) {
        m_diagnostics.error(decl->location, "a tagged record with limited components must be explicitly limited");
    }
    if (definition->kind == TypeDefKind::Record && definition->isLimited) {
        type->limitedRecord = true;
    }
    if (nonlimitedPrivateCompletion && hasLimitedParts(type) && !hasLanguageFinalization(type) && !hasTasks(type)) {
        m_diagnostics.error(decl->location, "a limited full view requires a limited private view");
    }
    if (type->dependentStorage && type->parentType != nullptr && !type->tagged) { type->tagName.clear(); }
    if ((type->tagged || type->dependentStorage) && type->tagName.empty()) {
        std::string name = "$" + mangle(decl->lower) + "__tag";
        std::size_t ordinal = ++m_tagNames[name];
        type->tagName = name + (ordinal > 1 ? "__" + std::to_string(ordinal) : "");
    }
    if (type->kind == TypeKind::Record || type->kind == TypeKind::Array) {
        type->accessIdentity = mangle(decl->lower) + ":"
            + std::filesystem::path(m_diagnostics.fileName(decl->location.file)).filename().string()
            + ":" + std::to_string(decl->location.line) + ":" + std::to_string(decl->location.column);
    }
    type->declarationScope = scope;
    if (type->kind == TypeKind::Access && type->accessProfile == nullptr) {
        type->collectionOwner = m_currentSubprogram;
        std::string name = "$" + mangle(decl->lower) + "__collection";
        std::size_t ordinal = ++m_tagNames[name];
        type->collectionName = name + (ordinal > 1 ? "__" + std::to_string(ordinal) : "");
        type->collectionOffset = -1;
        if (m_currentSubprogram != nullptr && type->target != nullptr
            && (needsCollection(type) || type->target->isIncomplete)) {
            m_currentSubprogram->needsFrame = true;
        }
    }
    if (type->tagged) {
        type->tagOwner = m_currentSubprogram;
        type->tagOffset = -1;
        if (m_currentSubprogram != nullptr) {
            m_currentSubprogram->needsFrame = true;
        }
    }
    if (type->kind == TypeKind::Access || type->tagged) {
        type->accessLevel = type->kind == TypeKind::Access && type->parentType != nullptr
            ? type->parentType->accessLevel : m_accessibilityLevel;
    }
    if (type->classWide != nullptr) {
        Type* wide = type->classWide;
        *wide = *type;
        wide->base = nullptr;
        wide->name = type->name + "'Class";
        wide->classRoot = type;
        wide->abstract = false;
        wide->primitives.clear();
        wide->declarationScope = nullptr;
        m_types.refreshRecordSubtypes(wide);
    }
    if (completing != nullptr && type->kind == TypeKind::Record) {
        m_types.refreshRecordSubtypes(type);
    }
    decl->declaredType = type;
    Symbol* symbol = nullptr;
    if (completing != nullptr && !predeclared) {
        // Reuse the name of the incomplete declaration, including its runtime
        // bounds storage when the full declaration supplies a dynamic array.
        for (Symbol* candidate : scope->lookupDeclaration(decl->lower)) {
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
    if (!type->boundExpressions.empty()) {
        type->boundsSymbol = symbol;
        symbol->owner = m_currentSubprogram;
        symbol->isGlobal = m_currentSubprogram == nullptr;
        symbol->qbeName = "$" + mangle(decl->lower + "__bounds");
        if (m_currentSubprogram != nullptr) {
            m_currentSubprogram->needsFrame = true;
        }
    }
    if (type->kind == TypeKind::Record && (hasRuntimeLayout(type) || type->incompleteLayout) && type->layoutSymbol == nullptr) {
        type->layoutSymbol = symbol;
        symbol->owner = m_currentSubprogram;
        symbol->isGlobal = m_currentSubprogram == nullptr;
        symbol->qbeName = "$" + mangle(decl->lower + "__layout");
        if (m_currentSubprogram != nullptr) { m_currentSubprogram->needsFrame = true; }
        m_types.refreshRecordSubtypes(type);
    }
    if (type->incompleteLayout) {
        type->deferredLayout = true;
        m_pendingLayouts.push_back(type);
    }
    bool progress = true;
    while (progress) {
        progress = false;
        for (auto pending = m_pendingLayouts.begin(); pending != m_pendingLayouts.end();) {
            Type* layout = *pending;
            bool ready = layout->kind == TypeKind::Array ? !hasIncompleteParts(layout->element)
                : std::none_of(layout->fields.begin(), layout->fields.end(),
                    [](const FieldInfo& field) { return hasIncompleteParts(field.type); });
            if (!ready) { ++pending; continue; }
            layout->incompleteLayout = false;
            layout->isLimited |= hasLimitedParts(layout);
            m_types.refreshRecordSubtypes(layout);
            decl->completedLayouts.push_back(layout);
            pending = m_pendingLayouts.erase(pending);
            progress = true;
        }
    }

}

void Sema::analyzeSubtypeDecl(SubtypeDecl* decl, Scope* scope)
{
    if (!scope->lookupLocal(decl->lower).empty()) {
        m_diagnostics.error(decl->location, "'" + decl->name + "' has already been declared in this scope");
        return;
    }

    Symbol* namedView = lookupName(decl->subtype->lower, scope);
    Type* incomplete = namedView != nullptr && namedView->kind == SymbolKind::TypeName
        ? baseType(completeView(namedView->type, scope)) : nullptr;
    if (incomplete != nullptr && incomplete->isIncomplete && incomplete->privateTo == nullptr
        && (decl->subtype->nullExclusion || decl->subtype->rangeLow || !decl->subtype->indexLows.empty())) {
        m_diagnostics.error(decl->location, "an incomplete subtype declaration cannot add constraints");
        return;
    }
    // Preserve dynamic constraints for capture when the declaration elaborates,
    // including symbolic generic bounds resolved by instantiation.
    Type* base = resolveSubtypeIndication(decl->subtype.get(), scope, true, true);
    if (base == nullptr) {
        return;
    }
    if (base->limitedView || (baseType(base)->isIncomplete && baseType(base)->privateTo == nullptr)) {
        decl->incompleteAlias = true;
        decl->declaredType = base;
        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, decl->lower, decl->name);
        symbol->type = base;
        symbol->location = decl->location;
        scope->add(symbol);
        return;
    }
    Type* type = m_types.makeSubtype(decl->name, base, base->low, base->high);
    type->decimalConstraint = base->decimalConstraint;
    if (base->decimalConstraint != nullptr) { type->scalarConstraintBase = base->scalarConstraintBase; }
    decl->declaredType = type;
    Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, decl->lower, decl->name);
    symbol->type = type;
    symbol->location = decl->location;
    if (type->kind == TypeKind::Array && !type->constrained
        && !decl->subtype->indexLows.empty()) {
        type->boundsSymbol = symbol;
        symbol->owner = m_currentSubprogram;
        symbol->isGlobal = m_currentSubprogram == nullptr;
        symbol->qbeName = "$" + mangle(decl->lower + "__bounds");
        if (m_currentSubprogram != nullptr) {
            m_currentSubprogram->needsFrame = true;
        }
        for (std::size_t i = 0; i < decl->subtype->indexLows.size(); ++i) {
            type->boundExpressions.push_back({ decl->subtype->indexLows[i].get(),
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
    long long offset = definition->extension ? typeSize(type->parentType)
        : type->tagged || type->protectedScope != nullptr ? 8 : 0;
    int fieldIndex = static_cast<int>(type->fields.size());
    Scope* componentScope = m_symbolTable.createScope(scope);
    Symbol* current = m_symbolTable.createSymbol(SymbolKind::Object, decl->lower, decl->name);
    current->type = type;
    current->currentInstance = type;
    current->aliased = definition->isLimited || type->limitedRecord
        || (definition->extension && hasLimitedParts(type->parentType))
        || type->protectedScope != nullptr;
    current->isConstant = !current->aliased;
    current->accessibilityLevel = m_accessibilityLevel;
    componentScope->add(current);
    if (definition->extension && decl->discriminants.empty()) {
        for (int i = 0; i < type->discriminantCount; ++i) {
            const FieldInfo& field = discriminantField(type, i);
            if (!fieldVisible(type, field)) { continue; }
            Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Object, field.name, field.displayName);
            symbol->type = field.type;
            symbol->isConstant = true;
            symbol->discriminantRecord = type;
            symbol->discriminantIndex = field.index;
            componentScope->add(symbol);
        }
    }

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
        info.type = resolveSubtypeIndication(field.subtype.get(), isDiscriminant ? scope : componentScope, true, true);
        checkCompleteView(info.type, field.location, "a record component");
        if (unknownDiscriminantView(info.type)) {
            m_diagnostics.error(field.location, "a record component requires a definite subtype view");
        }
        if (!unknownDiscriminantView(info.type) && baseType(info.type) != nullptr && baseType(info.type)->discriminantCount > 0
            && !hasKnownDiscriminants(info.type) && !hasDefaultDiscriminants(info.type)) {
            m_diagnostics.error(field.location, "a record component requires a definite discriminated subtype");
        }
        if (isScalar(info.type) && (discriminantDependency(field.subtype->rangeLow.get())
            || discriminantDependency(field.subtype->rangeHigh.get()))) {
            m_diagnostics.error(field.location, "a discriminant cannot constrain a scalar component");
        }
        type->incompleteLayout = type->incompleteLayout || hasIncompleteParts(info.type);
        if (abstractView(info.type)) {
            m_diagnostics.error(field.location, "a record component cannot have an abstract type");
            return;
        }
        if (info.type != nullptr && info.type->classRoot != nullptr) {
            m_diagnostics.error(field.location, "a record component requires a definite subtype");
            return;
        }
        if (info.type != nullptr && info.type->kind == TypeKind::Array && !info.type->constrained) {
            recordArrayConstraint(info.type, field.subtype.get());
            info.dependentBounds = info.type->discriminantRecord != nullptr;
            type->dependentStorage |= info.dependentBounds && !type->tagged;
            if (info.dependentBounds && type->tagged) {
                info.storageType = m_types.create(TypeKind::Record, anonymousTypeName());
                info.storageType->dependentStorage = true;
                info.storageType->dependentArrayStorage = true;
                info.storageType->arrayRank = info.type->arrayRank;
                info.storageType->tagName = "$" + mangle(decl->lower) + "__array_" + std::to_string(fieldIndex);
            }
            if (info.dependentBounds && (needsFinalization(info.type->element) || hasTasks(info.type->element))) {
                m_diagnostics.error(field.location, "discriminant-dependent arrays with controlled, task or protected elements are not yet supported");
            }
            if (info.type->boundsSymbol == nullptr && !info.dependentBounds) {
                m_diagnostics.error(field.subtype->location, "record components require a constrained array subtype");
            }
        }
        info.index = fieldIndex++;
        info.variant = variantIndex;
        info.isDiscriminant = isDiscriminant;
        info.aliased = field.aliased;
        if (field.defaultValue) {
            if (hasTasks(info.type)) {
                m_diagnostics.error(field.location, "components containing tasks cannot have explicit defaults yet");
            }
            Type* valueType = analyzeExpr(field.defaultValue.get(), isDiscriminant ? scope : componentScope, info.type);
            if (hasLimitedParts(info.type) && !buildsLimitedResult(field.defaultValue.get())) {
                m_diagnostics.error(field.location, "limited controlled components cannot be copied");
            }
            if (!typesCompatible(info.type, valueType)) {
                m_diagnostics.error(field.defaultValue->location, "the component default has an incompatible type");
            }
            adaptUniversal(field.defaultValue.get(), info.type);
            info.defaultValue = field.defaultValue.get();
        }

        long long alignment = info.dependentBounds ? 8 : typeAlignment(info.type);
        __int128 aligned = offset;
        if (alignment > 0 && offset % alignment != 0) { aligned += alignment - offset % alignment; }
        __int128 end = aligned + (info.dependentBounds ? adaArrayLocalSize(info.type->arrayRank) + (info.storageType != nullptr ? 8 : 0) : typeSize(info.type));
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
        if (type->tagged && discriminant.defaultValue != nullptr) {
            m_diagnostics.error(discriminant.location, "a tagged type cannot have defaulted discriminants");
        }
        if (bool(discriminant.defaultValue) != bool(decl->discriminants.front().defaultValue)) {
            m_diagnostics.error(discriminant.location, "defaults must be provided for all discriminants or none");
        }
        place(discriminant, -1, true);
        Type* discriminantType = type->fields.back().type;
        if (discriminantType != nullptr && discriminantType->accessDiscriminant
            && discriminant.defaultValue != nullptr && !definition->isLimited) {
            m_diagnostics.error(discriminant.location, "an access discriminant default requires a limited type");
        }
        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::Object, discriminant.lower, discriminant.name);
        symbol->type = discriminantType;
        symbol->isConstant = true;
        symbol->discriminantRecord = type;
        symbol->discriminantIndex = type->fields.back().index;
        componentScope->add(symbol);
        if (!isDiscrete(discriminantType) && !(discriminantType != nullptr && discriminantType->kind == TypeKind::Access)) {
            m_diagnostics.error(discriminant.location, "a discriminant has to be discrete, and '"
                                                           + (discriminantType != nullptr ? discriminantType->name
                                                                                          : std::string("it"))
                                                           + "' is not");
        }
    }
    if (!definition->extension) {
        type->discriminantCount = static_cast<int>(type->fields.size());
    }

    for (RecordField& field : definition->fields) {
        place(field, -1, false);
    }

    auto finish = [&]() {
        if (type->dependentStorage) {
            bool overflow = false;
            for (FieldInfo& field : type->fields) {
                if (__builtin_add_overflow(field.offset, 16LL, &field.offset)) {
                    field.offset = 0;
                    overflow = true;
                }
            }
            if (overflow || typeSize(type) == INT64_MAX) {
                m_diagnostics.error(decl->location, "record layout exceeds signed 64-bit storage capacity");
            }
        }
    };
    if (definition->variant == nullptr) {
        finish();
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
    finish();
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
        if (type != nullptr && type->tagged && !type->abstract) {
            for (Symbol* primitive : type->primitives) {
                if (primitive->abstract && primitive->inheritedFrom == nullptr
                    && type->privateViewAbstract) {
                    m_diagnostics.error(decl->location, "a nonabstract full view cannot have an abstract primitive '"
                        + primitive->displayName + "'");
                } else if (primitive->inheritedFrom != nullptr && (primitive->abstract
                    || (controllingResultType(primitive->returnType) == type
                        && ((primitive->returnType != nullptr && primitive->returnType->anonymousAccess)
                            || type->fields.size() > static_cast<std::size_t>(type->parentFieldCount))))) {
                    m_diagnostics.error(decl->location, "inherited operation '" + primitive->displayName
                        + "' must be overridden with a nonabstract subprogram");
                }
            }
        }
        if (typeDecl->declaredType != nullptr && typeDecl->declaredType->isIncomplete
            && !typeDecl->declaredType->incompleteInPrivatePart) {
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
    if (decl->attribute == "record") {
        analyzeRecordRepresentation(decl, scope, symbol->type, precedingType);
        return;
    }
    if (decl->attribute == "enumeration") {
        analyzeEnumerationRepresentation(decl, scope, symbol->type, precedingType);
        return;
    }
    if (decl->attribute == "alignment") {
        Type* type = symbol->type;
        if (type->isSubtype || type->isIncomplete || !representationVisible(type)
            || type->tagged || hasTasks(type) || hasProtected(type)) {
            m_diagnostics.error(decl->location, "an Alignment clause requires a complete, visible, untagged first subtype without task or protected parts");
            return;
        }
        if (type->alignmentSpecified) {
            m_diagnostics.error(decl->location, "duplicate Alignment clause");
            return;
        }
        if (type != precedingType || type->scalarBase != nullptr) {
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
        if (alignment > 0 && type->kind == TypeKind::Record && type->specifiedSize >= 0
            && typeSize(type) % alignment != 0) {
            m_diagnostics.error(decl->location, "Alignment conflicts with the preceding Size clause");
            return;
        }
        type->alignment = alignment;
        type->alignmentSpecified = true;
        m_types.refreshStorageRepresentation(type);
        if (type->kind == TypeKind::Enumeration) { m_types.refreshEnumerationRepresentation(type); }
        return;
    }
    if (decl->attribute == "machine_radix") {
        Type* type = symbol->type;
        if (type->kind != TypeKind::Fixed || !fixedIsDecimal(type->fixedScale)
            || type->isSubtype || type->parentType != nullptr || type != precedingType || type->scalarBase != nullptr
            || type->fixedRadixSpecified || !representationVisible(type)) {
            m_diagnostics.error(decl->location, "Machine_Radix requires an adjacent decimal fixed-point first subtype");
            return;
        }
        analyzeExpr(decl->value.get(), scope, m_types.integerType());
        long long radix = 0;
        if (!foldStatic(decl->value.get(), radix) || (radix != 2 && radix != 10)) {
            m_diagnostics.error(decl->location, "decimal Machine_Radix must be the static value 2 or 10");
            return;
        }
        type->fixedRadix = static_cast<int>(radix);
        type->fixedRadixSpecified = true;
        return;
    }
    if (decl->attribute == "small") {
        Type* type = symbol->type;
        if (type->kind != TypeKind::Fixed || fixedIsDecimal(type->fixedScale) || type->isSubtype || type->parentType != nullptr
            || !representationVisible(type)) {
            m_diagnostics.error(decl->location, "a Small clause requires a nonderived ordinary fixed-point first subtype");
        } else if (type->smallClause != decl->value.get()) {
            m_diagnostics.error(decl->location, "a Small clause must occur once, adjacent to its type declaration before representation use");
        }
        return;
    }
    if (decl->attribute != "size") {
        m_diagnostics.error(decl->location, "only a 'Size clause is understood");
        return;
    }

    Type* type = symbol->type;
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
        if (hasRuntimeLayout(type)
            || type->incompleteLayout || hasIncompleteParts(type)) {
            m_diagnostics.error(decl->location, "a composite Size clause requires a complete static layout");
            return;
        }
        if (bytes < typeLayoutSize(type) || bytes % typeAlignment(type) != 0) {
            m_diagnostics.error(decl->location, "composite Size must cover its components and be a multiple of its alignment");
            return;
        }
        if (bytes != typeSize(type)) {
            if (type->tagged && type->isSubtype) {
                m_diagnostics.error(decl->location, "a tagged subtype Size clause cannot change its type layout");
                return;
            }
            if (type != precedingType) {
                m_diagnostics.error(decl->location, "a size-changing clause must be adjacent to its type or subtype declaration, before representation use");
                return;
            }
            if ((!type->tagged && (needsFinalization(type) || type->isLimited)) || hasTasks(type) || hasProtected(type)) {
                m_diagnostics.error(decl->location, "size-changing clauses for controlled, limited, task or protected composites are not yet supported");
                return;
            }
            type->compositeSize = bytes;
            type->sizeBounds.clear();
            if (type->kind == TypeKind::Array) {
                Type* axis = type;
                for (int i = 0; i < type->arrayRank; ++i) {
                    type->sizeBounds.push_back({axis->indexLow, axis->indexHigh});
                    axis = axis->element;
                }
            }
        }
        type->specifiedSize = bytes;
        m_types.refreshStorageRepresentation(type);
        return;
    }
    if (bytes != typeSize(type) && type->isSubtype
        && (type->scalarBoundsSymbol != nullptr || type->low > type->high || type->isIncomplete)) {
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
        __int128 high = type->modulus != 0 && bits == 64
            ? (__int128)UINT64_MAX : std::numeric_limits<long long>::max();
        if (bits == 64) {
            low = std::numeric_limits<long long>::min();
        } else if (bits == 32 || scalarStorageSigned(type)) {
            low = -(1LL << (bits - 1));
            high = (1LL << (bits - 1)) - 1;
        } else {
            high = (1LL << bits) - 1;
        }
        if (!type->isSubtype && type->modulus != 0 && type->modulus - 1 > high) {
            m_diagnostics.error(decl->location, "unsupported representation size for this modular type");
            return;
        }
        if (type->kind == TypeKind::Enumeration && bits == 64) {
            m_diagnostics.error(decl->location, "enumeration storage supports only 8, 16 or 32 bits");
            return;
        }
        __int128 storageLow = type->low;
        __int128 storageHigh = type->high;
        if (!type->enumCodes.empty() && type->low >= 0 && type->high >= type->low
            && type->high < static_cast<long long>(type->enumCodes.size())) {
            storageLow = type->enumCodes[static_cast<std::size_t>(type->low)];
            storageHigh = type->enumCodes[static_cast<std::size_t>(type->high)];
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
        type->specifiedSize = bytes;
        return;
    }
    // Subtypes, field offsets and folded attributes can already contain the
    // old width. Until full freezing is implemented, change storage only at
    // the declaration, before another declaration can depend on its layout.
    if (type != precedingType || (!type->isSubtype && type->scalarBase != nullptr)) {
        m_diagnostics.error(decl->location,
            "a size-changing clause must be adjacent to its type or subtype declaration, before representation use");
        return;
    }
    if (type->alignment > bytes) {
        m_diagnostics.error(decl->location, "Size cannot be smaller than the specified Alignment");
        return;
    }
    type->byteSize = bytes;
    type->specifiedSize = bytes;
    m_types.refreshStorageRepresentation(type);
    if (type->kind == TypeKind::Enumeration && !type->isSubtype) { m_types.refreshEnumerationRepresentation(type); }
}

void Sema::recordScalarConstraint(Type* type, const SourceLocation& location, bool objectConstraint)
{
    type->scalarObjectConstraint = objectConstraint;
    if (type->scalarLow != nullptr) {
        std::string file = std::filesystem::path(m_diagnostics.fileName(location.file)).filename().string();
        for (char& c : file) {
            if (!std::isalnum(static_cast<unsigned char>(c))) { c = '_'; }
        }
        std::string name = "scalar_bounds_" + file + "_" + std::to_string(location.line)
            + "_" + std::to_string(location.column) + "_"
            + std::to_string(m_scalarConstraintDeclaration == nullptr ? 0 : m_scalarConstraintDeclaration->scalarConstraints.size());
        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, name, name);
        symbol->type = type;
        symbol->location = location;
        symbol->owner = m_currentSubprogram;
        symbol->isGlobal = m_currentSubprogram == nullptr;
        symbol->qbeName = "$" + mangle(name);
        type->scalarBoundsSymbol = symbol;
        if (m_currentSubprogram != nullptr) { m_currentSubprogram->needsFrame = true; }
    }
    if (m_scalarConstraintDeclaration != nullptr) {
        m_scalarConstraintDeclaration->scalarConstraints.push_back(type);
        m_scalarConstraintDeclaration->constraintOrder.push_back(type);
    }
}

Type* Sema::resolveSubtypeIndication(SubtypeIndication* indication, Scope* scope, bool allowDynamic, bool allowDynamicScalar)
{
    if (indication == nullptr) {
        return nullptr;
    }
    if (indication->accessDefinition != nullptr) { return resolveAnonymousAccess(indication, scope); }
    Type* base = resolveTypeName(indication->lower, scope, indication->location);
    if (base == nullptr) {
        return nullptr;
    }
    if (base->limitedView && (indication->nullExclusion || indication->rangeLow || indication->digits
        || !indication->indexLows.empty())) {
        m_diagnostics.error(indication->location, "an incomplete type view cannot be constrained");
        return base;
    }
    if (unknownDiscriminantView(base) && !indication->indexLows.empty()) {
        m_diagnostics.error(indication->location, "an unknown-discriminant view cannot have a discriminant constraint");
        return base;
    }
    if (base->accessProfile != nullptr) { rootType(base)->accessConventionFrozen = true; }
    if (indication->nullExclusion) {
        if (base->kind != TypeKind::Access) {
            m_diagnostics.error(indication->location, "a null exclusion requires an access subtype");
        } else if (base->excludesNull) {
            m_diagnostics.error(indication->location, "a null exclusion cannot apply to a subtype that already excludes null");
        } else {
            base = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
            base->excludesNull = true;
        }
    }
    indication->resolved = base;

    Type* decimalConstraint = nullptr;
    int decimalDigits = 0;
    if (indication->digits) {
        Type* digitsType = analyzeExpr(indication->digits.get(), scope, m_types.integerType());
        long long digits = 0;
        bool decimal = base->kind == TypeKind::Fixed && fixedIsDecimal(base->fixedScale);
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
            decimalConstraint->scalarBoundsSymbol = nullptr;
            decimalConstraint->digits = decimalDigits;
            if (!indication->rangeLow) {
                if (!allowDynamicScalar && (base->scalarBoundsSymbol != nullptr
                    || -(__int128)limit < base->low || (__int128)limit > base->high)) {
                    m_diagnostics.error(indication->location, "decimal digits constraints requiring an elaboration check need a local subtype declaration");
                }
                Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, -limit, limit);
                subtype->scalarBoundsSymbol = nullptr;
                subtype->digits = decimalDigits;
                subtype->scalarConstraintBase = base;
                subtype->decimalConstraint = decimalConstraint;
                recordScalarConstraint(subtype, indication->location, false);
                indication->resolved = subtype;
                return subtype;
            }
        }
    }

    if (indication->rangeLow && indication->rangeHigh && isReal(base)) {
        Type* lowType = analyzeExpr(indication->rangeLow.get(), scope, base);
        Type* highType = analyzeExpr(indication->rangeHigh.get(), scope, base);
        if (!typesCompatible(base, lowType) || !typesCompatible(base, highType)) {
            m_diagnostics.error(indication->location, "real subtype bounds must have the subtype's type");
            return base;
        }
        double low = 0.0, high = 0.0;
        bool known = foldStaticReal(indication->rangeLow.get(), low)
            && foldStaticReal(indication->rangeHigh.get(), high) && base->scalarBoundsSymbol == nullptr;
        if (!known && !allowDynamicScalar) {
            m_diagnostics.error(indication->location, "runtime real constraints are not supported in this context");
            return base;
        }
        Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
        subtype->hasRealRange = true;
        subtype->lowReal = low;
        subtype->highReal = high;
        if (!known) {
            subtype->scalarLow = indication->rangeLow.get();
            subtype->scalarHigh = indication->rangeHigh.get();
            subtype->scalarConstraintBase = base;
            recordScalarConstraint(subtype, indication->location, false);
        } else if (allowDynamicScalar) {
            subtype->scalarConstraintBase = base;
            recordScalarConstraint(subtype, indication->location, false);
        }
        indication->resolved = subtype;
        return subtype;
    }

    if (base->kind == TypeKind::Fixed && indication->rangeLow && indication->rangeHigh) {
        Type* lowType = analyzeExpr(indication->rangeLow.get(), scope, base);
        Type* highType = analyzeExpr(indication->rangeHigh.get(), scope, base);
        if (base->formalFixed && typesCompatible(base, lowType) && typesCompatible(base, highType)) {
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
        if (!staticLow || !staticHigh || base->scalarBoundsSymbol != nullptr) {
            if (!allowDynamicScalar) {
                m_diagnostics.error(indication->location, "runtime fixed-point constraints are not supported in this context");
                return base;
            }
            Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
            if (decimalConstraint != nullptr) {
                subtype->digits = decimalDigits;
                subtype->decimalConstraint = decimalConstraint;
            }
            subtype->scalarLow = indication->rangeLow.get();
            subtype->scalarHigh = indication->rangeHigh.get();
            subtype->scalarConstraintBase = base;
            recordScalarConstraint(subtype, indication->location, false);
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
            subtype->decimalConstraint = decimalConstraint;
            subtype->scalarConstraintBase = base;
            recordScalarConstraint(subtype, indication->location, false);
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
        if (!staticLow || !staticHigh || base->scalarBoundsSymbol != nullptr) {
            if (!allowDynamicScalar) {
                m_diagnostics.error(indication->location, "runtime scalar constraints are not supported in this context");
                return base;
            }
            Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
            subtype->scalarLow = indication->rangeLow.get();
            subtype->scalarHigh = indication->rangeHigh.get();
            subtype->scalarConstraintBase = base;
            recordScalarConstraint(subtype, indication->location, false);
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
        if (array != nullptr && array->kind == TypeKind::Access && array->target != nullptr
            && baseType(array->target)->kind == TypeKind::Record) {
            m_diagnostics.error(indication->location, "discriminant constraints on access subtypes are not yet supported");
            return base;
        }
        if (std::any_of(indication->discriminantNames.begin(), indication->discriminantNames.end(),
                [](const auto& names) { return !names.empty(); })) {
            m_diagnostics.error(indication->location, "named constraints require a discriminated record subtype");
            return base;
        }
        if (array == nullptr || array->kind != TypeKind::Array) {
            m_diagnostics.error(indication->location, "index constraints require an array type");
            return base;
        }
        if (indication->indexLows.size() != static_cast<std::size_t>(array->arrayRank)) {
            m_diagnostics.error(indication->location, "index constraint count must match the array rank");
            return base;
        }
        if (base->boundsSymbol != nullptr) {
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
                allStatic = allStatic && staticLow && staticHigh && original->index->scalarBoundsSymbol == nullptr;
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
        ok = ok && array->index->scalarBoundsSymbol == nullptr;
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
    if (unknownDiscriminantView(base)) {
        m_diagnostics.error(indication->location, "an unknown-discriminant view cannot have a discriminant constraint");
        return base;
    }
    if (record->discriminantCount == 0) {
        m_diagnostics.error(indication->location, "'" + record->name + "' has no discriminants to constrain");
        return base;
    }
    if (hasKnownDiscriminants(base)) {
        m_diagnostics.error(indication->location, "a constrained record subtype cannot be constrained again");
        return base;
    }
    std::vector<Expr*> values(record->discriminantCount, nullptr);
    bool named = false;
    int positional = 0;
    for (std::size_t association = 0; association < indication->indexLows.size(); ++association) {
        Expr* value = indication->indexLows[association].get();
        if (indication->indexHighs[association] != nullptr) {
            m_diagnostics.error(value->location, "a discriminant takes one value, not a range");
        }
        auto associate = [&](int index) {
            if (index < 0 || index >= record->discriminantCount) {
                m_diagnostics.error(value->location, "too many discriminant constraint values");
            } else if (values[index] != nullptr) {
                m_diagnostics.error(value->location, "duplicate discriminant constraint value");
            } else { values[index] = value; }
        };
        if (association >= indication->discriminantNames.size() || indication->discriminantNames[association].empty()) {
            if (named) { m_diagnostics.error(value->location, "positional discriminants must precede named associations"); }
            associate(positional++);
        } else {
            named = true;
            Type* associatedType = nullptr;
            for (const std::string& selector : indication->discriminantNames[association]) {
                auto field = std::find_if(record->fields.begin() + record->discriminantStart, record->fields.begin() + record->discriminantStart + record->discriminantCount,
                    [&](const FieldInfo& candidate) { return candidate.name == selector; });
                if (field == record->fields.begin() + record->discriminantStart + record->discriminantCount) {
                    m_diagnostics.error(value->location, "unknown discriminant '" + selector + "'");
                    continue;
                }
                if (associatedType != nullptr && rootType(associatedType) != rootType(field->type)) {
                    m_diagnostics.error(value->location, "grouped discriminant selectors must have the same type");
                }
                associatedType = field->type;
                associate(field->index - record->discriminantStart);
            }
        }
    }
    if (std::any_of(values.begin(), values.end(), [](Expr* value) { return value == nullptr; })) {
        m_diagnostics.error(indication->location, "a discriminant constraint requires exactly one value for each discriminant");
        return base;
    }

    Type* subtype = m_types.makeSubtype(anonymousTypeName(), base, base->low, base->high);
    subtype->name = record->name;
    subtype->discriminantsKnown = true;

    for (int i = 0; i < record->discriminantCount; ++i) {
        Expr* value = values[i];
        Type* discriminantType = discriminantField(record, i).type;
        Type* valueType = analyzeExpr(value, scope, discriminantType);
        if (!typesCompatible(discriminantType, valueType)) {
            m_diagnostics.error(value->location, "discriminant constraint has an incompatible type");
        }
        subtype->discriminantExpressions.push_back(value);
        Type* dependency = m_allocatorDiscriminantConstraint ? nullptr : discriminantDependency(value);
        subtype->discriminantPerObject.push_back(dependency != nullptr);
        if (dependency != nullptr) {
            subtype->discriminantRecord = dependency;
            bool currentAccess = value->kind == ExprKind::Attribute
                && static_cast<AttributeExpr*>(value)->prefix->kind == ExprKind::Identifier
                && static_cast<IdentifierExpr*>(static_cast<AttributeExpr*>(value)->prefix.get())->symbol != nullptr
                && static_cast<IdentifierExpr*>(static_cast<AttributeExpr*>(value)->prefix.get())->symbol->currentInstance != nullptr;
            if (value->kind != ExprKind::Identifier && !currentAccess) {
                m_diagnostics.error(value->location, "a discriminant in a component constraint must appear alone");
            }
        }

        long long fixed = 0;
        if ((discriminantType != nullptr && discriminantType->kind == TypeKind::Access)
            || !foldStatic(value, fixed) || (discriminantType != nullptr && discriminantType->scalarBoundsSymbol != nullptr)) {
            subtype->discriminantsKnown = false;
        } else if (discriminantType != nullptr && (fixed < discriminantType->low || fixed > discriminantType->high)) {
            m_diagnostics.error(value->location, describeValue(discriminantType, fixed) + " is not a value '"
                                                     + discriminantField(record, i).name
                                                     + "' can take");
        }
        subtype->discriminantValues.push_back(fixed);
    }

    if (!subtype->discriminantsKnown) {
        std::string file = std::filesystem::path(m_diagnostics.fileName(indication->location.file)).filename().string();
        for (char& character : file) {
            if (!std::isalnum(static_cast<unsigned char>(character))) { character = '_'; }
        }
        std::string name = "discriminants_" + file + "_"
            + std::to_string(indication->location.line) + "_" + std::to_string(indication->location.column)
            + "_" + std::to_string(m_scalarConstraintDeclaration->discriminantConstraints.size());
        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, name, name);
        symbol->type = subtype;
        symbol->owner = m_currentSubprogram;
        symbol->isGlobal = m_currentSubprogram == nullptr;
        symbol->qbeName = "$" + mangle(name);
        subtype->discriminantSymbol = symbol;
        subtype->discriminantPerEvaluation = m_allocatorDiscriminantConstraint;
        if (!subtype->discriminantPerEvaluation) {
            m_scalarConstraintDeclaration->discriminantConstraints.push_back(subtype);
            m_scalarConstraintDeclaration->constraintOrder.push_back(subtype);
        }
        if (m_currentSubprogram != nullptr) { m_currentSubprogram->needsFrame = true; }
    }
    indication->resolved = subtype;
    return subtype;
}

// Whether a constraint somewhere along the subtype chain has fixed the
// discriminants of the record.
bool Sema::hasKnownDiscriminants(Type* type) const
{
    return hasDiscriminantConstraint(type);
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
    if (target->anonymousAccess && compatibleAnonymousAccess(target, source)) { return true; }
    Type* left = rootType(target);
    Type* right = rootType(source);
    if (left->classRoot != nullptr) {
        return isTaggedAncestor(left->classRoot, right->classRoot != nullptr ? right->classRoot : right);
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
    if (type->scalarBoundsSymbol == nullptr && !isReal(type)) {
        auto literal = std::make_unique<IntegerLiteralExpr>();
        literal->location = location;
        literal->value = first ? type->low : type->high;
        literal->type = type;
        literal->isStatic = true;
        literal->staticValue = literal->value;
        literal->exactReal = ExactReal::make(first ? type->low : type->high);
        return literal;
    }
    auto prefix = std::make_unique<IdentifierExpr>();
    prefix->location = location;
    prefix->type = type;
    prefix->symbol = type->scalarBoundsSymbol;
    auto attribute = std::make_unique<AttributeExpr>();
    attribute->location = location;
    attribute->prefix = std::move(prefix);
    attribute->prefixType = type;
    attribute->type = type;
    attribute->name = attribute->lower = first ? "first" : "last";
    if (isReal(type) && type->scalarBoundsSymbol == nullptr) {
        double maximum = typeSize(type) == 4 ? std::numeric_limits<float>::max()
            : std::numeric_limits<double>::max();
        attribute->isStatic = true;
        attribute->staticReal = type->hasRealRange ? (first ? type->lowReal : type->highReal)
            : (first ? -maximum : maximum);
    }
    return attribute;
}

// Primitive profiles belong to the declaration of a type, not to its layout.
// A subtype shares that identity; a derivation receives new callable views.
void Sema::registerPrimitive(Symbol* symbol, Scope* scope)
{
    std::vector<Type*> types;
    auto consider = [&](Type* type) {
        type = rootType(type);
        if (type != nullptr && type->declarationScope == scope
            && std::find(types.begin(), types.end(), type) == types.end()) {
            types.push_back(type);
        }
    };
    consider(controllingResultType(symbol->returnType));
    for (Symbol* parameter : symbol->parameters) {
        consider(controllingParameterType(parameter->type));
    }
    if (scope == m_packageSpecScope && !m_inVisiblePart && !m_packages.empty()) {
        symbol->privatePrimitiveTo = m_packages.back();
    }
    for (Type* type : types) {
        if (type->tagged && type->classRoot == nullptr
            && (scope == m_packageSpecScope || symbol->overrides)) {
            if (scope == m_packageSpecScope && !m_inVisiblePart
                && controllingResultType(symbol->returnType) == type && !symbol->overrides) {
                for (const auto& declaration : m_symbolTable.symbols()) {
                    if (declaration->kind == SymbolKind::TypeName && declaration->type == type
                        && declaration->privatePrimitiveTo == nullptr) {
                        m_diagnostics.error(symbol->location,
                            "a private primitive with a controlling result must override a visible operation");
                        break;
                    }
                }
            }
            if (symbol->abstract && scope == m_packageSpecScope && !m_inVisiblePart) {
                bool visibleType = false;
                bool overridesAbstract = false;
                for (const auto& declaration : m_symbolTable.symbols()) {
                    if (declaration->kind == SymbolKind::TypeName && declaration->type == type
                        && declaration->privatePrimitiveTo == nullptr) { visibleType = true; }
                }
                for (Symbol* primitive : type->primitives) {
                    bool matches = primitive->inheritedFrom != nullptr && primitive->abstract
                        && primitive->privatePrimitiveTo == nullptr && primitive->name == symbol->name
                        && primitive->parameters.size() == symbol->parameters.size()
                        && SemaSupport::profileTypeMatches(primitive->returnType, symbol->returnType);
                    for (std::size_t i = 0; matches && i < symbol->parameters.size(); ++i) {
                        matches = rootType(primitive->parameters[i]->type) == rootType(symbol->parameters[i]->type);
                    }
                    overridesAbstract = overridesAbstract || matches;
                }
                if (visibleType && !overridesAbstract) {
                    m_diagnostics.error(symbol->location,
                        "a private abstract primitive must override a visible inherited abstract operation");
                }
            }
            if (symbol->controllingType != nullptr && symbol->controllingType != type) {
                m_diagnostics.error(symbol->location, "a dispatching operation cannot control multiple tagged types");
            }
            for (Symbol* parameter : symbol->parameters) {
                if (controllingParameterType(parameter->type) == type && parameter->hasDefault
                    && !parameter->defaultExpr->tagIndeterminate) {
                    m_diagnostics.error(parameter->location, "a controlling parameter default must be tag indeterminate");
                } else if (controllingParameterType(parameter->type) == type && parameter->hasDefault) {
                    dispatchIndeterminate(parameter->defaultExpr, type);
                }
            }
        }
        bool replaced = false;
        for (Symbol*& primitive : type->primitives) {
            if (primitive->name != symbol->name || primitive->inheritedFrom == nullptr
                || primitive->parameters.size() != symbol->parameters.size()
                || !SemaSupport::profileTypeMatches(primitive->returnType, symbol->returnType)) {
                continue;
            }
            bool matches = true;
            for (std::size_t i = 0; i < symbol->parameters.size(); ++i) {
                matches = matches && (rootType(primitive->parameters[i]->type) == rootType(symbol->parameters[i]->type)
                    || SemaSupport::staticallyMatches(primitive->parameters[i]->type, symbol->parameters[i]->type));
            }
            if (matches) {
                scope->remove(primitive);
                if (type->tagged && primitive->dispatchSlot >= 0) {
                    if (type->dispatchFrozen) {
                        m_diagnostics.error(symbol->location, "a tagged primitive cannot be overridden after its type is frozen");
                    }
                    symbol->dispatchSlot = primitive->dispatchSlot;
                    symbol->controllingType = type;
                    type->dispatchSlots[symbol->dispatchSlot] = symbol;
                }
                primitive = symbol;
                replaced = true;
            }
        }
        if (!replaced && (scope == m_packageSpecScope || symbol->overrides)) {
            type->primitives.push_back(symbol);
            if (type->tagged && type->classRoot == nullptr) {
                if (type->dispatchFrozen) {
                    m_diagnostics.error(symbol->location, "a tagged primitive cannot be declared after its type is frozen");
                }
                symbol->controllingType = type;
                symbol->dispatchSlot = static_cast<int>(type->dispatchSlots.size());
                type->dispatchSlots.push_back(symbol);
            }
        }
    }
}

void Sema::inheritPrimitives(Type* type, Type* parent, Scope* scope, const SourceLocation& location)
{
    // Constraints in the parent's profile survive derivation. In particular,
    // deriving from a constrained subtype does not narrow every inherited formal.
    auto corresponding = [&](Type* original) -> Type* {
        if (original != nullptr && original->kind == TypeKind::Access && original->anonymousAccess
            && rootType(original->target) == rootType(parent)) {
            Type* access = m_types.create(TypeKind::Access, anonymousTypeName());
            *access = *original;
            access->target = type;
            return access;
        }
        if (original == nullptr || rootType(original) != rootType(parent)) {
            return original;
        }
        if (type->tagged) {
            // Controlling tagged parameters use the corresponding first
            // subtype; parent constraints belong to the inherited body view.
            return type;
        }
        Type* subtype = m_types.create(original->kind, type->name);
        *subtype = *original;
        subtype->base = type;
        subtype->isSubtype = true;
        subtype->specifiedSize = -1;
        subtype->primitives.clear();
        if (original->kind == TypeKind::Record) {
            subtype->discriminantConstraintSource = original;
        }
        return subtype;
    };
    for (Symbol* primitive : rootType(parent)->primitives) {
        if (primitive->kind != SymbolKind::Subprogram
            || (primitive->privatePrimitiveTo != nullptr && !withinPackage(primitive->privatePrimitiveTo))) {
            continue;
        }
        if (type->tagged && !primitive->abstract && (primitive->name == "=" || primitive->name == "/=")
            && m_types.isBoolean(primitive->returnType) && primitive->parameters.size() == 2
            && rootType(primitive->parameters[0]->type) == rootType(parent)
            && rootType(primitive->parameters[1]->type) == rootType(parent)) {
            continue;
        }
        Symbol* inherited = m_symbolTable.createSymbol(SymbolKind::Subprogram, primitive->name, primitive->displayName);
        inherited->location = location;
        inherited->inheritedFrom = primitive;
        bool accessResult = primitive->returnType != nullptr && primitive->returnType->anonymousAccess
            && controllingResultType(primitive->returnType) == rootType(parent);
        inherited->abstract = primitive->abstract || (type->abstract
            && (accessResult || (rootType(primitive->returnType) == rootType(parent)
                && type->fields.size() > static_cast<std::size_t>(type->parentFieldCount))));
        inherited->dispatchSlot = primitive->dispatchSlot;
        inherited->controllingType = type;
        if (inherited->dispatchSlot >= 0) {
            type->dispatchSlots[inherited->dispatchSlot] = inherited;
        }
        inherited->declarationScope = scope;
        inherited->accessibilityLevel = m_accessibilityLevel;
        inherited->returnType = corresponding(primitive->returnType);
        inherited->hasBody = true;
        inherited->level = type->tagged && m_currentSubprogram != nullptr ? m_currentSubprogram->level + 1 : primitive->level;
        inherited->owner = type->tagged ? m_currentSubprogram : primitive->owner;
        if (type->tagged) {
            std::string operation = primitive->name;
            if (!operatorSymbol(operation).empty()) {
                operation = "operator";
                for (unsigned char character : primitive->name) {
                    operation += "_" + std::to_string(character);
                }
            }
            std::string name = "$" + mangle(type->name + "__" + operation + "__inherited");
            std::size_t ordinal = ++m_subprogramNames[name];
            inherited->qbeName = name + (ordinal > 1 ? "__" + std::to_string(ordinal) : "");
            if (inherited->owner != nullptr) { inherited->owner->needsFrame = true; }
        }
        if (scope == m_packageSpecScope && !m_inVisiblePart && !m_packages.empty()) {
            inherited->privatePrimitiveTo = m_packages.back();
        }
        for (Symbol* parameter : primitive->parameters) {
            Symbol* copy = m_symbolTable.createSymbol(SymbolKind::Parameter, parameter->name, parameter->displayName);
            *copy = *parameter;
            copy->type = corresponding(parameter->type);
            copy->owner = inherited;
            inherited->parameters.push_back(copy);
        }
        bool overridden = false;
        for (Symbol* declared : type->primitives) {
            if (declared->kind != SymbolKind::Subprogram || declared->name != inherited->name
                || !SemaSupport::profileTypeMatches(declared->returnType, inherited->returnType)
                || declared->parameters.size() != inherited->parameters.size()) {
                continue;
            }
            bool matches = true;
            for (std::size_t i = 0; i < declared->parameters.size(); ++i) {
                matches = matches && (rootType(declared->parameters[i]->type) == rootType(inherited->parameters[i]->type)
                    || SemaSupport::staticallyMatches(declared->parameters[i]->type, inherited->parameters[i]->type));
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
                if (type->tagged && !subtypeConformant) {
                    m_diagnostics.error(declared->location, "overriding dispatching profile must be subtype conformant with the inherited operation");
                }
                declared->overrides = true;
                if (type->tagged && inherited->dispatchSlot >= 0) {
                    declared->controllingType = type;
                    declared->dispatchSlot = inherited->dispatchSlot;
                    type->dispatchSlots[declared->dispatchSlot] = declared;
                }
                overridden = true;
            }
        }
        if (!overridden) {
            type->primitives.push_back(inherited);
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
    if (type->enumCodesSpecified) {
        m_diagnostics.error(decl->location, "duplicate enumeration representation clause");
        return;
    }
    if (type != precedingType || type->scalarBase != nullptr) {
        m_diagnostics.error(decl->location, "an enumeration representation clause must be adjacent to its type declaration, before representation use");
        return;
    }
    if (decl->value == nullptr || decl->value->kind != ExprKind::Aggregate) {
        m_diagnostics.error(decl->location, "an enumeration representation clause requires a one-dimensional array aggregate");
        return;
    }
    auto* aggregate = static_cast<AggregateExpr*>(decl->value.get());
    if (aggregate->ancestor != nullptr || aggregate->nullRecord) {
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
    if (type->specifiedSize >= 0) {
        bytes = type->specifiedSize;
        long long low = codes.front() < 0 ? -(1LL << (bytes * 8 - 1)) : 0;
        long long high = bytes == 4 || codes.front() < 0 ? (1LL << (bytes * 8 - 1)) - 1 : (1LL << (bytes * 8)) - 1;
        if (codes.front() < low || codes.back() > high) {
            m_diagnostics.error(decl->location, "enumeration representation codes do not fit the specified Size");
            return;
        }
    }
    if (type->alignment > bytes) {
        m_diagnostics.error(decl->location, "enumeration storage cannot be smaller than its specified Alignment");
        return;
    }
    type->enumCodes = std::move(codes);
    type->enumCodesSpecified = true;
    type->byteSize = bytes;
    m_types.refreshEnumerationRepresentation(type);
}

void Sema::recordArrayConstraint(Type* type, SubtypeIndication* indication)
{
    if (type == nullptr || type->kind != TypeKind::Array || type->constrained
        || type->boundsSymbol != nullptr || indication->indexLows.empty()) { return; }
    for (std::size_t i = 0; i < indication->indexLows.size(); ++i) {
        Type* dependency = discriminantDependency(indication->indexLows[i].get());
        if (dependency == nullptr) { dependency = discriminantDependency(indication->indexHighs[i].get()); }
        if (dependency != nullptr) { type->discriminantRecord = dependency; }
    }
    if (type->discriminantRecord != nullptr) {
        for (std::size_t i = 0; i < indication->indexLows.size(); ++i) {
            type->boundExpressions.push_back({indication->indexLows[i].get(), indication->indexHighs[i].get()});
            for (Expr* bound : {indication->indexLows[i].get(), indication->indexHighs[i].get()}) {
                if (discriminantDependency(bound) != nullptr && bound->kind != ExprKind::Identifier) {
                    m_diagnostics.error(bound->location, "a discriminant in a component constraint must appear alone");
                }
            }
        }
        Type* capture = m_types.makeSubtype(anonymousTypeName(), type, type->low, type->high);
        capture->discriminantRecord = nullptr;
        capture->partialDiscriminantBounds = true;
        for (auto [low, high] : type->boundExpressions) {
            capture->boundExpressions.push_back({discriminantDependency(low) ? nullptr : low,
                discriminantDependency(high) ? nullptr : high});
        }
        std::string file = std::filesystem::path(m_diagnostics.fileName(indication->location.file)).filename().string();
        for (char& character : file) {
            if (!std::isalnum(static_cast<unsigned char>(character))) { character = '_'; }
        }
        std::string name = "component_capture_" + file + "_" + std::to_string(indication->location.line) + "_"
            + std::to_string(indication->location.column) + "_"
            + std::to_string(m_scalarConstraintDeclaration->arrayConstraints.size());
        Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, name, name);
        symbol->type = capture;
        symbol->owner = m_currentSubprogram;
        symbol->isGlobal = m_currentSubprogram == nullptr;
        symbol->qbeName = "$" + mangle(name);
        capture->boundsSymbol = symbol;
        type->boundsSymbol = symbol;
        m_scalarConstraintDeclaration->arrayConstraints.push_back(capture);
        m_scalarConstraintDeclaration->constraintOrder.push_back(capture);
        if (m_currentSubprogram != nullptr) { m_currentSubprogram->needsFrame = true; }
        return;
    }
    std::string file = std::filesystem::path(m_diagnostics.fileName(indication->location.file)).filename().string();
    for (char& character : file) {
        if (!std::isalnum(static_cast<unsigned char>(character))) { character = '_'; }
    }
    std::string name = "array_bounds_" + file + "_" + std::to_string(indication->location.line) + "_"
        + std::to_string(indication->location.column) + "_"
        + std::to_string(m_scalarConstraintDeclaration->arrayConstraints.size());
    Symbol* symbol = m_symbolTable.createSymbol(SymbolKind::TypeName, name, name);
    symbol->type = type;
    symbol->owner = m_currentSubprogram;
    symbol->isGlobal = m_currentSubprogram == nullptr;
    symbol->qbeName = "$" + mangle(name);
    type->boundsSymbol = symbol;
    for (std::size_t i = 0; i < indication->indexLows.size(); ++i) {
        type->boundExpressions.push_back({indication->indexLows[i].get(), indication->indexHighs[i].get()});
    }
    m_scalarConstraintDeclaration->arrayConstraints.push_back(type);
    m_scalarConstraintDeclaration->constraintOrder.push_back(type);
    if (m_currentSubprogram != nullptr) { m_currentSubprogram->needsFrame = true; }
}
