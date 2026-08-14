#pragma once

#include "lexer.hpp"
#include "../ast/node.hpp"
#include <memory>
#include <stdexcept>
#include <string>

namespace eval::parser {

class ParseError : public std::runtime_error {
public:
    ParseError(const std::string& message, int line, int column)
        : std::runtime_error(message)
        , line_(line)
        , column_(column) {}

    int line() const { return line_; }
    int column() const { return column_; }

private:
    int line_;
    int column_;
};

class Parser {
public:
    explicit Parser(Lexer lexer) : lexer_(std::move(lexer)) {
        current_ = lexer_.next();
    }

    std::shared_ptr<ast::TranslationUnit> parseTranslationUnit();
    std::shared_ptr<ast::FunctionDecl> parseFunctionDecl();
    std::shared_ptr<ast::Node> parseStatement();
    std::shared_ptr<ast::Node> parseExpression();

private:
    Token advance();
    Token expect(TokenKind kind);
    bool check(TokenKind kind) const;
    bool match(TokenKind kind);

    std::shared_ptr<ast::Node> parseVarDecl(bool isConst);
    std::shared_ptr<ast::Node> parseIfStmt();
    std::shared_ptr<ast::Node> parseWhileStmt();
    std::shared_ptr<ast::Node> parseReturnStmt();
    std::shared_ptr<ast::Node> parseCallOrIdentifier();

    ast::SourceLocation currentLocation() const;
    bool isTypeName() const;

    Lexer lexer_;
    Token current_;
};

} // namespace eval::parser
