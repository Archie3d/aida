#include "Parser.h"
#include "Lexer.h"

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
            std::size_t start = m_position;
            auto unit = std::make_unique<CompilationUnit>();
            parseContextClause(*unit);
            unit->assertionsEnabled = m_assertionsEnabled;
            unit->suppressedChecks = m_configurationChecks;
            if (check(TokenKind::EndOfFile)) {
                break;
            }
            if (match(TokenKind::KwSeparate)) {
                expect(TokenKind::LeftParen, "after separate");
                parseCompoundName(unit->subunitParent);
                expect(TokenKind::RightParen, "after subunit parent");
            }
            m_libraryItem = true;
            bool privateUnit = match(TokenKind::KwPrivate);
            DeclPtr decl = parseLibraryUnit();
            if (decl != nullptr) {
                decl->privateUnit = privateUnit;
                unit->assertionPolicies[decl->location.file] = m_assertionsEnabled;
            }
            if (decl) {
                while (check(TokenKind::KwPragma) && peek(1).lower != "elaborate"
                    && peek(1).lower != "elaborate_all" && peek(1).lower != "assertion_policy"
                    && peek(1).lower != "suppress" && peek(1).lower != "unsuppress") {
                    if (peek(1).lower != "elaborate_body") {
                        DeclPtr pragma = parsePragma(true);
                        if (pragma != nullptr && static_cast<PragmaDecl*>(pragma.get())->lower == "assert") {
                            m_diagnostics.error(pragma->location, "pragma Assert is not allowed after a library unit");
                        }
                        if (pragma != nullptr) { unit->units.push_back(std::move(pragma)); }
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
                if (!unit->subunitParent.empty() && (privateUnit || decl->separateStub
                    || (decl->kind != DeclKind::SubprogramBody && decl->kind != DeclKind::PackageBody)
                    || (decl->kind == DeclKind::SubprogramBody && static_cast<SubprogramBody*>(decl.get())->isNull))) {
                    fail("a subunit requires a proper subprogram or package body");
                }
                unit->tokens = m_configurationTokens;
                unit->tokens.insert(unit->tokens.end(), m_tokens.begin() + start, m_tokens.begin() + m_position);
                unit->tokens.push_back(m_tokens.back());
                for (std::size_t i = 0; i + 1 < unit->tokens.size(); ++i) {
                    if (unit->tokens[i].kind == TokenKind::KwPragma && unit->tokens[i + 1].lower == "assert") {
                        WithClause dependency;
                        dependency.names.push_back("Ada.Assertions");
                        dependency.namesLower.push_back("ada.assertions");
                        dependency.location = unit->tokens[i].location;
                        unit->withClauses.push_back(std::move(dependency));
                        break;
                    }
                }
                unit->units.insert(unit->units.begin(), std::move(decl));
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
            appendDeclaration(declarations, std::move(decl));
        } catch (const ParseError&) {
            skipToSemicolon();
        }
    }

    return declarations;
}

void Parser::parseContextClause(CompilationUnit& unit)
{
    while (true) {
        if (check(TokenKind::KwWith) || check(TokenKind::KwLimited)
            || (check(TokenKind::KwPrivate) && peek(1).kind == TokenKind::KwWith)) {
            WithClause clause;
            clause.location = current().location;
            clause.limited = match(TokenKind::KwLimited);
            clause.privateWith = match(TokenKind::KwPrivate);
            expect(TokenKind::KwWith, "in context clause");
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
                        if (clause.limited) { continue; }
                        for (const std::string& name : clause.namesLower) {
                            withed = withed || name == lower || name.starts_with(lower + ".");
                        }
                    }
                    if (!withed) {
                        std::string resolved;
                        for (const UseDecl& use : unit.useClauses) {
                            if (use.typeOnly) {
                                continue;
                            }
                            for (const std::string& prefix : use.namesLower) {
                                std::string candidate = prefix + "." + lower;
                                for (const WithClause& clause : unit.withClauses) {
                                    if (clause.limited) { continue; }
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
                std::size_t start = m_position;
                DeclPtr pragma = parsePragma(false, true);
                if (pragma != nullptr && (pragma->kind == DeclKind::Pragma)) {
                    auto* policy = static_cast<PragmaDecl*>(pragma.get());
                    if (policy->lower == "assertion_policy" || policy->lower == "suppress" || policy->lower == "unsuppress") {
                        m_configurationTokens.insert(m_configurationTokens.end(), m_tokens.begin() + start,
                            m_tokens.begin() + m_position);
                    }
                }
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

bool Parser::expandStub(std::size_t start, const std::string& name, DeclKind kind)
{
    if (m_stubParent.empty()) {
        m_diagnostics.error(m_tokens[start].location,
            "a body stub must appear immediately within a compilation unit body");
        return false;
    }
    if (name.find('.') != std::string::npos || !operatorSymbol(name).empty()) {
        m_diagnostics.error(m_tokens[start].location, "a body stub requires a simple identifier");
        return false;
    }
    if (!m_stubNames.insert(toLower(name)).second) {
        m_diagnostics.error(m_tokens[start].location, "body stub names must be distinct");
        return false;
    }
    if (!m_subunitResolver) {
        return false;
    }
    auto tokens = m_subunitResolver(m_stubParent, name, kind, m_tokens[start].location);
    if (tokens.empty()) {
        return false;
    }
    auto info = tokens.front().subunit;
    if (kind == DeclKind::SubprogramBody) {
        info->stubTokens.assign(m_tokens.begin() + start, m_tokens.begin() + m_position - 3);
        info->stubTokens.push_back(m_tokens[m_position - 1]); // Replace 'is separate' by ';'.
        info->stubTokens.push_back(m_tokens.back());
    }
    m_tokens.erase(m_tokens.begin() + start, m_tokens.begin() + m_position);
    m_tokens.insert(m_tokens.begin() + start, tokens.begin(), tokens.end());
    m_position = start;
    return true;
}
