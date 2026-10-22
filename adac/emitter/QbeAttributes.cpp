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
    // Protected entry bodies execute in the calling task. Current_Task in an
    // entry body uses this same identity (the RM C.7.1 bounded-error choice).
    if (name == "caller") {
        std::string identity = newTemp();
        line(identity + " =l call $__ada_current_task()");
        return Value { identity, 'l' };
    }
    if (expr->m_taskAttribute) {
        Value identity;
        if (expr->m_taskCurrentInstance != nullptr) {
            identity = loadFrom(addressOf(expr->m_taskCurrentInstance), expr->m_taskCurrentInstance->type);
        } else {
            Value object = emitExpr(expr->prefix.get());
            if (prefixType->kind == TypeKind::Access) {
                checkNotNull(object);
            }
            std::string result = newTemp();
            line(result + " =l call $__ada_task_identity(l " + object.name + ")");
            emitExceptionCheck();
            identity = Value { result, 'l' };
        }
        if (name == "identity") {
            return identity;
        }
        std::string result = newTemp();
        line(result + " =w call $__ada_task_is_" + name + "(l " + identity.name + ")");
        emitExceptionCheck();
        return Value { result, 'w' };
    }
    if (name == "count") {
        Value object = addressOf(expr->m_protectedReceiver);
        std::string count = newTemp();
        line(count + " =w call $__ada_protected_count(l " + object.name
            + ", l " + expr->m_countEntry->qbeName + ".barrier)");
        if (qbeClass(expr->type) == 'l') {
            std::string wide = newTemp();
            line(wide + " =l extsw " + count);
            return Value { wide, 'l' };
        }
        return Value { count, 'w' };
    }
    if (name == "access") {
        Symbol* subprogram = expr->m_accessSubprogram;
        if (subprogram->m_renamedAccess != nullptr) {
            Symbol* binding = subprogram->m_renamedAccess;
            return loadFrom(addressOf(binding), binding->type);
        }
        if (subprogram->level == 0) {
            std::string descriptor = subprogram->qbeName + ".access";
            return Value { descriptor, 'l' };
        }
        Value frame = staticLinkFor(subprogram->level - 1);
        std::string descriptor = newTemp();
        line(descriptor + " =l add " + frame.name + ", " + std::to_string(subprogram->m_descriptorOffset));
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
            line(result + " =l call $__ada_fixed_rescale(l " + value.name + ", w "
                + std::to_string(argument->type->m_fixedScale) + ", w " + std::to_string(prefixType->m_fixedScale) + ")");
        } else {
            if (value.type == 's') {
                std::string wide = newTemp();
                line(wide + " =d exts " + value.name);
                value = Value { wide, 'd' };
            }
            line(result + " =l call $__ada_float_to_fixed(d " + value.name + ", w " + std::to_string(prefixType->m_fixedScale) + ")");
        }
        emitExceptionCheck();
        return Value { result, 'l' };
    }
    if (name == "fore" && prefixType->kind == TypeKind::Fixed) {
        Value bounds = scalarBounds(prefixType);
        std::string result = newTemp();
        line(result + " =l call $__ada_fixed_fore(l " + bounds.first + ", l " + bounds.last
            + ", w " + std::to_string(prefixType->m_fixedScale) + ")");
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
        line(result + " =l call $" + (prefixType->kind == TypeKind::Fixed ? "__ada_fixed_minmax" : "__ada_modular_minmax") + "(l " + a.name + ", l " + b.name
            + ", w " + (name == "min" ? "0" : "1") + ")");
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
        line(result + " =l call $__ada_modular_reduce(l " + value.name + ", w "
            + (argument->type->m_modulus != 0 ? "1" : "0") + ", l " + scalarText(prefixType->m_modulus) + ")");
        if (qbeClass(expr->type) == 'w') {
            std::string narrow = newTemp();
            line(narrow + " =w copy " + result);
            result = narrow;
        }
        return Value { result, qbeClass(expr->type) };
    }
    if (name == "pos") {
        Value value = emitExpr(expr->arguments.front().get());
        if (prefixType->m_modulus != 0 && expr->type->kind != TypeKind::UniversalInteger) {
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
        if (prefixType->m_modulus != 0) {
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
            line(result + " =" + value.type + " call $" + (value.type == 's' ? "nextafterf" : "nextafter")
                + "(" + value.type + " " + value.name + ", " + value.type + " " + bound + ")");
            return Value { result, value.type };
        }
        if (prefixType->m_modulus != 0) {
            return emitModularOperation(name == "succ" ? ModularAdd : ModularSubtract, value,
                Value { prefixType->m_modulus == 1 ? "0" : "1", value.type }, expr->type);
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
            capacity = std::max(64LL, (long long)base->m_fixedAft + 24);
        } else if (base != nullptr && base->kind == TypeKind::Enumeration) {
            for (const std::string& literal : base->literals) {
                capacity = std::max(capacity, (long long)literal.size() + 1);
            }
        }
        std::string buffer = allocScratch(capacity);
        std::string length = newTemp();
        std::string destination = "l " + buffer + ", w " + std::to_string(capacity) + ", ";
        if (base != nullptr && base->kind == TypeKind::Fixed) {
            line(length + " =w call $__ada_image_fixed(" + destination + "l " + value.name + ", w "
                 + std::to_string(base->m_fixedScale) + ", w " + std::to_string(base->m_fixedAft) + ")");
        } else if (isFloatClass(value.type)) {
            std::string wide = widenToDouble(value);
            line(length + " =w call $__ada_image_float(" + destination + "d " + wide + ", w "
                 + std::to_string(defaultAft(prefixType)) + ", w 3)");
        } else if (base != nullptr && base->kind == TypeKind::Enumeration && !base->literals.empty()) {
            line(length + " =w call $__ada_image_enum(" + destination + "w " + value.name + ", l "
                 + enumTableFor(base) + ", w " + std::to_string(base->literals.size()) + ")");
        } else if (base != nullptr && base->kind == TypeKind::Enumeration) {
            line(length + " =w call $__ada_image_character(" + destination + "w " + value.name + ")");
        } else if (base->m_modulus != 0) {
            Value wide = wideBound(value);
            line(length + " =w call $__ada_image_modular(" + destination + "l " + wide.name + ")");
        } else {
            line(length + " =w call $" + (value.type == 'l' ? "__ada_image_long_integer(" : "__ada_image_integer(")
                 + destination + (value.type == 'l' ? "l " : "w ") + value.name + ")");
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
        line(text + " =l call $__ada_tag_name(l " + tag.name + ")");
        emitExceptionCheck();
        line(length + " =l call $strlen(l " + text + ")");
        line(count + " =l copy " + length);
        return Value { text, 'l', "1", count };
    }
    if (name == "address") {
        Value address = isComposite(prefixType) ? emitExpr(expr->prefix.get()) : emitAddress(expr->prefix.get());
        return Value { address.name, 'l' };
    }
    if (name == "alignment") {
        // Evaluating an object view must not read an uninitialized scalar.
        if (expr->m_alignmentObjectView) {
            emitAddress(expr->prefix.get());
        } else {
            emitExpr(expr->prefix.get());
        }
        return Value { std::to_string(typeAlignment(prefixType)), qbeClass(expr->type) };
    }
    if (name == "size") {
        Value bytes;
        if (prefixType->m_classRoot != nullptr) {
            Value object = emitExpr(expr->prefix.get());
            std::string tag = newTemp(), slot = newTemp(), size = newTemp();
            line(tag + " =l loadl " + object.name);
            line(slot + " =l add " + tag + ", 8");
            line(size + " =l loadl " + slot);
            bytes = Value { size, 'l' };
        } else if (prefixType->kind == TypeKind::Array && !prefixType->constrained) {
            Value object = emitExpr(expr->prefix.get());
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
            line(temp + " =" + precision + " call $" + helper + "(l " + text.name + ", w " + length.name + ")");
        } else if (base != nullptr && base->kind == TypeKind::Fixed) {
            line(temp + " =l call $__ada_value_fixed(l " + text.name + ", w " + length.name
                 + ", w " + std::to_string(base->m_fixedScale) + ", l -9223372036854775808, l 9223372036854775807)");
        } else if (base != nullptr && base->kind == TypeKind::Enumeration && !base->literals.empty()) {
            line(temp + " =w call $__ada_value_enum(l " + text.name + ", w " + length.name + ", l "
                 + enumTableFor(base) + ", w " + std::to_string(base->literals.size()) + ")");
        } else if (base != nullptr && base->kind == TypeKind::Enumeration) {
            // Character, whose values are named by their spelling.
            line(temp + " =w call $__ada_value_character(l " + text.name + ", w " + length.name + ")");
        } else if (base->m_modulus != 0) {
            line(temp + " =l call $__ada_value_modular(l " + text.name + ", w " + length.name
                + ", l " + scalarText(base->m_modulus) + ")");
            if (qbeClass(prefixType) == 'w') {
                std::string narrow = newTemp();
                line(narrow + " =w copy " + temp);
                temp = narrow;
            }
        } else if (qbeClass(prefixType) == 'l') {
            line(temp + " =l call $__ada_value_long_integer(l " + text.name + ", w " + length.name + ", l "
                 + bounds.first + ", l " + bounds.last + ")");
        } else {
            line(temp + " =w call $__ada_value_integer(l " + text.name + ", w " + length.name + ", w "
                 + bounds.first + ", w " + bounds.last + ")");
        }
        emitExceptionCheck();
        if (base->m_modulus == 0 && base->kind != TypeKind::Float && prefixType->m_scalarBoundsSymbol != nullptr) {
            emitRangeCheck(Value { temp, qbeClass(prefixType) }, prefixType, expr->location);
        }
        return Value { temp, qbeClass(prefixType) };
    }
    if (name == "first" || name == "last" || name == "length") {
        Type* base = prefixType;
        if (base != nullptr && (isDiscrete(base) || base->kind == TypeKind::Fixed) && base->m_scalarBoundsSymbol != nullptr) {
            Value bounds = scalarBounds(base);
            return Value { name == "first" ? bounds.first : bounds.last, bounds.type };
        }
        if (base != nullptr && base->kind == TypeKind::Array) {
            Value array;
            if (expr->m_rangeFirst != nullptr && expr->m_rangeFirst != expr) {
                array = m_context->m_rangePrefixes.at(expr->m_rangeFirst);
                m_context->m_rangePrefixes.erase(expr->m_rangeFirst);
            } else {
                array = emitExpr(expr->prefix.get());
                if (expr->m_rangeFirst == expr) {
                    m_context->m_rangePrefixes[expr] = array;
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
// bounds of an array whose type does not fix them.
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
        line(std::string("call ") + (reading ? "$__ada_stream_read" : "$__ada_stream_write")
            + "(l " + stream.name + ", l " + slot.name + ", l " + std::to_string(size) + ")");
        emitExceptionCheck();
        if (reading) {
            Value value = loadFrom(slot, prefixType);
            emitRangeCheck(value, targetType, expr->location);
            if (name == "input") { return value; }
            storeInto(destination, value, targetType);
        }
        return Value {"0", 'w'};
    }

    if (name == "input") {
        if (isUnconstrainedArray(prefixType)) {
            std::string descriptor = allocScratch(adaArrayResultSize(1));
            line("call $__ada_stream_read_array(l " + descriptor + ", l " + stream.name + ", l "
                 + std::to_string(typeSize(prefixType->element)) + ")");
            emitExceptionCheck();
            Value result = consumeArrayResult(descriptor, 1, true);
            return convertArrayBounds(result, prefixType, prefixType, expr->location);
        }

        long long size = typeSize(prefixType);
        std::string slot = allocScratch(size < 1 ? 1 : size, typeAlignment(prefixType));
        long long streamSize = prefixType->kind == TypeKind::Array ? typeLayoutSize(prefixType) : size;
        line("call $__ada_stream_read(l " + stream.name + ", l " + slot + ", l " + std::to_string(streamSize) + ")");
        emitExceptionCheck();

        Value address { slot, 'l' };
        if (isComposite(prefixType)) {
            return withBounds(address, prefixType, nullptr);
        }
        Value value = loadFrom(address, prefixType);
        if (prefixType->kind == TypeKind::Fixed || !prefixType->m_enumCodes.empty()) {
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
    std::string size = std::to_string(typeSize(type));
    Type* array = baseType(type);
    if (array != nullptr && array->kind == TypeKind::Array) {
        if (!address.hasBounds()) {
            address = withBounds(address, type, nullptr);
        }
        Value length = lengthOf(address, type);
        std::string bytes = newTemp();
        line(bytes + " =l call $__ada_array_size(l " + address.first + ", l " + address.last
             + ", l " + arrayElementSize(address, type) + ")");
        emitExceptionCheck();
        size = bytes;

        if (name == "output" && isUnconstrainedArray(prefixType)) {
            line("call $__ada_stream_write_bounds(l " + stream.name + ", l " + address.first + ", l "
                 + address.last + ")");
            emitExceptionCheck();
        }
    }

    std::vector<StorageCopyBack> storageCopies;
    if (type->kind == TypeKind::Record) {
        address = adaptCompositeStorage(address, type, prefixType, reading ? ParameterMode::InOut : ParameterMode::In, storageCopies);
        size = std::to_string(typeSize(prefixType));
    }
    Value destination = address;
    if (reading && (type->kind == TypeKind::Fixed || !type->m_enumCodes.empty())) {
        address = Value { allocScratch(typeSize(type)), 'l' };
    }
    line(std::string("call ") + (reading ? "$__ada_stream_read" : "$__ada_stream_write") + "(l " + stream.name
         + ", l " + address.name + ", l " + size + ")");
    emitExceptionCheck();
    if (reading && (type->kind == TypeKind::Fixed || !type->m_enumCodes.empty())) {
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
