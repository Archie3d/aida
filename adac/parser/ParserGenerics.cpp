#include "Parser.h"

#include <utility>

// A generic unit is remembered as the tokens it was written with.  It is parsed
// once here so that its name is known and its syntax is checked, and then again
// for each instantiation with the formals bound to actuals.
DeclPtr Parser::parseGenericDeclaration(bool includeFollowingBody)
{
    SourceLocation location = current().location;
    expect(TokenKind::KwGeneric, "in generic declaration");

    auto decl = std::make_unique<GenericDecl>();
    decl->location = location;

    // A generic renaming has neither a formal part nor a subprogram profile.
    if ((check(TokenKind::KwPackage) || check(TokenKind::KwProcedure) || check(TokenKind::KwFunction))
        && peek(1).kind == TokenKind::Identifier) {
        std::size_t saved = m_position;
        decl->isPackage = check(TokenKind::KwPackage);
        decl->isFunction = check(TokenKind::KwFunction);
        advance();
        decl->name = parseCompoundName(decl->lower);
        if (match(TokenKind::KwRenames)) {
            decl->renames = parseCompoundName(decl->renamesLower);
            expect(TokenKind::Semicolon, "after generic renaming");
            decl->hasBody = true; // A renaming is a complete declaration.
            return decl;
        }
        m_position = saved;
    }

    while (!check(TokenKind::KwPackage) && !check(TokenKind::KwProcedure) && !check(TokenKind::KwFunction)) {
        if (check(TokenKind::EndOfFile)) {
            fail("expected a package, procedure or function after the generic formal part");
        }

        GenericFormal formal;
        formal.location = current().location;
        std::size_t formalStart = m_position;
        if (match(TokenKind::KwType)) {
            const Token& name = expect(TokenKind::Identifier, "in generic formal type");
            formal.kind = GenericFormalKind::TypeFormal;
            formal.name = name.text;
            formal.lower = name.lower;
            if (check(TokenKind::LeftParen) && peek(1).kind != TokenKind::Box) {
                parseDiscriminantPart(formal.discriminants);
                for (const RecordField& discriminant : formal.discriminants) {
                    if (discriminant.defaultValue != nullptr) {
                        fail("a generic formal discriminant cannot have a default");
                    }
                }
            } else if (match(TokenKind::LeftParen)) {
                expect(TokenKind::Box, "in generic formal unknown discriminants");
                expect(TokenKind::RightParen, "after generic formal unknown discriminants");
                formal.unknownDiscriminants = true;
            }
            expect(TokenKind::KwIs, "in generic formal type");
            formal.limited = check(TokenKind::KwLimited);
            // Parse the entire supported definition. Never infer private-type
            // semantics from an unrecognized or partially consumed form.
            auto rejectUnsupportedFormal = [&]() {
                switch (current().kind) {
                case TokenKind::KwAliased:
                case TokenKind::KwProtected:
                case TokenKind::KwRequeue:
                case TokenKind::KwUntil:
                case TokenKind::KwInterface:
                case TokenKind::KwSynchronized:
                    fail("unsupported reserved word '" + current().text + "' in generic formal type definition");
                default:
                    fail("generic formal type definition '" + current().text + "' is not yet supported");
                }
            };
            bool interfaceDefinition = check(TokenKind::KwInterface)
                || ((check(TokenKind::KwLimited) || check(TokenKind::KwSynchronized)
                     || check(TokenKind::KwTask) || check(TokenKind::KwProtected)) && peek(1).kind == TokenKind::KwInterface);
            if (interfaceDefinition || check(TokenKind::KwNew) || check(TokenKind::KwAbstract) || check(TokenKind::KwTagged)
                || (check(TokenKind::KwLimited) && peek(1).kind == TokenKind::KwNew)) {
                auto definition = parseTypeDefinition();
                formal.abstract = definition->abstract;
                formal.tagged = definition->tagged;
                formal.limited = definition->isLimited;
                formal.interfaceKind = definition->interfaceKind;
                if (definition->interfaceKind != InterfaceKind::None) {
                    formal.typeClass = FormalTypeClass::InterfaceType;
                } else if (definition->kind == TypeDefKind::Derived) {
                    formal.typeClass = FormalTypeClass::DerivedType;
                    if (definition->extension && !definition->privateExtension) {
                        fail("a formal derived extension requires with private");
                    }
                    if (!definition->progenitors.empty() && !definition->privateExtension) {
                        fail("a formal derived interface list requires with private");
                    }
                    if (!definition->parent->indexLows.empty() || definition->parent->rangeLow || definition->parent->nullExclusion) {
                        fail("a formal derived ancestor must be a subtype mark");
                    }
                } else if (definition->kind != TypeDefKind::Private || !definition->tagged) {
                    fail("expected a formal derived type or tagged private type");
                }
                if (!check(TokenKind::Semicolon)) { fail("expected ';' after generic formal type"); }
                formal.typeTokens.assign(m_tokens.begin() + static_cast<std::ptrdiff_t>(formalStart),
                    m_tokens.begin() + static_cast<std::ptrdiff_t>(m_position + 1));
                formal.typeTokens.push_back(m_tokens.back());
            } else if (check(TokenKind::KwAccess) || check(TokenKind::KwNot)) {
                formal.typeClass = FormalTypeClass::AccessType;
                parseTypeDefinition();
                if (!check(TokenKind::Semicolon)) {
                    fail("expected ';' after formal access type");
                }
                formal.typeTokens.assign(m_tokens.begin() + static_cast<std::ptrdiff_t>(formalStart),
                    m_tokens.begin() + static_cast<std::ptrdiff_t>(m_position + 1));
                formal.typeTokens.push_back(m_tokens.back());
            } else if (check(TokenKind::KwArray)) {
                formal.typeClass = FormalTypeClass::ArrayType;
                formal.arrayDefinition = parseTypeDefinition();
                for (const auto& index : formal.arrayDefinition->indexTypes) {
                    if (index->name.empty() || index->rangeLow || index->rangeHigh) {
                        fail("a constrained formal array index must be a subtype mark");
                    }
                }
                if (!check(TokenKind::Semicolon)) {
                    fail("expected ';' after formal array type");
                }
            } else if (match(TokenKind::KwMod)) {
                formal.typeClass = FormalTypeClass::ModularType;
                expect(TokenKind::Box, "after mod in generic formal type");
                if (!check(TokenKind::Semicolon)) {
                    fail("generic formal modular definition must end after '<>'");
                }
            } else if (match(TokenKind::KwRange)) {
                formal.typeClass = FormalTypeClass::IntegerType;
                expect(TokenKind::Box, "after range in generic formal type");
                if (!check(TokenKind::Semicolon)) {
                    fail("generic formal range definition must end after '<>'");
                }
            } else if (check(TokenKind::KwDelta)) {
                advance();
                formal.typeClass = FormalTypeClass::FixedType;
                expect(TokenKind::Box, "after delta in ordinary fixed-point formal type");
                if (match(TokenKind::KwDigits)) {
                    formal.typeClass = FormalTypeClass::DecimalType;
                    expect(TokenKind::Box, "after digits in decimal fixed-point formal type");
                }
                if (!check(TokenKind::Semicolon)) {
                    fail("expected ';' after ordinary fixed-point formal type");
                }
            } else if (match(TokenKind::KwDigits)) {
                formal.typeClass = FormalTypeClass::FloatType;
                expect(TokenKind::Box, "after digits in generic formal type");
                if (!check(TokenKind::Semicolon)) {
                    fail("generic formal digits definition must end after '<>'");
                }
            } else if (match(TokenKind::LeftParen)) {
                formal.typeClass = FormalTypeClass::Discrete;
                expect(TokenKind::Box, "in discrete generic formal type");
                expect(TokenKind::RightParen, "after discrete generic formal type");
            } else if (match(TokenKind::KwLimited)) {
                if (!check(TokenKind::KwPrivate)) {
                    rejectUnsupportedFormal();
                }
                advance();
            } else if (!match(TokenKind::KwPrivate)) {
                rejectUnsupportedFormal();
            }
            if (formal.unknownDiscriminants && formal.typeClass != FormalTypeClass::Any
                && formal.typeClass != FormalTypeClass::DerivedType) {
                fail("unknown discriminants require a private formal type");
            }
            if (!formal.discriminants.empty()) {
                if (formal.typeClass != FormalTypeClass::Any) {
                    fail("known discriminants require a private formal type");
                }
                formal.typeTokens.assign(m_tokens.begin() + static_cast<std::ptrdiff_t>(formalStart),
                    m_tokens.begin() + static_cast<std::ptrdiff_t>(m_position + 1));
                formal.typeTokens.push_back(m_tokens.back());
            }
            if (formal.unknownDiscriminants && formal.typeClass == FormalTypeClass::DerivedType) {
                // Ordinary type declarations do not allow this formal-only
                // discriminant part. Keep the definition tokens for replay.
                formal.typeTokens.erase(formal.typeTokens.begin() + 2, formal.typeTokens.begin() + 5);
            }
        } else if (match(TokenKind::KwWith)) {
            formal.kind = GenericFormalKind::SubprogramFormal;
            std::size_t start = m_position;
            SubprogramSpec spec = parseSubprogramSpec();
            formal.name = spec.name;
            formal.lower = spec.lower;
            formal.subprogramTokens.assign(m_tokens.begin() + static_cast<std::ptrdiff_t>(start),
                m_tokens.begin() + static_cast<std::ptrdiff_t>(m_position));
            if (match(TokenKind::KwIs)) {
                formal.boxDefault = match(TokenKind::Box);
                if (!formal.boxDefault) {
                    if (check(TokenKind::KwNull)) {
                        fail("null defaults for generic formal subprograms are not yet supported");
                    }
                    formal.defaultValue = parseExpression();
                }
            }
            formal.subprogramTokens.push_back(current()); // The terminating semicolon.
            formal.subprogramTokens.push_back(m_tokens.back());
        } else if (check(TokenKind::Identifier)) {
            std::size_t start = m_position;
            const Token& name = advance();
            formal.kind = GenericFormalKind::ObjectFormal;
            formal.name = name.text;
            formal.lower = name.lower;
            expect(TokenKind::Colon, "in generic formal object");
            formal.objectTokens.assign(m_tokens.begin() + static_cast<std::ptrdiff_t>(start),
                m_tokens.begin() + static_cast<std::ptrdiff_t>(m_position));
            if (match(TokenKind::KwIn)) {
                if (match(TokenKind::KwOut)) {
                    formal.mode = ParameterMode::InOut;
                }
            } else if (match(TokenKind::KwOut)) {
                m_diagnostics.error(formal.location, "a generic formal object mode must be in or in out");
            }
            start = m_position;
            formal.subtype = parseSubtypeIndication(true);
            if (match(TokenKind::Assign)) {
                formal.defaultValue = parseExpression();
                if (formal.mode != ParameterMode::In) {
                    m_diagnostics.error(formal.location, "an in out generic formal object cannot have a default");
                }
            }
            formal.objectTokens.insert(formal.objectTokens.end(),
                m_tokens.begin() + static_cast<std::ptrdiff_t>(start),
                m_tokens.begin() + static_cast<std::ptrdiff_t>(m_position));
            formal.objectTokens.push_back(current());
            formal.objectTokens.push_back(m_tokens.back());
        } else {
            fail("expected a generic formal parameter");
        }
        expect(TokenKind::Semicolon, "after generic formal parameter");
        decl->formals.push_back(std::move(formal));
    }

    decl->isPackage = check(TokenKind::KwPackage);

    std::size_t start = m_position;
    DeclPtr unit = decl->isPackage ? parsePackage(!includeFollowingBody) : parseSubprogramDeclOrBody();
    if (unit == nullptr) {
        fail("a generic declaration must name a unit");
    }
    decl->elaborateBody = unit->elaborateBody;
    switch (unit->kind) {
    case DeclKind::PackageSpecification:
        if (!static_cast<PackageSpecDecl*>(unit.get())->renamesLower.empty()) {
            fail("a generic renaming cannot have a formal part");
        }
        decl->name = static_cast<PackageSpecDecl*>(unit.get())->name;
        decl->lower = static_cast<PackageSpecDecl*>(unit.get())->lower;
        break;
    case DeclKind::SubprogramDeclaration:
        if (static_cast<SubprogramDecl*>(unit.get())->renamedName != nullptr) {
            fail("a generic renaming cannot have a formal part or subprogram profile");
        }
        decl->isFunction = static_cast<SubprogramDecl*>(unit.get())->spec.isFunction;
        decl->hasBody = static_cast<SubprogramDecl*>(unit.get())->renamedName != nullptr;
        decl->name = static_cast<SubprogramDecl*>(unit.get())->spec.name;
        decl->lower = static_cast<SubprogramDecl*>(unit.get())->spec.lower;
        break;
    case DeclKind::SubprogramBody:
        if (static_cast<SubprogramBody*>(unit.get())->isNull) {
            m_diagnostics.error(unit->location, "a generic subprogram cannot be declared as a null procedure");
        }
        decl->isFunction = static_cast<SubprogramBody*>(unit.get())->spec.isFunction;
        decl->hasBody = true;
        decl->name = static_cast<SubprogramBody*>(unit.get())->spec.name;
        decl->lower = static_cast<SubprogramBody*>(unit.get())->spec.lower;
        break;
    default:
        fail("a generic declaration must name a package or a subprogram");
    }

    // Local generic bodies are retained with their declaration for expansion.
    // Library bodies remain distinct items with their own context and identity.
    while (includeFollowingBody) {
        bool packageBody = check(TokenKind::KwPackage) && peek(1).kind == TokenKind::KwBody
            && peek(2).lower == decl->lower;
        bool subprogramBody = (check(TokenKind::KwProcedure) || check(TokenKind::KwFunction))
            && peek(1).lower == decl->lower;
        if (!packageBody && !subprogramBody) {
            break;
        }
        decl->hasBody = true;
        if (packageBody) {
            parsePackage();
        } else {
            DeclPtr body = parseSubprogramDeclOrBody();
            if (body->kind == DeclKind::SubprogramBody && static_cast<SubprogramBody*>(body.get())->isNull) {
                m_diagnostics.error(body->location, "a null procedure cannot complete a generic subprogram");
            }
        }
    }

    decl->tokens.assign(m_tokens.begin() + static_cast<std::ptrdiff_t>(start),
                        m_tokens.begin() + static_cast<std::ptrdiff_t>(m_position));
    if (decl->elaborateBody) {
        for (std::size_t i = 0; i + 1 < decl->tokens.size(); ++i) {
            if (decl->tokens[i].kind == TokenKind::KwPragma
                && decl->tokens[i + 1].lower == "elaborate_body") {
                std::size_t end = i;
                while (end < decl->tokens.size() && decl->tokens[end].kind != TokenKind::Semicolon) {
                    ++end;
                }
                decl->tokens.erase(decl->tokens.begin() + i, decl->tokens.begin() + end + 1);
                break;
            }
        }
    }
    decl->tokens.push_back(m_tokens.back());   // The end of file the parser stops on.
    return decl;
}

DeclPtr Parser::parseGenericInstantiation(const SourceLocation& location, const std::string& name,
                                          const std::string& lower, bool isPackage)
{
    auto decl = std::make_unique<GenericInstantiationDecl>();
    decl->location = location;
    decl->name = name;
    decl->lower = lower;
    decl->isPackage = isPackage;
    decl->genericName = parseCompoundName(decl->genericLower);

    if (match(TokenKind::LeftParen)) {
        while (true) {
            Association association;
            if ((check(TokenKind::Identifier) || (check(TokenKind::StringLiteral)
                    && !operatorSymbol(current().text).empty())) && peek(1).kind == TokenKind::Arrow) {
                association.name = current().text;
                association.nameLower = current().kind == TokenKind::StringLiteral
                    ? operatorSymbol(current().text) : current().lower;
                advance();
                advance();
            }
            association.value = parseExpression();
            decl->arguments.push_back(std::move(association));
            if (!match(TokenKind::Comma)) {
                break;
            }
        }
        expect(TokenKind::RightParen, "after generic actual parameters");
    }
    expect(TokenKind::Semicolon, "after generic instantiation");
    return decl;
}
