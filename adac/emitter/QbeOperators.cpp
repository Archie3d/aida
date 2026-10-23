#include "QbeEmitter.h"
#include "QbeSupport.h"

using QbeSupport::realLiteral;
using QbeSupport::isFloatClass;
using QbeSupport::comparisonInstruction;

Value QbeEmitter::emitBinary(BinaryExpr* expr)
{
    if (expr->operatorCall) {
        return emitExpr(expr->operatorCall.get());
    }
    switch (expr->op) {
    case BinaryOp::AndThen:
    case BinaryOp::OrElse:
        return emitShortCircuit(expr);
    case BinaryOp::Concatenate:
        return emitConcatenation(expr);
    default:
        break;
    }

    Value left = emitExpr(expr->left.get());
    Value right = emitExpr(expr->right.get());
    if (expr->fixedResult && expr->type->kind != TypeKind::Fixed) {
        std::string result = newTemp();
        bool real = isReal(expr->type);
        std::string arguments = "(w " + std::to_string(expr->op == BinaryOp::Multiply ? 2 : 3)
            + ", l " + left.name + ", w " + std::to_string(expr->left->type->fixedScale)
            + ", l " + right.name + ", w " + std::to_string(expr->right->type->fixedScale);
        emitCallInstruction(result + (real ? " =d call $__ada_fixed_real_operation" : " =l call $__ada_fixed_numeric_operation")
            + arguments + ", w " + (real ? (qbeClass(expr->type) == 's' ? "24" : "53")
                                         : (expr->type->modulus != 0 ? "1" : "0")) + ")", CallEffects::CheckException);
        Value value { result, real ? 'd' : 'l' };
        emitRangeCheck(value, m_sema.typeTable().scalarBaseType(expr->type), expr->location);
        char width = qbeClass(expr->type);
        if (width == 's' || width == 'w') {
            std::string narrow = newTemp();
            line(narrow + " =" + width + (width == 's' ? " truncd " : " copy ") + result);
            value = Value { narrow, width };
        }
        return value;
    }
    if (expr->type->kind == TypeKind::Fixed) {
        auto wide = [&](Value value) {
            if (value.type == 'w') {
                std::string name = newTemp();
                line(name + " =l extsw " + value.name);
                value = Value { name, 'l' };
            }
            return value;
        };
        left = wide(left);
        right = wide(right);
        FixedOperation operation = FixedInvalid;
        switch (expr->op) {
        case BinaryOp::Add: operation = FixedAdd; break;
        case BinaryOp::Subtract: operation = FixedSubtract; break;
        case BinaryOp::Multiply: operation = expr->roundFixed ? FixedMultiplyRounded : FixedMultiply; break;
        case BinaryOp::Divide: operation = expr->roundFixed ? FixedDivideRounded : FixedDivide; break;
        default: break;
        }
        std::string result = newTemp();
        emitCallInstruction(result + " =l call $__ada_fixed_operation(w " + std::to_string(operation) + ", l " + left.name
            + ", w " + std::to_string(expr->left->type->fixedScale) + ", l " + right.name
            + ", w " + std::to_string(expr->right->type->fixedScale) + ", w " + std::to_string(expr->type->fixedScale) + ")", CallEffects::CheckException);
        return Value { result, 'l' };
    }
    char type = left.type == 'l' || right.type == 'l' ? 'l' : left.type;
    if (isFloatClass(left.type)) {
        type = left.type;
    } else if (isFloatClass(right.type)) {
        type = right.type;
    }

    if (!isFloatClass(type)) {
        ModularOperation operation = ModularInvalid;
        switch (expr->op) {
        case BinaryOp::Add: operation = ModularAdd; break;
        case BinaryOp::Subtract: operation = ModularSubtract; break;
        case BinaryOp::Multiply: operation = ModularMultiply; break;
        case BinaryOp::Divide: operation = ModularDivide; break;
        case BinaryOp::Remainder: operation = ModularRemainder; break;
        case BinaryOp::Modulo: operation = ModularModulo; break;
        case BinaryOp::Power: operation = ModularPower; break;
        case BinaryOp::And: if (expr->type->modulus != 0) operation = ModularAnd; break;
        case BinaryOp::Or: if (expr->type->modulus != 0) operation = ModularOr; break;
        case BinaryOp::Xor: if (expr->type->modulus != 0) operation = ModularXor; break;
        default: break;
        }
        if (operation != ModularInvalid && expr->type->modulus != 0) {
            return emitModularOperation(operation, left, right, expr->type);
        }
        if (operation != ModularInvalid) {
            return emitIntegerOperation(static_cast<int>(operation), left, right, left.type);
        }
    }

    switch (expr->op) {
    case BinaryOp::Equal:
    case BinaryOp::NotEqual:
    case BinaryOp::Less:
    case BinaryOp::LessEqual:
    case BinaryOp::Greater:
    case BinaryOp::GreaterEqual: {
        if (expr->left->type != nullptr && expr->left->type->kind == TypeKind::Array) {
            return compareArrays(expr->op, left, expr->left->type, right, expr->right->type);
        }
        if (baseType(expr->left->type) != nullptr && baseType(expr->left->type)->kind == TypeKind::Record) {
            // The operands are addresses, so comparing them directly would be
            // asking whether they are the same object rather than equal ones.
            Value equal = compareRecords(left, right, expr->left->type);
            if (expr->op == BinaryOp::NotEqual) {
                std::string negated = newTemp();
                line(negated + " =w ceqw " + equal.name + ", 0");
                return Value { negated, 'w' };
            }
            return equal;
        }
        std::string temp = newTemp();
        line(temp + " =w " + comparisonInstruction(expr->op, type, expr->left->type->modulus != 0) + " " + left.name + ", " + right.name);
        return Value { temp, 'w' };
    }
    case BinaryOp::Modulo:
        return emitModulo(left, right, type);
    case BinaryOp::Power:
        return emitPower(left, right, type);
    default:
        break;
    }

    const char* instruction = "add";
    if (isFloatClass(type) && expr->op >= BinaryOp::Add && expr->op <= BinaryOp::Divide) {
        RealOperation operation = expr->op == BinaryOp::Add ? RealAdd
            : expr->op == BinaryOp::Subtract ? RealSubtract
            : expr->op == BinaryOp::Multiply ? RealMultiply : RealDivide;
        return emitRealOperation(operation, left, right, type);
    }
    switch (expr->op) {
    case BinaryOp::Add:
        instruction = "add";
        break;
    case BinaryOp::Subtract:
        instruction = "sub";
        break;
    case BinaryOp::Multiply:
        instruction = "mul";
        break;
    case BinaryOp::Divide:
        instruction = "div";
        break;
    case BinaryOp::Remainder:
        instruction = "rem";
        break;
    case BinaryOp::And:
        instruction = "and";
        break;
    case BinaryOp::Or:
        instruction = "or";
        break;
    case BinaryOp::Xor:
        instruction = "xor";
        break;
    default:
        break;
    }

    std::string temp = newTemp();
    line(temp + " =" + std::string(1, type) + " " + instruction + " " + left.name + ", " + right.name);
    return Value { temp, type };
}

Value QbeEmitter::emitUnary(UnaryExpr* expr)
{
    if (expr->operatorCall) {
        return emitExpr(expr->operatorCall.get());
    }
    Value operand = emitExpr(expr->operand.get());
    std::string temp = newTemp();

    if (expr->type->modulus != 0) {
        if (expr->op == UnaryOp::Negate) {
            return emitModularOperation(ModularSubtract, Value { "0", operand.type }, operand, expr->type);
        }
        if (expr->op == UnaryOp::Not) {
            return emitModularOperation(ModularNot, operand, Value { "0", operand.type }, expr->type);
        }
    }
    switch (expr->op) {
    case UnaryOp::Plus:
        return operand;
    case UnaryOp::Negate: {
        if (!isFloatClass(operand.type)) {
            return emitIntegerOperation(1, Value { "0", operand.type }, operand, operand.type);
        }
        std::string zero = isFloatClass(operand.type) ? realLiteral(0.0, operand.type) : std::string("0");
        line(temp + " =" + std::string(1, operand.type) + " sub " + zero + ", " + operand.name);
        return Value { temp, operand.type };
    }
    case UnaryOp::Not:
        line(temp + " =w ceqw " + operand.name + ", 0");
        return Value { temp, 'w' };
    case UnaryOp::Abs: {
        bool isFloat = isFloatClass(operand.type);
        long long slotSize = operand.type == 'l' || operand.type == 'd' ? 8 : 4;
        std::string slot = allocScratch(slotSize);
        const char* storeInstruction = isFloat ? (operand.type == 'd' ? "stored" : "stores")
                                               : (operand.type == 'l' ? "storel" : "storew");
        const char* loadInstruction = isFloat ? (operand.type == 'd' ? "loadd" : "loads")
                                              : (operand.type == 'l' ? "loadl" : "loadsw");
        std::string zero = isFloat ? realLiteral(0.0, operand.type) : std::string("0");
        line(std::string(storeInstruction) + " " + operand.name + ", " + slot);
        std::string negative = newTemp();
        line(negative + " =w " + comparisonInstruction(BinaryOp::Less, operand.type) + " " + operand.name + ", "
             + zero);
        std::string negate = newLabel("absneg");
        std::string done = newLabel("absdone");
        branch(Value { negative, 'w' }, negate, done);
        label(negate);
        std::string negated = newTemp();
        if (isFloat) {
            line(negated + " =" + std::string(1, operand.type) + " sub " + zero + ", " + operand.name);
        } else {
            negated = emitIntegerOperation(1, Value { "0", operand.type }, operand, operand.type).name;
        }
        line(std::string(storeInstruction) + " " + negated + ", " + slot);
        jump(done);
        label(done);
        std::string result = newTemp();
        line(result + " =" + std::string(1, operand.type) + " " + loadInstruction + " " + slot);
        return Value { result, operand.type };
    }
    }

    return operand;
}

Value QbeEmitter::emitDiscreteConversion(const Value& value, Type* source, Type* target)
{
    Value wide = wideBound(value);
    Value bounds = scalarBounds(target);
    Value low = wideBound(Value { bounds.first, bounds.type });
    Value high = wideBound(Value { bounds.last, bounds.type });
    std::string result = newTemp();
    emitCallInstruction(result + " =l call $__ada_modular_convert(l " + wide.name + ", w "
        + (source->modulus != 0 ? "1" : "0") + ", l " + low.name
        + ", l " + high.name + ", w " + (target->modulus != 0 ? "1" : "0") + ")", CallEffects::CheckException);
    if (qbeClass(target) == 'w') {
        std::string narrow = newTemp();
        line(narrow + " =w copy " + result);
        result = narrow;
    }
    return Value { result, qbeClass(target) };
}

Value QbeEmitter::emitModularOperation(ModularOperation operation, const Value& left, const Value& right, Type* type)
{
    auto widen = [&](const Value& value) {
        if (value.type == 'l') {
            return value.name;
        }
        std::string result = newTemp();
        line(result + " =l extsw " + value.name);
        return result;
    };
    std::string leftWide = widen(left);
    std::string rightWide = widen(right);
    std::string result = newTemp();
    emitCallInstruction(result + " =l call $__ada_modular_operation(w " + std::to_string(static_cast<int>(operation))
         + ", l " + scalarText(type->modulus) + ", l " + leftWide + ", l " + rightWide + ")", CallEffects::CheckException);
    if (qbeClass(type) == 'w') {
        std::string narrow = newTemp();
        line(narrow + " =w copy " + result);
        result = narrow;
    }
    return Value { result, qbeClass(type) };
}

Value QbeEmitter::emitIntegerOperation(int operation, const Value& left, const Value& right, char type)
{
    // Operands have already been evaluated. Keep the successful path entirely
    // in QBE, and use the ordinary exception edge only when a check fails.
    auto instruction = [&](char resultType, const std::string& op,
                           const std::string& a, const std::string& b) {
        std::string result = newTemp();
        line(result + " =" + resultType + " " + op + " " + a + ", " + b);
        return result;
    };
    auto check = [&](const std::string& invalid) {
        std::string bad = newLabel("arithmeticbad");
        std::string good = newLabel("arithmeticok");
        branch(Value { invalid, 'w' }, bad, good);
        label(bad);
        raiseConstraintError();
        label(good);
    };
    auto widen = [&](const Value& value) {
        if (value.type == 'l') {
            return value.name;
        }
        std::string wide = newTemp();
        line(wide + " =l extsw " + value.name);
        return wide;
    };
    std::string suffix(1, type);
    std::string low = type == 'w' ? "-2147483648" : "-9223372036854775808";
    if (operation == ModularPower) {
        check(instruction('w', "cslt" + std::string(1, right.type), right.name, "0"));
        std::string resultSlot = allocScratch(8);
        std::string baseSlot = allocScratch(8);
        std::string exponentSlot = allocScratch(8);
        line("store" + suffix + " 1, " + resultSlot);
        line("store" + suffix + " " + left.name + ", " + baseSlot);
        line("storel " + widen(right) + ", " + exponentSlot);
        std::string test = newLabel("powtest");
        std::string body = newLabel("powbody");
        std::string multiply = newLabel("powmultiply");
        std::string shift = newLabel("powshift");
        std::string square = newLabel("powsquare");
        std::string done = newLabel("powdone");
        auto load = [&](const std::string& slot, char loadType) {
            std::string value = newTemp();
            line(value + " =" + loadType + (loadType == 'l' ? " loadl " : " loadsw ") + slot);
            return Value { value, loadType };
        };
        jump(test);
        label(test);
        Value exponent = load(exponentSlot, 'l');
        branch(Value { instruction('w', "cnel", exponent.name, "0"), 'w' }, body, done);
        label(body);
        Value base = load(baseSlot, type);
        branch(Value { instruction('l', "and", exponent.name, "1"), 'l' }, multiply, shift);
        label(multiply);
        Value product = emitIntegerOperation(ModularMultiply, load(resultSlot, type), base, type);
        line("store" + suffix + " " + product.name + ", " + resultSlot);
        jump(shift);
        label(shift);
        std::string next = instruction('l', "shr", exponent.name, "1");
        line("storel " + next + ", " + exponentSlot);
        branch(Value { instruction('w', "cnel", next, "0"), 'w' }, square, done);
        label(square);
        Value squared = emitIntegerOperation(ModularMultiply, base, base, type);
        line("store" + suffix + " " + squared.name + ", " + baseSlot);
        jump(test);
        label(done);
        return load(resultSlot, type);
    }
    if (operation <= ModularMultiply) {
        const char* op = operation == ModularAdd ? "add" : operation == ModularSubtract ? "sub" : "mul";
        if (type == 'w') {
            // Every product of two signed words fits in a signed long.
            std::string result = instruction('l', op, widen(left), widen(right));
            std::string below = instruction('w', "csltl", result, low);
            std::string above = instruction('w', "csgtl", result, "2147483647");
            check(instruction('w', "or", below, above));
            std::string narrowed = newTemp();
            line(narrowed + " =w copy " + result);
            return Value { narrowed, type };
        }
        if (operation == ModularMultiply) {
            // The common case needs no division: two signed words always
            // have a representable signed-long product.
            std::string aWord = newTemp();
            std::string bWord = newTemp();
            line(aWord + " =l extsw " + left.name);
            line(bWord + " =l extsw " + right.name);
            std::string fits = instruction('w', "and", instruction('w', "ceql", left.name, aWord),
                                           instruction('w', "ceql", right.name, bWord));
            std::string multiply = newLabel("mulfit");
            std::string wide = newLabel("mulwide");
            branch(Value { fits, 'w' }, multiply, wide);
            label(wide);
            // Unsigned magnitudes include abs(Long_Integer'First). Bound the
            // product before multiplying; b | (b = 0) makes the division safe
            // even for zero, without a second branch on the hot path.
            std::string signA = instruction('l', "sar", left.name, "63");
            std::string signB = instruction('l', "sar", right.name, "63");
            std::string a = instruction('l', "sub", instruction('l', "xor", left.name, signA), signA);
            std::string b = instruction('l', "sub", instruction('l', "xor", right.name, signB), signB);
            std::string negative = instruction('l', "and", instruction('l', "xor", signA, signB), "1");
            std::string limit = instruction('l', "add", "9223372036854775807", negative);
            std::string zero = instruction('w', "ceql", b, "0");
            std::string zeroLong = newTemp();
            line(zeroLong + " =l extuw " + zero);
            std::string divisor = instruction('l', "or", b, zeroLong);
            std::string bound = instruction('l', "udiv", limit, divisor);
            // A zero multiplier permits every magnitude, including 2**63.
            std::string tooLarge = instruction('w', "cugtl", a, bound);
            std::string nonzero = instruction('w', "cnel", b, "0");
            check(instruction('w', "and", tooLarge, nonzero));
            jump(multiply);
            label(multiply);
            return Value { instruction('l', "mul", left.name, right.name), type };
        }
        std::string result = instruction('l', op, left.name, right.name);
        // Add overflows when equal-sign operands produce a different sign;
        // subtract overflows when differing-sign operands change the left sign.
        std::string changed = instruction('l', "xor", left.name, result);
        std::string other = instruction('l', "xor", operation == ModularAdd ? right.name : left.name,
                                        operation == ModularAdd ? result : right.name);
        std::string signs = instruction('l', "and", changed, other);
        check(instruction('w', "csltl", signs, "0"));
        return Value { result, type };
    }
    check(instruction('w', "ceq" + suffix, right.name, "0"));
    std::string minimum = instruction('w', "ceq" + suffix, left.name, low);
    std::string minusOne = instruction('w', "ceq" + suffix, right.name, "-1");
    std::string special = instruction('w', "and", minimum, minusOne);
    if (operation == ModularDivide) {
        check(special);
        return Value { instruction(type, "div", left.name, right.name), type };
    }
    // MIN rem -1 and MIN mod -1 are zero, but hardware signed division
    // may trap even when only its remainder is requested. Use divisor +1.
    std::string specialWide = special;
    if (type == 'l') {
        specialWide = newTemp();
        line(specialWide + " =l extuw " + special);
    }
    std::string divisor = instruction(type, "add", right.name, instruction(type, "shl", specialWide, "1"));
    std::string result = instruction(type, "rem", left.name, divisor);
    if (operation == ModularModulo) {
        std::string differentSigns = instruction('w', "cslt" + suffix,
                                                 instruction(type, "xor", result, right.name), "0");
        std::string adjust = instruction('w', "and", differentSigns,
                                         instruction('w', "cne" + suffix, result, "0"));
        // A mask selects either zero or the divisor; the adjusted result is
        // always in range and has the divisor's sign.
        std::string wideAdjust = adjust;
        if (type == 'l') {
            wideAdjust = newTemp();
            line(wideAdjust + " =l extuw " + adjust);
        }
        std::string mask = instruction(type, "sub", "0", wideAdjust);
        result = instruction(type, "add", result, instruction(type, "and", mask, right.name));
    }
    return Value { result, type };
}

Value QbeEmitter::emitRealOperation(RealOperation operation, const Value& left, const Value& right, char type)
{
    std::string result = newTemp();
    std::string width(1, type);
    emitCallInstruction(result + " =" + width + " call $__ada_real_operation" + (type == 's' ? "_f32" : "")
        + "(w " + std::to_string(operation) + ", " + width + " " + left.name + ", " + width + " " + right.name + ")", CallEffects::CheckException);
    return Value { result, type };
}

Value QbeEmitter::emitShortCircuit(BinaryExpr* expr)
{
    std::string slot = allocScratch(4);
    Value left = emitExpr(expr->left.get());
    line("storew " + left.name + ", " + slot);

    std::string evaluate = newLabel("shortcircuit");
    std::string done = newLabel("shortdone");
    if (expr->op == BinaryOp::AndThen) {
        branch(left, evaluate, done);
    } else {
        branch(left, done, evaluate);
    }
    label(evaluate);
    Value right = emitExpr(expr->right.get());
    line("storew " + right.name + ", " + slot);
    jump(done);
    label(done);

    std::string result = newTemp();
    line(result + " =w loadsw " + slot);
    return Value { result, 'w' };
}

Value QbeEmitter::emitModulo(const Value& left, const Value& right, char type)
{
    std::string slot = allocScratch(type == 'l' ? 8 : 4);
    std::string remainder = newTemp();
    line(remainder + " =" + std::string(1, type) + " rem " + left.name + ", " + right.name);
    line(std::string(type == 'l' ? "storel " : "storew ") + remainder + ", " + slot);

    std::string sign = newTemp();
    line(sign + " =" + std::string(1, type) + " xor " + remainder + ", " + right.name);
    std::string negative = newTemp();
    line(negative + " =w " + std::string(type == 'l' ? "csltl" : "csltw") + " " + sign + ", 0");
    std::string nonZero = newTemp();
    line(nonZero + " =w " + std::string(type == 'l' ? "cnel" : "cnew") + " " + remainder + ", 0");
    std::string adjust = newTemp();
    line(adjust + " =w and " + negative + ", " + nonZero);

    std::string fix = newLabel("modfix");
    std::string done = newLabel("moddone");
    branch(Value { adjust, 'w' }, fix, done);
    label(fix);
    std::string adjusted = newTemp();
    line(adjusted + " =" + std::string(1, type) + " add " + remainder + ", " + right.name);
    line(std::string(type == 'l' ? "storel " : "storew ") + adjusted + ", " + slot);
    jump(done);
    label(done);

    std::string result = newTemp();
    line(result + " =" + std::string(1, type) + " " + (type == 'l' ? "loadl " : "loadsw ") + slot);
    return Value { result, type };
}

Value QbeEmitter::emitPower(const Value& left, const Value& right, char type)
{
    std::string result = newTemp();
    std::string width(1, type);
    emitCallInstruction(result + " =" + width + " call $__ada_real_power" + (type == 's' ? "_f32" : "")
        + "(" + width + " " + left.name + ", w " + right.name + ")", CallEffects::CheckException);
    return Value { result, type };
}
