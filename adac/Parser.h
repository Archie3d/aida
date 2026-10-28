#pragma once

#include "Ast.h"
#include "Diagnostics.h"
#include "Token.h"

#include <vector>
#include <functional>
#include <unordered_set>

class Parser
{
public:
    Parser(std::vector<Token> tokens, Diagnostics& diagnostics);

    using SubunitResolver = std::function<std::vector<Token>(const std::string&, const std::string&, DeclKind, const SourceLocation&)>;
    void setSubunitResolver(SubunitResolver resolver) { m_subunitResolver = std::move(resolver); }
    std::vector<CompilationUnitPtr> parseCompilation();

    // Parses a run of declarations, which is how a generic unit is expanded
    // from the tokens it was written with.
    DeclList parseDeclarations();
    DeclPtr parseFormalPackageInstantiation();

private:
    struct ParseError
    {
    };

    // Token navigation and compilation units (Parser.cpp).
    const Token& current() const;
    const Token& peek(int offset) const;
    bool check(TokenKind kind) const;
    bool match(TokenKind kind);
    const Token& advance();
    const Token& expect(TokenKind kind, const char* context);
    [[noreturn]] void fail(const std::string& message);
    void skipToSemicolon();
    bool expandStub(std::size_t start, const std::string& name, DeclKind kind);
    void parseContextClause(CompilationUnit& unit);
    DeclPtr parseLibraryUnit();

    // Declarations (ParserDecl.cpp).
    DeclList parseDeclarativePart(bool stopAtPrivate = false, const std::string& stubParent = {});
    DeclPtr parseDeclarativeItem();
    DeclPtr parseTask();
    DeclPtr parseProtected();
    DeclPtr parseObjectOrNumberDecl();
    void appendDeclaration(DeclList& declarations, DeclPtr declaration);
    DeclPtr parseUseClause();
    DeclPtr parsePragma(bool allowShared = false);
    DeclPtr parseRepresentationClause();

    // Types and ranges (ParserTypes.cpp).
    DeclPtr parseTypeDecl();
    DeclPtr parseSubtypeDecl();
    TypeDefinitionPtr parseTypeDefinition(bool allowProtectedAccess = false);
    void parseDiscriminantPart(std::vector<RecordField>& discriminants);
    void parseRecordComponents(std::vector<RecordField>& fields);
    VariantPartPtr parseVariantPart();
    SubtypeIndicationPtr parseSubtypeIndication(bool allowAccess = false);
    void parseDiscreteRange(std::string& typeName, std::string& typeLower, ExprPtr& low, ExprPtr& high);

    // Subprograms (ParserSubprograms.cpp).
    SubprogramSpec parseSubprogramSpec(bool allowInstantiation = false);
    void parseParameterList(SubprogramSpec& spec);
    DeclPtr parseSubprogramDeclOrBody();

    // Packages (ParserPackages.cpp).
    DeclPtr parsePackage(bool library = false);
    void parseElaborateBody(Decl& decl, const std::string& name, bool inside = false);

    // Generics (ParserGenerics.cpp).
    DeclPtr parseGenericDeclaration(bool includeFollowingBody = true);
    DeclPtr parseGenericInstantiation(const SourceLocation& location, const std::string& name,
                                      const std::string& lower, bool isPackage, bool formalPackage = false);

    // Statements (ParserStatements.cpp).
    StmtList parseSequenceOfStatements();
    std::vector<ExceptionHandler> parseExceptionHandlers();
    StmtPtr parseStatement();
    StmtPtr parseIfStatement();
    StmtPtr parseCaseStatement();
    StmtPtr parseLoopStatement(const std::string& label);
    StmtPtr parseBlockStatement(const std::string& label);
    StmtPtr parseExitStatement();
    StmtPtr parseReturnStatement();
    StmtPtr parseRaiseStatement();

    // Expressions (ParserExpr.cpp).
    ExprPtr parseExpression();
    ExprPtr parseRelation();
    ExprPtr parseSimpleExpression();
    ExprPtr parseTerm();
    ExprPtr parseFactor();
    ExprPtr parsePrimary();
    ExprPtr parseParenthesizedOrAggregate();
    ExprPtr makeBinary(BinaryOp op, ExprPtr left, ExprPtr right, const SourceLocation& location);

    // Names (ParserNames.cpp).
    std::vector<std::string> parseIdentifierList(std::vector<std::string>& lowered);
    std::string parseCompoundName(std::string& lowered);
    std::string parseSubtypeMark(std::string& lowered);
    void parseClosingName(const std::string& lower, bool allowSimpleName = false);
    ExprPtr parseNameSuffixes(ExprPtr prefix);

    std::vector<Token> m_tokens;
    Diagnostics& m_diagnostics;
    std::size_t m_position = 0;
    bool m_libraryItem = false;
    std::string m_stubParent;
    std::unordered_set<std::string> m_stubNames;
    SubunitResolver m_subunitResolver;
};
