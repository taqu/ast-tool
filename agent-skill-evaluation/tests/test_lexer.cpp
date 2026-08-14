#include <catch2/catch_test_macros.hpp>
#include "parser/lexer.hpp"
#include <vector>

using namespace eval::parser;

static std::vector<Token> tokenizeAll(const std::string& src) {
    Lexer lex(src, "<test>");
    std::vector<Token> tokens;
    while (true) {
        Token t = lex.next();
        tokens.push_back(t);
        if (t.kind == TokenKind::EndOfFile) break;
    }
    return tokens;
}

TEST_CASE("Lexer: empty input produces only EOF", "[lexer]") {
    auto tokens = tokenizeAll("");
    REQUIRE(tokens.size() == 1);
    CHECK(tokens[0].kind == TokenKind::EndOfFile);
}

TEST_CASE("Lexer: identifiers are tokenized correctly", "[lexer]") {
    auto tokens = tokenizeAll("foo bar _baz");
    REQUIRE(tokens.size() == 4); // 3 identifiers + EOF
    CHECK(tokens[0].kind == TokenKind::Identifier);
    CHECK(tokens[0].text == "foo");
    CHECK(tokens[1].kind == TokenKind::Identifier);
    CHECK(tokens[1].text == "bar");
    CHECK(tokens[2].kind == TokenKind::Identifier);
    CHECK(tokens[2].text == "_baz");
}

TEST_CASE("Lexer: keywords are distinguished from identifiers", "[lexer]") {
    auto tokens = tokenizeAll("int float void if while for return const");
    REQUIRE(tokens.size() == 9); // 8 keywords + EOF
    CHECK(tokens[0].kind == TokenKind::KwInt);
    CHECK(tokens[1].kind == TokenKind::KwFloat);
    CHECK(tokens[2].kind == TokenKind::KwVoid);
    CHECK(tokens[3].kind == TokenKind::KwIf);
    CHECK(tokens[4].kind == TokenKind::KwWhile);
    CHECK(tokens[5].kind == TokenKind::KwFor);
    CHECK(tokens[6].kind == TokenKind::KwReturn);
    CHECK(tokens[7].kind == TokenKind::KwConst);
}

TEST_CASE("Lexer: integer literals", "[lexer]") {
    auto tokens = tokenizeAll("0 42 1234");
    REQUIRE(tokens.size() == 4); // 3 literals + EOF
    CHECK(tokens[0].kind == TokenKind::IntLiteral);
    CHECK(tokens[0].text == "0");
    CHECK(tokens[1].kind == TokenKind::IntLiteral);
    CHECK(tokens[1].text == "42");
    CHECK(tokens[2].kind == TokenKind::IntLiteral);
    CHECK(tokens[2].text == "1234");
}

TEST_CASE("Lexer: operators single and double char", "[lexer]") {
    auto tokens = tokenizeAll("+ - == != <= >= && ||");
    // 8 operators + EOF
    REQUIRE(tokens.size() == 9);
    CHECK(tokens[0].kind == TokenKind::Plus);
    CHECK(tokens[1].kind == TokenKind::Minus);
    CHECK(tokens[2].kind == TokenKind::EqualEqual);
    CHECK(tokens[3].kind == TokenKind::BangEqual);
    CHECK(tokens[4].kind == TokenKind::LessEqual);
    CHECK(tokens[5].kind == TokenKind::GreaterEqual);
    CHECK(tokens[6].kind == TokenKind::AmpAmp);
    CHECK(tokens[7].kind == TokenKind::PipePipe);
}

TEST_CASE("Lexer: tokenKindName returns expected strings", "[lexer]") {
    CHECK(tokenKindName(TokenKind::IntLiteral) == "IntLiteral");
    CHECK(tokenKindName(TokenKind::Identifier) == "Identifier");
    CHECK(tokenKindName(TokenKind::KwReturn)   == "kw_return");
    CHECK(tokenKindName(TokenKind::EndOfFile)  == "EOF");
    CHECK(tokenKindName(TokenKind::EqualEqual) == "==");
}

TEST_CASE("Lexer: line comments are skipped", "[lexer]") {
    auto tokens = tokenizeAll("a // this is a comment\nb");
    REQUIRE(tokens.size() == 3); // a, b, EOF
    CHECK(tokens[0].text == "a");
    CHECK(tokens[1].text == "b");
}

TEST_CASE("Lexer: float literals", "[lexer]") {
    auto tokens = tokenizeAll("3.14 0.5");
    REQUIRE(tokens.size() == 3);
    CHECK(tokens[0].kind == TokenKind::FloatLiteral);
    CHECK(tokens[0].text == "3.14");
    CHECK(tokens[1].kind == TokenKind::FloatLiteral);
    CHECK(tokens[1].text == "0.5");
}
