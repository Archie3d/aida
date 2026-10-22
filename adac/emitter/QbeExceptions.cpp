#include "QbeEmitter.h"
#include "QbeSupport.h"

#include <cctype>
#include <limits>

using QbeSupport::realLiteral;
using QbeSupport::comparisonInstruction;

namespace
{

std::string upperCase(const std::string& text)
{
    std::string result;
    result.reserve(text.size());
    for (char c : text) {
        result.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
    }
    return result;
}

}

// The unit declaring an exception is the one holding the object that stands
// for it, so a handler anywhere in the program compares against one address.
void QbeEmitter::emitExceptionObjects(const std::vector<Symbol*>& exceptions)
{
    for (Symbol* exception : exceptions) {
        std::string name = stringData(upperCase(exception->displayName));
        m_data << "export data " << exception->exceptionObject << " = align 8 { l " << name << " }\n";
    }
}

void QbeEmitter::emitRaise(Symbol* exception, const SourceLocation& location, Expr* message)
{
    if (exception == nullptr) {
        if (m_context->activeExceptions.empty()) {
            m_diagnostics.error(location, "internal error: bare raise without an active handler");
            return;
        }
        emitCallInstruction("call $__ada_reraise(l " + m_context->activeExceptions.back() + ")", CallEffects::DeferredCheck);
    } else if (message != nullptr) {
        Value text = emitExpr(message);
        Value length = narrowArrayLength(lengthOf(text, message->type));
        emitCallInstruction("call $__ada_raise_message(l " + exception->exceptionObject + ", l " + text.name
             + ", w " + length.name + ")", CallEffects::DeferredCheck);
    } else {
        emitCallInstruction("call $__ada_raise(l " + exception->exceptionObject + ")", CallEffects::DeferredCheck);
    }
    if (!m_context->handlerLabels.empty()) {
        jump(m_context->handlerLabels.back());
    } else {
        m_context->usesPropagate = true;
        jump(m_context->propagateLabel);
    }
}

void QbeEmitter::emitExceptionCheck()
{
    std::string pending = newTemp();
    std::string raised = newTemp();
    std::string next = newLabel("nothrow");
    line(pending + " =l loadl %.taskContext");
    line(raised + " =w cnel " + pending + ", 0");
    if (!m_context->handlerLabels.empty()) {
        branch(Value { raised, 'w' }, m_context->handlerLabels.back(), next);
    } else {
        m_context->usesPropagate = true;
        branch(Value { raised, 'w' }, m_context->propagateLabel, next);
    }
    label(next);
}

void QbeEmitter::emitHandlers(std::vector<ExceptionHandler>& handlers, const std::string& afterLabel,
                              const std::string& dispatchLabel)
{
    label(dispatchLabel);
    rewindStorage(m_context->handlerStorage.at(dispatchLabel), false);

    // Save the occurrence before clearing the pending status. Nested handlers
    // and calls may replace the context's exception while this handler is active.
    std::string pending = newTemp();
    line(pending + " =l loadl %.taskContext");
    std::vector<std::string> bodyLabels;
    for (std::size_t i = 0; i < handlers.size(); ++i) {
        bodyLabels.push_back(newLabel("handle"));
    }

    for (std::size_t i = 0; i < handlers.size(); ++i) {
        ExceptionHandler& handler = handlers[i];
        if (handler.isOthers) {
            jump(bodyLabels[i]);
            break;
        }
        std::string match;
        for (Symbol* caught : handler.exceptions) {
            std::string test = newTemp();
            line(test + " =w ceql " + pending + ", " + caught->exceptionObject);
            if (match.empty()) {
                match = test;
            } else {
                std::string combined = newTemp();
                line(combined + " =w or " + match + ", " + test);
                match = combined;
            }
        }
        std::string next = newLabel("nexthandler");
        if (match.empty()) {
            jump(next);
        } else {
            branch(Value { match, 'w' }, bodyLabels[i], next);
        }
        label(next);
    }

    if (!m_context->terminated) {
        if (m_context->handlerLabels.empty()) {
            m_context->usesPropagate = true;
            jump(m_context->propagateLabel);
        } else {
            jump(m_context->handlerLabels.back());
        }
    }

    for (std::size_t i = 0; i < handlers.size(); ++i) {
        label(bodyLabels[i]);
        std::string occurrence;
        if (Symbol* choice = handlers[i].choiceSymbol) {
            if (choice->isUplevel) {
                choice->frameOffset = reserveFrame(typeSize(choice->type));
            } else {
                m_context->locals[choice] = allocScratch(typeSize(choice->type));
            }
            occurrence = addressOf(choice).name;
        } else {
            occurrence = allocScratch(typeSize(m_sema.exceptionOccurrenceType()));
        }
        emitCallInstruction("call $__ada_exception_capture(l " + occurrence + ", l " + storageArena(false, true) + ")", CallEffects::CheckException);
        m_context->activeExceptions.push_back(occurrence);
        emitStatements(handlers[i].body);
        m_context->activeExceptions.pop_back();
        jump(afterLabel);
    }
}

void QbeEmitter::emitRangeCheck(const Value& value, Type* type, const SourceLocation& location)
{
    if (type != nullptr && type->kind == TypeKind::Float) {
        double maximum = typeSize(type) == 4 ? std::numeric_limits<float>::max()
            : std::numeric_limits<double>::max();
        char type_ = value.type;
        std::string lowTest = newTemp();
        std::string highTest = newTemp();
        std::string combined = newTemp();
        line(lowTest + " =w " + comparisonInstruction(BinaryOp::GreaterEqual, type_) + " " + value.name + ", "
             + realLiteral(type->hasRealRange ? type->lowReal : -maximum, type_));
        line(highTest + " =w " + comparisonInstruction(BinaryOp::LessEqual, type_) + " " + value.name + ", "
             + realLiteral(type->hasRealRange ? type->highReal : maximum, type_));
        line(combined + " =w and " + lowTest + ", " + highTest);

        std::string ok = newLabel("inrange");
        std::string bad = newLabel("outofrange");
        branch(Value { combined, 'w' }, ok, bad);
        label(bad);
        raiseConstraintError();
        label(ok);
        return;
    }
    if (type == nullptr || (!isDiscrete(type) && type->kind != TypeKind::Fixed)) {
        return;
    }
    Value bounds = scalarBounds(type);
    Value checked = value;
    char width = value.type;
    if (type->m_scalarBoundsSymbol != nullptr && (value.type == 'l' || bounds.type == 'l')) {
        width = 'l';
        if (checked.type == 'w') {
            checked.name = newTemp();
            line(checked.name + " =l extsw " + value.name);
        }
        if (bounds.type == 'w') {
            std::string low = newTemp();
            std::string high = newTemp();
            line(low + " =l extsw " + bounds.first);
            line(high + " =l extsw " + bounds.last);
            bounds.first = low;
            bounds.last = high;
        }
    }
    std::string lowTest = newTemp();
    std::string highTest = newTemp();
    std::string combined = newTemp();
    line(lowTest + " =w " + comparisonInstruction(BinaryOp::GreaterEqual, width, type->m_modulus != 0) + " " + checked.name + ", "
         + bounds.first);
    line(highTest + " =w " + comparisonInstruction(BinaryOp::LessEqual, width, type->m_modulus != 0) + " " + checked.name + ", "
         + bounds.last);
    line(combined + " =w and " + lowTest + ", " + highTest);

    std::string ok = newLabel("inrange");
    std::string bad = newLabel("outofrange");
    branch(Value { combined, 'w' }, ok, bad);
    label(bad);
    (void)location;
    raiseConstraintError();
    label(ok);
}

// A component of a variant part is only there when the discriminant selects
// that alternative.  Where nothing fixed the discriminant, the value carries
// the answer and reaching for the wrong component raises Constraint_Error.
Value QbeEmitter::variantCondition(const Value& address, Type* record, int variant)
{
    const VariantInfo& alternative = record->variants[variant];
    const FieldInfo& discriminant = record->fields[alternative.discriminant];
    Value slot = address;
    if (discriminant.offset != 0) {
        std::string moved = newTemp();
        line(moved + " =l add " + address.name + ", " + std::to_string(discriminant.offset));
        slot = Value { moved, 'l' };
    }
    Value value = loadFrom(slot, discriminant.type);
    std::string matched = newTemp();
    line(matched + " =w copy 0");
    for (std::size_t i = 0; i < record->variants.size(); ++i) {
        const VariantInfo& other = record->variants[i];
        if (alternative.isOthers ? (other.parent != alternative.parent || other.isOthers)
                                 : static_cast<int>(i) != variant) {
            continue;
        }
        for (const VariantChoice& choice : other.choices) {
            std::string within = newTemp();
            line(within + " =w " + rangeTest(value, choice.low, choice.high));
            std::string combined = newTemp();
            line(combined + " =w or " + matched + ", " + within);
            matched = combined;
        }
    }
    if (alternative.isOthers) {
        std::string inverted = newTemp();
        line(inverted + " =w xor " + matched + ", 1");
        matched = inverted;
    }
    if (alternative.parent >= 0) {
        Value parent = variantCondition(address, record, alternative.parent);
        std::string combined = newTemp();
        line(combined + " =w and " + matched + ", " + parent.name);
        matched = combined;
    }
    return Value { matched, 'w' };
}

void QbeEmitter::checkVariant(const Value& address, Type* record, int variant)
{
    Value matches = variantCondition(address, record, variant);
    std::string ok = newLabel("rightvariant");
    std::string bad = newLabel("wrongvariant");
    branch(matches, ok, bad);
    label(bad);
    raiseConstraintError();
    label(ok);
}

// The instruction that answers whether a discriminant value falls in a range.
std::string QbeEmitter::rangeTest(const Value& value, long long low, long long high)
{
    if (low == high) {
        return std::string(comparisonInstruction(BinaryOp::Equal, value.type)) + " " + value.name + ", "
            + std::to_string(low);
    }
    std::string atLeast = newTemp();
    std::string atMost = newTemp();
    line(atLeast + " =w " + comparisonInstruction(BinaryOp::GreaterEqual, value.type) + " " + value.name + ", "
         + std::to_string(low));
    line(atMost + " =w " + comparisonInstruction(BinaryOp::LessEqual, value.type) + " " + value.name + ", "
         + std::to_string(high));
    return "and " + atLeast + ", " + atMost;
}

void QbeEmitter::checkNotNull(const Value& pointer)
{
    // null designates no object at all, so reaching through it is an error.
    std::string isNull = newTemp();
    line(isNull + " =w ceql " + pointer.name + ", 0");

    std::string bad = newLabel("nullaccess");
    std::string ok = newLabel("notnull");
    branch(Value { isNull, 'w' }, bad, ok);
    label(bad);
    raiseConstraintError();
    label(ok);
}

void QbeEmitter::raiseConstraintError()
{
    emitCallInstruction("call $__ada_raise(l $__ada_exc_constraint_error)", CallEffects::DeferredCheck);
    if (!m_context->handlerLabels.empty()) {
        jump(m_context->handlerLabels.back());
    } else {
        m_context->usesPropagate = true;
        jump(m_context->propagateLabel);
    }
}

void QbeEmitter::emitRecordConstraintCheck(const Value& value, Type* type)
{
    for (int i = 0; i < type->discriminantCount; ++i) {
        long long fixed = 0;
        if (!discriminantValueOf(type, i, fixed)) {
            continue;
        }
        const FieldInfo& field = type->fields[i];
        std::string slot = newTemp();
        line(slot + " =l add " + value.name + ", " + std::to_string(field.offset));
        Value actual = loadFrom(Value { slot, 'l' }, field.type);
        std::string same = newTemp();
        line(same + " =w ceq" + actual.type + " " + actual.name + ", " + std::to_string(fixed));
        std::string good = newLabel("conversionconstraint");
        std::string bad = newLabel("conversionconstraintbad");
        branch(Value { same, 'w' }, good, bad);
        label(bad);
        raiseConstraintError();
        label(good);
    }
}
