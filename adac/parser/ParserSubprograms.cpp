#include "Parser.h"

#include <utility>

SubprogramSpec Parser::parseSubprogramSpec(bool allowInstantiation)
{
    SubprogramSpec spec;
    spec.location = current().location;
    if (match(TokenKind::KwNot)) {
        expect(TokenKind::KwOverriding, "after not");
        spec.overriding = -1;
    } else if (match(TokenKind::KwOverriding)) {
        spec.overriding = 1;
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
        spec.returnType = parseSubtypeIndication(true);
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

        bool explicitMode = check(TokenKind::KwIn) || check(TokenKind::KwOut);
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
            parameter.subtype = parseSubtypeIndication(true);
            parameter.subtype->accessParameter = true;
            if (parameter.subtype->accessDefinition != nullptr && explicitMode) {
                fail("an access parameter cannot have an explicit parameter mode");
            }
            if (match(TokenKind::Assign)) {
                std::size_t defaultStart = m_position;
                parameter.defaultValue = parseExpression();
                parameter.defaultTokens.assign(m_tokens.begin() + defaultStart, m_tokens.begin() + m_position);
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
    bool libraryItem = std::exchange(m_libraryItem, false);
    auto subunit = m_tokens[start].subunit;
    SubprogramSpec spec = parseSubprogramSpec(true);

    if (match(TokenKind::KwRenames)) {
        auto decl = std::make_unique<SubprogramDecl>();
        decl->location = location;
        decl->spec = std::move(spec);
        std::size_t nameStart = m_position;
        decl->renamedName = parseExpression();
        decl->renamedTokens.assign(m_tokens.begin() + nameStart, m_tokens.begin() + m_position);
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
    if (match(TokenKind::KwAbstract)) {
        spec.abstract = true;
        expect(TokenKind::Semicolon, "after abstract subprogram declaration");
        auto decl = std::make_unique<SubprogramDecl>();
        decl->location = location;
        decl->spec = std::move(spec);
        return decl;
    }
    if (match(TokenKind::KwNew)) {
        DeclPtr instance = parseGenericInstantiation(location, spec.name, spec.lower, false);
        static_cast<GenericInstantiationDecl*>(instance.get())->isFunction = spec.isFunction;
        static_cast<GenericInstantiationDecl*>(instance.get())->overriding = spec.overriding;
        return instance;
    }
    if (match(TokenKind::KwSeparate)) {
        expect(TokenKind::Semicolon, "after body stub");
        if (expandStub(start, spec.name, DeclKind::SubprogramBody)) {
            return parseSubprogramDeclOrBody();
        }
        auto stub = std::make_unique<SubprogramBody>();
        stub->location = location;
        stub->spec = std::move(spec);
        stub->separateStub = true;
        stub->tokens.assign(m_tokens.begin() + start, m_tokens.begin() + m_position);
        return stub;
    }

    auto body = std::make_unique<SubprogramBody>();
    body->location = location;
    body->subunit = subunit;
    body->spec = std::move(spec);
    if (match(TokenKind::KwNull)) {
        body->isNull = true;
        body->spec.nullProcedure = true;
        if (body->spec.isFunction) {
            fail("only a procedure can have a null body");
        }
        expect(TokenKind::Semicolon, "after null procedure");
        body->tokens.assign(m_tokens.begin() + static_cast<std::ptrdiff_t>(start),
                            m_tokens.begin() + static_cast<std::ptrdiff_t>(m_position));
        return body;
    }
    body->declarations = parseDeclarativePart(false, subunit ? subunit->fullName : libraryItem ? body->spec.lower : "");
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

// Task bodies reuse lexical subprogram frames and cleanup, with a separate
// activation handshake before their handled sequence of statements.
DeclPtr Parser::parseTask()
{
    SourceLocation location = expect(TokenKind::KwTask, "in task declaration").location;
    if (match(TokenKind::KwBody)) {
        const Token& name = expect(TokenKind::Identifier, "after task body");
        auto body = std::make_unique<SubprogramBody>();
        body->location = location;
        body->taskBody = true;
        body->taskName = name.lower;
        body->spec.name = name.text;
        body->spec.lower = "__task_" + name.lower;
        body->spec.location = location;
        expect(TokenKind::KwIs, "in task body");
        if (check(TokenKind::KwSeparate)) {
            fail("task body stubs are not yet supported");
        }
        body->declarations = parseDeclarativePart();
        expect(TokenKind::KwBegin, "in task body");
        body->body = parseSequenceOfStatements();
        if (check(TokenKind::KwException)) {
            body->handlers = parseExceptionHandlers();
        }
        expect(TokenKind::KwEnd, "after task body");
        parseClosingName(name.lower, true);
        expect(TokenKind::Semicolon, "after task body");
        return body;
    }
    bool isType = match(TokenKind::KwType);
    const Token& name = expect(TokenKind::Identifier, "in task declaration");
    auto decl = std::make_unique<TypeDecl>();
    decl->location = location;
    decl->name = name.text;
    decl->lower = isType ? name.lower : "__single_task_" + name.lower;
    decl->definition = std::make_unique<TypeDefinition>(TypeDefKind::Task);
    decl->definition->location = location;
    auto rejectDefinition = [&](const std::string& message) -> DeclPtr {
        m_diagnostics.error(current().location, message);
        int parentheses = 0;
        while (!check(TokenKind::EndOfFile)) {
            if (parentheses == 0 && match(TokenKind::Semicolon)) {
                return nullptr;
            }
            if (parentheses == 0 && match(TokenKind::KwIs)) {
                break;
            }
            if (match(TokenKind::LeftParen)) {
                ++parentheses;
            } else if (match(TokenKind::RightParen)) {
                --parentheses;
            } else {
                advance();
            }
        }
        while (!check(TokenKind::EndOfFile)) {
            if (check(TokenKind::KwEnd) && (peek(1).kind == TokenKind::Semicolon
                || (peek(1).kind == TokenKind::Identifier && peek(1).lower == name.lower))) {
                advance();
                skipToSemicolon();
                break;
            }
            advance();
        }
        return nullptr;
    };
    if (check(TokenKind::LeftParen)) {
        return rejectDefinition("task discriminants are not yet supported");
    }
    if (check(TokenKind::KwIs) && peek(1).kind != TokenKind::KwEnd) {
        return rejectDefinition("task entries, interfaces and task definition items are not yet supported");
    }
    if (match(TokenKind::KwIs)) {
        advance();
        parseClosingName(name.lower, true);
    }
    expect(TokenKind::Semicolon, "after task declaration");
    if (!isType) {
        auto object = std::make_unique<ObjectDecl>();
        object->location = location;
        object->names.push_back(name.text);
        object->namesLower.push_back(name.lower);
        object->subtype = std::make_unique<SubtypeIndication>();
        object->subtype->location = location;
        object->subtype->name = decl->name;
        object->subtype->lower = decl->lower;
        decl->taskObjects.push_back(std::move(object));
    }
    return decl;
}
