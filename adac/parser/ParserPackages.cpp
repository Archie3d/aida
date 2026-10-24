#include "Parser.h"

DeclPtr Parser::parsePackage(bool library)
{
    SourceLocation location = current().location;
    expect(TokenKind::KwPackage, "in package declaration");

    if (match(TokenKind::KwBody)) {
        auto body = std::make_unique<PackageBodyDecl>();
        body->location = location;
        // The whole body is kept as tokens too, since a generic declared in one
        // file has its body in another and every instance parses it again.
        std::size_t start = m_position - 2;
        body->name = parseCompoundName(body->lower);
        expect(TokenKind::KwIs, "in package body");
        if (check(TokenKind::KwSeparate)) {
            fail("package body stubs ('is separate') are not yet supported");
        }
        body->declarations = parseDeclarativePart();
        if (match(TokenKind::KwBegin)) {
            body->body = parseSequenceOfStatements();
            if (check(TokenKind::KwException)) {
                body->handlers = parseExceptionHandlers();
            }
        }
        expect(TokenKind::KwEnd, "at end of package body");
        parseClosingName(body->lower, true);
        expect(TokenKind::Semicolon, "after package body");
        body->tokens.assign(m_tokens.begin() + static_cast<std::ptrdiff_t>(start),
                            m_tokens.begin() + static_cast<std::ptrdiff_t>(m_position));
        return body;
    }

    auto spec = std::make_unique<PackageSpecDecl>();
    spec->location = location;
    spec->name = parseCompoundName(spec->lower);
    if (match(TokenKind::KwRenames)) {
        spec->renames = parseCompoundName(spec->renamesLower);
        expect(TokenKind::Semicolon, "after package renaming");
        return spec;
    }
    expect(TokenKind::KwIs, "in package specification");
    if (match(TokenKind::KwNew)) {
        return parseGenericInstantiation(location, spec->name, spec->lower, true);
    }
    while (library && check(TokenKind::KwPragma)) {
        if (peek(1).lower == "elaborate_body") {
            parseElaborateBody(*spec, spec->lower, true);
        } else {
            DeclPtr pragma = parsePragma();
            if (pragma) {
                spec->publicPart.push_back(std::move(pragma));
            }
        }
    }
    DeclList declarations = parseDeclarativePart(true);
    for (DeclPtr& declaration : declarations) {
        spec->publicPart.push_back(std::move(declaration));
    }
    if (match(TokenKind::KwPrivate)) {
        spec->privatePart = parseDeclarativePart();
    }
    expect(TokenKind::KwEnd, "at end of package specification");
    parseClosingName(spec->lower, true);
    expect(TokenKind::Semicolon, "after package specification");
    return spec;
}

void Parser::parseElaborateBody(Decl& decl, const std::string& name, bool inside)
{
    if (decl.elaborateBody) {
        m_diagnostics.error(current().location, "duplicate pragma Elaborate_Body");
    }
    advance();
    advance();
    if (match(TokenKind::LeftParen)) {
        std::string lower;
        if (inside) {
            lower = expect(TokenKind::Identifier, "as Elaborate_Body unit name").lower;
        } else {
            parseCompoundName(lower);
        }
        std::string expected = inside ? name.substr(name.find_last_of('.') + 1) : name;
        if (lower != expected) {
            fail("pragma Elaborate_Body must name its own library unit");
        }
        expect(TokenKind::RightParen, "after Elaborate_Body unit name");
    }
    expect(TokenKind::Semicolon, "after Elaborate_Body");
    decl.elaborateBody = true;
}
