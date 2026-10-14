#include "Parser.h"

#include <utility>

Parser::Parser(std::vector<Token> tokens, Diagnostics& diagnostics)
    : m_tokens(std::move(tokens))
    , m_diagnostics(diagnostics)
{
}

const Token& Parser::current() const
{
    return m_tokens[m_position];
}

const Token& Parser::peek(int offset) const
{
    std::size_t index = m_position + static_cast<std::size_t>(offset);
    if (index >= m_tokens.size()) {
        return m_tokens.back();
    }
    return m_tokens[index];
}

bool Parser::check(TokenKind kind) const
{
    return current().kind == kind;
}

bool Parser::match(TokenKind kind)
{
    if (check(kind)) {
        advance();
        return true;
    }
    return false;
}

const Token& Parser::advance()
{
    const Token& token = m_tokens[m_position];
    if (m_position + 1 < m_tokens.size()) {
        ++m_position;
    }
    return token;
}

const Token& Parser::expect(TokenKind kind, const char* context)
{
    if (check(kind)) {
        return advance();
    }
    fail(std::string("expected '") + tokenKindName(kind) + "' " + context + ", found '"
         + (current().text.empty() ? tokenKindName(current().kind) : current().text) + "'");
}

void Parser::fail(const std::string& message)
{
    m_diagnostics.error(current().location, message);
    throw ParseError {};
}

void Parser::skipToSemicolon()
{
    while (!check(TokenKind::EndOfFile)) {
        if (match(TokenKind::Semicolon)) {
            return;
        }
        advance();
    }
}

std::vector<CompilationUnitPtr> Parser::parseCompilation()
{
    std::vector<CompilationUnitPtr> units;

    while (!check(TokenKind::EndOfFile)) {
        try {
            auto unit = std::make_unique<CompilationUnit>();
            parseContextClause(*unit);
            if (check(TokenKind::EndOfFile)) {
                break;
            }
            DeclPtr decl = parseLibraryUnit();
            if (decl) {
                while (check(TokenKind::KwPragma) && peek(1).lower != "elaborate"
                    && peek(1).lower != "elaborate_all") {
                    if (peek(1).lower != "elaborate_body") {
                        parsePragma();
                        continue;
                    }
                    std::string name;
                    if (decl->kind == DeclKind::SubprogramDeclaration) {
                        name = static_cast<SubprogramDecl*>(decl.get())->spec.lower;
                    } else if (decl->kind == DeclKind::GenericDeclaration
                        && !static_cast<GenericDecl*>(decl.get())->isPackage) {
                        name = static_cast<GenericDecl*>(decl.get())->lower;
                    } else if (decl->kind == DeclKind::GenericInstantiation) {
                        name = static_cast<GenericInstantiationDecl*>(decl.get())->lower;
                    } else {
                        fail("pragma Elaborate_Body after a library unit requires a subprogram declaration or generic instantiation");
                    }
                    if (peek(2).kind != TokenKind::LeftParen) {
                        fail("pragma Elaborate_Body after a library unit requires its name");
                    }
                    parseElaborateBody(*decl, name);
                }
                unit->units.push_back(std::move(decl));
                units.push_back(std::move(unit));
            }
        } catch (const ParseError&) {
            skipToSemicolon();
        }
    }

    return units;
}

DeclList Parser::parseDeclarations()
{
    DeclList declarations;

    while (!check(TokenKind::EndOfFile)) {
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

void Parser::parseContextClause(CompilationUnit& unit)
{
    while (true) {
        if (check(TokenKind::KwLimited)
            && (peek(1).kind == TokenKind::KwWith
                || (peek(1).kind == TokenKind::KwPrivate && peek(2).kind == TokenKind::KwWith))) {
            fail("limited with clauses are not yet supported");
        }
        if (check(TokenKind::KwPrivate) && peek(1).kind == TokenKind::KwWith) {
            fail("private with clauses are not yet supported");
        }
        if (check(TokenKind::KwWith)) {
            WithClause clause;
            clause.location = current().location;
            advance();
            while (true) {
                std::string lowered;
                std::string name = parseCompoundName(lowered);
                clause.names.push_back(name);
                clause.namesLower.push_back(lowered);
                if (!match(TokenKind::Comma)) {
                    break;
                }
            }
            expect(TokenKind::Semicolon, "after with clause");
            unit.withClauses.push_back(std::move(clause));
            continue;
        }
        if (check(TokenKind::KwUse)) {
            DeclPtr use = parseUseClause();
            unit.useClauses.push_back(std::move(*static_cast<UseDecl*>(use.get())));
            continue;
        }
        if (check(TokenKind::KwPragma)) {
            if (peek(1).lower == "elaborate" || peek(1).lower == "elaborate_all") {
                ElaborationPragma pragma;
                pragma.location = current().location;
                advance();
                pragma.all = advance().lower == "elaborate_all";
                expect(TokenKind::LeftParen, "after elaboration pragma");
                do {
                    std::string lower;
                    parseCompoundName(lower);
                    bool withed = false;
                    for (const WithClause& clause : unit.withClauses) {
                        for (const std::string& name : clause.namesLower) {
                            withed = withed || name == lower || name.starts_with(lower + ".");
                        }
                    }
                    if (!withed) {
                        std::string resolved;
                        for (const UseDecl& use : unit.useClauses) {
                            if (use.m_typeOnly) {
                                continue;
                            }
                            for (const std::string& prefix : use.namesLower) {
                                std::string candidate = prefix + "." + lower;
                                for (const WithClause& clause : unit.withClauses) {
                                    for (const std::string& name : clause.namesLower) {
                                        if (name == candidate || name.starts_with(candidate + ".")) {
                                            if (!resolved.empty() && resolved != candidate) {
                                                fail("ambiguous library unit name in elaboration pragma");
                                            }
                                            resolved = candidate;
                                        }
                                    }
                                }
                            }
                        }
                        if (!resolved.empty()) {
                            lower = resolved;
                            withed = true;
                        }
                    }
                    if (!withed) {
                        m_diagnostics.error(pragma.location,
                            "elaboration pragma requires a preceding with clause for '" + lower + "'");
                    }
                    pragma.names.push_back(lower);
                } while (match(TokenKind::Comma));
                expect(TokenKind::RightParen, "after elaboration pragma arguments");
                expect(TokenKind::Semicolon, "after elaboration pragma");
                unit.elaborationPragmas.push_back(std::move(pragma));
            } else {
                parsePragma();
            }
            continue;
        }
        break;
    }
}

DeclPtr Parser::parseLibraryUnit()
{
    if (check(TokenKind::KwGeneric)) {
        return parseGenericDeclaration(false);
    }
    if (check(TokenKind::KwProcedure) || check(TokenKind::KwFunction)
        || check(TokenKind::KwOverriding) || check(TokenKind::KwNot)) {
        return parseSubprogramDeclOrBody();
    }
    if (check(TokenKind::KwPackage)) {
        return parsePackage(true);
    }
    fail("expected a procedure, function or package declaration");
}
