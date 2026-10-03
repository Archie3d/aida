#include "Parser.h"

#include <utility>

SubprogramSpec Parser::parseSubprogramSpec(bool allowInstantiation)
{
    SubprogramSpec spec;
    spec.location = current().location;
    if (match(TokenKind::KwNot)) {
        expect(TokenKind::KwOverriding, "after not");
        spec.m_overriding = -1;
    } else if (match(TokenKind::KwOverriding)) {
        spec.m_overriding = 1;
    }
    if (match(TokenKind::KwFunction)) {
        spec.isFunction = true;
    } else {
        expect(TokenKind::KwProcedure, "in subprogram specification");
    }

    // A library unit may be a child, as Ada.Unchecked_Deallocation is.
    if (spec.isFunction && check(TokenKind::StringLiteral)) {
        const Token& designator = advance();
        if (operatorSymbol(designator.text).empty()) {
            fail("invalid operator designator");
        }
        spec.name = designator.text;
        spec.lower = operatorSymbol(designator.text);
    } else {
        spec.name = parseCompoundName(spec.lower);
    }

    if (check(TokenKind::LeftParen)) {
        parseParameterList(spec);
    }
    if (spec.isFunction && !(allowInstantiation && check(TokenKind::KwIs) && peek(1).kind == TokenKind::KwNew)) {
        expect(TokenKind::KwReturn, "in function specification");
        spec.returnType = parseSubtypeIndication();
    }
    return spec;
}

void Parser::parseParameterList(SubprogramSpec& spec)
{
    expect(TokenKind::LeftParen, "in parameter list");
    while (true) {
        SourceLocation location = current().location;
        std::vector<std::string> lowered;
        std::vector<std::string> names = parseIdentifierList(lowered);
        expect(TokenKind::Colon, "in parameter specification");

        ParameterMode mode = ParameterMode::In;
        if (match(TokenKind::KwIn)) {
            mode = match(TokenKind::KwOut) ? ParameterMode::InOut : ParameterMode::In;
        } else if (match(TokenKind::KwOut)) {
            mode = ParameterMode::Out;
        }

        // Parse a separate owned subtree for each name in a grouped profile.
        // Each omitted actual must evaluate its own default, including side effects.
        std::size_t subtypeStart = m_position;
        for (std::size_t i = 0; i < names.size(); ++i) {
            m_position = subtypeStart;
            ParameterDecl parameter;
            parameter.name = names[i];
            parameter.lower = lowered[i];
            parameter.mode = mode;
            parameter.location = location;
            parameter.subtype = parseSubtypeIndication();
            if (match(TokenKind::Assign)) {
                std::size_t defaultStart = m_position;
                parameter.defaultValue = parseExpression();
                parameter.m_defaultTokens.assign(m_tokens.begin() + defaultStart, m_tokens.begin() + m_position);
            }
            spec.parameters.push_back(std::move(parameter));
        }

        if (!match(TokenKind::Semicolon)) {
            break;
        }
    }
    expect(TokenKind::RightParen, "after parameter list");
}

DeclPtr Parser::parseSubprogramDeclOrBody()
{
    SourceLocation location = current().location;
    std::size_t start = m_position;
    SubprogramSpec spec = parseSubprogramSpec(true);

    if (match(TokenKind::KwRenames)) {
        auto decl = std::make_unique<SubprogramDecl>();
        decl->location = location;
        decl->spec = std::move(spec);
        std::size_t nameStart = m_position;
        decl->m_renamedName = parseExpression();
        decl->m_renamedTokens.assign(m_tokens.begin() + nameStart, m_tokens.begin() + m_position);
        expect(TokenKind::Semicolon, "after subprogram renaming");
        return decl;
    }

    if (match(TokenKind::Semicolon)) {
        auto decl = std::make_unique<SubprogramDecl>();
        decl->location = location;
        decl->spec = std::move(spec);
        return decl;
    }

    expect(TokenKind::KwIs, "in subprogram body");
    if (match(TokenKind::KwNew)) {
        return parseGenericInstantiation(location, spec.name, spec.lower, false);
    }
    if (check(TokenKind::KwSeparate)) {
        fail("subprogram stubs ('is separate') are not yet supported");
    }

    auto body = std::make_unique<SubprogramBody>();
    body->location = location;
    body->spec = std::move(spec);
    if (match(TokenKind::KwNull)) {
        if (body->spec.isFunction) {
            fail("only a procedure can have a null body");
        }
        expect(TokenKind::Semicolon, "after null procedure");
        body->tokens.assign(m_tokens.begin() + static_cast<std::ptrdiff_t>(start),
                            m_tokens.begin() + static_cast<std::ptrdiff_t>(m_position));
        return body;
    }
    body->declarations = parseDeclarativePart();
    expect(TokenKind::KwBegin, "in subprogram body");
    body->body = parseSequenceOfStatements();
    if (check(TokenKind::KwException)) {
        body->handlers = parseExceptionHandlers();
    }
    expect(TokenKind::KwEnd, "at end of subprogram body");
    parseClosingName(body->spec.lower, true);
    expect(TokenKind::Semicolon, "after subprogram body");
    body->tokens.assign(m_tokens.begin() + static_cast<std::ptrdiff_t>(start),
                        m_tokens.begin() + static_cast<std::ptrdiff_t>(m_position));
    return body;
}
