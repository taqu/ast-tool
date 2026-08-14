#include "test_extractor_python.h"
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

bool testClasses()
{
    std::cout << "  testPython_Classes... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-python/samples/classes.py",
        {
            {u8"Animal",           SymbolKind::Class},
            {u8"Animal.species",   SymbolKind::Field},
            {u8"Animal.__init__",  SymbolKind::Constructor},
            {u8"Animal.speak",     SymbolKind::Method},
            {u8"Animal.move",      SymbolKind::Method},
            {u8"Dog",              SymbolKind::Class},
            {u8"Dog.speak",        SymbolKind::Method},
            {u8"Dog.fetch",        SymbolKind::Method},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testFunctions()
{
    std::cout << "  testPython_Functions... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-python/samples/functions.py",
        {
            {u8"add",     SymbolKind::Function},
            {u8"greet",   SymbolKind::Function},
            {u8"_helper", SymbolKind::Function},
            {u8"compute", SymbolKind::Function},
        });

    // local variable (assignment) inside compute must NOT be extracted
    AST tree = parse(u8"test/ast-extractor-python/samples/functions.py");
    if(tree) {
        auto syms = extract_symbols(tree);
        ok &= test::check(!test::hasFQN(syms, u8"compute.total", SymbolKind::Variable),
                          "local assignment 'total' inside compute is suppressed");
    }
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testVariables()
{
    std::cout << "  testPython_Variables... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-python/samples/variables.py",
        {
            {u8"MODULE_CONSTANT", SymbolKind::Variable},
            {u8"debug_mode",      SymbolKind::Variable},
            {u8"_internal",       SymbolKind::Variable},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testNestedClasses()
{
    std::cout << "  testPython_NestedClasses... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-python/samples/nested.py",
        {
            {u8"Outer",              SymbolKind::Class},
            {u8"Outer.x",           SymbolKind::Field},
            {u8"Outer.Inner",       SymbolKind::Class},
            {u8"Outer.Inner.__init__", SymbolKind::Constructor},
            {u8"Outer.Inner.method",  SymbolKind::Method},
            {u8"Outer.outer_method",  SymbolKind::Method},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testEdgeCases()
{
    std::cout << "  testPython_EdgeCases... ";
    AST tree = parse(u8"test/ast-extractor-python/samples/edge_cases.py");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, u8"Empty", SymbolKind::Class),
                      "Empty class extracted");
    ok &= test::check(test::hasFQN(syms, u8"outer", SymbolKind::Function),
                      "outer function extracted");
    // Python extractor extracts nested function definitions (no function-scope guard)
    ok &= test::check(test::hasFQN(syms, u8"outer.inner", SymbolKind::Function),
                      "nested inner function IS extracted");
    // local variable (assignment) inside function IS suppressed
    ok &= test::check(!test::hasFQN(syms, u8"outer.local", SymbolKind::Variable),
                      "local variable assignment suppressed");
    ok &= test::check(test::hasFQN(syms, u8"WithStaticMethod", SymbolKind::Class),
                      "WithStaticMethod class");
    ok &= test::check(test::hasFQN(syms, u8"WithStaticMethod.static_method", SymbolKind::Method),
                      "WithStaticMethod.static_method");
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

} // anonymous namespace

bool run_tests_python()
{
    std::cout << "=== Python extractor tests ===" << std::endl;
    bool ok = true;
    ok &= testClasses();
    ok &= testFunctions();
    ok &= testVariables();
    ok &= testNestedClasses();
    ok &= testEdgeCases();
    std::cout << "=== Python: " << (ok ? "PASS" : "FAIL") << " ===" << std::endl;
    return ok;
}

} // namespace ast
