#include "Sema.h"
#include "SemaSupport.h"

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <limits>

using SemaSupport::adaptUniversal;
using SemaSupport::objectView;

namespace
{

// Without an explicit range a floating point type spans what its machine
// representation can hold.
double floatLowBound(const Type* type)
{
    if (type->hasRealRange) {
        return type->lowReal;
    }
    return typeSize(type) == 8 ? -DBL_MAX : -static_cast<double>(FLT_MAX);
}

double floatHighBound(const Type* type)
{
    if (type->hasRealRange) {
        return type->highReal;
    }
    return typeSize(type) == 8 ? DBL_MAX : static_cast<double>(FLT_MAX);
}

// Ada's T'Width: the length of the longest image the type can produce.  For an
// enumeration that is its longest literal, and for a number its widest run of
// digits plus the column the sign or its blank occupies.
long long widthOf(const Type* type)
{
    if (type->low > type->high) {
        return 0;
    }
    const Type* base = type;
    while (base != nullptr && base->base != nullptr && base->literals.empty()
           && base->kind == TypeKind::Enumeration) {
        base = base->base;
    }

    if (base != nullptr && base->kind == TypeKind::Enumeration) {
        // Character names its values by their spelling, so its widest image is
        // one character between two quotes.
        if (base->literals.empty()) {
            return 3;
        }
        std::size_t longest = 0;
        for (long long i = type->low; i <= type->high && i < static_cast<long long>(base->literals.size()); ++i) {
            longest = std::max(longest, base->literals[static_cast<std::size_t>(i)].size());
        }
        return static_cast<long long>(longest);
    }

    auto imageWidth = [](__int128 value) {
        long long width = 1;
        unsigned __int128 magnitude = value < 0 ? -value : value;
        do { ++width; magnitude /= 10; } while (magnitude != 0);
        return width;
    };
    return std::max(imageWidth(type->low), imageWidth(type->high));
}

// A subtype is declared without repeating the digits of the type it comes from.
int digitsOf(const Type* type)
{
    for (const Type* current = type; current != nullptr; current = current->base) {
        if (current->digits > 0) {
            return current->digits;
        }
    }
    return 6;
}

}

bool Sema::matchesAccessProfile(Symbol* candidate, Type* access) const
{
    if (access == nullptr || access->m_accessProfile == nullptr || candidate->kind != SymbolKind::Subprogram) {
        return false;
    }
    Symbol* profile = access->m_accessProfile;
    bool sameResult = candidate->returnType == profile->returnType
        || SemaSupport::staticallyMatches(candidate->returnType, profile->returnType);
    if (!sameResult || candidate->parameters.size() != profile->parameters.size()) {
        return false;
    }
    for (std::size_t i = 0; i < profile->parameters.size(); ++i) {
        if (!SemaSupport::staticallyMatches(candidate->parameters[i]->type, profile->parameters[i]->type)
            || candidate->parameters[i]->mode != profile->parameters[i]->mode) {
            return false;
        }
    }
    return true;
}

Type* Sema::analyzeAttribute(AttributeExpr* expr, Scope* scope, Type* expected)
{
    if (expr->lower == "caller") {
        auto names = expressionNames(expr->prefix.get(), scope);
        Symbol* entry = names.size() == 1 ? names.front() : nullptr;
        bool inside = false;
        for (Symbol* owner = m_currentSubprogram; owner != nullptr; owner = owner->owner) {
            inside |= owner == entry;
        }
        bool current = expr->prefix->kind != ExprKind::Selected
            || (entry != nullptr && isProtectedCurrentInstance(
                static_cast<SelectedExpr*>(expr->prefix.get())->prefix.get(), entry->m_protectedType, scope));
        if (entry == nullptr || !entry->m_entry || !inside || !current || !expr->arguments.empty()) {
            m_diagnostics.error(expr->location, "'Caller requires the enclosing entry and no arguments");
            return nullptr;
        }
        expr->type = m_taskIdType;
        return expr->type;
    }
    if (expr->lower == "count") {
        auto names = expressionNames(expr->prefix.get(), scope);
        Symbol* entry = names.size() == 1 ? names.front() : nullptr;
        Symbol* receiver = entry != nullptr ? protectedReceiver(entry->m_protectedType) : nullptr;
        bool current = expr->prefix->kind != ExprKind::Selected
            || (entry != nullptr && isProtectedCurrentInstance(
                static_cast<SelectedExpr*>(expr->prefix.get())->prefix.get(), entry->m_protectedType, scope));
        if (entry == nullptr || !entry->m_entry || receiver == nullptr || !current || !expr->arguments.empty()) {
            m_diagnostics.error(expr->location, "entry 'Count requires an entry of the current protected instance");
            return nullptr;
        }
        noteReference(receiver);
        expr->m_protectedReceiver = receiver;
        expr->m_countEntry = entry;
        expr->type = m_types.universalInteger();
        return expr->type;
    }
    if (expr->lower == "access") {
        if (expected == nullptr || expected->m_accessProfile == nullptr || !expr->arguments.empty()) {
            m_diagnostics.error(expr->location, "subprogram 'Access requires an access-to-subprogram type context");
            return nullptr;
        }
        Symbol* chosen = nullptr;
        for (Symbol* candidate : expressionNames(expr->prefix.get(), scope)) {
            if (matchesAccessProfile(candidate, expected)) {
                if (chosen != nullptr) {
                    m_diagnostics.error(expr->location, "ambiguous subprogram 'Access");
                    return nullptr;
                }
                chosen = candidate;
            }
        }
        if (chosen == nullptr) {
            m_diagnostics.error(expr->location, "no subprogram matches the designated access profile");
            return nullptr;
        }
        if (chosen->m_protectedType != nullptr) {
            m_diagnostics.error(expr->location, "access to protected subprograms is not yet supported");
            return nullptr;
        }
        // A renaming is another view of the same callable entity.
        while (chosen->m_renamedSubprogram != nullptr) {
            chosen = chosen->m_renamedSubprogram;
        }
        if (chosen->m_inheritedFrom != nullptr) {
            bool tagged = chosen->returnType != nullptr && chosen->returnType->m_tagged;
            for (Symbol* parameter : chosen->parameters) {
                tagged = tagged || parameter->type->m_tagged;
            }
            if (tagged) {
                m_diagnostics.error(expr->location, "'Access of inherited tagged subprograms is not yet supported");
                return nullptr;
            }
        }
        if (chosen->m_intrinsicRenaming || chosen->m_inheritedFrom != nullptr) {
            m_diagnostics.error(expr->location, "'Access of intrinsic subprograms is not allowed");
            return nullptr;
        }
        bool accessible = false;
        Scope* declarationScope = chosen->m_renamedAccess != nullptr
            ? chosen->m_renamedAccess->type->m_accessScope : chosen->m_declarationScope;
        for (Scope* enclosing = expected->m_accessScope; enclosing != nullptr; enclosing = enclosing->parent()) {
            accessible = accessible || enclosing == declarationScope;
        }
        // Library subprograms have no captured activation.
        if (!accessible && chosen->level > 0) {
            m_diagnostics.error(expr->location, "subprogram is deeper than the access type");
            return nullptr;
        }
        if (chosen->builtin != BuiltinKind::None) {
            m_diagnostics.error(expr->location, "'Access of imported subprograms is not yet supported");
            return nullptr;
        }
        if (chosen->m_renamedAccess != nullptr) {
            noteReference(chosen->m_renamedAccess);
        } else {
            chosen->m_addressTaken = true;
        }
        expr->m_accessSubprogram = chosen;
        recordContractName(expr->prefix.get(), chosen);
        expr->type = expected;
        return expr->type;
    }
    bool prefixIsType = false;
    Type* prefixType = nullptr;

    if (expr->prefix->kind == ExprKind::Identifier) {
        auto* identifier = static_cast<IdentifierExpr*>(expr->prefix.get());
        std::vector<Symbol*> candidates = scope->lookup(identifier->lower);
        for (Symbol* candidate : candidates) {
            if (candidate->kind == SymbolKind::TypeName) {
                prefixIsType = true;
                prefixType = candidate->type;
                identifier->symbol = candidate;
                identifier->type = candidate->type;
                break;
            }
        }
    }
    if (!prefixIsType) {
        prefixType = analyzeExpr(expr->prefix.get(), scope, nullptr);
    }
    if (expr->prefix->kind == ExprKind::Selected) {
        auto* selected = static_cast<SelectedExpr*>(expr->prefix.get());
        prefixIsType = selected->symbol != nullptr && selected->symbol->kind == SymbolKind::TypeName;
    } else if (expr->prefix->kind == ExprKind::Attribute) {
        prefixIsType = static_cast<AttributeExpr*>(expr->prefix.get())->lower == "base"
            || static_cast<AttributeExpr*>(expr->prefix.get())->lower == "class";
    }
    if (!prefixIsType && prefixType != nullptr && prefixType->kind == TypeKind::Access
        && prefixType->target != nullptr && prefixType->target->kind == TypeKind::Array
        && (expr->lower == "first" || expr->lower == "last" || expr->lower == "length")) {
        auto dereference = std::make_unique<SelectedExpr>();
        dereference->location = expr->prefix->location;
        dereference->selector = dereference->selectorLower = "all";
        dereference->isDereference = true;
        dereference->prefix = std::move(expr->prefix);
        expr->prefix = std::move(dereference);
        prefixType = analyzeExpr(expr->prefix.get(), scope, nullptr);
    }
    expr->prefixType = prefixType;
    if (expr->lower == "identity" || expr->lower == "callable" || expr->lower == "terminated") {
        Type* task = prefixType;
        if (task != nullptr && task->kind == TypeKind::Access) {
            task = task->target;
        }
        task = baseType(task);
        if (task != nullptr && task->m_taskBody != nullptr
            && representationVisible(prefixType) && representationVisible(task)) {
            bool valid = expr->arguments.empty();
            if (prefixIsType) {
                Symbol* named = expr->prefix->kind == ExprKind::Identifier
                    ? static_cast<IdentifierExpr*>(expr->prefix.get())->symbol
                    : (expr->prefix->kind == ExprKind::Selected
                        ? static_cast<SelectedExpr*>(expr->prefix.get())->symbol : nullptr);
                for (Symbol* owner = m_currentSubprogram; owner != nullptr; owner = owner->owner) {
                    if (owner == task->m_taskBody && named != nullptr && !named->type->isSubtype
                        && owner->name == "__task_" + named->name) {
                        expr->m_taskCurrentInstance = owner->m_taskIdentity;
                        noteReference(expr->m_taskCurrentInstance);
                        break;
                    }
                }
                valid &= expr->m_taskCurrentInstance != nullptr;
            } else {
                valid &= objectView(expr->prefix.get()) != 0
                    || (prefixType != nullptr && prefixType->kind == TypeKind::Access);
            }
            if (!valid) {
                m_diagnostics.error(expr->location, "task attribute requires a task object or its current instance and no arguments");
                return nullptr;
            }
            expr->m_taskAttribute = true;
            expr->type = expr->lower == "identity" ? m_taskIdType : m_types.booleanType();
            return expr->type;
        }
        if (expr->lower != "identity") {
            m_diagnostics.error(expr->location, "task attribute requires a task object");
            return nullptr;
        }
    }
    if (expr->lower == "identity") {
        Symbol* symbol = nullptr;
        if (expr->prefix->kind == ExprKind::Identifier) {
            symbol = static_cast<IdentifierExpr*>(expr->prefix.get())->symbol;
        } else if (expr->prefix->kind == ExprKind::Selected) {
            symbol = static_cast<SelectedExpr*>(expr->prefix.get())->symbol;
        }
        if (symbol == nullptr || symbol->kind != SymbolKind::Exception || !expr->arguments.empty()) {
            m_diagnostics.error(expr->location, "'Identity requires an exception name and no arguments");
            return nullptr;
        }
        expr->exceptionSymbol = symbol;
        expr->type = m_exceptionIdType;
        return expr->type;
    }
    if (expr->lower == "class") {
        if (!prefixIsType || !taggedViewVisible(prefixType) || prefixType->m_classRoot != nullptr) {
            m_diagnostics.error(expr->location, "'Class requires a specific tagged type");
            return nullptr;
        }
        expr->type = classWideType(prefixType);
        return expr->type;
    }
    if (expr->lower == "tag" || expr->lower == "external_tag") {
        if (!taggedViewVisible(prefixType) || !expr->arguments.empty()
            || (prefixIsType && prefixType->m_classRoot != nullptr)
            || (expr->lower == "external_tag" && !prefixIsType)) {
            m_diagnostics.error(expr->location, "tag attributes require a specific tagged subtype or a tagged object");
            return nullptr;
        }
        expr->type = expr->lower == "tag" ? m_tagType : m_types.stringType();
        expr->prefixType = prefixType;
        return expr->type;
    }
    if (expr->lower == "base") {
        if (!prefixIsType || !expr->arguments.empty() || !representationVisible(prefixType)) {
            m_diagnostics.error(expr->location, "'Base requires a scalar subtype mark");
            return nullptr;
        }
        expr->type = m_types.scalarBaseType(prefixType);
        if (expr->type == nullptr) {
            m_diagnostics.error(expr->location, "'Base requires a scalar subtype mark");
        }
        return expr->type;
    }

    const std::string& name = expr->lower;
    if (name == "machine_overflows" || name == "machine_rounds") {
        if (!prefixIsType || prefixType == nullptr || prefixType->kind != TypeKind::Float
            || !representationVisible(prefixType) || !expr->arguments.empty()) {
            m_diagnostics.error(expr->location, "'" + expr->name + " requires a floating point subtype mark and no arguments");
            return nullptr;
        }
        expr->type = m_types.booleanType();
        expr->isStatic = true;
        expr->staticValue = 1;
        return expr->type;
    }
    bool bounds = name == "first" || name == "last" || name == "length";
    bool discreteFunction = name == "pos" || name == "val";
    bool adjacent = name == "succ" || name == "pred";
    if (bounds || discreteFunction || adjacent) {
        bool array = prefixType != nullptr && prefixType->kind == TypeKind::Array;
        bool scalar = isDiscrete(prefixType) || isReal(prefixType)
            || (prefixType != nullptr && prefixType->kind == TypeKind::Fixed);
        if (!representationVisible(prefixType) || (bounds ? (!array && (!scalar || !prefixIsType))
            : (!prefixIsType || (discreteFunction ? !isDiscrete(prefixType) : !scalar)))) {
            m_diagnostics.error(expr->location, "'" + expr->name + " requires "
                + (bounds ? "an array prefix or a scalar subtype mark" : discreteFunction
                    ? "a discrete subtype mark" : "a scalar subtype mark"));
            return nullptr;
        }
        if (name == "length" && !array) {
            m_diagnostics.error(expr->location, "'Length requires an array prefix");
            return nullptr;
        }
        if (!array && expr->arguments.size() != (bounds ? 0u : 1u)) {
            m_diagnostics.error(expr->location, "'" + expr->name
                + (bounds ? " takes no arguments" : " takes exactly one argument"));
            return nullptr;
        }
    }

    for (const ExprPtr& argument : expr->arguments) {
        // An aggregate waits for the stream attributes below, which know the
        // type it is meant to have.
        if (argument->kind != ExprKind::Aggregate) {
            bool contextual = expr->lower == "min" || expr->lower == "max" || expr->lower == "image" || expr->lower == "pos"
                || expr->lower == "succ" || expr->lower == "pred";
            Type* context = contextual ? prefixType : nullptr;
            if ((discreteFunction || adjacent || name == "min" || name == "max") && name != "val") {
                context = m_types.scalarBaseType(prefixType);
            }
            analyzeExpr(argument.get(), scope, context);
        }
    }

    if (discreteFunction || adjacent) {
        Expr* argument = expr->arguments.front().get();
        Type* argumentType = argument->type;
        bool valid = name == "val"
            ? argumentType != nullptr && (argumentType->kind == TypeKind::Integer
                || argumentType->kind == TypeKind::UniversalInteger) && representationVisible(argumentType)
            : argumentType != nullptr && typesCompatible(prefixType, argumentType);
        if (!valid) {
            m_diagnostics.error(argument->location, "'" + expr->name
                + (name == "val" ? " requires an integer argument" : " requires an argument of the prefix type"));
            return nullptr;
        }
        if (name != "val" && argument->isStatic) {
            Type* base = m_types.scalarBaseType(prefixType);
            __int128 integer = 0;
            StaticFailure failure = StaticFailure::None;
            bool inRange = isReal(base)
                ? argument->staticReal >= floatLowBound(base) && argument->staticReal <= floatHighBound(base)
                : foldWideInteger(argument, integer, failure) && integer >= base->low && integer <= base->high;
            if (!inRange) {
                expr->staticFailure = StaticFailure::AttributeRange;
            }
        }
    }

    bool staticArrayPrefix = prefixIsType;
    if (expr->prefix->kind == ExprKind::Identifier && expr->prefix->m_implicitCall == nullptr) {
        Symbol* symbol = static_cast<IdentifierExpr*>(expr->prefix.get())->symbol;
        staticArrayPrefix = staticArrayPrefix || (symbol != nullptr
            && (symbol->kind == SymbolKind::Object || symbol->kind == SymbolKind::Parameter));
    }

    // Subtypes carry their own constraint, so the prefix type is used as written.
    Type* base = prefixType;
    if (base != nullptr && base->kind == TypeKind::Fixed) {
        if (name == "fore" || name == "aft") {
            if (!prefixIsType || !expr->arguments.empty()) {
                m_diagnostics.error(expr->location, "fixed-point formatting attributes require a subtype and no arguments");
            }
            expr->type = m_types.universalInteger();
            expr->isStatic = !base->m_formalFixed;
            expr->staticValue = base->m_fixedAft;
            if (name == "fore") {
                __int128 low = base->low, high = base->high;
                if (low < 0) {
                    low = -low;
                }
                if (high < 0) {
                    high = -high;
                }
                __int128 whole = std::max(low, high) >> base->m_fixedBits;
                expr->staticValue = 2;
                while (whole >= 10) {
                    whole /= 10;
                    ++expr->staticValue;
                }
            }
            return expr->type;
        }
        if (name == "small" || name == "delta") {
            if (!prefixIsType || !expr->arguments.empty()) {
                m_diagnostics.error(expr->location, "fixed-point scale attributes require a subtype and no arguments");
            }
            expr->type = m_types.universalReal();
            if (base->m_formalFixed) {
                return expr->type;
            }
            expr->isStatic = true;
            expr->m_exactReal = ExactReal::make(1, (__int128)1 << base->m_fixedBits);
            if (name == "delta" && base->m_delta != nullptr) {
                exactValue(base->m_delta, expr->m_exactReal);
            }
            expr->staticReal = static_cast<double>(expr->m_exactReal.m_numerator)
                / static_cast<double>(expr->m_exactReal.m_denominator);
            return expr->type;
        }
        if (name == "first" || name == "last") {
            expr->type = prefixType;
            expr->isStatic = !base->m_formalFixed;
            if (expr->isStatic) {
                expr->m_exactReal = ExactReal::make(name == "first" ? prefixType->low : prefixType->high);
                expr->m_exactReal.integer(expr->staticValue);
            }
            return expr->type;
        }
        if (name != "base" && name != "size" && name != "address"
            && name != "image" && name != "value" && name != "succ" && name != "pred") {
            m_diagnostics.error(expr->location, "this attribute is not yet supported for fixed-point types");
            return nullptr;
        }
    }

    if (base != nullptr && base->kind == TypeKind::Array
        && (name == "first" || name == "last" || name == "length")) {
        if (prefixIsType && !base->constrained && base->m_boundsSymbol == nullptr) {
            m_diagnostics.error(expr->location, "array bound attributes require an object or a constrained array subtype");
            return nullptr;
        }
        long long dimension = 1;
        Type* dimensionType = expr->arguments.empty() ? nullptr : rootType(expr->arguments.front()->type);
        if (expr->arguments.size() > 1
            || (!expr->arguments.empty() && (!foldStatic(expr->arguments.front().get(), dimension)
                || dimensionType == nullptr
                || (dimensionType->kind != TypeKind::Integer && dimensionType->kind != TypeKind::UniversalInteger)))
            || dimension < 1 || dimension > base->arrayRank) {
            m_diagnostics.error(expr->location, "array dimension must be a static value within the array rank");
            return nullptr;
        }
        for (long long i = 1; i < dimension; ++i) {
            base = base->element;
        }
        expr->prefixType = base;
    }

    if (name == "read" || name == "write" || name == "input" || name == "output") {
        if (hasIncompleteParts(base)) {
            m_deferredProtectedUses.push_back({base, expr->location});
        }
        if (hasProtected(base)) {
            m_diagnostics.error(expr->location, "protected objects do not support streaming");
            return nullptr;
        }
        if (hasTasks(base)) {
            m_diagnostics.error(expr->location, "types containing tasks do not support streaming yet");
            return nullptr;
        }
        if (base != nullptr && base->m_classRoot != nullptr) {
            m_diagnostics.error(expr->location, "class-wide streaming requires tag-aware stream support");
            return nullptr;
        }
        if (needsFinalization(base)) {
            m_diagnostics.error(expr->location, "controlled streaming is not yet supported");
            return nullptr;
        }
        if (base != nullptr && base->m_scalarBoundsSymbol != nullptr) {
            m_diagnostics.error(expr->location, "stream attributes for runtime scalar subtypes are not yet supported");
            return nullptr;
        }
        if (base != nullptr && base->m_boundsSymbol != nullptr) {
            m_diagnostics.error(expr->location, "stream attributes for runtime-constrained array subtypes are not yet supported");
            return nullptr;
        }
        if (base != nullptr && base->kind == TypeKind::Array && base->arrayRank > 1 && !base->constrained) {
            m_diagnostics.error(expr->location, "stream attributes for unconstrained multidimensional arrays are not yet supported");
            return nullptr;
        }
        bool reads = name == "read" || name == "input";
        bool yieldsValue = name == "input";
        std::size_t wanted = yieldsValue ? 1u : 2u;

        if (!prefixIsType) {
            m_diagnostics.error(expr->location, "'" + expr->name + "' applies to a type");
            return nullptr;
        }
        if (expr->arguments.size() != wanted) {
            m_diagnostics.error(expr->location,
                                "'" + expr->name + "' expects "
                                    + (yieldsValue ? "a stream" : "a stream and an item"));
            return nullptr;
        }
        if (expr->arguments[0]->type == nullptr
            || baseType(expr->arguments[0]->type)->kind != TypeKind::Access) {
            m_diagnostics.error(expr->arguments[0]->location, "'" + expr->name + "' expects a stream access");
            return nullptr;
        }
        if (yieldsValue) {
            m_sharedEscapes.push_back(expr->prefix.get());
            expr->type = base;
            return expr->type;
        }

        m_sharedEscapes.push_back(expr->prefix.get());
        Expr* item = expr->arguments[1].get();
        m_sharedEscapes.push_back(item);
        if (item->type == nullptr) {
            analyzeExpr(item, scope, base);
        }
        // A literal takes the type the attribute names, so that it occupies
        // the width that type does rather than a universal one.
        adaptUniversal(item, base);
        if (reads) {
            checkAssignable(item, scope);
        }
        if (!typesCompatible(base, expr->arguments[1]->type)) {
            m_diagnostics.error(expr->arguments[1]->location,
                                "the item does not have type '" + (base != nullptr ? base->name : "") + "'");
        }
        expr->type = m_types.voidType();
        return expr->type;
    }

    if ((name == "min" || name == "max") && base != nullptr && base->m_modulus != 0) {
        if (!prefixIsType || !representationVisible(base) || expr->arguments.size() != 2) {
            m_diagnostics.error(expr->location, "Min and Max require a modular subtype and two arguments");
            return nullptr;
        }
        expr->type = m_types.scalarBaseType(prefixType);
        __int128 left = 0, right = 0;
        StaticFailure failure = StaticFailure::None;
        for (auto& argument : expr->arguments) {
            adaptUniversal(argument.get(), expr->type);
            if (!typesCompatible(expr->type, argument->type)) {
                m_diagnostics.error(argument->location, "Min and Max arguments must have the prefix type");
            }
        }
        if (foldWideInteger(expr->arguments[0].get(), left, failure)
            && foldWideInteger(expr->arguments[1].get(), right, failure)) {
            expr->isStatic = true;
            expr->m_exactReal = ExactReal::make(name == "min" ? std::min(left, right) : std::max(left, right));
            expr->m_exactReal.integer(expr->staticValue);
        }
        return expr->type;
    }
    if (name == "mod") {
        if (!prefixIsType || base == nullptr || !representationVisible(base) || base->m_modulus == 0 || expr->arguments.size() != 1) {
            m_diagnostics.error(expr->location, "Mod requires a modular subtype prefix and one integer argument");
            return nullptr;
        }
        Expr* argument = expr->arguments.front().get();
        if (!isDiscrete(argument->type) || argument->type->kind == TypeKind::Enumeration) {
            m_diagnostics.error(argument->location, "Mod requires an integer argument");
        }
        expr->type = m_types.scalarBaseType(prefixType);
        __int128 value = 0;
        StaticFailure failure = StaticFailure::None;
        if (foldWideInteger(argument, value, failure)) {
            value %= base->m_modulus;
            if (value < 0) { value += base->m_modulus; }
            expr->isStatic = true;
            expr->m_exactReal = ExactReal::make(value);
            expr->m_exactReal.integer(expr->staticValue);
        }
        return expr->type;
    }

    if (name == "modulus") {
        if (!prefixIsType || base == nullptr || !representationVisible(base) || base->m_modulus == 0 || !expr->arguments.empty()) {
            m_diagnostics.error(expr->location, "Modulus requires a modular type prefix and no arguments");
        }
        expr->type = m_types.universalInteger();
        expr->isStatic = true;
        expr->m_exactReal = ExactReal::make(base != nullptr ? base->m_modulus : 0);
        expr->m_exactReal.integer(expr->staticValue);
        return expr->type;
    }

    if (name == "first" || name == "last") {
        if (base != nullptr && base->kind == TypeKind::Array) {
            expr->type = base->index != nullptr ? m_types.scalarBaseType(base->index) : m_types.integerType();
            if (base->constrained && staticArrayPrefix) {
                expr->isStatic = true;
                expr->staticValue = name == "first" ? base->indexLow : base->indexHigh;
            }
            return expr->type;
        }
        if (isDiscrete(base)) {
            expr->type = prefixType;
            expr->isStatic = prefixType->m_scalarBoundsSymbol == nullptr;
            if (expr->isStatic) {
                expr->m_exactReal = ExactReal::make(name == "first" ? prefixType->low : prefixType->high);
                expr->m_exactReal.integer(expr->staticValue);
            }
            return expr->type;
        }
        if (base != nullptr && base->kind == TypeKind::Float) {
            expr->type = prefixType;
            expr->isStatic = true;
            expr->staticReal = name == "first" ? floatLowBound(base) : floatHighBound(base);
            return expr->type;
        }
        m_diagnostics.error(expr->location, "'" + expr->name + " requires an array or scalar prefix");
        return nullptr;
    }

    if (name == "digits") {
        if (base == nullptr || base->kind != TypeKind::Float) {
            m_diagnostics.error(expr->location, "'Digits requires a floating point prefix");
            return nullptr;
        }
        expr->type = m_types.integerType();
        expr->isStatic = true;
        expr->staticValue = base->digits;
        return expr->type;
    }

    if (name == "length") {
        expr->type = expected != nullptr && expected->kind == TypeKind::Integer ? expected : m_types.universalInteger();
        if (base != nullptr && base->kind == TypeKind::Array && base->constrained && staticArrayPrefix) {
            expr->isStatic = true;
            expr->staticValue = arrayLength(base);
        }
        return expr->type;
    }

    if (name == "pos") {
        if (expr->arguments.size() != 1) {
            m_diagnostics.error(expr->location, "'Pos takes exactly one argument");
            return nullptr;
        }
        adaptUniversal(expr->arguments.front().get(), prefixType);
        expr->type = m_types.universalInteger();
        expr->isStatic = expr->arguments.front()->isStatic;
        expr->staticValue = expr->arguments.front()->staticValue;
        expr->m_exactReal = expr->arguments.front()->m_exactReal;
        if (expr->arguments.front()->staticFailure != StaticFailure::None) {
            expr->staticFailure = StaticFailure::Dependent;
        }
        return expr->type;
    }

    if (name == "val" || name == "succ" || name == "pred") {
        if (expr->arguments.size() != 1) {
            m_diagnostics.error(expr->location, "'" + expr->name + " takes exactly one argument");
            return nullptr;
        }
        Type* resultType = m_types.scalarBaseType(prefixType);
        Type* argumentType = name == "val" ? nullptr : resultType;
        if (argumentType != nullptr) {
            adaptUniversal(expr->arguments.front().get(), argumentType);
        }
        expr->type = resultType;
        if (expr->arguments.front()->staticFailure != StaticFailure::None) {
            expr->staticFailure = StaticFailure::Dependent;
        }
        if (expr->staticFailure != StaticFailure::None) {
            expr->isStatic = true;
            return expr->type;
        }
        if (expr->arguments.front()->isStatic && isReal(resultType)) {
            double argument = expr->arguments.front()->staticReal;
            double direction = name == "succ" ? std::numeric_limits<double>::infinity()
                : -std::numeric_limits<double>::infinity();
            double value = typeSize(resultType) == 4
                ? std::nextafter(static_cast<float>(argument), static_cast<float>(direction))
                : std::nextafter(argument, direction);
            if (std::isfinite(value)) {
                expr->isStatic = true;
                expr->staticReal = value;
            } else {
                expr->isStatic = true;
                expr->staticFailure = StaticFailure::Check;
            }
        }
        if (expr->arguments.front()->isStatic && !isReal(resultType)) {
            __int128 value = 0;
            StaticFailure failure = StaticFailure::None;
            expr->isStatic = true;
            if (!foldWideInteger(expr->arguments.front().get(), value, failure)) {
                expr->staticFailure = StaticFailure::Dependent;
            } else {
                if (value < resultType->low || value > resultType->high) {
                    expr->staticFailure = StaticFailure::Check;
                    return expr->type;
                }
                value += name == "succ" ? 1 : name == "pred" ? -1 : 0;
                if (resultType->m_modulus != 0 && name != "val") {
                    value = (value + resultType->m_modulus) % resultType->m_modulus;
                }
                if (value < resultType->low || value > resultType->high) {
                    expr->staticFailure = StaticFailure::Check;
                } else {
                    expr->staticValue = static_cast<long long>(value);
                    expr->m_exactReal = ExactReal::make(value);
                }
            }
        }
        return expr->type;
    }

    if (name == "image") {
        if (expr->arguments.size() != 1) {
            m_diagnostics.error(expr->location, "'Image takes exactly one argument");
            return nullptr;
        }
        adaptUniversal(expr->arguments.front().get(), prefixType);
        if (base != nullptr && base->kind == TypeKind::Fixed
            && (!prefixIsType || !typesCompatible(prefixType, expr->arguments.front()->type))) {
            m_diagnostics.error(expr->location, "fixed-point 'Image requires an argument of the prefix type");
        }
        expr->type = m_types.stringType();
        return expr->type;
    }

    // Where an object is and how much room it takes.  Together they let a
    // generic body hand a value to the run time without either side having to
    // know what shape it has.
    if (name == "address") {
        m_sharedEscapes.push_back(expr->prefix.get());
        if (prefixIsType) {
            m_diagnostics.error(expr->location, "'Address requires an object, not a type");
            return nullptr;
        }
        expr->type = m_addressType;
        return expr->type;
    }

    if (name == "size") {
        if (prefixType == nullptr) {
            m_diagnostics.error(expr->location, "'Size requires a type or an object");
            return nullptr;
        }
        if (prefixIsType && prefixType->m_boundsSymbol != nullptr) {
            m_diagnostics.error(expr->location, "'Size for runtime-constrained array subtypes is not yet supported");
            return nullptr;
        }
        expr->type = expected != nullptr && expected->kind == TypeKind::Integer ? expected : m_types.universalInteger();
        if (prefixType->m_classRoot != nullptr) {
            if (prefixIsType) {
                m_diagnostics.error(expr->location, "class-wide subtype 'Size is not yet supported");
            }
            return expr->type;
        }
        if (prefixType->kind == TypeKind::Array && !prefixType->constrained) {
            if (prefixIsType) {
                m_diagnostics.error(expr->location, "'Size for unconstrained array subtypes requires an object prefix");
            }
            return expr->type;
        }
        expr->isStatic = true;
        if (__builtin_mul_overflow(typeSize(prefixType), 8LL, &expr->staticValue)) {
            m_diagnostics.error(expr->location, "'Size exceeds signed 64-bit result capacity");
        }
        return expr->type;
    }

    // How wide the longest image of the type is, which is what a generic body
    // uses to lay a value out in a column without knowing the type.
    if (name == "width") {
        if (!isDiscrete(base)) {
            m_diagnostics.error(expr->location, "'Width requires a discrete prefix");
            return nullptr;
        }
        expr->type = m_types.integerType();
        expr->isStatic = true;
        if (prefixType->m_scalarBoundsSymbol != nullptr) {
            expr->isStatic = false;
            m_diagnostics.error(expr->location, "'Width for runtime scalar subtypes is not yet supported");
            return nullptr;
        }
        expr->staticValue = widthOf(prefixType);
        return expr->type;
    }

    // The other way round from 'Image: the value a string spells out.
    if (name == "value") {
        if (expr->arguments.size() != 1) {
            m_diagnostics.error(expr->location, "'Value takes exactly one argument");
            return nullptr;
        }
        if (!isDiscrete(base) && (base == nullptr || (base->kind != TypeKind::Fixed && base->kind != TypeKind::Float))) {
            m_diagnostics.error(expr->location, "'Value requires a scalar prefix");
            return nullptr;
        }
        Type* argumentType = expr->arguments.front()->type;
        if (argumentType != nullptr && !typesCompatible(m_types.stringType(), argumentType)) {
            m_diagnostics.error(expr->location, "'Value reads its value from a string");
        }
        if (base != nullptr && (base->kind == TypeKind::Fixed || base->kind == TypeKind::Float) && !prefixIsType) {
            m_diagnostics.error(expr->location, base->kind == TypeKind::Fixed
                ? "fixed-point 'Value requires a subtype prefix" : "floating-point 'Value requires a subtype prefix");
        }
        expr->type = base != nullptr && (base->m_modulus != 0 || base->kind == TypeKind::Fixed || base->kind == TypeKind::Float)
            ? m_types.scalarBaseType(prefixType) : prefixType;
        return expr->type;
    }

    m_diagnostics.error(expr->location, "unsupported attribute '" + expr->name + "'");
    return nullptr;
}
