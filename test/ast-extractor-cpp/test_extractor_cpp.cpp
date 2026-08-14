#include "test_extractor_cpp.h"
#include "../test_helpers.h"
#include "ast-ir.h"
#include "ast-extractor.h"
#include <iostream>
#include <vector>

namespace ast {
namespace {

bool parseAndCheck(const char8_t* path, std::vector<std::pair<const char8_t*, SymbolKind>> expected)
{
    AST tree = parse(path);
    if(!tree) {
        std::cerr << "    FAIL: could not parse " << (const char*)path << "\n";
        return false;
    }
    auto syms = extract_symbols(tree);
    bool ok = true;
    for(const auto& [fqn, kind] : expected)
        ok &= test::check(test::hasFQN(syms, fqn, kind), (const char*)fqn);
    return ok;
}

bool testNamespaces()
{
    std::cout << "  testCpp_Namespaces... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-cpp/samples/namespaces.cpp",
        {
            {u8"outer",             SymbolKind::Namespace},
            {u8"outer::x",          SymbolKind::Variable},
            {u8"outer::inner",      SymbolKind::Namespace},
            {u8"outer::inner::y",   SymbolKind::Variable},
            // anonymous namespace: members use enclosing prefix (none here)
            {u8"anon_var",          SymbolKind::Variable},
            {u8"anon_func",         SymbolKind::Function},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testClasses()
{
    std::cout << "  testCpp_Classes... ";
    AST tree = parse(u8"test/ast-extractor-cpp/samples/classes.cpp");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, u8"Widget",          SymbolKind::Class),       "Widget class");
    ok &= test::check(test::hasFQN(syms, u8"Widget::Widget",  SymbolKind::Constructor), "Widget constructor");
    ok &= test::check(test::hasFQN(syms, u8"Widget::~Widget", SymbolKind::Destructor),  "Widget destructor");
    ok &= test::check(test::hasFQN(syms, u8"Widget::update",  SymbolKind::Method),      "Widget::update method");
    ok &= test::check(test::hasFQN(syms, u8"Widget::create",  SymbolKind::Method),      "Widget::create method");
    ok &= test::check(test::hasFQN(syms, u8"Widget::reset",   SymbolKind::Method),      "Widget::reset method");
    ok &= test::check(test::hasFQN(syms, u8"Widget::value_",  SymbolKind::Field),       "Widget::value_ field");

    const auto* create = test::findSymbol(syms, u8"Widget::create", SymbolKind::Method);
    ok &= test::check(create && create->isStatic, "Widget::create isStatic");

    const auto* reset = test::findSymbol(syms, u8"Widget::reset", SymbolKind::Method);
    ok &= test::check(reset && reset->isInline, "Widget::reset isInline");

    const auto* value = test::findSymbol(syms, u8"Widget::value_", SymbolKind::Field);
    ok &= test::check(value && value->access == Access::Private, "Widget::value_ access=Private");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testEnums()
{
    std::cout << "  testCpp_Enums... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-cpp/samples/enums.cpp",
        {
            {u8"Color",       SymbolKind::Enum},
            {u8"Color::Red",  SymbolKind::EnumValue},
            {u8"Color::Green",SymbolKind::EnumValue},
            {u8"Color::Blue", SymbolKind::EnumValue},
            {u8"Plain",       SymbolKind::Enum},
            {u8"Plain::AlphaA", SymbolKind::EnumValue},
            {u8"Plain::AlphaB", SymbolKind::EnumValue},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testFunctions()
{
    std::cout << "  testCpp_Functions... ";
    AST tree = parse(u8"test/ast-extractor-cpp/samples/functions.cpp");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, u8"add",     SymbolKind::Function), "add function");
    ok &= test::check(test::hasFQN(syms, u8"s_count", SymbolKind::Variable), "s_count variable");
    ok &= test::check(test::hasFQN(syms, u8"LIMIT",   SymbolKind::Variable), "LIMIT variable");
    ok &= test::check(test::hasFQN(syms, u8"log_msg", SymbolKind::Function), "log_msg function");

    const auto* sc = test::findSymbol(syms, u8"s_count", SymbolKind::Variable);
    ok &= test::check(sc && sc->isStatic, "s_count isStatic");

    const auto* lim = test::findSymbol(syms, u8"LIMIT", SymbolKind::Variable);
    ok &= test::check(lim && lim->isConstexpr, "LIMIT isConstexpr");

    const auto* lm = test::findSymbol(syms, u8"log_msg", SymbolKind::Function);
    ok &= test::check(lm && lm->isInline, "log_msg isInline");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testOperators()
{
    std::cout << "  testCpp_Operators... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-cpp/samples/operators.cpp",
        {
            {u8"Vector",              SymbolKind::Struct},
            {u8"Vector::x",           SymbolKind::Field},
            {u8"Vector::y",           SymbolKind::Field},
            {u8"Vector::operator+",   SymbolKind::Method},
            {u8"Vector::operator+=",  SymbolKind::Method},
            {u8"Vector::operator bool", SymbolKind::Method},
            {u8"operator-",           SymbolKind::Function},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testUsingAlias()
{
    std::cout << "  testCpp_UsingAlias... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-cpp/samples/using_alias.cpp",
        {
            {u8"MyInt",      SymbolKind::UsingAlias},
            {u8"StringType", SymbolKind::UsingAlias},
            {u8"ns",         SymbolKind::Namespace},
            {u8"ns::Value",  SymbolKind::UsingAlias},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testNestedScopes()
{
    std::cout << "  testCpp_NestedScopes... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-cpp/samples/nested.cpp",
        {
            {u8"A",                  SymbolKind::Namespace},
            {u8"A::B",               SymbolKind::Namespace},
            {u8"A::B::Inner",        SymbolKind::Struct},
            {u8"A::B::Inner::data",  SymbolKind::Field},
            {u8"A::B::Outer",        SymbolKind::Class},
            {u8"A::B::Outer::Nested",SymbolKind::Class},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testAnonymousNamespace()
{
    std::cout << "  testCpp_AnonymousNamespace... ";
    AST tree = parse(u8"test/ast-extractor-cpp/samples/anonymous_ns.cpp");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, u8"outer",          SymbolKind::Namespace), "outer namespace");
    // Anonymous namespace members get the outer prefix (empty anon frame is skipped)
    ok &= test::check(test::hasFQN(syms, u8"outer::Hidden",  SymbolKind::Class),     "outer::Hidden (inside anon ns)");
    ok &= test::check(test::hasFQN(syms, u8"outer::secret",  SymbolKind::Variable),  "outer::secret (inside anon ns)");
    ok &= test::check(test::hasFQN(syms, u8"outer::Visible", SymbolKind::Class),     "outer::Visible");
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testTemplates()
{
    std::cout << "  testCpp_Templates... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-cpp/samples/templates.cpp",
        {
            {u8"Pair",        SymbolKind::Struct},
            {u8"Pair::first", SymbolKind::Field},
            {u8"Pair::second",SymbolKind::Field},
            {u8"max_val",     SymbolKind::Function},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testForwardDeclarations()
{
    std::cout << "  testCpp_ForwardDeclarations... ";
    AST tree = parse(u8"test/ast-extractor-cpp/samples/forward_decls.cpp");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, u8"Widget", SymbolKind::Class),  "Widget forward decl");
    ok &= test::check(test::hasFQN(syms, u8"Point",  SymbolKind::Struct), "Point forward decl");
    ok &= test::check(test::hasFQN(syms, u8"process",SymbolKind::Function), "process function");
    // process must appear exactly once (forward decl + definition deduplicated)
    size_t cnt = 0;
    for(const auto& s : syms)
        if(s.fqn == u8"process" && s.kind == SymbolKind::Function) ++cnt;
    ok &= test::check(cnt == 1, "process deduplicated to 1 entry");
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

} // anonymous namespace

bool run_tests_cpp()
{
    std::cout << "=== C++ extractor tests ===" << std::endl;
    bool ok = true;
    ok &= testNamespaces();
    ok &= testClasses();
    ok &= testEnums();
    ok &= testFunctions();
    ok &= testOperators();
    ok &= testUsingAlias();
    ok &= testNestedScopes();
    ok &= testAnonymousNamespace();
    ok &= testTemplates();
    ok &= testForwardDeclarations();
    std::cout << "=== C++: " << (ok ? "PASS" : "FAIL") << " ===" << std::endl;
    return ok;
}

} // namespace ast
