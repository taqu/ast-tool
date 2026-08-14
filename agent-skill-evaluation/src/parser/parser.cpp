#include "parser/parser.hpp"
#include <sstream>

namespace eval::parser {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

ast::SourceLocation Parser::currentLocation() const {
    return ast::SourceLocation{
        lexer_.filename(),
        current_.line,
        current_.column
    };
}

bool Parser::isTypeName() const {
    return current_.kind == TokenKind::KwInt
        || current_.kind == TokenKind::KwFloat
        || current_.kind == TokenKind::KwVoid
        || current_.kind == TokenKind::Identifier;
}

Token Parser::advance() {
    Token prev = current_;
    current_ = lexer_.next();
    return prev;
}

Token Parser::expect(TokenKind kind) {
    if (current_.kind != kind) {
        std::ostringstream oss;
        oss << "Expected " << tokenKindName(kind)
            << " but got '" << current_.text << "'";
        throw ParseError(oss.str(), current_.line, current_.column);
    }
    return advance();
}

bool Parser::check(TokenKind kind) const {
    return current_.kind == kind;
}

bool Parser::match(TokenKind kind) {
    if (check(kind)) {
        advance();
        return true;
    }
    return false;
}

// ---------------------------------------------------------------------------
// Public parse methods
// ---------------------------------------------------------------------------

std::shared_ptr<ast::TranslationUnit> Parser::parseTranslationUnit() {
    auto loc = currentLocation();
    auto tu = std::make_shared<ast::TranslationUnit>(loc);

    while (!check(TokenKind::EndOfFile)) {
        if (current_.kind == TokenKind::KwConst || isTypeName()) {
            // Could be a function decl or a variable decl at top level
            // Peek ahead: type name ( -> function
            // We need to look at the token after the identifier
            if (current_.kind == TokenKind::KwConst) {
                tu->addChild(parseStatement());
            } else {
                // type identifier
                Token typeTok = advance();
                if (check(TokenKind::Identifier)) {
                    Token nameTok = advance();
                    if (check(TokenKind::LParen)) {
                        // function declaration
                        advance(); // consume (
                        std::vector<ast::FunctionDecl::Param> params;
                        while (!check(TokenKind::RParen) && !check(TokenKind::EndOfFile)) {
                            std::string paramType;
                            if (current_.kind == TokenKind::KwConst) {
                                paramType = "const ";
                                advance();
                            }
                            if (isTypeName()) {
                                paramType += advance().text;
                            }
                            std::string paramName;
                            if (check(TokenKind::Identifier)) {
                                paramName = advance().text;
                            }
                            params.push_back({paramType, paramName});
                            if (!match(TokenKind::Comma)) break;
                        }
                        expect(TokenKind::RParen);

                        auto funcLoc = ast::SourceLocation{
                            lexer_.filename(), nameTok.line, nameTok.column};
                        auto func = std::make_shared<ast::FunctionDecl>(
                            funcLoc, nameTok.text, typeTok.text, std::move(params));

                        if (check(TokenKind::LBrace)) {
                            advance(); // {
                            while (!check(TokenKind::RBrace) && !check(TokenKind::EndOfFile)) {
                                func->addChild(parseStatement());
                            }
                            expect(TokenKind::RBrace);
                        } else {
                            expect(TokenKind::Semicolon);
                        }
                        tu->addChild(func);
                    } else {
                        // variable decl at top level
                        auto varLoc = ast::SourceLocation{
                            lexer_.filename(), nameTok.line, nameTok.column};
                        auto var = std::make_shared<ast::VarDecl>(
                            varLoc, nameTok.text, typeTok.text, false);
                        expect(TokenKind::Semicolon);
                        tu->addChild(var);
                    }
                } else {
                    // just skip unknown token
                    if (!check(TokenKind::EndOfFile)) advance();
                }
            }
        } else {
            advance(); // skip unknown
        }
    }

    return tu;
}

std::shared_ptr<ast::FunctionDecl> Parser::parseFunctionDecl() {
    auto loc = currentLocation();
    std::string retType;
    if (isTypeName()) {
        retType = advance().text;
    }

    std::string name;
    if (check(TokenKind::Identifier)) {
        name = advance().text;
    }

    expect(TokenKind::LParen);
    std::vector<ast::FunctionDecl::Param> params;
    while (!check(TokenKind::RParen) && !check(TokenKind::EndOfFile)) {
        std::string paramType;
        if (current_.kind == TokenKind::KwConst) {
            paramType = "const ";
            advance();
        }
        if (isTypeName()) {
            paramType += advance().text;
        }
        std::string paramName;
        if (check(TokenKind::Identifier)) {
            paramName = advance().text;
        }
        params.push_back({paramType, paramName});
        if (!match(TokenKind::Comma)) break;
    }
    expect(TokenKind::RParen);

    auto func = std::make_shared<ast::FunctionDecl>(loc, name, retType, std::move(params));

    if (check(TokenKind::LBrace)) {
        advance();
        while (!check(TokenKind::RBrace) && !check(TokenKind::EndOfFile)) {
            func->addChild(parseStatement());
        }
        expect(TokenKind::RBrace);
    } else {
        expect(TokenKind::Semicolon);
    }

    return func;
}

std::shared_ptr<ast::Node> Parser::parseStatement() {
    if (check(TokenKind::KwIf)) {
        return parseIfStmt();
    }
    if (check(TokenKind::KwWhile)) {
        return parseWhileStmt();
    }
    if (check(TokenKind::KwReturn)) {
        return parseReturnStmt();
    }
    if (check(TokenKind::KwConst)) {
        return parseVarDecl(true);
    }
    if (isTypeName()) {
        // Could be var decl: look at next token
        Token saved = current_;
        Token typeTok = advance();
        if (check(TokenKind::Identifier)) {
            Token nameTok = advance();
            if (check(TokenKind::Semicolon) || check(TokenKind::Equal)) {
                // variable decl
                auto loc = ast::SourceLocation{
                    lexer_.filename(), nameTok.line, nameTok.column};
                // skip optional initializer
                if (check(TokenKind::Equal)) {
                    advance();
                    parseExpression();
                }
                expect(TokenKind::Semicolon);
                return std::make_shared<ast::VarDecl>(loc, nameTok.text, typeTok.text, false);
            } else if (check(TokenKind::LParen)) {
                // function call statement: nameTok(...)
                advance(); // (
                int argCount = 0;
                while (!check(TokenKind::RParen) && !check(TokenKind::EndOfFile)) {
                    parseExpression();
                    ++argCount;
                    if (!match(TokenKind::Comma)) break;
                }
                expect(TokenKind::RParen);
                expect(TokenKind::Semicolon);
                auto loc = ast::SourceLocation{
                    lexer_.filename(), nameTok.line, nameTok.column};
                return std::make_shared<ast::CallExpr>(loc, nameTok.text, argCount);
            } else {
                // fallback: put back and return a literal or identifier
                auto loc = ast::SourceLocation{
                    lexer_.filename(), nameTok.line, nameTok.column};
                (void)saved;
                return std::make_shared<ast::Identifier>(loc, nameTok.text);
            }
        } else if (check(TokenKind::LParen)) {
            // function call statement: typeTok(...)
            auto loc = ast::SourceLocation{
                lexer_.filename(), typeTok.line, typeTok.column};
            advance(); // consume (
            int argCount = 0;
            while (!check(TokenKind::RParen) && !check(TokenKind::EndOfFile)) {
                parseExpression();
                ++argCount;
                if (!match(TokenKind::Comma)) break;
            }
            expect(TokenKind::RParen);
            match(TokenKind::Semicolon);
            return std::make_shared<ast::CallExpr>(loc, typeTok.text, argCount);
        } else {
            // just a type token on its own — weird, treat as identifier
            auto loc = ast::SourceLocation{
                lexer_.filename(), typeTok.line, typeTok.column};
            (void)saved;
            return std::make_shared<ast::Identifier>(loc, typeTok.text);
        }
    }

    // expression statement
    auto expr = parseExpression();
    match(TokenKind::Semicolon);
    return expr;
}

std::shared_ptr<ast::Node> Parser::parseVarDecl(bool isConst) {
    if (isConst) {
        advance(); // consume 'const'
    }
    auto loc = currentLocation();
    std::string type;
    if (isTypeName()) {
        type = advance().text;
    }
    std::string name;
    if (check(TokenKind::Identifier)) {
        name = advance().text;
    }
    if (check(TokenKind::Equal)) {
        advance();
        parseExpression();
    }
    expect(TokenKind::Semicolon);
    return std::make_shared<ast::VarDecl>(loc, name, type, isConst);
}

std::shared_ptr<ast::Node> Parser::parseIfStmt() {
    auto loc = currentLocation();
    expect(TokenKind::KwIf);
    expect(TokenKind::LParen);

    std::string condText;
    int depth = 1;
    while (!check(TokenKind::EndOfFile) && depth > 0) {
        if (check(TokenKind::LParen)) {
            ++depth;
        } else if (check(TokenKind::RParen)) {
            --depth;
            if (depth == 0) break;
        }
        condText += current_.text;
        advance();
    }
    expect(TokenKind::RParen);

    auto ifNode = std::make_shared<ast::IfStmt>(loc, condText);

    if (check(TokenKind::LBrace)) {
        advance();
        while (!check(TokenKind::RBrace) && !check(TokenKind::EndOfFile)) {
            ifNode->addChild(parseStatement());
        }
        expect(TokenKind::RBrace);
    } else {
        ifNode->addChild(parseStatement());
    }

    if (check(TokenKind::KwElse)) {
        advance();
        if (check(TokenKind::LBrace)) {
            advance();
            while (!check(TokenKind::RBrace) && !check(TokenKind::EndOfFile)) {
                ifNode->addChild(parseStatement());
            }
            expect(TokenKind::RBrace);
        } else {
            ifNode->addChild(parseStatement());
        }
    }

    return ifNode;
}

std::shared_ptr<ast::Node> Parser::parseWhileStmt() {
    auto loc = currentLocation();
    expect(TokenKind::KwWhile);
    expect(TokenKind::LParen);

    std::string condText;
    int depth = 1;
    while (!check(TokenKind::EndOfFile) && depth > 0) {
        if (check(TokenKind::LParen)) {
            ++depth;
        } else if (check(TokenKind::RParen)) {
            --depth;
            if (depth == 0) break;
        }
        condText += current_.text;
        advance();
    }
    expect(TokenKind::RParen);

    auto whileNode = std::make_shared<ast::WhileStmt>(loc, condText);

    if (check(TokenKind::LBrace)) {
        advance();
        while (!check(TokenKind::RBrace) && !check(TokenKind::EndOfFile)) {
            whileNode->addChild(parseStatement());
        }
        expect(TokenKind::RBrace);
    } else {
        whileNode->addChild(parseStatement());
    }

    return whileNode;
}

std::shared_ptr<ast::Node> Parser::parseReturnStmt() {
    auto loc = currentLocation();
    expect(TokenKind::KwReturn);
    auto retNode = std::make_shared<ast::ReturnStmt>(loc);
    if (!check(TokenKind::Semicolon)) {
        retNode->addChild(parseExpression());
    }
    expect(TokenKind::Semicolon);
    return retNode;
}

std::shared_ptr<ast::Node> Parser::parseExpression() {
    auto loc = currentLocation();

    // Handle primary
    if (check(TokenKind::IntLiteral) || check(TokenKind::FloatLiteral)
        || check(TokenKind::StringLiteral)) {
        auto tok = advance();
        auto lit = std::make_shared<ast::Literal>(loc, tok.text);
        // check for binary operator
        if (check(TokenKind::Plus) || check(TokenKind::Minus)
            || check(TokenKind::Star) || check(TokenKind::Slash)
            || check(TokenKind::EqualEqual) || check(TokenKind::BangEqual)
            || check(TokenKind::Less) || check(TokenKind::Greater)
            || check(TokenKind::LessEqual) || check(TokenKind::GreaterEqual)) {
            auto opTok = advance();
            auto binLoc = ast::SourceLocation{lexer_.filename(), opTok.line, opTok.column};
            auto binExpr = std::make_shared<ast::BinaryExpr>(binLoc, opTok.text);
            binExpr->addChild(lit);
            binExpr->addChild(parseExpression());
            return binExpr;
        }
        return lit;
    }

    if (check(TokenKind::Identifier)) {
        return parseCallOrIdentifier();
    }

    if (check(TokenKind::LParen)) {
        advance();
        auto inner = parseExpression();
        expect(TokenKind::RParen);
        return inner;
    }

    // fallback: consume and return unknown identifier
    if (!check(TokenKind::EndOfFile)) {
        auto tok = advance();
        return std::make_shared<ast::Identifier>(loc, tok.text);
    }

    return std::make_shared<ast::Literal>(loc, "");
}

std::shared_ptr<ast::Node> Parser::parseCallOrIdentifier() {
    auto loc = currentLocation();
    Token nameTok = advance(); // identifier

    if (check(TokenKind::LParen)) {
        advance(); // (
        int argCount = 0;
        while (!check(TokenKind::RParen) && !check(TokenKind::EndOfFile)) {
            parseExpression();
            ++argCount;
            if (!match(TokenKind::Comma)) break;
        }
        expect(TokenKind::RParen);
        auto callNode = std::make_shared<ast::CallExpr>(loc, nameTok.text, argCount);
        return callNode;
    }

    // Check for binary operator after identifier
    if (check(TokenKind::Plus) || check(TokenKind::Minus)
        || check(TokenKind::Star) || check(TokenKind::Slash)
        || check(TokenKind::EqualEqual) || check(TokenKind::BangEqual)
        || check(TokenKind::Less) || check(TokenKind::Greater)
        || check(TokenKind::LessEqual) || check(TokenKind::GreaterEqual)) {
        auto opTok = advance();
        auto binLoc = ast::SourceLocation{lexer_.filename(), opTok.line, opTok.column};
        auto binExpr = std::make_shared<ast::BinaryExpr>(binLoc, opTok.text);
        binExpr->addChild(std::make_shared<ast::Identifier>(loc, nameTok.text));
        binExpr->addChild(parseExpression());
        return binExpr;
    }

    return std::make_shared<ast::Identifier>(loc, nameTok.text);
}

} // namespace eval::parser
