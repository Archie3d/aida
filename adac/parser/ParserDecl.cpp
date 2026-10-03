#include "Parser.h"

#include "Lexer.h"

#include <utility>

DeclList Parser::parseDeclarativePart(bool stopAtPrivate)
{
    DeclList declarations;

    while (true) {
        if (check(TokenKind::EndOfFile) || check(TokenKind::KwBegin) || check(TokenKind::KwEnd)
            || check(TokenKind::KwException)) {
            break;
        }
        if (stopAtPrivate && check(TokenKind::KwPrivate)) {
            break;
        }

        try {
            DeclPtr decl = parseDeclarativeItem();
            if (decl) {
                declarations.push_back(std::move(decl));
            }
        } catch (const ParseError&) {
            skipToSemicolon();
        }
    }

    return declarations;
}

DeclPtr Parser::parseDeclarativeItem()
{
    switch (current().kind) {
    case TokenKind::KwGeneric:
        return parseGenericDeclaration();
    case TokenKind::KwType:
        return parseTypeDecl();
    case TokenKind::KwSubtype:
        return parseSubtypeDecl();
    case TokenKind::KwOverriding:
    case TokenKind::KwNot:
    case TokenKind::KwProcedure:
    case TokenKind::KwFunction:
        return parseSubprogramDeclOrBody();
    case TokenKind::KwPackage:
        return parsePackage();
    case TokenKind::KwUse:
        return parseUseClause();
    case TokenKind::KwPragma:
        return parsePragma();
    case TokenKind::KwFor:
        return parseRepresentationClause();
    case TokenKind::Identifier:
        return parseObjectOrNumberDecl();
    default:
        fail("expected a declaration");
    }
}

DeclPtr Parser::parseObjectOrNumberDecl()
{
    SourceLocation location = current().location;
    std::vector<std::string> lowered;
    std::vector<std::string> names = parseIdentifierList(lowered);
    expect(TokenKind::Colon, "in object declaration");

    if (match(TokenKind::KwException)) {
        auto decl = std::make_unique<ExceptionDecl>();
        decl->location = location;
        decl->names = std::move(names);
        decl->namesLower = std::move(lowered);
        // 'Status_Error : exception renames IO_Exceptions.Status_Error;' is a
        // second name for one exception, not a second exception.
        if (match(TokenKind::KwRenames)) {
            decl->renames = parseCompoundName(decl->renamesLower);
        }
        expect(TokenKind::Semicolon, "after exception declaration");
        return decl;
    }

    bool isConstant = match(TokenKind::KwConstant);
    if (isConstant && check(TokenKind::Assign)) {
        auto decl = std::make_unique<NumberDecl>();
        decl->location = location;
        decl->names = std::move(names);
        decl->namesLower = std::move(lowered);
        advance();
        decl->value = parseExpression();
        expect(TokenKind::Semicolon, "after number declaration");
        return decl;
    }

    auto decl = std::make_unique<ObjectDecl>();
    decl->location = location;
    decl->names = std::move(names);
    decl->namesLower = std::move(lowered);
    decl->isConstant = isConstant;
    decl->subtype = parseSubtypeIndication();
    if (match(TokenKind::KwRenames)) {
        if (decl->names.size() != 1 || isConstant) {
            fail("an object renaming requires one name and no constant keyword");
        }
        decl->m_isRenaming = true;
        decl->initializer = parseExpression();
    } else if (match(TokenKind::Assign)) {
        decl->initializer = parseExpression();
    }
    expect(TokenKind::Semicolon, "after object declaration");
    return decl;
}

DeclPtr Parser::parseUseClause()
{
    auto decl = std::make_unique<UseDecl>();
    decl->location = current().location;
    expect(TokenKind::KwUse, "in use clause");
    decl->m_all = match(TokenKind::KwAll);
    decl->m_typeOnly = match(TokenKind::KwType);
    if (decl->m_all && !decl->m_typeOnly) {
        fail("expected type after use all");
    }
    while (true) {
        std::string lowered;
        std::string name = parseCompoundName(lowered);
        decl->names.push_back(name);
        decl->namesLower.push_back(lowered);
        if (!match(TokenKind::Comma)) {
            break;
        }
    }
    expect(TokenKind::Semicolon, "after use clause");
    return decl;
}

// Keep Import for semantic analysis. Inline, Pure and Preelaborate are
// explicitly accepted as advisory only; reject every other pragma rather than
// silently losing requirements on representation, checks or execution.
DeclPtr Parser::parsePragma()
{
    SourceLocation location = current().location;
    expect(TokenKind::KwPragma, "in pragma");

    const Token& name = expect(TokenKind::Identifier, "as pragma name");
    if (name.lower == "inline" || name.lower == "pure" || name.lower == "preelaborate") {
        if (match(TokenKind::LeftParen)) {
            do {
                if (check(TokenKind::Identifier) && peek(1).kind == TokenKind::Arrow) {
                    advance();
                    advance();
                }
                parseExpression();
            } while (match(TokenKind::Comma));
            expect(TokenKind::RightParen, "after pragma arguments");
        } else if (name.lower == "inline") {
            fail("expected arguments for pragma Inline");
        }
        expect(TokenKind::Semicolon, "after pragma");
        return nullptr;
    }
    if (name.lower != "import") {
        m_diagnostics.error(name.location, "unsupported or unknown pragma '" + name.text
            + "'; supported pragmas are Import, Inline, Pure and Preelaborate");
        skipToSemicolon();
        return nullptr;
    }

    auto pragma = std::make_unique<PragmaDecl>();
    pragma->location = location;
    pragma->name = name.text;
    pragma->lower = name.lower;

    if (match(TokenKind::LeftParen)) {
        // Convention validation is a separate roadmap step.
        if (check(TokenKind::Identifier)) {
            advance();
        }
        if (match(TokenKind::Comma) && (check(TokenKind::Identifier)
            || (check(TokenKind::StringLiteral) && current().text == "**"))) {
            pragma->entity = current().text;
            pragma->entityLower = toLower(current().text);
            advance();
        }
        if (match(TokenKind::Comma) && check(TokenKind::StringLiteral)) {
            pragma->linkName = current().text;
            advance();
        }
    }

    while (!check(TokenKind::Semicolon) && !check(TokenKind::EndOfFile)) {
        advance();
    }
    expect(TokenKind::Semicolon, "after pragma");

    if (!pragma->entityLower.empty() && !pragma->linkName.empty()) {
        return pragma;
    }
    return nullptr;
}

// Only Size attribute definition clauses are implemented. Diagnose other
// forms here, including record clauses whose internal semicolons need special
// recovery, instead of dropping their requested layout.
DeclPtr Parser::parseRepresentationClause()
{
    SourceLocation location = current().location;
    expect(TokenKind::KwFor, "in representation clause");

    auto decl = std::make_unique<RepresentationDecl>();
    decl->location = location;
    decl->name = parseCompoundName(decl->lower);
    if (match(TokenKind::KwUse)) {
        if (match(TokenKind::KwRecord)) {
            m_diagnostics.error(location, "record representation clauses are not yet supported");
            while (!check(TokenKind::EndOfFile)) {
                if (check(TokenKind::KwEnd) && peek(1).kind == TokenKind::KwRecord) {
                    advance();
                    advance();
                    break;
                }
                advance();
            }
        } else {
            m_diagnostics.error(location, check(TokenKind::KwAt)
                ? "address clauses are not yet supported"
                : "enumeration representation clauses are not yet supported");
        }
        skipToSemicolon();
        return nullptr;
    }
    expect(TokenKind::Tick, "in representation clause");

    const Token& attribute = expect(TokenKind::Identifier, "in representation clause");
    decl->attribute = attribute.lower;
    if (decl->attribute != "size") {
        m_diagnostics.error(location, "unsupported representation attribute '" + attribute.text
            + "'; only 'Size clauses are supported");
        skipToSemicolon();
        return nullptr;
    }

    expect(TokenKind::KwUse, "in representation clause");
    decl->value = parseExpression();
    expect(TokenKind::Semicolon, "after representation clause");
    return decl;
}
