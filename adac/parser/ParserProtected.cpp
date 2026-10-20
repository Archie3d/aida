#include "Parser.h"

DeclPtr Parser::parseProtected()
{
    SourceLocation location = expect(TokenKind::KwProtected, "in protected declaration").location;
    bool isBody = match(TokenKind::KwBody);
    bool isType = !isBody && match(TokenKind::KwType);
    const Token& name = expect(TokenKind::Identifier, "in protected unit");
    auto decl = std::make_unique<TypeDecl>();
    decl->location = location;
    decl->name = name.text;
    decl->lower = isBody || isType ? name.lower : "__single_protected_" + name.lower;
    decl->m_protectedBody = isBody;
    decl->definition = std::make_unique<TypeDefinition>(TypeDefKind::Protected);
    decl->definition->location = location;
    try {
        if (check(TokenKind::LeftParen)) {
            fail("protected discriminants are not yet supported");
        }
        expect(TokenKind::KwIs, "in protected unit");
        if (check(TokenKind::KwNew)) {
            fail("protected interfaces are not yet supported");
        }
        bool privatePart = false;
        while (!check(TokenKind::KwEnd) && !check(TokenKind::EndOfFile)) {
            if (!isBody && match(TokenKind::KwPrivate)) {
                if (privatePart) {
                    fail("duplicate protected private part");
                }
                privatePart = true;
            } else if (check(TokenKind::KwProcedure) || check(TokenKind::KwFunction)) {
                DeclPtr operation = parseSubprogramDeclOrBody();
                if (operation->kind != DeclKind::SubprogramDeclaration && operation->kind != DeclKind::SubprogramBody) {
                    fail("generic instantiations in protected bodies are not yet supported");
                }
                const SubprogramSpec& spec = operation->kind == DeclKind::SubprogramDeclaration
                    ? static_cast<SubprogramDecl*>(operation.get())->spec
                    : static_cast<SubprogramBody*>(operation.get())->spec;
                if (!operatorSymbol(spec.lower).empty() || spec.lower.find('.') != std::string::npos) {
                    fail("protected operations currently require a simple identifier");
                }
                if (!isBody && operation->kind != DeclKind::SubprogramDeclaration) {
                    fail("a protected definition requires operation declarations; bodies belong in the protected body");
                }
                if (operation->kind == DeclKind::SubprogramDeclaration) {
                    auto* subprogram = static_cast<SubprogramDecl*>(operation.get());
                    if (subprogram->m_renamedName != nullptr) {
                        fail("protected operation renaming declarations are not yet supported");
                    }
                    subprogram->m_protectedPrivate = privatePart;
                }
                decl->m_protectedDeclarations.push_back(std::move(operation));
            } else if (!isBody && privatePart && check(TokenKind::Identifier)) {
                SourceLocation fieldLocation = current().location;
                std::vector<std::string> lowered;
                auto names = parseIdentifierList(lowered);
                expect(TokenKind::Colon, "in protected component");
                std::size_t start = m_position;
                for (std::size_t i = 0; i < names.size(); ++i) {
                    m_position = start;
                    RecordField field;
                    field.location = fieldLocation;
                    field.name = names[i];
                    field.lower = lowered[i];
                    field.subtype = parseSubtypeIndication();
                    if (match(TokenKind::Assign)) {
                        field.defaultValue = parseExpression();
                    }
                    decl->definition->fields.push_back(std::move(field));
                }
                expect(TokenKind::Semicolon, "after protected component");
            } else if (check(TokenKind::KwEntry)) {
                SourceLocation entryLocation = advance().location;
                const Token& entryName = expect(TokenKind::Identifier, "in protected entry");
                SubprogramSpec spec;
                spec.location = entryLocation;
                spec.name = entryName.text;
                spec.lower = entryName.lower;
                spec.m_entry = true;
                if (check(TokenKind::LeftParen)) {
                    if (peek(1).kind != TokenKind::Identifier || (peek(2).kind != TokenKind::Colon
                        && peek(2).kind != TokenKind::Comma)) {
                        fail("protected entry families are not yet supported");
                    }
                    parseParameterList(spec);
                }
                if (!isBody) {
                    expect(TokenKind::Semicolon, "after entry declaration");
                    auto entry = std::make_unique<SubprogramDecl>();
                    entry->location = entryLocation;
                    entry->spec = std::move(spec);
                    entry->m_protectedPrivate = privatePart;
                    decl->m_protectedDeclarations.push_back(std::move(entry));
                } else {
                    if (check(TokenKind::KwFor)) {
                        fail("protected entry families are not yet supported");
                    }
                    expect(TokenKind::KwWhen, "before protected entry barrier");
                    auto barrier = std::make_unique<SubprogramBody>();
                    barrier->location = entryLocation;
                    barrier->spec.location = entryLocation;
                    barrier->spec.name = "__entry_barrier_" + entryName.lower
                        + "_" + std::to_string(decl->m_protectedDeclarations.size());
                    barrier->spec.lower = barrier->spec.name;
                    barrier->spec.isFunction = true;
                    barrier->spec.m_entryBarrier = true;
                    barrier->spec.returnType = std::make_unique<SubtypeIndication>();
                    barrier->spec.returnType->location = entryLocation;
                    barrier->spec.returnType->name = "Boolean";
                    barrier->spec.returnType->lower = "boolean";
                    auto result = std::make_unique<ReturnStmt>();
                    result->location = current().location;
                    result->value = parseExpression();
                    barrier->body.push_back(std::move(result));
                    SubprogramBody* barrierBody = barrier.get();
                    decl->m_protectedDeclarations.push_back(std::move(barrier));
                    expect(TokenKind::KwIs, "in entry body");
                    auto entry = std::make_unique<SubprogramBody>();
                    entry->location = entryLocation;
                    entry->m_barrierBody = barrierBody;
                    entry->spec = std::move(spec);
                    entry->declarations = parseDeclarativePart();
                    expect(TokenKind::KwBegin, "in entry body");
                    entry->body = parseSequenceOfStatements();
                    if (check(TokenKind::KwException)) {
                        entry->handlers = parseExceptionHandlers();
                    }
                    expect(TokenKind::KwEnd, "after entry body");
                    parseClosingName(entryName.lower, true);
                    expect(TokenKind::Semicolon, "after entry body");
                    decl->m_protectedDeclarations.push_back(std::move(entry));
                }
            } else {
                fail("unsupported protected unit item; expected a procedure, function or private component");
            }
        }
        expect(TokenKind::KwEnd, "after protected unit");
        parseClosingName(name.lower, true);
        expect(TokenKind::Semicolon, "after protected unit");
    } catch (const ParseError&) {
        // Keep an unsupported entry or representation item from consuming the
        // enclosing program's end. Nested operation bodies may have their own
        // unnamed ends, so track their statement/declaration boundaries.
        int depth = 0;
        while (!check(TokenKind::EndOfFile)) {
            if (check(TokenKind::KwEnd)) {
                if ((peek(1).kind == TokenKind::Identifier && peek(1).lower == name.lower)
                    || (peek(1).kind == TokenKind::Semicolon && depth == 0)) {
                    advance();
                    skipToSemicolon();
                    return nullptr;
                }
                if (depth > 0) {
                    --depth;
                }
            } else if (check(TokenKind::KwBegin) || check(TokenKind::KwIf)
                || check(TokenKind::KwLoop) || check(TokenKind::KwCase)
                || check(TokenKind::KwRecord) || check(TokenKind::KwSelect)) {
                if (m_position == 0 || m_tokens[m_position - 1].kind != TokenKind::KwEnd) {
                    ++depth;
                }
            }
            advance();
        }
        return nullptr;
    }
    if (!isBody && !isType) {
        auto object = std::make_unique<ObjectDecl>();
        object->location = location;
        object->names = {name.text};
        object->namesLower = {name.lower};
        object->subtype = std::make_unique<SubtypeIndication>();
        object->subtype->location = location;
        object->subtype->name = decl->name;
        object->subtype->lower = decl->lower;
        decl->m_taskObjects.push_back(std::move(object));
    }
    return decl;
}
