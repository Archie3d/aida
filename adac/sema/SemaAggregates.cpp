#include "Sema.h"
#include "SemaSupport.h"

#include <algorithm>
#include <utility>
#include <unordered_map>

using SemaSupport::adaptUniversal;

Type* Sema::analyzeAggregate(AggregateExpr* expr, Scope* scope, Type* expected)
{
    if (expected != nullptr && expected->classRoot != nullptr) {
        m_diagnostics.error(expr->location, "an aggregate requires a specific type");
        return nullptr;
    }
    Type* target = expected != nullptr && expected->tagged ? rootType(expected) : expected;
    if (target == nullptr || (target->kind != TypeKind::Array && target->kind != TypeKind::Record)) {
        m_diagnostics.error(expr->location, "an aggregate needs a known array or record type");
        for (AggregateComponent& component : expr->components) {
            analyzeExpr(component.value.get(), scope, nullptr);
        }
        return nullptr;
    }
    if (hasIncompleteParts(target)) {
        m_deferredProtectedUses.push_back({target, expr->location});
    }
    if (hasProtected(target)) {
        m_diagnostics.error(expr->location, "aggregates containing protected objects are not yet supported");
        return nullptr;
    }
    if (hasTasks(target)) {
        m_diagnostics.error(expr->location, "aggregates containing tasks are not yet supported");
        return nullptr;
    }
    if (target->abstract) {
        m_diagnostics.error(expr->location, "abstract aggregates are not supported");
        return nullptr;
    }
    if (!checkNotPrivate(target, expr->location, "an aggregate spells out the components")) {
        return nullptr;
    }

    std::size_t firstField = 0;
    if (expr->ancestor != nullptr) {
        Type* extension = rootType(target);
        if (!extension->tagged || extension->parentType == nullptr) {
            m_diagnostics.error(expr->location, "an extension aggregate requires a record extension type");
            return nullptr;
        }
        for (Symbol* candidate : expressionNames(expr->ancestor.get(), scope)) {
            if (candidate->kind == SymbolKind::TypeName) {
                expr->ancestorType = candidate->type;
                expr->ancestorIsType = true;
                recordContractName(expr->ancestor.get(), candidate);
                break;
            }
        }
        if (!expr->ancestorIsType) {
            for (Type* candidate : expressionTypes(expr->ancestor.get(), scope)) {
                if (isTaggedAncestor(candidate, extension) && rootType(candidate) != extension) {
                    if (expr->ancestorType != nullptr && rootType(expr->ancestorType) != rootType(candidate)) {
                        m_diagnostics.error(expr->location, "ambiguous ancestor expression in extension aggregate");
                        return nullptr;
                    }
                    expr->ancestorType = candidate;
                }
            }
        }
        if (!isTaggedAncestor(expr->ancestorType, extension) || rootType(expr->ancestorType) == extension) {
            m_diagnostics.error(expr->location, "extension aggregate ancestor must be a proper tagged ancestor of the result type");
            return nullptr;
        }
        if (!expr->ancestorIsType) {
            analyzeExpr(expr->ancestor.get(), scope, expr->ancestorType);
        }
        firstField = expr->ancestorType->fields.size();
    } else if (target->tagged && rootType(target)->parentType != nullptr) {
        m_diagnostics.error(expr->location, "a record extension requires an extension aggregate");
        return nullptr;
    }
    if (target->kind != TypeKind::Record && expr->nullRecord) {
        m_diagnostics.error(expr->location, "null record requires a record aggregate");
        return nullptr;
    }
    if (target->kind == TypeKind::Record) {
        // A record with a variant part carries the components of one
        // alternative and no other, so which one it is has to be settled before
        // the aggregate can be read.
        Type* record = baseType(target);
        std::vector<bool> active(record->variants.size(), false);
        bool unknown = false;
        for (std::size_t i = 0; i < record->variants.size(); ++i) {
            const VariantInfo& info = record->variants[i];
            if (info.parent >= 0 && !active[info.parent]) {
                continue;
            }
            long long chosen = 0;
            if (!discriminantValue(target, info.discriminant, chosen)
                && !aggregateDiscriminant(expr, record, scope, info.discriminant, chosen)) {
                unknown = true;
                break;
            }
            active[i] = variantFor(record, chosen, info.parent) == static_cast<int>(i);
        }
        if (unknown) {
            m_diagnostics.error(expr->location, "an aggregate for '" + baseType(target)->name
                                                    + "' needs its discriminant fixed, as in '" + baseType(target)->name
                                                    + " (...)', so that its components are known");
            for (AggregateComponent& component : expr->components) {
                analyzeExpr(component.value.get(), scope, nullptr);
            }
            return nullptr;
        }

        // The aggregate says what the discriminant is too, and it has to be the
        // one the subtype fixed: otherwise the components it goes on to name
        // are those of a different value than the one being made.
        if (!checkAggregateDiscriminants(expr, target, scope)) {
            for (AggregateComponent& component : expr->components) {
                analyzeExpr(component.value.get(), scope, nullptr);
            }
            return nullptr;
        }

        auto present = [&](const FieldInfo& field) {
            return static_cast<std::size_t>(field.index) >= firstField && (field.variant < 0 || active[field.variant]);
        };
        if (!expr->nullRecord && std::none_of(target->fields.begin(), target->fields.end(), present)) {
            m_diagnostics.error(expr->location, "an aggregate with no needed record components requires null record");
        }
        // A grouped association has one expected type, but its expression is
        // evaluated separately for each field by the emitter.
        std::unordered_map<Expr*, Type*> associationTypes;
        auto analyzeComponent = [&](Expr* value, Type* fieldType) {
            // A box has no common expected type: every selected component
            // uses its own declaration's default or subtype initialization.
            if (value->kind == ExprKind::Box) {
                return;
            }
            auto [entry, inserted] = associationTypes.emplace(value, fieldType);
            if (!inserted) {
                if (baseType(entry->second) != baseType(fieldType)) {
                    m_diagnostics.error(value->location,
                        "components in one record association must have the same type");
                }
                return;
            }
            Type* valueType = analyzeExpr(value, scope, fieldType);
            if (hasLimitedControlledParts(fieldType) && !buildsLimitedResult(value)) {
                m_diagnostics.error(value->location, "limited controlled components cannot be copied");
            }
            if (!typesCompatible(fieldType, valueType)) {
                m_diagnostics.error(value->location, "the record component has an incompatible type");
            }
            adaptUniversal(value, fieldType);
        };
        bool complained = false;

        expr->resolvedFields.assign(target->fields.size(), nullptr);
        std::size_t positional = firstField;
        bool hasNamed = false;
        bool hasOthers = false;
        for (AggregateComponent& component : expr->components) {
            if (component.isOthers) {
                if (hasOthers || &component != &expr->components.back()) {
                    m_diagnostics.error(component.value->location,
                        "others must appear once, as the final aggregate association");
                }
                hasOthers = true;
                hasNamed = true;
                bool associated = false;
                for (std::size_t i = 0; i < target->fields.size(); ++i) {
                    if (expr->resolvedFields[i] == nullptr && present(target->fields[i])) {
                        associated = true;
                        analyzeComponent(component.value.get(), target->fields[i].type);
                        expr->resolvedFields[i] = component.value.get();
                    }
                }
                if (!associated && component.value->kind != ExprKind::Box) {
                    m_diagnostics.error(component.value->location, "record association has no components");
                }
                continue;
            }
            if (component.choiceLows.empty()) {
                if (hasNamed) {
                    m_diagnostics.error(component.value->location,
                        "positional record components must precede named associations");
                    continue;
                }
                while (positional < target->fields.size() && !present(target->fields[positional])) {
                    ++positional;
                }
                if (positional >= target->fields.size()) {
                    m_diagnostics.error(component.value->location, "too many components in record aggregate");
                    break;
                }
                Type* fieldType = target->fields[positional].type;
                analyzeComponent(component.value.get(), fieldType);
                expr->resolvedFields[positional] = component.value.get();
                ++positional;
                continue;
            }
            hasNamed = true;
            if (std::any_of(component.names.begin(), component.names.end(),
                    [](const std::string& name) { return name.empty(); })) {
                m_diagnostics.error(component.value->location,
                    "record aggregate choices must be component names");
                continue;
            }
            for (const std::string& fieldName : component.names) {
                bool found = false;
                for (const FieldInfo& field : target->fields) {
                    if (field.name != fieldName) {
                        continue;
                    }
                    found = true;
                    if (static_cast<std::size_t>(field.index) < firstField) {
                        m_diagnostics.error(component.value->location, "component '" + field.displayName
                            + "' is already supplied by the ancestor part");
                        break;
                    }
                    if (expr->resolvedFields[field.index] != nullptr) {
                        m_diagnostics.error(component.value->location, "duplicate record aggregate component '" + field.displayName + "'");
                        break;
                    }
                    if (!present(field)) {
                        // One message for the aggregate rather than one for
                        // every component of the alternative it named.
                        if (!complained) {
                            m_diagnostics.error(component.value->location,
                                                "'" + field.displayName + "' belongs to another variant of '"
                                                    + baseType(target)->name + "' than this one");
                            complained = true;
                        }
                        break;
                    }
                    analyzeComponent(component.value.get(), field.type);
                    expr->resolvedFields[field.index] = component.value.get();
                    break;
                }
                if (!found) {
                    m_diagnostics.error(component.value->location,
                                        "'" + fieldName + "' is not a component of '" + target->name + "'");
                }
            }
        }

        for (std::size_t i = 0; i < expr->resolvedFields.size(); ++i) {
            Expr* value = expr->resolvedFields[i];
            const FieldInfo& field = target->fields[i];
            if (value == nullptr || value->kind != ExprKind::Box || !field.isDiscriminant) {
                continue;
            }
            if (field.defaultValue == nullptr) {
                m_diagnostics.error(value->location, "a discriminant without a default requires an expression");
            }
            long long defaultValue = 0;
            if (std::any_of(record->variants.begin(), record->variants.end(), [&](const VariantInfo& variant) {
                    return variant.discriminant == field.index;
                }) && (field.defaultValue == nullptr || !foldStatic(field.defaultValue, defaultValue))) {
                m_diagnostics.error(value->location, "a discriminant governing a variant requires a static expression");
            }
        }
        for (std::size_t i = 0; i < expr->resolvedFields.size() && !complained; ++i) {
            if (expr->resolvedFields[i] == nullptr && present(target->fields[i])) {
                m_diagnostics.error(expr->location, "component '" + target->fields[i].displayName
                                                        + "' has no value in the aggregate");
            }
        }
        expr->type = expected;
        return expr->type;
    }

    bool hasNamed = false;
    bool hasPositional = false;
    bool hasOthers = false;
    std::vector<std::pair<long long, long long>> staticChoices;
    for (AggregateComponent& component : expr->components) {
        for (std::size_t i = 0; i < component.choiceLows.size(); ++i) {
            analyzeExpr(component.choiceLows[i].get(), scope, target->index);
            adaptUniversal(component.choiceLows[i].get(),
                           target->index != nullptr ? target->index : m_types.integerType());
            if (component.choiceHighs[i]) {
                analyzeExpr(component.choiceHighs[i].get(), scope, target->index);
                adaptUniversal(component.choiceHighs[i].get(),
                               target->index != nullptr ? target->index : m_types.integerType());
            }
        }
        if (component.value->kind == ExprKind::Box && target->arrayRank == 1) {
            continue;
        }
        bool invalidRow = target->arrayRank > 1 && component.value->kind != ExprKind::Aggregate
            && !(target->arrayRank == 2 && component.value->kind == ExprKind::StringLiteral);
        if (invalidRow) {
            m_diagnostics.error(component.value->location, "multidimensional aggregate requires nested subaggregates");
            if (component.value->kind == ExprKind::Box) {
                continue;
            }
        }
        Type* valueType = analyzeExpr(component.value.get(), scope, target->element);
        if (hasLimitedControlledParts(target->element) && !buildsLimitedResult(component.value.get())) {
            m_diagnostics.error(component.value->location, "limited controlled components cannot be copied");
        }
        if (!invalidRow && !typesCompatible(target->element, valueType)) {
            m_diagnostics.error(component.value->location, "aggregate component has an incompatible type");
        }
        adaptUniversal(component.value.get(), target->element);
    }
    for (std::size_t c = 0; c < expr->components.size(); ++c) {
        AggregateComponent& component = expr->components[c];
        if (component.isOthers) {
            if (hasOthers || c + 1 != expr->components.size()) {
                m_diagnostics.error(component.value->location, "others must appear once, as the final aggregate association");
            }
            hasOthers = true;
            continue;
        }
        if (component.choiceLows.empty()) {
            hasPositional = true;
            continue;
        }
        hasNamed = true;
        for (std::size_t i = 0; i < component.choiceLows.size(); ++i) {
            long long low = 0;
            long long high = 0;
            bool known = foldStatic(component.choiceLows[i].get(), low);
            if (component.choiceHighs[i]) {
                known = foldStatic(component.choiceHighs[i].get(), high) && known;
            } else {
                high = low;
            }
            if (!isDiscrete(component.choiceLows[i]->type)
                || !typesCompatible(target->index, component.choiceLows[i]->type)
                || (component.choiceHighs[i] && (!isDiscrete(component.choiceHighs[i]->type)
                    || !typesCompatible(target->index, component.choiceHighs[i]->type)))) {
                m_diagnostics.error(component.choiceLows[i]->location, "aggregate choice is incompatible with the index type");
            }
            if ((!known || high < low)
                && (expr->components.size() != 1 || component.choiceLows.size() != 1)) {
                m_diagnostics.error(component.choiceLows[i]->location,
                    "a dynamic or null aggregate choice must be the only choice");
            }
            if (known) {
                component.choiceLows[i]->staticValue = low;
                if (component.choiceHighs[i]) {
                    component.choiceHighs[i]->staticValue = high;
                }
                if (high >= low) {
                    staticChoices.emplace_back(low, high);
                }
            }
        }
    }
    if (hasNamed && hasPositional) {
        m_diagnostics.error(expr->location, "positional and named array associations cannot be mixed");
    }
    std::sort(staticChoices.begin(), staticChoices.end());
    for (std::size_t i = 1; i < staticChoices.size(); ++i) {
        if (staticChoices[i].first <= staticChoices[i - 1].second) {
            m_diagnostics.error(expr->location, "array aggregate choices overlap");
            break;
        }
        if (!hasOthers && staticChoices[i].first - 1 != staticChoices[i - 1].second) {
            m_diagnostics.error(expr->location, "array aggregate choices must cover a contiguous range");
            break;
        }
    }
    expr->type = expected;
    return expr->type;
}

// What a record aggregate writes for one of the discriminants, if it writes
// anything static there.  Discriminants come first among the components, so a
// positional aggregate names them by being long enough.
bool Sema::aggregateDiscriminant(AggregateExpr* expr, Type* record, Scope* scope, int index,
                                 long long& value, Expr** source)
{
    std::size_t positional = 0;

    for (AggregateComponent& component : expr->components) {
        if (component.isOthers) {
            if (component.value->kind == ExprKind::Box) {
                if (source != nullptr) { *source = component.value.get(); }
                Expr* defaultValue = record->fields[static_cast<std::size_t>(index)].defaultValue;
                return defaultValue != nullptr && foldStatic(defaultValue, value);
            }
            break;
        }

        int which = -1;
        if (component.names.empty() || component.names.front().empty()) {
            which = static_cast<int>(positional++);
        } else {
            for (const FieldInfo& field : record->fields) {
                if (field.name == component.names.front()) {
                    which = field.index;
                    break;
                }
            }
        }
        if (which != index) {
            continue;
        }
        Expr* expression = component.value.get();
        if (source != nullptr) { *source = expression; }
        if (expression->kind == ExprKind::Box) {
            expression = record->fields[static_cast<std::size_t>(index)].defaultValue;
            return expression != nullptr && foldStatic(expression, value);
        }
        analyzeExpr(expression, scope, record->fields[static_cast<std::size_t>(index)].type);
        return foldStatic(component.value.get(), value);
    }
    return false;
}

// A record aggregate names the discriminants along with everything else, and
// what it names them has to be what the subtype fixed them to.  Answers whether
// the aggregate is worth reading any further.
bool Sema::checkAggregateDiscriminants(AggregateExpr* expr, Type* target, Scope* scope)
{
    Type* record = baseType(target);

    for (int which = 0; which < record->discriminantCount; ++which) {
        long long fixed = 0;
        long long given = 0;
        Expr* source = nullptr;
        if (!discriminantValue(target, which, fixed)
            || !aggregateDiscriminant(expr, record, scope, which, given, &source) || given == fixed) {
            continue;
        }
        const FieldInfo& discriminant = record->fields[static_cast<std::size_t>(which)];
        m_diagnostics.error(source->location, "'" + discriminant.displayName + "' was fixed at "
                                                  + describeValue(discriminant.type, fixed)
                                                  + " when the object was declared");
        return false;
    }
    return true;
}
