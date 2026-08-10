#include "test_extractor_cpp.h"
#include "../test_helpers.h"
#include "ast-ir.h"
#include "ast-extractor.h"
#include <iostream>
#include <vector>

namespace ast {
namespace {

bool parseAndCheck(const char* path, std::vector<std::pair<const char*, SymbolKind>> expected)
{
    AST tree = parse(path);
    if(!tree) {
        std::cerr << "    FAIL: could not parse " << path << "\n";
        return false;
    }
    auto syms = extract_symbols(tree);
    bool ok = true;
    for(const auto& [fqn, kind] : expected)
        ok &= test::check(test::hasFQN(syms, fqn, kind), fqn);
    return ok;
}

bool testNamespaces()
{
    std::cout << "  testCpp_Namespaces... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-cpp/samples/namespaces.cpp",
        {
            {"outer",             SymbolKind::Namespace},
            {"outer::x",          SymbolKind::Variable},
            {"outer::inner",      SymbolKind::Namespace},
            {"outer::inner::y",   SymbolKind::Variable},
            // anonymous namespace: members use enclosing prefix (none here)
            {"anon_var",          SymbolKind::Variable},
            {"anon_func",         SymbolKind::Function},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testClasses()
{
    std::cout << "  testCpp_Classes... ";
    AST tree = parse("test/ast-extractor-cpp/samples/classes.cpp");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "Widget",          SymbolKind::Class),       "Widget class");
    ok &= test::check(test::hasFQN(syms, "Widget::Widget",  SymbolKind::Constructor), "Widget constructor");
    ok &= test::check(test::hasFQN(syms, "Widget::~Widget", SymbolKind::Destructor),  "Widget destructor");
    ok &= test::check(test::hasFQN(syms, "Widget::update",  SymbolKind::Method),      "Widget::update method");
    ok &= test::check(test::hasFQN(syms, "Widget::create",  SymbolKind::Method),      "Widget::create method");
    ok &= test::check(test::hasFQN(syms, "Widget::reset",   SymbolKind::Method),      "Widget::reset method");
    ok &= test::check(test::hasFQN(syms, "Widget::value_",  SymbolKind::Field),       "Widget::value_ field");

    const auto* create = test::findSymbol(syms, "Widget::create", SymbolKind::Method);
    ok &= test::check(create && create->isStatic, "Widget::create isStatic");

    const auto* reset = test::findSymbol(syms, "Widget::reset", SymbolKind::Method);
    ok &= test::check(reset && reset->isInline, "Widget::reset isInline");

    const auto* value = test::findSymbol(syms, "Widget::value_", SymbolKind::Field);
    ok &= test::check(value && value->access == Access::Private, "Widget::value_ access=Private");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testEnums()
{
    std::cout << "  testCpp_Enums... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-cpp/samples/enums.cpp",
        {
            {"Color",       SymbolKind::Enum},
            {"Color::Red",  SymbolKind::EnumValue},
            {"Color::Green",SymbolKind::EnumValue},
            {"Color::Blue", SymbolKind::EnumValue},
            {"Plain",       SymbolKind::Enum},
            {"Plain::AlphaA", SymbolKind::EnumValue},
            {"Plain::AlphaB", SymbolKind::EnumValue},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testFunctions()
{
    std::cout << "  testCpp_Functions... ";
    AST tree = parse("test/ast-extractor-cpp/samples/functions.cpp");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "add",     SymbolKind::Function), "add function");
    ok &= test::check(test::hasFQN(syms, "s_count", SymbolKind::Variable), "s_count variable");
    ok &= test::check(test::hasFQN(syms, "LIMIT",   SymbolKind::Variable), "LIMIT variable");
    ok &= test::check(test::hasFQN(syms, "log_msg", SymbolKind::Function), "log_msg function");

    const auto* sc = test::findSymbol(syms, "s_count", SymbolKind::Variable);
    ok &= test::check(sc && sc->isStatic, "s_count isStatic");

    const auto* lim = test::findSymbol(syms, "LIMIT", SymbolKind::Variable);
    ok &= test::check(lim && lim->isConstexpr, "LIMIT isConstexpr");

    const auto* lm = test::findSymbol(syms, "log_msg", SymbolKind::Function);
    ok &= test::check(lm && lm->isInline, "log_msg isInline");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testOperators()
{
    std::cout << "  testCpp_Operators... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-cpp/samples/operators.cpp",
        {
            {"Vector",              SymbolKind::Struct},
            {"Vector::x",           SymbolKind::Field},
            {"Vector::y",           SymbolKind::Field},
            {"Vector::operator+",   SymbolKind::Method},
            {"Vector::operator+=",  SymbolKind::Method},
            {"Vector::operator bool", SymbolKind::Method},
            {"operator-",           SymbolKind::Function},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testUsingAlias()
{
    std::cout << "  testCpp_UsingAlias... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-cpp/samples/using_alias.cpp",
        {
            {"MyInt",      SymbolKind::UsingAlias},
            {"StringType", SymbolKind::UsingAlias},
            {"ns",         SymbolKind::Namespace},
            {"ns::Value",  SymbolKind::UsingAlias},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testNestedScopes()
{
    std::cout << "  testCpp_NestedScopes... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-cpp/samples/nested.cpp",
        {
            {"A",                  SymbolKind::Namespace},
            {"A::B",               SymbolKind::Namespace},
            {"A::B::Inner",        SymbolKind::Struct},
            {"A::B::Inner::data",  SymbolKind::Field},
            {"A::B::Outer",        SymbolKind::Class},
            {"A::B::Outer::Nested",SymbolKind::Class},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testAnonymousNamespace()
{
    std::cout << "  testCpp_AnonymousNamespace... ";
    AST tree = parse("test/ast-extractor-cpp/samples/anonymous_ns.cpp");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "outer",          SymbolKind::Namespace), "outer namespace");
    // Anonymous namespace members get the outer prefix (empty anon frame is skipped)
    ok &= test::check(test::hasFQN(syms, "outer::Hidden",  SymbolKind::Class),     "outer::Hidden (inside anon ns)");
    ok &= test::check(test::hasFQN(syms, "outer::secret",  SymbolKind::Variable),  "outer::secret (inside anon ns)");
    ok &= test::check(test::hasFQN(syms, "outer::Visible", SymbolKind::Class),     "outer::Visible");
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testTemplates()
{
    std::cout << "  testCpp_Templates... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-cpp/samples/templates.cpp",
        {
            {"Pair",        SymbolKind::Struct},
            {"Pair::first", SymbolKind::Field},
            {"Pair::second",SymbolKind::Field},
            {"max_val",     SymbolKind::Function},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testForwardDeclarations()
{
    std::cout << "  testCpp_ForwardDeclarations... ";
    AST tree = parse("test/ast-extractor-cpp/samples/forward_decls.cpp");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "Widget", SymbolKind::Class),  "Widget forward decl");
    ok &= test::check(test::hasFQN(syms, "Point",  SymbolKind::Struct), "Point forward decl");
    ok &= test::check(test::hasFQN(syms, "process",SymbolKind::Function), "process function");
    // process must appear exactly once (forward decl + definition deduplicated)
    size_t cnt = 0;
    for(const auto& s : syms)
        if(s.fqn == "process" && s.kind == SymbolKind::Function) ++cnt;
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
