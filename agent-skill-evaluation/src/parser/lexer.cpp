#include "parser/lexer.hpp"
#include <unordered_map>

namespace eval::parser {

static const std::unordered_map<std::string, TokenKind> kKeywords = {
    {"int",    TokenKind::KwInt},
    {"float",  TokenKind::KwFloat},
    {"void",   TokenKind::KwVoid},
    {"if",     TokenKind::KwIf},
    {"else",   TokenKind::KwElse},
    {"while",  TokenKind::KwWhile},
    {"for",    TokenKind::KwFor},
    {"return", TokenKind::KwReturn},
    {"const",  TokenKind::KwConst},
};

std::string tokenKindName(TokenKind kind) {
    switch (kind) {
        case TokenKind::IntLiteral:    return "IntLiteral";
        case TokenKind::FloatLiteral:  return "FloatLiteral";
        case TokenKind::StringLiteral: return "StringLiteral";
        case TokenKind::KwInt:         return "kw_int";
        case TokenKind::KwFloat:       return "kw_float";
        case TokenKind::KwVoid:        return "kw_void";
        case TokenKind::KwIf:          return "kw_if";
        case TokenKind::KwElse:        return "kw_else";
        case TokenKind::KwWhile:       return "kw_while";
        case TokenKind::KwFor:         return "kw_for";
        case TokenKind::KwReturn:      return "kw_return";
        case TokenKind::KwConst:       return "kw_const";
        case TokenKind::Identifier:    return "Identifier";
        case TokenKind::LParen:        return "(";
        case TokenKind::RParen:        return ")";
        case TokenKind::LBrace:        return "{";
        case TokenKind::RBrace:        return "}";
        case TokenKind::LBracket:      return "[";
        case TokenKind::RBracket:      return "]";
        case TokenKind::Semicolon:     return ";";
        case TokenKind::Comma:         return ",";
        case TokenKind::Dot:           return ".";
        case TokenKind::Plus:          return "+";
        case TokenKind::Minus:         return "-";
        case TokenKind::Star:          return "*";
        case TokenKind::Slash:         return "/";
        case TokenKind::Percent:       return "%";
        case TokenKind::Ampersand:     return "&";
        case TokenKind::Pipe:          return "|";
        case TokenKind::Caret:         return "^";
        case TokenKind::Tilde:         return "~";
        case TokenKind::Bang:          return "!";
        case TokenKind::Equal:         return "=";
        case TokenKind::Less:          return "<";
        case TokenKind::Greater:       return ">";
        case TokenKind::EqualEqual:    return "==";
        case TokenKind::BangEqual:     return "!=";
        case TokenKind::LessEqual:     return "<=";
        case TokenKind::GreaterEqual:  return ">=";
        case TokenKind::AmpAmp:        return "&&";
        case TokenKind::PipePipe:      return "||";
        case TokenKind::PlusPlus:      return "++";
        case TokenKind::MinusMinus:    return "--";
        case TokenKind::Arrow:         return "->";
        case TokenKind::EndOfFile:     return "EOF";
        case TokenKind::Unknown:       return "Unknown";
        default:                       return "?";
    }
}

// ---------------------------------------------------------------------------
// Lexer implementation
// ---------------------------------------------------------------------------

Lexer::Lexer(std::string source, std::string filename)
    : source_(std::move(source))
    , filename_(std::move(filename))
    , pos_(0)
    , line_(1)
    , column_(1)
    , hasPeeked_(false)
    , peeked_{TokenKind::Unknown, "", 0, 0}
{}

char Lexer::current() const {
    if (pos_ >= source_.size()) return '\0';
    return source_[pos_];
}

char Lexer::advance() {
    char c = current();
    ++pos_;
    if (c == '\n') {
        ++line_;
        column_ = 1;
    } else {
        ++column_;
    }
    return c;
}

bool Lexer::match(char expected) {
    if (pos_ < source_.size() && source_[pos_] == expected) {
        advance();
        return true;
    }
    return false;
}

bool Lexer::atEnd() const {
    if (hasPeeked_) return peeked_.kind == TokenKind::EndOfFile;
    return pos_ >= source_.size();
}

Token Lexer::makeToken(TokenKind kind, std::string text) {
    return Token{kind, std::move(text), line_, column_};
}

void Lexer::skipWhitespaceAndComments() {
    while (pos_ < source_.size()) {
        char c = current();
        if (c == ' ' || c == '\t' || c == '\r' || c == '\n') {
            advance();
        } else if (c == '/' && pos_ + 1 < source_.size()) {
            if (source_[pos_ + 1] == '/') {
                // Line comment
                while (pos_ < source_.size() && current() != '\n') {
                    advance();
                }
            } else if (source_[pos_ + 1] == '*') {
                // Block comment
                advance(); advance(); // consume /*
                while (pos_ + 1 < source_.size()) {
                    if (current() == '*' && source_[pos_ + 1] == '/') {
                        advance(); advance(); // consume */
                        break;
                    }
                    advance();
                }
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

Token Lexer::lexIdentifierOrKeyword() {
    int startLine = line_;
    int startCol = column_;
    std::string text;
    while (pos_ < source_.size()) {
        char c = current();
        if (std::isalnum(static_cast<unsigned char>(c)) || c == '_') {
            text += advance();
        } else {
            break;
        }
    }
    auto it = kKeywords.find(text);
    TokenKind kind = (it != kKeywords.end()) ? it->second : TokenKind::Identifier;
    return Token{kind, text, startLine, startCol};
}

Token Lexer::lexNumber() {
    int startLine = line_;
    int startCol = column_;
    std::string text;
    bool isFloat = false;
    while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(current()))) {
        text += advance();
    }
    if (pos_ < source_.size() && current() == '.') {
        isFloat = true;
        text += advance();
        while (pos_ < source_.size() && std::isdigit(static_cast<unsigned char>(current()))) {
            text += advance();
        }
    }
    TokenKind kind = isFloat ? TokenKind::FloatLiteral : TokenKind::IntLiteral;
    return Token{kind, text, startLine, startCol};
}

Token Lexer::lexString() {
    int startLine = line_;
    int startCol = column_;
    std::string text;
    text += advance(); // consume opening "
    while (pos_ < source_.size() && current() != '"') {
        if (current() == '\\') {
            text += advance();
        }
        text += advance();
    }
    if (pos_ < source_.size()) {
        text += advance(); // consume closing "
    }
    return Token{TokenKind::StringLiteral, text, startLine, startCol};
}

Token Lexer::next() {
    if (hasPeeked_) {
        hasPeeked_ = false;
        return peeked_;
    }

    skipWhitespaceAndComments();

    if (pos_ >= source_.size()) {
        return Token{TokenKind::EndOfFile, "", line_, column_};
    }

    int startLine = line_;
    int startCol = column_;
    char c = current();

    if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
        return lexIdentifierOrKeyword();
    }
    if (std::isdigit(static_cast<unsigned char>(c))) {
        return lexNumber();
    }
    if (c == '"') {
        return lexString();
    }

    advance();
    switch (c) {
        case '(': return Token{TokenKind::LParen,    "(", startLine, startCol};
        case ')': return Token{TokenKind::RParen,    ")", startLine, startCol};
        case '{': return Token{TokenKind::LBrace,    "{", startLine, startCol};
        case '}': return Token{TokenKind::RBrace,    "}", startLine, startCol};
        case '[': return Token{TokenKind::LBracket,  "[", startLine, startCol};
        case ']': return Token{TokenKind::RBracket,  "]", startLine, startCol};
        case ';': return Token{TokenKind::Semicolon, ";", startLine, startCol};
        case ',': return Token{TokenKind::Comma,     ",", startLine, startCol};
        case '.': return Token{TokenKind::Dot,       ".", startLine, startCol};
        case '~': return Token{TokenKind::Tilde,     "~", startLine, startCol};
        case '^': return Token{TokenKind::Caret,     "^", startLine, startCol};
        case '%': return Token{TokenKind::Percent,   "%", startLine, startCol};
        case '+':
            if (match('+')) return Token{TokenKind::PlusPlus,  "++", startLine, startCol};
            return Token{TokenKind::Plus, "+", startLine, startCol};
        case '-':
            if (match('-')) return Token{TokenKind::MinusMinus, "--", startLine, startCol};
            if (match('>')) return Token{TokenKind::Arrow,      "->", startLine, startCol};
            return Token{TokenKind::Minus, "-", startLine, startCol};
        case '*': return Token{TokenKind::Star,  "*", startLine, startCol};
        case '/': return Token{TokenKind::Slash, "/", startLine, startCol};
        case '&':
            if (match('&')) return Token{TokenKind::AmpAmp,    "&&", startLine, startCol};
            return Token{TokenKind::Ampersand, "&", startLine, startCol};
        case '|':
            if (match('|')) return Token{TokenKind::PipePipe, "||", startLine, startCol};
            return Token{TokenKind::Pipe, "|", startLine, startCol};
        case '!':
            if (match('=')) return Token{TokenKind::BangEqual, "!=", startLine, startCol};
            return Token{TokenKind::Bang, "!", startLine, startCol};
        case '=':
            if (match('=')) return Token{TokenKind::EqualEqual, "==", startLine, startCol};
            return Token{TokenKind::Equal, "=", startLine, startCol};
        case '<':
            if (match('=')) return Token{TokenKind::LessEqual, "<=", startLine, startCol};
            return Token{TokenKind::Less, "<", startLine, startCol};
        case '>':
            if (match('=')) return Token{TokenKind::GreaterEqual, ">=", startLine, startCol};
            return Token{TokenKind::Greater, ">", startLine, startCol};
        default:
            return Token{TokenKind::Unknown, std::string(1, c), startLine, startCol};
    }
}

Token Lexer::peek() {
    if (!hasPeeked_) {
        peeked_ = next();
        hasPeeked_ = true;
    }
    return peeked_;
}

} // namespace eval::parser
