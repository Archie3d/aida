#include "Parser.h"

#include <utility>

DeclPtr Parser::parseTypeDecl()
{
    auto decl = std::make_unique<TypeDecl>();
    decl->location = current().location;
    expect(TokenKind::KwType, "in type declaration");
    const Token& name = expect(TokenKind::Identifier, "in type declaration");
    decl->name = name.text;
    decl->lower = name.lower;

    // 'type Shape (Kind : Figure)' names the discriminants an object of the
    // type is fixed with when it is declared.
    if (check(TokenKind::LeftParen) && peek(1).kind == TokenKind::Box) {
        advance();
        advance();
        expect(TokenKind::RightParen, "after unknown discriminants");
        decl->unknownDiscriminants = true;
    } else if (check(TokenKind::LeftParen)) {
        parseDiscriminantPart(decl->discriminants);
    }

    // 'type Node;' names a type and says no more about it, which is how a
    // record and an access type pointing at it are declared in either order.
    if (match(TokenKind::Semicolon)) {
        return decl;
    }

    expect(TokenKind::KwIs, "in type declaration");
    if (check(TokenKind::KwTagged) && peek(1).kind == TokenKind::Semicolon) {
        advance();
        advance();
        decl->taggedIncomplete = true;
        return decl;
    }
    if (decl->unknownDiscriminants) { fail("unknown discriminants on full or private type declarations are not yet supported"); }
    decl->definition = parseTypeDefinition();
    expect(TokenKind::Semicolon, "after type declaration");
    return decl;
}

DeclPtr Parser::parseSubtypeDecl()
{
    auto decl = std::make_unique<SubtypeDecl>();
    decl->location = current().location;
    expect(TokenKind::KwSubtype, "in subtype declaration");
    const Token& name = expect(TokenKind::Identifier, "in subtype declaration");
    decl->name = name.text;
    decl->lower = name.lower;
    expect(TokenKind::KwIs, "in subtype declaration");
    decl->subtype = parseSubtypeIndication();
    expect(TokenKind::Semicolon, "after subtype declaration");
    return decl;
}

TypeDefinitionPtr Parser::parseTypeDefinition(bool allowProtectedAccess)
{
    SourceLocation location = current().location;

    if (check(TokenKind::KwInterface) || check(TokenKind::KwSynchronized)
        || ((check(TokenKind::KwLimited) || check(TokenKind::KwTask)
             || check(TokenKind::KwProtected)) && peek(1).kind == TokenKind::KwInterface)) {
        fail("interface types are not yet supported");
    }

    if (match(TokenKind::KwAbstract)) {
        auto definition = parseTypeDefinition();
        definition->abstract = true;
        definition->location = location;
        return definition;
    }

    if (match(TokenKind::KwTagged)) {
        if (!check(TokenKind::KwRecord) && !check(TokenKind::KwNull)
            && !check(TokenKind::KwPrivate) && !check(TokenKind::KwLimited)) {
            fail("expected record, null record or private after tagged");
        }
        auto definition = parseTypeDefinition();
        definition->tagged = true;
        definition->location = location;
        return definition;
    }

    if (check(TokenKind::KwLimited) && peek(1).kind != TokenKind::KwPrivate) {
        advance();
        if (!check(TokenKind::KwRecord) && !check(TokenKind::KwNull)) {
            fail("expected record or null record after limited");
        }
        auto definition = parseTypeDefinition();
        definition->isLimited = true;
        return definition;
    }

    if (check(TokenKind::LeftParen)) {
        auto definition = std::make_unique<TypeDefinition>(TypeDefKind::Enumeration);
        definition->location = location;
        advance();
        while (true) {
            if (check(TokenKind::Identifier) || check(TokenKind::CharacterLiteral)) {
                const Token& literal = advance();
                bool character = literal.kind == TokenKind::CharacterLiteral;
                definition->literals.push_back(character ? "'" + literal.text + "'" : literal.text);
                definition->literalsLower.push_back(character ? "'" + literal.text + "'" : literal.lower);
            } else {
                fail("expected an enumeration literal");
            }
            if (!match(TokenKind::Comma)) {
                break;
            }
        }
        expect(TokenKind::RightParen, "after enumeration literals");
        return definition;
    }

    // 'is private' and 'is limited private' name a type without saying what it
    // is made of; the private part of the package says that.
    if (check(TokenKind::KwPrivate) || (check(TokenKind::KwLimited) && peek(1).kind == TokenKind::KwPrivate)) {
        auto definition = std::make_unique<TypeDefinition>(TypeDefKind::Private);
        definition->location = location;
        definition->isLimited = match(TokenKind::KwLimited);
        expect(TokenKind::KwPrivate, "in private type declaration");
        return definition;
    }

    if (check(TokenKind::KwRange)) {
        auto definition = std::make_unique<TypeDefinition>(TypeDefKind::IntegerRange);
        definition->location = location;
        advance();
        definition->rangeLow = parseSimpleExpression();
        expect(TokenKind::DoubleDot, "in range");
        definition->rangeHigh = parseSimpleExpression();
        return definition;
    }

    if (match(TokenKind::KwMod)) {
        auto definition = std::make_unique<TypeDefinition>(TypeDefKind::Modular);
        definition->location = location;
        definition->rangeHigh = parseExpression();
        return definition;
    }

    if (match(TokenKind::KwDelta)) {
        auto definition = std::make_unique<TypeDefinition>(TypeDefKind::FixedDelta);
        definition->location = location;
        definition->delta = parseSimpleExpression();
        if (match(TokenKind::KwDigits)) {
            definition->digits = parseSimpleExpression();
            if (!check(TokenKind::KwRange)) { return definition; }
        }
        expect(TokenKind::KwRange, "in fixed-point definition");
        definition->rangeLow = parseSimpleExpression();
        expect(TokenKind::DoubleDot, "in fixed-point range");
        definition->rangeHigh = parseSimpleExpression();
        return definition;
    }

    if (check(TokenKind::KwDigits)) {
        auto definition = std::make_unique<TypeDefinition>(TypeDefKind::FloatDigits);
        definition->location = location;
        advance();
        definition->digits = parseSimpleExpression();
        if (match(TokenKind::KwRange)) {
            definition->rangeLow = parseSimpleExpression();
            expect(TokenKind::DoubleDot, "in range");
            definition->rangeHigh = parseSimpleExpression();
        }
        return definition;
    }

    if (check(TokenKind::KwArray)) {
        auto definition = std::make_unique<TypeDefinition>(TypeDefKind::Array);
        definition->location = location;
        advance();
        expect(TokenKind::LeftParen, "in array type definition");
        bool firstUnconstrained = false;
        bool mixedIndexesReported = false;
        while (true) {
            auto index = std::make_unique<SubtypeIndication>();
            index->location = current().location;
            bool unconstrained = false;
            std::size_t saved = m_position;
            if (check(TokenKind::Identifier)) {
                index->name = parseSubtypeMark(index->lower);
                if (match(TokenKind::KwRange)) {
                    if (match(TokenKind::Box)) {
                        unconstrained = true;
                        definition->unconstrainedIndexes = true;
                    } else {
                        index->rangeLow = parseSimpleExpression();
                        expect(TokenKind::DoubleDot, "in index range");
                        index->rangeHigh = parseSimpleExpression();
                    }
                } else if (check(TokenKind::DoubleDot)) {
                    m_position = saved;
                    index->name.clear();
                    index->lower.clear();
                    index->rangeLow = parseSimpleExpression();
                    expect(TokenKind::DoubleDot, "in index range");
                    index->rangeHigh = parseSimpleExpression();
                }
            } else {
                index->rangeLow = parseSimpleExpression();
                expect(TokenKind::DoubleDot, "in index range");
                index->rangeHigh = parseSimpleExpression();
            }
            // RM 3.6: an array definition uses index_subtype_definitions
            // (all boxed) or discrete_subtype_definitions (none boxed).
            if (definition->indexTypes.empty()) {
                firstUnconstrained = unconstrained;
            } else if (unconstrained != firstUnconstrained && !mixedIndexesReported) {
                m_diagnostics.error(index->location, "array indices must be either all constrained or all unconstrained");
                mixedIndexesReported = true;
            }
            definition->indexTypes.push_back(std::move(index));
            if (!match(TokenKind::Comma)) {
                break;
            }
        }
        expect(TokenKind::RightParen, "after array index list");
        expect(TokenKind::KwOf, "in array type definition");
        definition->aliasedElements = match(TokenKind::KwAliased);
        definition->elementType = parseSubtypeIndication(true);
        return definition;
    }

    if (check(TokenKind::KwRecord) || (check(TokenKind::KwNull) && peek(1).kind == TokenKind::KwRecord)) {
        auto definition = std::make_unique<TypeDefinition>(TypeDefKind::Record);
        definition->location = location;
        if (match(TokenKind::KwNull)) {
            expect(TokenKind::KwRecord, "in null record definition");
            return definition;
        }
        advance();
        parseRecordComponents(definition->fields);
        // A variant part comes last, after the components every value has.
        if (check(TokenKind::KwCase)) {
            definition->variant = parseVariantPart();
        }
        expect(TokenKind::KwEnd, "at end of record definition");
        expect(TokenKind::KwRecord, "at end of record definition");
        return definition;
    }

    if (check(TokenKind::KwNew)) {
        auto definition = std::make_unique<TypeDefinition>(TypeDefKind::Derived);
        definition->location = location;
        advance();
        definition->parent = parseSubtypeIndication();
        if (match(TokenKind::KwWith)) {
            if (!check(TokenKind::KwRecord) && !check(TokenKind::KwNull)) {
                fail("only record extensions are supported after with");
            }
            auto extension = parseTypeDefinition();
            definition->extension = true;
            definition->fields = std::move(extension->fields);
            definition->variant = std::move(extension->variant);
        }
        return definition;
    }

    if (check(TokenKind::KwAccess) || check(TokenKind::KwNot)) {
        bool nullExclusion = match(TokenKind::KwNot);
        if (nullExclusion) { expect(TokenKind::KwNull, "after not in null exclusion"); }
        auto definition = std::make_unique<TypeDefinition>(TypeDefKind::Access);
        definition->location = location;
        definition->nullExclusion = nullExclusion;
        expect(TokenKind::KwAccess, "in access type definition");
        if (check(TokenKind::KwProtected)) {
            if (!allowProtectedAccess) { fail("access-to-protected-subprogram types are not yet supported"); }
            advance();
            definition->accessProtected = true;
        }
        definition->generalAccess = match(TokenKind::KwAll);
        if (definition->generalAccess && check(TokenKind::KwConstant)) {
            fail("'access all' cannot be combined with 'constant'");
        }
        definition->accessConstant = match(TokenKind::KwConstant);
        definition->generalAccess |= definition->accessConstant;
        if (definition->generalAccess && (check(TokenKind::KwProcedure) || check(TokenKind::KwFunction))) {
            fail("a general access modifier requires an object subtype");
        }
        if (check(TokenKind::KwFunction) || check(TokenKind::KwProcedure)) {
            definition->accessProfile = std::make_unique<SubprogramSpec>();
            SubprogramSpec& profile = *definition->accessProfile;
            profile.isFunction = match(TokenKind::KwFunction);
            if (!profile.isFunction) {
                expect(TokenKind::KwProcedure, "in access-to-subprogram type");
            }
            if (check(TokenKind::LeftParen)) {
                parseParameterList(profile);
            }
            if (profile.isFunction) {
                expect(TokenKind::KwReturn, "in access-to-function type");
                profile.returnType = parseSubtypeIndication(true);
            }
        } else {
            definition->parent = parseSubtypeIndication();
        }
        return definition;
    }

    fail("unsupported type definition");
}

// '(Kind : Figure; Size : Positive)' after a type name.  The components it
// names come first in every value of the type; defaults permit unconstrained
// variables whose discriminants can change by whole-object assignment.
void Parser::parseDiscriminantPart(std::vector<RecordField>& discriminants)
{
    expect(TokenKind::LeftParen, "in discriminant part");
    while (true) {
        SourceLocation location = current().location;
        std::vector<std::string> lowered;
        std::vector<std::string> names = parseIdentifierList(lowered);
        expect(TokenKind::Colon, "in discriminant");
        std::size_t subtypeStart = m_position;
        for (std::size_t i = 0; i < names.size(); ++i) {
            RecordField field;
            field.name = names[i];
            field.lower = lowered[i];
            field.location = location;
            m_position = subtypeStart;
            field.subtype = parseSubtypeIndication(true);
            field.subtype->accessDiscriminant = true;
            if (match(TokenKind::Assign)) {
                field.defaultValue = parseExpression();
            }
            discriminants.push_back(std::move(field));
        }

        if (!match(TokenKind::Semicolon)) {
            break;
        }
    }
    expect(TokenKind::RightParen, "after discriminant part");
}

// The component declarations of a record definition or of one alternative of a
// variant part, up to whatever ends them.
void Parser::parseRecordComponents(std::vector<RecordField>& fields)
{
    while (!check(TokenKind::KwEnd) && !check(TokenKind::KwWhen) && !check(TokenKind::KwCase)
           && !check(TokenKind::KwBegin) && !check(TokenKind::KwException)
           && !check(TokenKind::EndOfFile)) {
        try {
            if (match(TokenKind::KwNull)) {
                expect(TokenKind::Semicolon, "after null component");
                continue;
            }
            SourceLocation fieldLocation = current().location;
            std::vector<std::string> lowered;
            std::vector<std::string> names = parseIdentifierList(lowered);
            expect(TokenKind::Colon, "in record component");
            // As with grouped parameters, retain a complete owned syntax tree for
            // every name. Semantic analysis may annotate each subtree separately.
            std::size_t subtypeStart = m_position;
            for (std::size_t i = 0; i < names.size(); ++i) {
                RecordField field;
                field.name = names[i];
                field.lower = lowered[i];
                field.location = fieldLocation;
                m_position = subtypeStart;
                field.aliased = match(TokenKind::KwAliased);
                field.subtype = parseSubtypeIndication(true);
                if (match(TokenKind::Assign)) {
                    field.defaultValue = parseExpression();
                }
                fields.push_back(std::move(field));
            }
            expect(TokenKind::Semicolon, "after record component");
        } catch (const ParseError&) {
            // Recover inside the record. Letting this escape would make its
            // end record look like the end of the enclosing subprogram.
            while (!check(TokenKind::KwEnd) && !check(TokenKind::KwWhen)
                && !check(TokenKind::KwCase) && !check(TokenKind::KwBegin)
                && !check(TokenKind::KwException) && !check(TokenKind::EndOfFile)) {
                if (match(TokenKind::Semicolon)) {
                    break;
                }
                advance();
            }
        }
    }
}

// The components a record has only when its discriminant holds one of the
// values named, written as a case at the end of the record definition.
VariantPartPtr Parser::parseVariantPart()
{
    auto part = std::make_unique<VariantPart>();
    part->location = current().location;
    expect(TokenKind::KwCase, "in variant part");
    const Token& name = expect(TokenKind::Identifier, "in variant part");
    part->discriminant = name.text;
    part->discriminantLower = name.lower;
    expect(TokenKind::KwIs, "in variant part");

    while (match(TokenKind::KwWhen)) {
        RecordVariant variant;
        variant.location = current().location;
        if (match(TokenKind::KwOthers)) {
            variant.isOthers = true;
        } else {
            while (true) {
                variant.choiceLows.push_back(parseSimpleExpression());
                if (match(TokenKind::DoubleDot)) {
                    variant.choiceHighs.push_back(parseSimpleExpression());
                } else {
                    variant.choiceHighs.push_back(nullptr);
                }
                if (!match(TokenKind::Bar)) {
                    break;
                }
            }
        }
        expect(TokenKind::Arrow, "in variant alternative");
        parseRecordComponents(variant.fields);
        if (check(TokenKind::KwCase)) {
            variant.nested = parseVariantPart();
        }
        part->variants.push_back(std::move(variant));
    }

    expect(TokenKind::KwEnd, "at end of variant part");
    expect(TokenKind::KwCase, "at end of variant part");
    expect(TokenKind::Semicolon, "after variant part");
    return part;
}

SubtypeIndicationPtr Parser::parseSubtypeIndication(bool allowAccess)
{
    if (check(TokenKind::KwAliased)) {
        fail("aliased declarations are not yet supported");
    }
    auto indication = std::make_unique<SubtypeIndication>();
    indication->location = current().location;
    indication->nullExclusion = match(TokenKind::KwNot);
    if (indication->nullExclusion) { expect(TokenKind::KwNull, "after not in null exclusion"); }
    if (check(TokenKind::KwAccess)) {
        if (!allowAccess) { fail("an anonymous access definition is not allowed in this context"); }
        if (peek(1).kind == TokenKind::KwAll) { fail("anonymous access definitions cannot specify 'all'"); }
        indication->accessDefinition = parseTypeDefinition(true);
        const auto& parent = indication->accessDefinition->parent;
        if (parent != nullptr && parent->nullExclusion) {
            fail("an anonymous access definition requires a designated subtype mark without a null exclusion");
        }
        if (parent != nullptr && (parent->rangeLow != nullptr || !parent->indexLows.empty()
            || parent->digits != nullptr || parent->delta != nullptr)) {
            fail("an anonymous access definition requires a subtype mark without a constraint");
        }
        return indication;
    }
    indication->name = parseSubtypeMark(indication->lower);

    if (match(TokenKind::KwDigits)) {
        indication->digits = parseSimpleExpression();
    }

    if (match(TokenKind::KwRange)) {
        indication->rangeLow = parseSimpleExpression();
        expect(TokenKind::DoubleDot, "in range constraint");
        indication->rangeHigh = parseSimpleExpression();
        return indication;
    }

    if (check(TokenKind::LeftParen)) {
        advance();
        while (true) {
            std::vector<std::string> selectors;
            if (check(TokenKind::Identifier) && (peek(1).kind == TokenKind::Arrow || peek(1).kind == TokenKind::Bar)) {
                do { selectors.push_back(expect(TokenKind::Identifier, "in discriminant association").lower); }
                while (match(TokenKind::Bar));
                expect(TokenKind::Arrow, "after discriminant selector");
            }
            indication->discriminantNames.push_back(std::move(selectors));
            ExprPtr low = parseExpression();
            ExprPtr high;
            if (match(TokenKind::DoubleDot)) {
                high = parseSimpleExpression();
            }
            indication->indexLows.push_back(std::move(low));
            indication->indexHighs.push_back(std::move(high));
            if (!match(TokenKind::Comma)) {
                break;
            }
        }
        expect(TokenKind::RightParen, "after index constraint");
    }

    return indication;
}

void Parser::parseDiscreteRange(std::string& typeName, std::string& typeLower, ExprPtr& low, ExprPtr& high)
{
    std::size_t saved = m_position;

    if (check(TokenKind::Identifier)) {
        std::string lowered;
        std::string name = parseSubtypeMark(lowered);
        if (match(TokenKind::KwRange)) {
            typeName = name;
            typeLower = lowered;
            low = parseSimpleExpression();
            expect(TokenKind::DoubleDot, "in discrete range");
            high = parseSimpleExpression();
            return;
        }
        // A name on its own is a type mark, but only where the range ends: a
        // name that something follows, as in 'Length + 1 .. Width', starts an
        // expression like any other.
        if (check(TokenKind::KwLoop) || check(TokenKind::Semicolon) || check(TokenKind::RightParen)
            || check(TokenKind::Comma) || check(TokenKind::Arrow) || check(TokenKind::Bar)
            || check(TokenKind::KwThen) || check(TokenKind::KwAnd) || check(TokenKind::KwOr)
            || check(TokenKind::KwXor)) {
            typeName = name;
            typeLower = lowered;
            return;
        }
        m_position = saved;
    }

    std::size_t rangeStart = m_position;
    ExprPtr first = parseSimpleExpression();
    if (first->kind == ExprKind::Attribute) {
        auto* attribute = static_cast<AttributeExpr*>(first.get());
        if (attribute->lower == "range") {
            auto makeAttribute = [&](const char* name) {
                m_position = rangeStart;
                ExprPtr copy = parseSimpleExpression();
                auto* expr = static_cast<AttributeExpr*>(copy.get());
                expr->name = name;
                expr->lower = name;
                return copy;
            };
            low = makeAttribute("first");
            high = makeAttribute("last");
            auto* firstBound = static_cast<AttributeExpr*>(low.get());
            firstBound->rangeFirst = firstBound;
            static_cast<AttributeExpr*>(high.get())->rangeFirst = firstBound;
            return;
        }
    }

    low = std::move(first);
    expect(TokenKind::DoubleDot, "in discrete range");
    high = parseSimpleExpression();
}
