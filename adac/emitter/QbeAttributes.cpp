#include "QbeEmitter.h"
#include "QbeSupport.h"

#include <algorithm>
#include <cfloat>

using QbeSupport::constantValue;
using QbeSupport::isFloatClass;
using QbeSupport::isUnconstrainedArray;

Value QbeEmitter::emitAttribute(AttributeExpr* expr)
{
    const std::string& name = expr->lower;
    Type* prefixType = expr->prefixType;
    if (name == "constrained") {
        Value object = emitExpr(expr->prefix.get());
        return recordConstrained(expr->prefix.get(), &object);
    }
    // Protected entry bodies execute in the calling task. Current_Task in an
    // entry body uses this same identity (the RM C.7.1 bounded-error choice).
    if (name == "width") {
        Value bounds = scalarBounds(prefixType);
        Value result {newTemp(), 'w'};
        if (prefixType->kind == TypeKind::Enumeration) {
            if (prefixType->literals.empty()) {
                std::string nonNull = newTemp();
                line(nonNull + " =w csgew " + bounds.last + ", " + bounds.first);
                line(result.name + " =w mul " + nonNull + ", 3");
            } else {
                emitCallInstruction(result.name + " =w call $__ada_width_enum(w " + bounds.first
                    + ", w " + bounds.last + ", l " + enumTableFor(prefixType) + ", w "
                    + std::to_string(prefixType->literals.size()) + ")", CallEffects::CheckException);
            }
        } else {
            if (bounds.type == 'w') {
                std::string low = newTemp(), high = newTemp();
                std::string extend = prefixType->modulus != 0 ? "extuw " : "extsw ";
                line(low + " =l " + extend + bounds.first);
                line(high + " =l " + extend + bounds.last);
                bounds.first = low;
                bounds.last = high;
            }
            emitCallInstruction(result.name + " =w call $__ada_width_integer(l " + bounds.first
                + ", l " + bounds.last + ", w " + std::to_string(prefixType->modulus != 0) + ")",
                CallEffects::CheckException);
        }
        return result;
    }
    if (name == "caller") {
        std::string identity = newTemp();
        emitCallInstruction(identity + " =l call $__ada_current_task()", CallEffects::DeferredCheck);
        return Value { identity, 'l' };
    }
    if (expr->taskAttribute) {
        Value identity;
        if (expr->taskCurrentInstance != nullptr) {
            identity = loadFrom(addressOf(expr->taskCurrentInstance), expr->taskCurrentInstance->type);
        } else {
            Value object = emitExpr(expr->prefix.get());
            if (prefixType->kind == TypeKind::Access) {
                checkNotNull(object);
            }
            std::string result = newTemp();
            emitCallInstruction(result + " =l call $__ada_task_identity(l " + object.name + ")", CallEffects::CheckException);
            identity = Value { result, 'l' };
        }
        if (name == "identity") {
            return identity;
        }
        std::string result = newTemp();
        emitCallInstruction(result + " =w call $__ada_task_is_" + name + "(l " + identity.name + ")", CallEffects::CheckException);
        return Value { result, 'w' };
    }
    if (name == "count") {
        Value object = addressOf(expr->protectedReceiver);
        std::string count = newTemp();
        emitCallInstruction(count + " =w call $__ada_protected_count(l " + object.name
            + ", l " + expr->countEntry->qbeName + ".barrier)", CallEffects::DeferredCheck);
        if (qbeClass(expr->type) == 'l') {
            std::string wide = newTemp();
            line(wide + " =l extsw " + count);
            return Value { wide, 'l' };
        }
        return Value { count, 'w' };
    }
    if (name == "access" || name == "unchecked_access") {
        if (expr->objectAccess) {
            Value object = emitAddress(expr->prefix.get());
            // Keep library-level accessibility when forwarding through an
            // anonymous access parameter as well as for this attribute itself.
            if (name == "unchecked_access") { object.accessLevel = "0"; }
            else if (expr->accessObject != nullptr) { object.accessLevel = accessLevelFor(expr->accessObject); }
            else if (object.accessLevel.empty()) { object.accessLevel = currentAccessLevel(); }
            if (name != "unchecked_access" && !expr->type->accessParameter && !expr->type->accessDiscriminant
                && (expr->objectAccessLevel > expr->type->accessLevel || object.accessLevel[0] == '%')) {
                emitCallInstruction("call $__ada_accessibility_check(w " + object.accessLevel
                    + ", w " + accessLevelFor(expr->type) + ")", CallEffects::CheckException);
            }
            if (name != "unchecked_access" && expr->prefixType->tagged
                && !expr->type->accessParameter && !expr->type->accessDiscriminant) {
                emitCallInstruction("call $__ada_tag_check_accessibility(l " + object.name + ", w "
                    + std::to_string(expr->type->accessLevel) + ")", CallEffects::CheckException);
            }
            if (expr->type->target->kind == TypeKind::Array && !expr->type->target->constrained) {
                std::string descriptor = allocScratch(adaArrayLocalSize(expr->prefixType->arrayRank));
                line("storel " + object.name + ", " + descriptor);
                storeArrayView(descriptor, object, expr->prefixType, false);
                Value collection = collectionFor(expr->type);
                emitCallInstruction("call $__ada_array_borrow(l " + collection.name + ", l " + object.name
                    + ", l " + descriptor + ", w " + std::to_string(expr->prefixType->arrayRank)
                    + ", l " + stringData(rootType(expr->type->target)->accessIdentity) + ")", CallEffects::CheckException);
            }
            if (!expr->type->accessConstant && expr->type->target->kind == TypeKind::Record && hasDefaultDiscriminants(expr->type->target)) {
                Value constrained = recordConstrained(expr->prefix.get(), &object);
                Value collection = collectionFor(expr->type);
                emitCallInstruction("call $__ada_object_borrow(l " + collection.name + ", l " + object.name
                    + ", l " + stringData(rootType(expr->type->target)->accessIdentity) + ", w " + constrained.name + ")", CallEffects::CheckException);
            }
            return object;
        }
        Symbol* subprogram = expr->accessSubprogram;
        if (expr->type->accessProtected) {
            Value object;
            if (expr->protectedObject != nullptr) {
                object = expr->protectedObject->type->kind == TypeKind::Access
                    ? emitExpr(expr->protectedObject) : emitAddress(expr->protectedObject);
                checkNotNull(object);
            } else { object = addressOf(expr->protectedReceiver); }
            std::string level = newTemp();
            emitCallInstruction(level + " =w call $__ada_protected_access_level(l " + object.name + ")", CallEffects::CheckException);
            if (!expr->type->accessParameter) {
                emitCallInstruction("call $__ada_accessibility_check(w " + level + ", w " + accessLevelFor(expr->type) + ")", CallEffects::CheckException);
            }
            Value collection = collectionFor(expr->type);
            Value descriptor {newTemp(), 'l'};
            descriptor.accessLevel = level;
            emitCallInstruction(descriptor.name + " =l call $__ada_collection_allocate(l " + collection.name + ", l 24)", CallEffects::CheckException);
            line("storel " + subprogram->qbeName + ", " + descriptor.name);
            std::string link = subprogram->level > 0 ? staticLinkFor(subprogram->level - 1).name : "0";
            std::string linkSlot = newTemp(), objectSlot = newTemp();
            line(linkSlot + " =l add " + descriptor.name + ", 8");
            line("storel " + link + ", " + linkSlot);
            line(objectSlot + " =l add " + descriptor.name + ", 16");
            line("storel " + object.name + ", " + objectSlot);
            return descriptor;
        }
        if (subprogram->renamedAccess != nullptr) {
            Symbol* binding = subprogram->renamedAccess;
            return loadFrom(addressOf(binding), binding->type);
        }
        if (subprogram->builtin == BuiltinKind::Runtime) {
            std::string descriptor = subprogram->qbeName + ".import_access";
            if (m_importedAccessDescriptors.insert(subprogram).second) {
                m_data << "data " << descriptor << " = align 8 { l " << subprogram->runtimeSymbol << ", l 0 }\n";
            }
            if (expr->type->accessConventionC) {
                Value code {newTemp(), 'l'};
                line(code.name + " =l loadl " + descriptor);
                return code;
            }
            return Value {descriptor, 'l'};
        }
        if (subprogram->level == 0) {
            std::string descriptor = subprogram->qbeName + ".access";
            return Value { descriptor, 'l' };
        }
        Value frame = staticLinkFor(subprogram->level - 1);
        std::string descriptor = newTemp();
        line(descriptor + " =l add " + frame.name + ", " + std::to_string(subprogram->descriptorOffset));
        return Value { descriptor, 'l' };
    }
    if (name == "identity") {
        return Value { expr->exceptionSymbol->exceptionObject, 'l' };
    }

    if (name == "read" || name == "write" || name == "input" || name == "output") {
        return emitStreamAttribute(expr);
    }
    if (name == "round" && prefixType->kind == TypeKind::Fixed) {
        Expr* argument = expr->arguments[0].get();
        Value value = emitExpr(argument);
        std::string result = newTemp();
        if (argument->type->kind == TypeKind::Fixed) {
            emitCallInstruction(result + " =l call $__ada_fixed_rescale(l " + value.name + ", w "
                + std::to_string(argument->type->fixedScale) + ", w " + std::to_string(prefixType->fixedScale) + ")", CallEffects::DeferredCheck);
        } else {
            if (value.type == 's') {
                std::string wide = newTemp();
                line(wide + " =d exts " + value.name);
                value = Value { wide, 'd' };
            }
            emitCallInstruction(result + " =l call $__ada_float_to_fixed(d " + value.name + ", w " + std::to_string(prefixType->fixedScale) + ")", CallEffects::DeferredCheck);
        }
        emitExceptionCheck();
        return Value { result, 'l' };
    }
    if (name == "fore" && prefixType->kind == TypeKind::Fixed) {
        Value bounds = scalarBounds(prefixType);
        std::string result = newTemp();
        emitCallInstruction(result + " =l call $__ada_fixed_fore(l " + bounds.first + ", l " + bounds.last
            + ", w " + std::to_string(prefixType->fixedScale) + ")", CallEffects::DeferredCheck);
        if (qbeClass(expr->type) == 'w') {
            std::string narrow = newTemp();
            line(narrow + " =w copy " + result);
            result = narrow;
        }
        return Value { result, qbeClass(expr->type) };
    }
    if (name == "min" || name == "max") {
        Value left = emitExpr(expr->arguments[0].get());
        Value right = emitExpr(expr->arguments[1].get());
        std::string result = newTemp();
        Value a = wideBound(left), b = wideBound(right);
        emitCallInstruction(result + " =l call $" + (prefixType->kind == TypeKind::Fixed ? "__ada_fixed_minmax" : "__ada_modular_minmax") + "(l " + a.name + ", l " + b.name
            + ", w " + (name == "min" ? "0" : "1") + ")", CallEffects::DeferredCheck);
        if (qbeClass(expr->type) == 'w') {
            std::string narrow = newTemp();
            line(narrow + " =w copy " + result);
            result = narrow;
        }
        return Value { result, qbeClass(expr->type) };
    }
    if (name == "mod") {
        Expr* argument = expr->arguments.front().get();
        Value value = wideBound(emitExpr(argument));
        std::string result = newTemp();
        emitCallInstruction(result + " =l call $__ada_modular_reduce(l " + value.name + ", w "
            + (argument->type->modulus != 0 ? "1" : "0") + ", l " + scalarText(prefixType->modulus) + ")", CallEffects::DeferredCheck);
        if (qbeClass(expr->type) == 'w') {
            std::string narrow = newTemp();
            line(narrow + " =w copy " + result);
            result = narrow;
        }
        return Value { result, qbeClass(expr->type) };
    }
    if (name == "pos") {
        Value value = emitExpr(expr->arguments.front().get());
        if (prefixType->modulus != 0 && expr->type->kind != TypeKind::UniversalInteger) {
            return emitDiscreteConversion(value, prefixType, m_sema.typeTable().scalarBaseType(expr->type));
        }
        if (value.type == 'w' && qbeClass(expr->type) == 'l') {
            std::string wide = newTemp();
            line(wide + " =l extsw " + value.name);
            value = Value { wide, 'l' };
        }
        if (isDiscrete(expr->type) && expr->type->kind != TypeKind::UniversalInteger) {
            emitRangeCheck(value, m_sema.typeTable().scalarBaseType(expr->type), expr->location);
        }
        char width = qbeClass(expr->type);
        if (value.type != width) {
            std::string converted = newTemp();
            line(converted + " =" + width + (width == 'l' ? " extsw " : " copy ") + value.name);
            return Value { converted, width };
        }
        return value;
    }
    if (name == "val") {
        Value value = emitExpr(expr->arguments.front().get());
        if (prefixType->modulus != 0) {
            return emitDiscreteConversion(value, expr->arguments.front()->type, expr->type);
        }
        if (value.type == 'w' && qbeClass(expr->type) == 'l') {
            std::string wide = newTemp();
            line(wide + " =l extsw " + value.name);
            value = Value { wide, 'l' };
        }
        emitRangeCheck(value, expr->type, expr->location);
        char type = qbeClass(expr->type);
        if (value.type != type) {
            std::string converted = newTemp();
            line(converted + " =" + std::string(1, type)
                 + (type == 'l' ? " extsw " : " copy ") + value.name);
            return Value { converted, type };
        }
        return value;
    }
    if (name == "succ" || name == "pred") {
        Value value = emitExpr(expr->arguments.front().get());
        if (isFloatClass(value.type)) {
            double endpoint = value.type == 's' ? FLT_MAX : DBL_MAX;
            if (name == "pred") {
                endpoint = -endpoint;
            }
            std::string bound = QbeSupport::realLiteral(endpoint, value.type);
            std::string valid = newTemp();
            line(valid + " =w " + (name == "succ" ? "clt" : "cgt") + value.type + " " + value.name + ", " + bound);
            std::string good = newLabel("adjacentvalid");
            std::string bad = newLabel("adjacentinvalid");
            branch(Value { valid, 'w' }, good, bad);
            label(bad);
            raiseConstraintError();
            label(good);
            std::string result = newTemp();
            emitCallInstruction(result + " =" + value.type + " call $" + (value.type == 's' ? "nextafterf" : "nextafter")
                + "(" + value.type + " " + value.name + ", " + value.type + " " + bound + ")", CallEffects::DeferredCheck);
            return Value { result, value.type };
        }
        if (prefixType->modulus != 0) {
            return emitModularOperation(name == "succ" ? ModularAdd : ModularSubtract, value,
                Value { prefixType->modulus == 1 ? "0" : "1", value.type }, expr->type);
        }
        Value result = emitIntegerOperation(name == "succ" ? 0 : 1, value, Value { "1", value.type }, value.type);
        emitRangeCheck(result, expr->type, expr->location);
        return result;
    }
    if (name == "image") {
        Value value = emitExpr(expr->arguments.front().get());
        Type* base = baseType(prefixType);
        long long capacity = 64;
        if (base != nullptr && base->kind == TypeKind::Fixed) {
            capacity = std::max(64LL, (long long)base->fixedAft + 24);
        } else if (base != nullptr && base->kind == TypeKind::Enumeration) {
            for (const std::string& literal : base->literals) {
                capacity = std::max(capacity, (long long)literal.size() + 1);
            }
        }
        std::string buffer = allocScratch(capacity);
        std::string length = newTemp();
        std::string destination = "l " + buffer + ", w " + std::to_string(capacity) + ", ";
        if (base != nullptr && base->kind == TypeKind::Fixed) {
            emitCallInstruction(length + " =w call $__ada_image_fixed(" + destination + "l " + value.name + ", w "
                 + std::to_string(base->fixedScale) + ", w " + std::to_string(base->fixedAft) + ")", CallEffects::DeferredCheck);
        } else if (isFloatClass(value.type)) {
            std::string wide = widenToDouble(value);
            emitCallInstruction(length + " =w call $__ada_image_float(" + destination + "d " + wide + ", w "
                 + std::to_string(defaultAft(prefixType)) + ", w 3)", CallEffects::DeferredCheck);
        } else if (base != nullptr && base->kind == TypeKind::Enumeration && !base->literals.empty()) {
            emitCallInstruction(length + " =w call $__ada_image_enum(" + destination + "w " + value.name + ", l "
                 + enumTableFor(base) + ", w " + std::to_string(base->literals.size()) + ")", CallEffects::DeferredCheck);
        } else if (base != nullptr && base->kind == TypeKind::Enumeration) {
            emitCallInstruction(length + " =w call $__ada_image_character(" + destination + "w " + value.name + ")", CallEffects::DeferredCheck);
        } else if (base->modulus != 0) {
            Value wide = wideBound(value);
            emitCallInstruction(length + " =w call $__ada_image_modular(" + destination + "l " + wide.name + ")", CallEffects::DeferredCheck);
        } else {
            emitCallInstruction(length + " =w call $" + (value.type == 'l' ? "__ada_image_long_integer(" : "__ada_image_integer(")
                 + destination + (value.type == 'l' ? "l " : "w ") + value.name + ")", CallEffects::DeferredCheck);
        }
        emitExceptionCheck();
        return Value { buffer, 'l', "1", wideBound(Value { length, 'w' }).name };
    }

    if (name == "tag" || name == "external_tag") {
        Symbol* symbol = nullptr;
        if (expr->prefix->kind == ExprKind::Identifier) {
            symbol = static_cast<IdentifierExpr*>(expr->prefix.get())->symbol;
        } else if (expr->prefix->kind == ExprKind::Selected) {
            symbol = static_cast<SelectedExpr*>(expr->prefix.get())->symbol;
        }
        Value tag;
        if (symbol != nullptr && symbol->kind == SymbolKind::TypeName) {
            tag = typeTag(prefixType);
        } else {
            Value object = emitExpr(expr->prefix.get());
            tag = Value { newTemp(), 'l' };
            line(tag.name + " =l loadl " + object.name);
        }
        if (name == "tag") {
            return tag;
        }
        std::string text = newTemp();
        std::string length = newTemp();
        std::string count = newTemp();
        emitCallInstruction(text + " =l call $__ada_tag_name(l " + tag.name + ")", CallEffects::CheckException);
        emitCallInstruction(length + " =l call $strlen(l " + text + ")", CallEffects::DeferredCheck);
        line(count + " =l copy " + length);
        return Value { text, 'l', "1", count };
    }
    if (name == "address") {
        Value address = isComposite(prefixType) ? emitExpr(expr->prefix.get()) : emitAddress(expr->prefix.get());
        return Value { address.name, 'l' };
    }
    if (name == "position" || name == "first_bit" || name == "last_bit") {
        auto result = [&](const std::string& wide) {
            if (qbeClass(expr->type) == 'l') { return Value {wide, 'l'}; }
            std::string narrow = newTemp();
            line(narrow + " =w copy " + wide);
            return Value {narrow, 'w'};
        };
        auto* component = static_cast<SelectedExpr*>(expr->prefix.get());
        Type* record = component->prefix->type;
        Value object = emitExpr(component->prefix.get());
        if (record->kind == TypeKind::Access) { checkNotNull(object); record = record->target; }
        record = baseType(record);
        if (component->checkedVariant >= 0) { checkVariant(object, record, component->checkedVariant); }
        const FieldInfo& field = record->fields[component->fieldIndex];
        if (name == "first_bit") { return constantValue(0, qbeClass(expr->type)); }
        if (name == "position") {
            Value address = fieldStorage(object, record, field);
            std::string offset = newTemp();
            line(offset + " =l sub " + address.name + ", " + object.name);
            return result(offset);
        }
        std::string bytes = field.representationSize >= 0 ? std::to_string(field.representationSize)
            : objectBytes(fieldAddress(object, record, field), field.type);
        std::string bits = newTemp(), last = newTemp();
        line(bits + " =l mul " + bytes + ", 8");
        line(last + " =l sub " + bits + ", 1");
        return result(last);
    }
    if (name == "alignment") {
        // Evaluating an object view must not read an uninitialized scalar.
        if (expr->alignmentObjectView) {
            emitAddress(expr->prefix.get());
        } else {
            emitExpr(expr->prefix.get());
        }
        return Value { std::to_string(typeAlignment(prefixType)), qbeClass(expr->type) };
    }
    if (name == "size") {
        Value bytes;
        if (prefixType->classRoot != nullptr) {
            Value object = emitExpr(expr->prefix.get());
            std::string tag = newTemp(), slot = newTemp(), size = newTemp();
            line(tag + " =l loadl " + object.name);
            line(slot + " =l add " + tag + ", 8");
            line(size + " =l loadl " + slot);
            bytes = Value { size, 'l' };
        } else if (hasRuntimeLayout(prefixType)) {
            Value object = expr->sizeTypePrefix ? withBounds(Value {}, prefixType, nullptr) : emitExpr(expr->prefix.get());
            bytes = Value { objectBytes(object, prefixType), 'l' };
        } else {
            bytes = Value { std::to_string(typeSize(prefixType)), 'l' };
        }
        Value bits = emitIntegerOperation(2, bytes, Value { "8", 'l' }, 'l');
        emitRangeCheck(bits, m_sema.typeTable().scalarBaseType(expr->type), expr->location);
        if (qbeClass(expr->type) == 'w') {
            std::string narrow = newTemp();
            line(narrow + " =w copy " + bits.name);
            return Value { narrow, 'w' };
        }
        return bits;
    }
    if (name == "value") {
        Value text = emitExpr(expr->arguments.front().get());
        Value length = narrowArrayLength(lengthOf(text, expr->arguments.front()->type));
        Value bounds = scalarBounds(prefixType);
        Type* base = baseType(prefixType);
        std::string temp = newTemp();
        if (base != nullptr && base->kind == TypeKind::Float) {
            char precision = qbeClass(expr->type);
            std::string helper = precision == 's' ? "__ada_value_real_f32" : "__ada_value_real";
            emitCallInstruction(temp + " =" + precision + " call $" + helper + "(l " + text.name + ", w " + length.name + ")", CallEffects::DeferredCheck);
        } else if (base != nullptr && base->kind == TypeKind::Fixed) {
            emitCallInstruction(temp + " =l call $__ada_value_fixed(l " + text.name + ", w " + length.name
                 + ", w " + std::to_string(base->fixedScale) + ", l -9223372036854775808, l 9223372036854775807)", CallEffects::DeferredCheck);
        } else if (base != nullptr && base->kind == TypeKind::Enumeration && !base->literals.empty()) {
            emitCallInstruction(temp + " =w call $__ada_value_enum(l " + text.name + ", w " + length.name + ", l "
                 + enumTableFor(base) + ", w " + std::to_string(base->literals.size()) + ")", CallEffects::DeferredCheck);
        } else if (base != nullptr && base->kind == TypeKind::Enumeration) {
            // Character, whose values are named by their spelling.
            emitCallInstruction(temp + " =w call $__ada_value_character(l " + text.name + ", w " + length.name + ")", CallEffects::DeferredCheck);
        } else if (base->modulus != 0) {
            emitCallInstruction(temp + " =l call $__ada_value_modular(l " + text.name + ", w " + length.name
                + ", l " + scalarText(base->modulus) + ")", CallEffects::DeferredCheck);
            if (qbeClass(prefixType) == 'w') {
                std::string narrow = newTemp();
                line(narrow + " =w copy " + temp);
                temp = narrow;
            }
        } else if (qbeClass(prefixType) == 'l') {
            emitCallInstruction(temp + " =l call $__ada_value_long_integer(l " + text.name + ", w " + length.name + ", l "
                 + bounds.first + ", l " + bounds.last + ")", CallEffects::DeferredCheck);
        } else {
            emitCallInstruction(temp + " =w call $__ada_value_integer(l " + text.name + ", w " + length.name + ", w "
                 + bounds.first + ", w " + bounds.last + ")", CallEffects::DeferredCheck);
        }
        emitExceptionCheck();
        if (base->modulus == 0 && base->kind != TypeKind::Float && prefixType->scalarBoundsSymbol != nullptr) {
            emitRangeCheck(Value { temp, qbeClass(prefixType) }, prefixType, expr->location);
        }
        return Value { temp, qbeClass(prefixType) };
    }
    if (name == "first" || name == "last" || name == "length") {
        Type* base = prefixType;
        if (base != nullptr && (isScalar(base)) && base->scalarBoundsSymbol != nullptr) {
            Value bounds = scalarBounds(base);
            return Value { name == "first" ? bounds.first : bounds.last, bounds.type };
        }
        if (base != nullptr && base->kind == TypeKind::Array) {
            Value array;
            if (expr->rangeFirst != nullptr && expr->rangeFirst != expr) {
                array = m_context->rangePrefixes.at(expr->rangeFirst);
                m_context->rangePrefixes.erase(expr->rangeFirst);
            } else {
                array = emitExpr(expr->prefix.get());
                if (expr->rangeFirst == expr) {
                    m_context->rangePrefixes[expr] = array;
                }
            }
            Type* axis = expr->prefix->type;
            long long dimension = expr->arguments.empty() ? 1 : expr->arguments.front()->staticValue;
            for (long long i = 1; i < dimension; ++i) {
                array = arrayRow(array, axis);
                axis = axis->element;
            }
            if (name == "length") {
                Value length = lengthOf(array, axis);
                emitRangeCheck(length, m_sema.typeTable().scalarBaseType(expr->type), expr->location);
                if (qbeClass(expr->type) == 'w') {
                    std::string narrow = newTemp();
                    line(narrow + " =w copy " + length.name);
                    return Value { narrow, 'w' };
                }
                return length;
            }
            std::string bound = name == "first" ? array.first : array.last;
            if (qbeClass(expr->type) == 'l') {
                std::string wide = newTemp();
                line(wide + " =l copy " + bound);
                return Value { wide, 'l' };
            }
            std::string narrow = newTemp();
            line(narrow + " =w copy " + bound);
            return Value { narrow, 'w' };
        }
        long long value = base == nullptr ? 0 : (name == "first" ? base->low : base->high);
        return constantValue(value, qbeClass(expr->type));
    }

    m_diagnostics.error(expr->location, "unsupported attribute '" + expr->name + "'");
    return Value { "0", 'w' };
}

// 'Read, 'Write, 'Output and 'Input move a value between an object and a
// stream as the bytes it occupies.  'Output and 'Input additionally carry the
// bounds of every array dimension.
Value QbeEmitter::emitStreamAttribute(AttributeExpr* expr)
{
    const std::string& name = expr->lower;
    Type* prefixType = expr->prefixType;
    Value stream = emitExpr(expr->arguments.front().get());

    if (isScalar(prefixType)) {
        long long size = typeSize(prefixType);
        Value slot {allocScratch(size, typeAlignment(prefixType)), 'l'};
        bool reading = name == "read" || name == "input";
        Value destination;
        Type* targetType = prefixType;
        if (name == "read") {
            Expr* item = expr->arguments[1].get();
            destination = emitAddress(item);
            targetType = item->type;
        } else if (!reading) {
            Value value = emitExpr(expr->arguments[1].get());
            storeInto(slot, value, prefixType);
        }
        emitCallInstruction(std::string("call ") + (reading ? "$__ada_stream_read" : "$__ada_stream_write")
            + "(l " + stream.name + ", l " + slot.name + ", l " + std::to_string(size) + ")", CallEffects::CheckException);
        if (reading) {
            Value value = loadFrom(slot, prefixType);
            emitRangeCheck(value, targetType, expr->location);
            if (name == "input") { return value; }
            storeInto(destination, value, targetType);
        }
        return Value {"0", 'w'};
    }

    if (name == "input") {
        if (prefixType->kind == TypeKind::Array) {
            Type* resultType = expr->type;
            Value result;
            for (int dimension = 0; dimension < resultType->arrayRank; ++dimension) {
                std::string pair = allocScratch(16), lastSlot = newTemp();
                line(lastSlot + " =l add " + pair + ", 8");
                emitCallInstruction("call $__ada_stream_read(l " + stream.name + ", l " + pair + ", l 16)", CallEffects::CheckException);
                std::string first = newTemp(), last = newTemp();
                line(first + " =l loadl " + pair);
                line(last + " =l loadl " + lastSlot);
                if (dimension == 0) { result.first = first; result.last = last; }
                else { result.innerBounds.push_back({first, last}); }
            }
            result = arrayView(result, resultType);
            // Incoming bounds describe the stream, even for a constrained
            // first subtype. Validate them before applying that subtype's shape.
            Type incomingType = *resultType;
            incomingType.constrained = false;
            incomingType.boundsSymbol = nullptr;
            result = convertArrayBounds(result, &incomingType, &incomingType, expr->location);
            if (resultType->constrained || resultType->boundsSymbol != nullptr) {
                result = convertArrayBounds(result, &incomingType, resultType, expr->location);
            }
            std::string bytes = objectBytes(result, resultType);
            std::string allocationSize = resultType->constrained && !hasRuntimeLayout(resultType)
                ? std::to_string(typeSize(resultType)) : bytes;
            result.name = newTemp();
            result.type = 'l';
            emitCallInstruction(result.name + " =l call $__ada_array_local(l " + storageArena(true, true)
                + ", l 1, l 1, l " + allocationSize + ")", CallEffects::CheckException);
            emitCallInstruction("call $__ada_stream_read(l " + stream.name + ", l " + result.name + ", l " + bytes + ")", CallEffects::CheckException);
            return result;
        }

        std::string slot = allocTypeScratch(prefixType);
        std::string streamSize = hasRuntimeLayout(prefixType) ? runtimeSize(prefixType)
            : std::to_string(prefixType->kind == TypeKind::Array ? typeLayoutSize(prefixType) : typeSize(prefixType));
        emitCallInstruction("call $__ada_stream_read(l " + stream.name + ", l " + slot + ", l " + streamSize + ")", CallEffects::CheckException);

        Value address { slot, 'l' };
        if (isComposite(prefixType)) {
            return withBounds(address, prefixType, nullptr);
        }
        Value value = loadFrom(address, prefixType);
        if (prefixType->kind == TypeKind::Fixed || !prefixType->enumCodes.empty()) {
            emitRangeCheck(value, prefixType, expr->location);
        }
        return value;
    }

    Expr* item = expr->arguments[1].get();
    Type* type = item->type;
    bool reading = name == "read";

    Value address;
    if (isComposite(type)) {
        address = emitExpr(item);
    } else if (reading) {
        address = emitAddress(item);
    } else {
        // A value being written need not live anywhere of its own.
        long long size = typeSize(type);
        Value value = emitExpr(item);
        address = Value { allocScratch(size < 1 ? 1 : size), 'l' };
        storeInto(address, value, type);
    }

    // An array occupies as many bytes as it currently holds, which the type
    // alone does not say.
    std::string size = type->kind == TypeKind::Array ? "" : runtimeSize(type);
    Type* array = baseType(type);
    if (array != nullptr && array->kind == TypeKind::Array) {
        if (!address.hasBounds()) {
            address = withBounds(address, type, nullptr);
        }
        Value length = lengthOf(address, type);
        std::string bytes = newTemp();
        emitCallInstruction(bytes + " =l call $__ada_array_size(l " + address.first + ", l " + address.last
             + ", l " + arrayElementSize(address, type) + ")", CallEffects::CheckException);
        size = bytes;

        if (name == "output") {
            for (int dimension = 0; dimension < type->arrayRank; ++dimension) {
                emitCallInstruction("call $__ada_stream_write_bounds(l " + stream.name + ", l " + address.axisFirst(dimension)
                    + ", l " + address.axisLast(dimension) + ")", CallEffects::CheckException);
            }
        }
    }

    std::vector<StorageCopyBack> storageCopies;
    if (type->kind == TypeKind::Record) {
        address = adaptCompositeStorage(address, type, prefixType, reading ? ParameterMode::InOut : ParameterMode::In, storageCopies);
        size = runtimeSize(prefixType);
    }
    Value destination = address;
    if (reading && (type->kind == TypeKind::Fixed || !type->enumCodes.empty())) {
        address = Value { allocScratch(typeSize(type)), 'l' };
    }
    emitCallInstruction(std::string("call ") + (reading ? "$__ada_stream_read" : "$__ada_stream_write") + "(l " + stream.name
         + ", l " + address.name + ", l " + size + ")", CallEffects::CheckException);
    if (reading && (type->kind == TypeKind::Fixed || !type->enumCodes.empty())) {
        Value value = loadFrom(address, type);
        emitRangeCheck(value, type, expr->location);
        storeInto(destination, value, type);
    }
    emitStorageCopyBacks(storageCopies);
    return Value { "0", 'w' };
}

// The run time formats every real value as a double.
std::string QbeEmitter::widenToDouble(const Value& value)
{
    if (value.type == 'd') {
        return value.name;
    }
    std::string wide = newTemp();
    line(wide + " =d exts " + value.name);
    return wide;
}

// Ada writes one digit before the point and the rest after it.
int QbeEmitter::defaultAft(const Type* type)
{
    int digits = type != nullptr && type->digits > 0 ? type->digits : 6;
    return digits - 1;
}
