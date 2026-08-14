#pragma once

#include <string>

namespace eval::parser {

enum class TokenKind {
    // Literals
    IntLiteral,
    FloatLiteral,
    StringLiteral,
    // Keywords
    KwInt,
    KwFloat,
    KwVoid,
    KwIf,
    KwElse,
    KwWhile,
    KwFor,
    KwReturn,
    KwConst,
    // Identifier
    Identifier,
    // Punctuation
    LParen,
    RParen,
    LBrace,
    RBrace,
    LBracket,
    RBracket,
    Semicolon,
    Comma,
    Dot,
    // Operators
    Plus,
    Minus,
    Star,
    Slash,
    Percent,
    Ampersand,
    Pipe,
    Caret,
    Tilde,
    Bang,
    Equal,
    Less,
    Greater,
    // Double-char operators
    EqualEqual,
    BangEqual,
    LessEqual,
    GreaterEqual,
    AmpAmp,
    PipePipe,
    PlusPlus,
    MinusMinus,
    Arrow,
    // Special
    EndOfFile,
    Unknown
};

std::string tokenKindName(TokenKind kind);

struct Token {
    TokenKind kind;
    std::string text;
    int line;
    int column;
};

class Lexer {
public:
    Lexer(std::string source, std::string filename);

    Token next();
    Token peek();
    bool atEnd() const;
    const std::string& filename() const { return filename_; }

private:
    void skipWhitespaceAndComments();
    Token lexIdentifierOrKeyword();
    Token lexNumber();
    Token lexString();
    Token makeToken(TokenKind kind, std::string text);
    char current() const;
    char advance();
    bool match(char expected);

    std::string source_;
    std::string filename_;
    size_t pos_;
    int line_;
    int column_;
    bool hasPeeked_;
    Token peeked_;
};

} // namespace eval::parser
