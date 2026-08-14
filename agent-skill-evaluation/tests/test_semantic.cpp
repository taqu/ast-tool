#include <catch2/catch_test_macros.hpp>
#include "semantic/symbol.hpp"
#include "semantic/analyzer.hpp"
#include "ast/node.hpp"
#include "parser/lexer.hpp"
#include "parser/parser.hpp"

using namespace eval;

static std::shared_ptr<ast::TranslationUnit> parse(const std::string& src) {
    parser::Lexer lex(src, "<test>");
    parser::Parser p(std::move(lex));
    return p.parseTranslationUnit();
}

TEST_CASE("Scope: define and lookup symbol", "[semantic]") {
    semantic::Scope scope("test");

    semantic::Symbol sym;
    sym.name = "myVar";
    sym.qualifiedName = "myVar";
    sym.kind = semantic::SymbolKind::Variable;
    sym.type = "int";
    sym.isConst = false;
    sym.definition = {"file.c", 1, 1};

    bool defined = scope.define(sym);
    CHECK(defined == true);

    semantic::Symbol* found = scope.lookup("myVar");
    REQUIRE(found != nullptr);
    CHECK(found->name == "myVar");
    CHECK(found->type == "int");
}

TEST_CASE("Scope: duplicate definition returns false", "[semantic]") {
    semantic::Scope scope("test");

    semantic::Symbol sym;
    sym.name = "x";
    sym.qualifiedName = "x";
    sym.kind = semantic::SymbolKind::Variable;
    sym.type = "int";
    sym.isConst = false;
    sym.definition = {"f.c", 1, 1};

    CHECK(scope.define(sym) == true);
    CHECK(scope.define(sym) == false);
}

TEST_CASE("Scope: lookup walks parent chain", "[semantic]") {
    semantic::Scope parent("parent");
    semantic::Scope child("child", &parent);

    semantic::Symbol sym;
    sym.name = "outerVar";
    sym.qualifiedName = "outerVar";
    sym.kind = semantic::SymbolKind::Variable;
    sym.type = "float";
    sym.isConst = false;
    sym.definition = {"f.c", 1, 1};

    parent.define(sym);

    // child can find parent's symbol
    const semantic::Symbol* found = child.lookup("outerVar");
    REQUIRE(found != nullptr);
    CHECK(found->name == "outerVar");

    // child cannot find something that doesn't exist
    CHECK(child.lookup("nope") == nullptr);
}

TEST_CASE("Scope: qualifiedName walks scopes", "[semantic]") {
    semantic::Scope global("");
    semantic::Scope ns("myNS", &global);
    semantic::Scope fn("myFunc", &ns);

    CHECK(global.qualifiedName() == "");
    CHECK(ns.qualifiedName() == "myNS");
    CHECK(fn.qualifiedName() == "myNS::myFunc");
}

TEST_CASE("Analyzer: function and variable symbols are recorded", "[semantic]") {
    auto tu = parse("int x; void foo(int a) { int y; }");

    semantic::Analyzer analyzer;
    analyzer.analyze(*tu);

    const semantic::Symbol* xSym = analyzer.globalScope()->lookup("x");
    REQUIRE(xSym != nullptr);
    CHECK(xSym->kind == semantic::SymbolKind::Variable);

    const semantic::Symbol* fooSym = analyzer.globalScope()->lookup("foo");
    REQUIRE(fooSym != nullptr);
    CHECK(fooSym->kind == semantic::SymbolKind::Function);
    CHECK(fooSym->type == "void");
}

TEST_CASE("Analyzer: undefined symbol call produces error", "[semantic]") {
    // bar is not defined before calling it
    auto tu = parse("void foo() { bar(); }");

    semantic::Analyzer analyzer;
    analyzer.analyze(*tu);

    const auto& errors = analyzer.errors();
    bool hasUndefined = false;
    for (const auto& err : errors) {
        if (err.message.find("bar") != std::string::npos) {
            hasUndefined = true;
        }
    }
    CHECK(hasUndefined == true);
}

TEST_CASE("Analyzer: calling defined function records reference", "[semantic]") {
    auto tu = parse("void helper() {} void main() { helper(); }");

    semantic::Analyzer analyzer;
    analyzer.analyze(*tu);

    // No undefined error for helper
    for (const auto& err : analyzer.errors()) {
        CHECK(err.message.find("helper") == std::string::npos);
    }

    const semantic::Symbol* helperSym = analyzer.globalScope()->lookup("helper");
    REQUIRE(helperSym != nullptr);
    // Should have one reference from main
    CHECK(helperSym->references.size() == 1);
}
