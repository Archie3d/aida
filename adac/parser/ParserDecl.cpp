#include "Parser.h"

#include "Lexer.h"

#include <utility>

DeclList Parser::parseDeclarativePart(bool stopAtPrivate, const std::string& stubParent)
{
    std::string savedParent = std::exchange(m_stubParent, stubParent);
    auto savedNames = std::move(m_stubNames);
    m_stubNames.clear();
    DeclList declarations;

    while (true) {
        if (check(TokenKind::EndOfFile) || check(TokenKind::KwBegin) || check(TokenKind::KwEnd)
            || check(TokenKind::KwException)) {
            break;
        }
        if (stopAtPrivate && check(TokenKind::KwPrivate)) {
            break;
        }

        std::size_t declarationStart = m_position;
        try {
            DeclPtr decl = parseDeclarativeItem();
            appendDeclaration(declarations, std::move(decl));
        } catch (const ParseError&) {
            // A missing delimiter can leave us at the next declaration. Keep
            // its introducer instead of consuming its first internal semicolon.
            // Require progress so a rejected introducer cannot loop forever.
            if (m_position > declarationStart
                && (check(TokenKind::KwType) || check(TokenKind::KwSubtype)
                    || check(TokenKind::KwProcedure) || check(TokenKind::KwFunction)
                    || check(TokenKind::KwPackage) || check(TokenKind::KwGeneric))) {
                continue;
            }
            skipToSemicolon();
        }
    }

    m_stubParent = std::move(savedParent);
    m_stubNames = std::move(savedNames);
    return declarations;
}

DeclPtr Parser::parseDeclarativeItem()
{
    switch (current().kind) {
    case TokenKind::KwTask:
        return parseTask();
    case TokenKind::KwProtected:
        return parseProtected();
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
        return parsePragma(true);
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

    bool aliased = match(TokenKind::KwAliased);
    bool isConstant = match(TokenKind::KwConstant);
    if (aliased && isConstant && check(TokenKind::Assign)) { fail("an aliased constant requires a subtype"); }
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

    if (check(TokenKind::KwArray)) {
        auto group = std::make_unique<ObjectDecl>();
        group->location = location;
        std::size_t definitionStart = m_position;
        // Each object gets its own type, bounds evaluation and initializer,
        // including when a defining_identifier_list contains several names.
        for (std::size_t i = 0; i < names.size(); ++i) {
            m_position = definitionStart;
            auto type = std::make_unique<TypeDecl>();
            type->location = current().location;
            type->name = "anonymous array for " + names[i];
            type->lower = "__anonymous_array_" + lowered[i] + "_"
                + std::to_string(location.line) + "_" + std::to_string(location.column);
            type->definition = parseTypeDefinition();
            auto object = std::make_unique<ObjectDecl>();
            object->location = location;
            object->names = {names[i]};
            object->namesLower = {lowered[i]};
            object->isConstant = isConstant;
            object->aliased = aliased;
            object->subtype = std::make_unique<SubtypeIndication>();
            object->subtype->location = type->location;
            object->subtype->name = type->name;
            object->subtype->lower = type->lower;
            if (check(TokenKind::KwRenames)) {
                fail("an object renaming requires a subtype mark");
            }
            if (match(TokenKind::Assign)) {
                object->initializer = parseExpression();
            } else if (isConstant) {
                fail("an anonymous array constant requires an initializer");
            }
            expect(TokenKind::Semicolon, "after object declaration");
            group->anonymousArrayDeclarations.push_back(std::move(type));
            group->anonymousArrayDeclarations.push_back(std::move(object));
        }
        return group;
    }

    auto decl = std::make_unique<ObjectDecl>();
    decl->location = location;
    decl->names = std::move(names);
    decl->namesLower = std::move(lowered);
    decl->isConstant = isConstant;
    decl->aliased = aliased;
    decl->subtype = parseSubtypeIndication(true);
    if (match(TokenKind::KwRenames)) {
        if (decl->names.size() != 1 || isConstant || aliased) {
            fail("an object renaming requires one name and no constant keyword");
        }
        decl->isRenaming = true;
        decl->initializer = parseExpression();
    } else if (match(TokenKind::Assign)) {
        decl->initializer = parseExpression();
    }
    expect(TokenKind::Semicolon, "after object declaration");
    return decl;
}

void Parser::appendDeclaration(DeclList& declarations, DeclPtr declaration)
{
    if (declaration != nullptr && declaration->kind == DeclKind::Object) {
        auto* object = static_cast<ObjectDecl*>(declaration.get());
        if (!object->anonymousArrayDeclarations.empty()) {
            for (DeclPtr& expanded : object->anonymousArrayDeclarations) {
                declarations.push_back(std::move(expanded));
            }
            return;
        }
    }
    if (declaration != nullptr) { declarations.push_back(std::move(declaration)); }
}

DeclPtr Parser::parseUseClause()
{
    auto decl = std::make_unique<UseDecl>();
    decl->location = current().location;
    expect(TokenKind::KwUse, "in use clause");
    decl->all = match(TokenKind::KwAll);
    decl->typeOnly = match(TokenKind::KwType);
    if (decl->all && !decl->typeOnly) {
        fail("expected type after use all");
    }
    while (true) {
        std::string lowered;
        std::string name = decl->typeOnly ? parseSubtypeMark(lowered) : parseCompoundName(lowered);
        decl->names.push_back(name);
        decl->namesLower.push_back(lowered);
        if (!match(TokenKind::Comma)) {
            break;
        }
    }
    expect(TokenKind::Semicolon, "after use clause");
    return decl;
}

// Keep semantic pragmas for analysis. Inline is advisory. Keep unsupported
// requirements on representation, checks or execution visible as diagnostics.
DeclPtr Parser::parsePragma(bool allowShared, bool configuration)
{
    SourceLocation location = current().location;
    expect(TokenKind::KwPragma, "in pragma");

    const Token& name = expect(TokenKind::Identifier, "as pragma name");
    if (name.lower == "page" || name.lower == "list" || name.lower == "optimize") {
        if (name.lower != "page") {
            expect(TokenKind::LeftParen, "after pragma name");
            const Token& argument = expect(TokenKind::Identifier, "as pragma argument");
            bool valid = name.lower == "list" ? argument.lower == "on" || argument.lower == "off"
                : argument.lower == "time" || argument.lower == "space" || argument.lower == "off";
            if (!valid) { fail(name.lower == "list" ? "List requires On or Off" : "Optimize requires Time, Space or Off"); }
            expect(TokenKind::RightParen, "after pragma argument");
        }
        expect(TokenKind::Semicolon, "after pragma");
        // No listing is generated. All optimization modes use the same QBE
        // pipeline; the language defines Optimize as implementation advice.
        return nullptr;
    }
    if (name.lower == "assert" || name.lower == "assertion_policy"
        || name.lower == "suppress" || name.lower == "unsuppress" || name.lower == "no_return") {
        if (name.lower == "assertion_policy" && !configuration) {
            fail("pragma Assertion_Policy is only allowed as a configuration pragma in Ada 2005");
        }
        if ((name.lower == "assert" || name.lower == "no_return") && configuration) {
            fail("pragma " + name.text + " is not a configuration pragma");
        }
        if (!configuration && !allowShared && name.lower != "assert") {
            fail("pragma " + name.text + " is only allowed in a declarative part or package specification");
        }
        auto pragma = std::make_unique<PragmaDecl>();
        pragma->location = location;
        pragma->name = name.text;
        pragma->lower = name.lower;
        expect(TokenKind::LeftParen, "after pragma name");
        if (name.lower == "assert") {
            do {
                Association argument;
                if (check(TokenKind::Identifier) && peek(1).kind == TokenKind::Arrow) {
                    argument.name = advance().text;
                    argument.nameLower = toLower(argument.name);
                    advance();
                }
                argument.value = parseExpression();
                pragma->arguments.push_back(std::move(argument));
            } while (match(TokenKind::Comma));
        } else {
            do {
                Association argument;
                const Token& entity = expect(TokenKind::Identifier, "as pragma argument");
                auto identifier = std::make_unique<IdentifierExpr>();
                identifier->location = entity.location;
                identifier->name = entity.text;
                identifier->lower = entity.lower;
                argument.value = std::move(identifier);
                pragma->arguments.push_back(std::move(argument));
            } while (match(TokenKind::Comma));
        }
        expect(TokenKind::RightParen, "after pragma arguments");
        expect(TokenKind::Semicolon, "after pragma");
        if (name.lower == "assertion_policy" || name.lower == "suppress" || name.lower == "unsuppress") {
            if (pragma->arguments.size() != 1) {
                m_diagnostics.error(location, "pragma " + name.text + " requires one identifier");
                return nullptr;
            }
            std::string argument = static_cast<IdentifierExpr*>(pragma->arguments.front().value.get())->lower;
            if (name.lower == "assertion_policy") {
                if (argument != "check" && argument != "ignore") {
                    m_diagnostics.error(location, "assertion policy must be Check or Ignore");
                    return nullptr;
                }
                m_assertionsEnabled = argument == "check";
            } else {
                pragma->checkMask = CheckPolicy::namedCheck(argument);
                if (pragma->checkMask == 0) {
                    m_diagnostics.error(location, "unknown language-defined check '" + argument + "'");
                    return nullptr;
                }
                if (configuration) {
                    if (name.lower == "suppress") { m_configurationChecks |= pragma->checkMask; }
                    else { m_configurationChecks &= ~pragma->checkMask; }
                }
            }
        }
        return pragma;
    }
    if (name.lower == "elaborate" || name.lower == "elaborate_all"
        || name.lower == "elaborate_body") {
        m_diagnostics.error(name.location, name.lower == "elaborate_body"
            ? "pragma Elaborate_Body must appear at the start of a library package specification"
            : "pragma " + name.text + " is only allowed in a context clause");
        skipToSemicolon();
        return nullptr;
    }
    if (name.lower == "pure" || name.lower == "preelaborate") {
        fail("categorization pragma requires the start of a library package specification or a library unit declaration");
    }
    if (name.lower == "inline") {
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
    if (name.lower == "controlled" || name.lower == "preelaborable_initialization") {
        if (!allowShared) { fail("pragma " + name.text + " is only allowed in a declarative part or package specification"); }
        auto pragma = std::make_unique<PragmaDecl>();
        pragma->location = location;
        pragma->name = name.text;
        pragma->lower = name.lower;
        expect(TokenKind::LeftParen, name.lower == "controlled" ? "after pragma Controlled" : "after pragma Preelaborable_Initialization");
        const Token& entity = expect(TokenKind::Identifier, name.lower == "controlled" ? "as Controlled type name" : "as Preelaborable_Initialization type name");
        pragma->entity = entity.text;
        pragma->entityLower = entity.lower;
        expect(TokenKind::RightParen, name.lower == "controlled" ? "after Controlled type name" : "after Preelaborable_Initialization type name");
        expect(TokenKind::Semicolon, "after pragma");
        return pragma;
    }
    if (name.lower == "atomic" || name.lower == "volatile") {
        if (!allowShared) {
            fail("shared-variable pragmas are only supported in declarative parts");
        }
        auto pragma = std::make_unique<PragmaDecl>();
        pragma->location = location;
        pragma->name = name.text;
        pragma->lower = name.lower;
        expect(TokenKind::LeftParen, "after shared-variable pragma");
        const Token& entity = expect(TokenKind::Identifier, "as shared-variable pragma entity");
        pragma->entity = entity.text;
        pragma->entityLower = entity.lower;
        expect(TokenKind::RightParen, "after shared-variable pragma entity");
        expect(TokenKind::Semicolon, "after pragma");
        return pragma;
    }
    if (name.lower == "convention") {
        auto pragma = std::make_unique<PragmaDecl>();
        pragma->location = location;
        pragma->name = name.text;
        pragma->lower = name.lower;
        expect(TokenKind::LeftParen, "after pragma Convention");
        const Token& convention = expect(TokenKind::Identifier, "as Convention name");
        if (convention.lower != "ada" && convention.lower != "c") {
            fail("pragma Convention supports only Ada and C for access-to-subprogram types");
        }
        pragma->conventionAda = convention.lower == "ada";
        expect(TokenKind::Comma, "after Convention name");
        const Token& entity = expect(TokenKind::Identifier, "as Convention entity");
        pragma->entity = entity.text;
        pragma->entityLower = entity.lower;
        expect(TokenKind::RightParen, "after Convention arguments");
        expect(TokenKind::Semicolon, "after pragma");
        return pragma;
    }
    if (name.lower != "import") {
        m_diagnostics.error(name.location, "unsupported or unknown pragma '" + name.text
            + "'; supported pragmas include Import, Convention, Inline, Pure, Preelaborate, Preelaborable_Initialization, Atomic, Volatile, Assert, Assertion_Policy, Suppress, Unsuppress, No_Return, Controlled, List, Page and Optimize");
        skipToSemicolon();
        return nullptr;
    }

    auto pragma = std::make_unique<PragmaDecl>();
    pragma->location = location;
    pragma->name = name.text;
    pragma->lower = name.lower;

    auto rejectNamedArgument = [&]() {
        if (check(TokenKind::Identifier) && peek(1).kind == TokenKind::Arrow) {
            fail("named arguments in pragma Import are not yet supported");
        }
    };

    expect(TokenKind::LeftParen, "after pragma Import");
    rejectNamedArgument();
    const Token& convention = expect(TokenKind::Identifier, "as Import convention");
    if (convention.lower != "c" && convention.lower != "ada") {
        m_diagnostics.error(convention.location, "unsupported Import convention '" + convention.text
            + "'; only C and Ada are supported");
        skipToSemicolon();
        return nullptr;
    }
    pragma->conventionAda = convention.lower == "ada";
    expect(TokenKind::Comma, "after Import convention");
    rejectNamedArgument();
    if (check(TokenKind::Identifier)
        || (check(TokenKind::StringLiteral) && current().text == "**")) {
        pragma->entity = current().text;
        pragma->entityLower = toLower(current().text);
        advance();
    } else {
        fail("expected a subprogram name in pragma Import");
    }
    if (!match(TokenKind::Comma)) {
        fail("pragma Import requires an explicit external name");
    }
    rejectNamedArgument();
    const Token& externalName = expect(TokenKind::StringLiteral, "as Import external name");
    if (externalName.text.empty()) {
        m_diagnostics.error(externalName.location, "pragma Import requires a nonempty external name");
        skipToSemicolon();
        return nullptr;
    }
    pragma->linkName = externalName.text;

    if (match(TokenKind::Comma)) {
        rejectNamedArgument();
        fail("pragma Import supports only three positional arguments");
    }
    expect(TokenKind::RightParen, "after Import arguments");
    expect(TokenKind::Semicolon, "after pragma");

    return pragma;
}

// Parse each representation form explicitly; unsupported forms must never
// silently discard a requested layout.
DeclPtr Parser::parseRepresentationClause()
{
    SourceLocation location = current().location;
    expect(TokenKind::KwFor, "in representation clause");

    auto decl = std::make_unique<RepresentationDecl>();
    decl->location = location;
    decl->name = parseCompoundName(decl->lower);
    if (match(TokenKind::KwUse)) {
        if (check(TokenKind::LeftParen)) {
            decl->attribute = "enumeration";
            decl->value = parseParenthesizedOrAggregate();
            expect(TokenKind::Semicolon, "after enumeration representation clause");
            return decl;
        }
        if (match(TokenKind::KwRecord)) {
            decl->attribute = "record";
            if (match(TokenKind::KwAt)) {
                expect(TokenKind::KwMod, "in record mod clause");
                decl->modulus = parseExpression();
                expect(TokenKind::Semicolon, "after record mod clause");
            }
            while (!check(TokenKind::KwEnd) && !check(TokenKind::EndOfFile)) {
                ComponentClause component;
                component.location = current().location;
                const Token& name = expect(TokenKind::Identifier, "as component name");
                component.name = name.text;
                component.lower = name.lower;
                expect(TokenKind::KwAt, "after component name");
                component.position = parseExpression();
                expect(TokenKind::KwRange, "after component position");
                component.firstBit = parseSimpleExpression();
                expect(TokenKind::DoubleDot, "in component bit range");
                component.lastBit = parseSimpleExpression();
                expect(TokenKind::Semicolon, "after component clause");
                decl->components.push_back(std::move(component));
            }
            expect(TokenKind::KwEnd, "after component clauses");
            expect(TokenKind::KwRecord, "after end");
            expect(TokenKind::Semicolon, "after record representation clause");
            return decl;
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
    if (decl->attribute == "class") {
        decl->classWide = true;
        expect(TokenKind::Tick, "after class-wide representation prefix");
        decl->attribute = expect(TokenKind::Identifier, "as class-wide attribute").lower;
    }
    if (decl->attribute != "storage_size" && decl->attribute != "alignment" && decl->attribute != "size" && decl->attribute != "small" && decl->attribute != "stream_size" && decl->attribute != "machine_radix"
        && decl->attribute != "read" && decl->attribute != "write"
        && decl->attribute != "input" && decl->attribute != "output") {
        m_diagnostics.error(location, "unsupported representation attribute '" + attribute.text
            + "'; only 'Alignment, 'Size, 'Small and decimal 'Machine_Radix clauses are supported");
        skipToSemicolon();
        return nullptr;
    }

    expect(TokenKind::KwUse, "in representation clause");
    decl->value = parseExpression();
    expect(TokenKind::Semicolon, "after representation clause");
    return decl;
}
