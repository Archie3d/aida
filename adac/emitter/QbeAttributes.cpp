#include "QbeEmitter.h"
#include "QbeSupport.h"

#include <algorithm>
#include <cfloat>
#include <climits>

using QbeSupport::constantValue;
using QbeSupport::isFloatClass;
using QbeSupport::isUnconstrainedArray;

Value QbeEmitter::emitAttribute(AttributeExpr* expr)
{
    const std::string& name = expr->lower;
    Type* prefixType = expr->prefixType;
    if (name == "definite") {
        return constantValue(expr->staticValue, 'w');
    }
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
                emitCallInstruction(result.name + " =w call $__ada_wide_character_width(w " + bounds.first
                    + ", w " + bounds.last + ", w " + std::to_string(expr->textWidth) + ")", CallEffects::CheckException);
            } else {
                emitCallInstruction(result.name + " =w call $__ada_width_enum_text(w " + bounds.first
                    + ", w " + bounds.last + ", l " + enumTableFor(prefixType) + ", w "
                    + std::to_string(prefixType->literals.size()) + ", w " + std::to_string(expr->textWidth) + ")", CallEffects::CheckException);
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
            if (expr->type->target->classRoot != nullptr) {
                emitRecordConstraintCheck(object, expr->type->target);
            }
            // Keep library-level accessibility when forwarding through an
            // anonymous access parameter as well as for this attribute itself.
            if (name == "unchecked_access") { object.accessLevel = "0"; }
            else if (expr->accessObject != nullptr) { object.accessLevel = accessLevelFor(expr->accessObject); }
            else if (object.accessLevel.empty()) { object.accessLevel = currentAccessLevel(); }
            if (name != "unchecked_access" && !expr->type->accessParameter && !expr->type->accessDiscriminant
                && (expr->objectAccessLevel > expr->type->accessLevel || object.accessLevel[0] == '%'
                    || (expr->prefix->kind == ExprKind::Identifier
                        && static_cast<IdentifierExpr*>(expr->prefix.get())->symbol->currentInstance != nullptr))) {
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
        if (expr->dereferencedSubprogramAccess) {
            auto* selected = static_cast<SelectedExpr*>(expr->prefix.get());
            Value callback = emitExpr(selected->prefix.get());
            // The dereference must check null even when no call follows.
            // Reusing the descriptor retains both the code and captured frame.
            checkNotNull(callback);
            return callback;
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
        if (prefixType->modulus == 0 && prefixType->kind != TypeKind::Fixed) {
            std::string chooseLeft = newTemp();
            std::string leftLabel = newLabel("minmaxleft"), rightLabel = newLabel("minmaxright"), done = newLabel("minmaxdone");
            line(chooseLeft + " =w " + QbeSupport::comparisonInstruction(name == "min" ? BinaryOp::LessEqual : BinaryOp::GreaterEqual, left.type)
                + " " + left.name + ", " + right.name);
            branch(Value { chooseLeft, 'w' }, leftLabel, rightLabel);
            label(leftLabel);
            jump(done);
            label(rightLabel);
            jump(done);
            label(done);
            line(result + " =" + left.type + " phi " + leftLabel + " " + left.name + ", " + rightLabel + " " + right.name);
            return Value { result, left.type };
        }
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
        if (base != nullptr && base->kind == TypeKind::Enumeration && base->literals.empty()) {
            std::string buffer = allocScratch(capacity * expr->textWidth);
            std::string length = newTemp();
            emitCallInstruction(length + " =w call $__ada_wide_character_image(l " + buffer + ", w " + std::to_string(capacity)
                + ", w " + value.name + ", w " + std::to_string(expr->textWidth) + ")", CallEffects::CheckException);
            return Value { buffer, 'l', "1", wideBound(Value { length, 'w' }).name };
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
        if (expr->textWidth > 1) {
            std::string wide = allocScratch(capacity * 3 * expr->textWidth);
            std::string count = newTemp();
            emitCallInstruction(count + " =w call $__ada_image_widen(l " + wide + ", w " + std::to_string(capacity * 3)
                + ", l " + buffer + ", w " + length + ", w " + std::to_string(expr->textWidth) + ")", CallEffects::CheckException);
            return Value { wide, 'l', "1", wideBound(Value { count, 'w' }).name };
        }
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
    if (name == "stream_size") { return constantValue(streamSizeBits(prefixType), 'l'); }
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
        Type* characterBase = baseType(prefixType);
        if (characterBase != nullptr && characterBase->kind == TypeKind::Enumeration && characterBase->literals.empty()) {
            Value result { newTemp(), 'w' };
            emitCallInstruction(result.name + " =w call $__ada_wide_character_value(l " + text.name + ", w " + length.name
                + ", w " + std::to_string(expr->textWidth) + ", w " + std::to_string(static_cast<long long>(characterBase->high)) + ")", CallEffects::CheckException);
            return result;
        }
        if (expr->textWidth > 1) {
            std::string descriptor = allocScratch(adaArrayResultSize(1));
            emitCallInstruction("call $__ada_wide_to_utf8(l " + descriptor + ", l " + text.name + ", w " + length.name
                + ", w " + std::to_string(expr->textWidth) + ")", CallEffects::CheckException);
            text = consumeArrayResult(descriptor, 1, true);
            length = narrowArrayLength(lengthOf(text, m_sema.typeTable().stringType()));
        }
        // RM 3.5: Value returns S'Base, even when S has runtime bounds.
        Value bounds = scalarBounds(expr->type);
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
Type QbeEmitter::streamStorageType(Type* type)
{
    Type storage = *rootType(type);
    storage.byteSize = streamSizeBits(type) / 8;
    // Stream_Size belongs to the type, independently of subtype Size clauses.
    storage.base = nullptr;
    storage.isSubtype = false;
    if (isDiscrete(type) && type->kind != TypeKind::Enumeration && type->modulus == 0) {
        long long signedMax = storage.byteSize == 8 ? LLONG_MAX : (1LL << (storage.byteSize * 8 - 1)) - 1;
        if (storage.low >= 0 && storage.high <= signedMax) { storage.low = -1; }
    }
    return storage;
}

void QbeEmitter::emitStreamScalarCheck(const Value& value, Type* type, const Type& storage)
{
    if (type->kind == TypeKind::Float || type->kind == TypeKind::Access) { return; }
    int bits = static_cast<int>(storage.byteSize * 8);
    if (bits == 64) { return; }
    bool signedValue = scalarStorageSigned(&storage) || type->kind == TypeKind::Fixed;
    Type limits(TypeKind::Integer, "stream representation");
    limits.modulus = !signedValue ? (__int128(1) << bits) : 0;
    limits.low = signedValue ? -(1LL << (bits - 1)) : 0;
    limits.high = signedValue ? (1LL << (bits - 1)) - 1 : (1LL << bits) - 1;
    emitRangeCheck(enumCode(value, type), &limits, m_context->sourceLocation);
}

// These ordinary Ada functions share the normal result ownership and exception
// ABI. A local tag pairs their code with the declaring activation's static link.
void QbeEmitter::emitStreamWrappers(Type* type)
{
    Type channelType(TypeKind::Access, "stream parameter");
    channelType.target = m_sema.typeTable().integerType();
    channelType.anonymousAccess = true;
    channelType.accessParameter = true;
    const char* names[] = {"read", "write", "input", "output"};
    for (int i = 0; i < 4; ++i) {
        if (!streamAvailable(type, i)) { continue; }
        Symbol operation;
        operation.kind = SymbolKind::Subprogram;
        operation.qbeName = type->tagName + ".stream" + std::to_string(i);
        operation.displayName = type->name + "'" + names[i];
        operation.level = type->tagOwner == nullptr ? 0 : type->tagOwner->level + 1;
        operation.owner = type->tagOwner;
        operation.usesFinalization = true;
        operation.streamWrapper = true;
        operation.returnType = i == 2 ? type : nullptr;
        Symbol stream, item;
        stream.kind = SymbolKind::Parameter;
        stream.name = "stream";
        stream.type = &channelType;
        stream.owner = &operation;
        operation.parameters.push_back(&stream);
        item.kind = SymbolKind::Parameter;
        item.name = "item";
        item.type = type;
        item.owner = &operation;
        item.byReference = true;
        item.mode = i == 0 ? ParameterMode::Out : ParameterMode::In;
        if (i != 2) { operation.parameters.push_back(&item); }
        auto attribute = std::make_unique<AttributeExpr>();
        attribute->name = attribute->lower = names[i];
        attribute->prefixType = type;
        attribute->type = i == 2 ? type : m_sema.typeTable().voidType();
        auto channel = std::make_unique<IdentifierExpr>();
        channel->symbol = &stream;
        channel->type = &channelType;
        attribute->arguments.push_back(std::move(channel));
        if (i != 2) {
            auto value = std::make_unique<IdentifierExpr>();
            value->symbol = &item;
            value->type = type;
            attribute->arguments.push_back(std::move(value));
        }
        SubprogramBody body;
        body.symbol = &operation;
        if (i == 2) {
            auto result = std::make_unique<ReturnStmt>();
            result->value = std::move(attribute);
            body.body.push_back(std::move(result));
        } else {
            auto call = std::make_unique<ProcedureCallStmt>();
            call->call = std::move(attribute);
            body.body.push_back(std::move(call));
        }
        emitSubprogram(&body);
    }
}

Value QbeEmitter::emitStreamCall(Symbol* operation, const Value& stream, const Value* item, Type* type,
                                  const std::string& constrained, Expr* actual)
{
    Symbol channel;
    channel.type = operation->parameters[0]->type;
    channel.owner = m_context->symbol;
    Value channelSlot {allocTypeScratch(channel.type), 'l'};
    storeInto(channelSlot, stream, channel.type);
    m_context->locals[&channel] = channelSlot.name;
    IdentifierExpr channelExpr;
    channelExpr.type = channel.type;
    channelExpr.symbol = &channel;
    Symbol value;
    IdentifierExpr valueExpr;
    CallExpr call;
    call.form = CallForm::Subprogram;
    call.subprogram = operation;
    call.type = operation->returnType;
    call.resolvedArguments.push_back(&channelExpr);
    if (actual != nullptr) { call.resolvedArguments.push_back(actual); }
    else if (item != nullptr) {
        value.type = type;
        value.owner = m_context->symbol;
        m_context->locals[&value] = item->name;
        m_context->bounds[&value] = *item;
        m_context->streamConstraints[&value] = constrained;
        valueExpr.type = type;
        valueExpr.symbol = &value;
        call.resolvedArguments.push_back(&valueExpr);
    }
    Value result = emitCall(&call);
    m_context->locals.erase(&channel);
    m_context->locals.erase(&value);
    m_context->bounds.erase(&value);
    m_context->streamConstraints.erase(&value);
    return result;
}

Value QbeEmitter::emitStreamRecordInput(const Value& stream, Type* type)
{
    Type* record = baseType(type);
    Type constrainedType = *record;
    constrainedType.isSubtype = true;
    constrainedType.base = record;
    constrainedType.discriminantsKnown = false;
    constrainedType.discriminantValues.clear();
    constrainedType.discriminantConstraintSource = nullptr;
    constrainedType.discriminantRecord = nullptr;
    Symbol capture;
    constrainedType.discriminantSymbol = &capture;
    constrainedType.discriminantPerEvaluation = true;
    Value captured {allocScratch(record->discriminantCount * 32), 'l'};
    m_context->discriminantBounds[&constrainedType] = captured;
    for (int i = 0; i < record->discriminantCount; ++i) {
        Value slot {newTemp(), 'l'};
        line(slot.name + " =l add " + captured.name + ", " + std::to_string(i * 32));
        emitStreamComponents(stream, true, slot, discriminantField(record, i).type);
    }
    Value result {allocTypeScratch(&constrainedType), 'l'};
    bool saved = m_context->initializingTemporary;
    m_context->initializingTemporary = true;
    prepareControlledObject(result, &constrainedType);
    emitDefaultInit(result, &constrainedType);
    m_context->initializingTemporary = saved;
    // Input's normal initialization precedes its call of Read. A specified
    // Read sees that initialized object, including its streamed discriminants.
    if (Symbol* operation = streamAttribute(record, 0)) {
        emitStreamCall(operation, stream, &result, &constrainedType);
    } else {
        emitStreamComponents(stream, true, result, &constrainedType, true);
    }
    m_context->discriminantBounds.erase(&constrainedType);
    return result;
}

// Stream values describe logical components, independently of record padding,
// array descriptors and the tag stored at the start of a tagged object.
void QbeEmitter::emitStreamComponents(const Value& stream, bool reading, Value object, Type* type,
                                      bool skipDiscriminants, const std::string& constrained)
{
    if (Symbol* operation = streamAttribute(type, reading ? 0 : 1)) {
        emitStreamCall(operation, stream, &object, type, constrained);
        return;
    }
    if (type->kind == TypeKind::Array) {
        Value array = object.hasBounds() ? object : withBounds(object, type, nullptr);
        Value length = lengthOf(array, type);
        std::string count = newTemp();
        line(count + " =l copy " + length.name);
        std::string stride = arrayElementSize(array, type);
        std::string cursor = allocScratch(8);
        line("storel 0, " + cursor);
        std::string head = newLabel("stream_elements"), body = newLabel("stream_element");
        std::string done = newLabel("stream_elements_done");
        jump(head);
        label(head);
        std::string index = newTemp(), more = newTemp();
        line(index + " =l loadl " + cursor);
        line(more + " =w cultl " + index + ", " + count);
        branch(Value {more, 'w'}, body, done);
        label(body);
        Value cell = arrayRow(array, type);
        std::string offset = newTemp();
        line(offset + " =l mul " + index + ", " + stride);
        cell.name = newTemp();
        line(cell.name + " =l add " + array.name + ", " + offset);
        emitStreamComponents(stream, reading, cell, type->element);
        std::string next = newTemp();
        line(next + " =l add " + index + ", 1");
        line("storel " + next + ", " + cursor);
        jump(head);
        label(done);
        return;
    }
    if (type->kind == TypeKind::Record) {
        Type* record = baseType(type);
        if (reading && !skipDiscriminants && hasDefaultDiscriminants(record)) {
            Value decoded = emitStreamRecordInput(stream, record);
            Symbol temporary;
            temporary.type = record;
            temporary.owner = m_context->symbol;
            m_context->locals[&temporary] = decoded.name;
            IdentifierExpr value;
            value.type = record;
            value.symbol = &temporary;
            assignInto(object, type, &value, false, constrained);
            m_context->locals.erase(&temporary);
            return;
        }
        if (record->tagged && record->parentType != nullptr) {
            emitStreamComponents(stream, reading, object, record->parentType);
        }
        for (const FieldInfo& field : record->fields) {
            if (record->tagged && record->parentType != nullptr && field.index < record->parentFieldCount) { continue; }
            if (field.isDiscriminant && (skipDiscriminants || !hasDefaultDiscriminants(record))) { continue; }
            std::string done;
            if (field.variant >= 0) {
                std::string active = newLabel("stream_variant");
                done = newLabel("stream_variant_done");
                branch(variantCondition(object, record, field.variant), active, done);
                label(active);
            }
            emitStreamComponents(stream, reading, fieldAddress(object, record, field), field.type, false, constrained);
            if (!done.empty()) { jump(done); label(done); }
        }
        return;
    }
    Symbol* operation = reading ? m_sema.streamRead() : m_sema.streamWrite();
    Type storage = streamStorageType(type);
    Value slot {allocTypeScratch(&storage), 'l'};
    if (!reading) {
        Value value = loadFrom(object, type);
        emitStreamScalarCheck(value, type, storage);
        storeInto(slot, value, &storage);
    }
    emitCallInstruction(std::string("call $__ada_stream_value_") + (reading ? "read" : "write")
        + "(l " + stream.name + ", w " + std::to_string(operation != nullptr ? operation->dispatchSlot : reading ? 0 : 1)
        + ", l " + slot.name + ", l " + std::to_string(typeSize(&storage)) + ")", CallEffects::CheckException);
    if (reading) {
        Value value = loadFrom(slot, &storage);
        emitRangeCheck(value, type, {});
        storeInto(object, value, type);
    }
}

Value QbeEmitter::emitStreamAttribute(AttributeExpr* expr)
{
    const std::string& name = expr->lower;
    Type* prefixType = expr->prefixType;
    int attribute = name == "read" ? 0 : name == "write" ? 1 : name == "input" ? 2 : 3;
    Value stream = emitExpr(expr->arguments.front().get());
    checkNotNull(stream);
    if (Symbol* operation = streamAttribute(prefixType, attribute)) {
        if (name == "input") { return emitStreamCall(operation, stream, nullptr, prefixType); }
        Expr* item = expr->arguments[1].get();
        return emitStreamCall(operation, stream, nullptr, item->type, "1", item);
    }
    if (prefixType->classRoot != nullptr) {
        Type* root = rootType(prefixType->classRoot);
        Value item {"0", 'l'};
        Value tag {newTemp(), 'l'};
        if (name == "input") {
            emitCallInstruction(tag.name + " =l call $__ada_stream_input_tag(l " + stream.name
                + ", w 0, l " + typeTag(root).name + ")", CallEffects::CheckException);
        } else {
            item = emitExpr(expr->arguments[1].get());
            line(tag.name + " =l loadl " + item.name);
            if (name == "output") {
                emitCallInstruction("call $__ada_stream_external_tag(l " + stream.name
                    + ", w 1, l " + tag.name + ", l " + typeTag(root).name + ")", CallEffects::CheckException);
            }
        }
        Value result {newTemp(), 'l'};
        emitCallInstruction(result.name + " =l call $__ada_stream_tag_call(l " + stream.name
            + ", w " + (stream.accessLevel.empty() ? "0" : stream.accessLevel)
            + ", l " + tag.name + ", w " + std::to_string(attribute) + ", l " + item.name
            + ", l " + m_context->temporaryFinalizationChain + ", l " + storageArena(true, true) + ")", CallEffects::CheckException);
        if (name == "input") { emitRecordConstraintCheck(result, prefixType); return result; }
        return Value {"0", 'w'};
    }
    auto transfer = [&](bool reading, const std::string& address, const std::string& size) {
        Symbol* operation = reading ? m_sema.streamRead() : m_sema.streamWrite();
        emitCallInstruction(std::string("call $__ada_stream_value_") + (reading ? "read" : "write")
            + "(l " + stream.name + ", w " + std::to_string(operation != nullptr ? operation->dispatchSlot : reading ? 0 : 1)
            + ", l " + address + ", l " + size + ")", CallEffects::CheckException);
    };

    if (isScalar(prefixType)) {
        bool reading = name == "read" || name == "input";
        Type storage = streamAttribute(prefixType, reading ? 0 : 1) != nullptr
            ? *prefixType : streamStorageType(prefixType);
        long long size = typeSize(&storage);
        Value slot {allocTypeScratch(&storage), 'l'};
        Value destination;
        Type* targetType = prefixType;
        if (name == "read") {
            Expr* item = expr->arguments[1].get();
            destination = emitAddress(item);
            targetType = item->type;
        } else if (!reading) {
            Value value = emitExpr(expr->arguments[1].get());
            emitStreamScalarCheck(value, prefixType, storage);
            storeInto(slot, value, &storage);
        }
        if (Symbol* operation = streamAttribute(prefixType, reading ? 0 : 1)) {
            emitStreamCall(operation, stream, &slot, prefixType);
        } else {
            transfer(reading, slot.name, std::to_string(size));
        }
        if (reading) {
            Value value = loadFrom(slot, &storage);
            emitRangeCheck(value, targetType, expr->location);
            if (name == "input") { return value; }
            storeInto(destination, value, targetType);
        }
        return Value {"0", 'w'};
    }

    auto initializeResult = [&](const Value& object, Type* type) {
        bool saved = m_context->initializingTemporary;
        m_context->initializingTemporary = true;
        prepareControlledObject(object, type);
        emitDefaultInit(object, type);
        m_context->initializingTemporary = saved;
    };
    if (name == "input") {
        if (prefixType->kind == TypeKind::Record && baseType(prefixType)->discriminantCount > 0
            && !hasDefaultDiscriminants(baseType(prefixType))) {
            Value result = emitStreamRecordInput(stream, prefixType);
            emitRecordConstraintCheck(result, prefixType);
            return result;
        }
        if (prefixType->kind == TypeKind::Array) {
            Type* resultType = expr->type;
            Value result;
            for (int dimension = 0; dimension < resultType->arrayRank; ++dimension) {
                Type* axis = resultType;
                for (int i = 0; i < dimension; ++i) { axis = axis->element; }
                Type* index = m_sema.typeTable().scalarBaseType(axis->index);
                Value firstSlot {allocTypeScratch(index), 'l'}, lastSlot {allocTypeScratch(index), 'l'};
                emitStreamComponents(stream, true, firstSlot, index);
                emitStreamComponents(stream, true, lastSlot, index);
                std::string first = wideBound(loadFrom(firstSlot, index)).name;
                std::string last = wideBound(loadFrom(lastSlot, index)).name;
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
            initializeResult(result, resultType);
            emitStreamComponents(stream, true, result, resultType);
            return result;
        }

        bool providedOccurrence = prefixType->exceptionOccurrence
            && (!m_context->resultTarget.empty() || !m_context->resultDescriptor.empty());
        std::string target = !m_context->resultTarget.empty() ? m_context->resultTarget : m_context->resultDescriptor;
        if (providedOccurrence) {
            m_context->resultTarget.clear();
            m_context->resultDescriptor.clear();
            m_context->resultProvided.clear();
            m_context->resultTargetOwner.clear();
            m_context->resultTargetArena.clear();
        }
        Value result {providedOccurrence ? target : allocTypeScratch(prefixType), 'l'};
        if (!providedOccurrence) { initializeResult(result, prefixType); }
        else {
            emitCallInstruction("call $__ada_occurrence_initialize(l " + result.name + ")", CallEffects::DeferredCheck);
            activateControlledObject(result, prefixType);
        }
        emitStreamComponents(stream, true, result, prefixType);
        return withBounds(result, prefixType, nullptr);
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

    Type* array = baseType(type);
    if (array != nullptr && array->kind == TypeKind::Array) {
        if (!address.hasBounds()) {
            address = withBounds(address, type, nullptr);
        }
        if (name == "output") {
            for (int dimension = 0; dimension < type->arrayRank; ++dimension) {
                Type* axis = type;
                for (int i = 0; i < dimension; ++i) { axis = axis->element; }
                Type* index = m_sema.typeTable().scalarBaseType(axis->index);
                Value slot {allocTypeScratch(index), 'l'};
                storeInto(slot, Value {address.axisFirst(dimension), 'l'}, index);
                emitStreamComponents(stream, false, slot, index);
                storeInto(slot, Value {address.axisLast(dimension), 'l'}, index);
                emitStreamComponents(stream, false, slot, index);
            }
        }
    }

    std::vector<StorageCopyBack> storageCopies;
    if (type->kind == TypeKind::Record) {
        address = adaptCompositeStorage(address, type, prefixType, reading ? ParameterMode::InOut : ParameterMode::In, storageCopies);
    }
    if (name == "output" && prefixType->kind == TypeKind::Record
        && !hasDefaultDiscriminants(baseType(prefixType))) {
        Type* record = baseType(prefixType);
        for (int i = 0; i < record->discriminantCount; ++i) {
            const FieldInfo& field = discriminantField(record, i);
            emitStreamComponents(stream, false, fieldAddress(address, record, field), field.type);
        }
    }
    std::string constrained = reading && prefixType->kind == TypeKind::Record
        ? recordConstrained(item, &address).name : "1";
    emitStreamComponents(stream, reading, address, prefixType, false, constrained);
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
