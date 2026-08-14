#include <catch2/catch_test_macros.hpp>
#include "parser/lexer.hpp"
#include "parser/parser.hpp"
#include "ast/node.hpp"
#include <memory>

using namespace eval;

static std::shared_ptr<ast::TranslationUnit> parse(const std::string& src) {
    parser::Lexer lex(src, "<test>");
    parser::Parser p(std::move(lex));
    return p.parseTranslationUnit();
}

TEST_CASE("Parser: empty source produces empty TranslationUnit", "[parser]") {
    auto tu = parse("");
    REQUIRE(tu != nullptr);
    CHECK(tu->kind() == ast::NodeKind::TranslationUnit);
    CHECK(tu->children().empty());
}

TEST_CASE("Parser: simple function declaration", "[parser]") {
    auto tu = parse("int add(int a, int b) { return a; }");
    REQUIRE(tu != nullptr);
    REQUIRE(tu->children().size() == 1);
    auto& child = tu->children()[0];
    REQUIRE(child->kind() == ast::NodeKind::FunctionDecl);
    auto* fn = static_cast<ast::FunctionDecl*>(child.get());
    CHECK(fn->name() == "add");
    CHECK(fn->returnType() == "int");
    CHECK(fn->params().size() == 2);
    CHECK(fn->params()[0].second == "a");
    CHECK(fn->params()[1].second == "b");
}

TEST_CASE("Parser: variable declaration", "[parser]") {
    auto tu = parse("int x;");
    REQUIRE(tu != nullptr);
    REQUIRE(tu->children().size() == 1);
    auto& child = tu->children()[0];
    REQUIRE(child->kind() == ast::NodeKind::VarDecl);
    auto* var = static_cast<ast::VarDecl*>(child.get());
    CHECK(var->name() == "x");
    CHECK(var->type() == "int");
    CHECK(var->isConst() == false);
}

TEST_CASE("Parser: function with if statement body", "[parser]") {
    const char* src = R"(
        void check(int n) {
            if (n) {
                return n;
            }
        }
    )";
    auto tu = parse(src);
    REQUIRE(tu != nullptr);
    REQUIRE(tu->children().size() == 1);
    auto& fn = tu->children()[0];
    CHECK(fn->kind() == ast::NodeKind::FunctionDecl);
    // function body has at least the if statement
    bool hasIf = false;
    for (const auto& c : fn->children()) {
        if (c->kind() == ast::NodeKind::IfStmt) hasIf = true;
    }
    CHECK(hasIf);
}

TEST_CASE("Parser: ParseError is thrown on syntax error", "[parser]") {
    // Missing closing paren for function params
    parser::Lexer lex("int foo(int", "<test>");
    parser::Parser p(std::move(lex));
    CHECK_THROWS_AS(p.parseFunctionDecl(), parser::ParseError);
}

TEST_CASE("Parser: nodeKindName covers all kinds", "[parser]") {
    CHECK(ast::nodeKindName(ast::NodeKind::TranslationUnit) == "TranslationUnit");
    CHECK(ast::nodeKindName(ast::NodeKind::FunctionDecl)    == "FunctionDecl");
    CHECK(ast::nodeKindName(ast::NodeKind::VarDecl)         == "VarDecl");
    CHECK(ast::nodeKindName(ast::NodeKind::IfStmt)          == "IfStmt");
    CHECK(ast::nodeKindName(ast::NodeKind::WhileStmt)       == "WhileStmt");
    CHECK(ast::nodeKindName(ast::NodeKind::ReturnStmt)      == "ReturnStmt");
    CHECK(ast::nodeKindName(ast::NodeKind::CallExpr)        == "CallExpr");
    CHECK(ast::nodeKindName(ast::NodeKind::Literal)         == "Literal");
}
