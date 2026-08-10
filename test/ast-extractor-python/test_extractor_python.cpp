#include "test_extractor_python.h"
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

bool testClasses()
{
    std::cout << "  testPython_Classes... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-python/samples/classes.py",
        {
            {"Animal",           SymbolKind::Class},
            {"Animal.species",   SymbolKind::Field},
            {"Animal.__init__",  SymbolKind::Constructor},
            {"Animal.speak",     SymbolKind::Method},
            {"Animal.move",      SymbolKind::Method},
            {"Dog",              SymbolKind::Class},
            {"Dog.speak",        SymbolKind::Method},
            {"Dog.fetch",        SymbolKind::Method},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testFunctions()
{
    std::cout << "  testPython_Functions... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-python/samples/functions.py",
        {
            {"add",     SymbolKind::Function},
            {"greet",   SymbolKind::Function},
            {"_helper", SymbolKind::Function},
            {"compute", SymbolKind::Function},
        });

    // local variable (assignment) inside compute must NOT be extracted
    AST tree = parse("test/ast-extractor-python/samples/functions.py");
    if(tree) {
        auto syms = extract_symbols(tree);
        ok &= test::check(!test::hasFQN(syms, "compute.total", SymbolKind::Variable),
                          "local assignment 'total' inside compute is suppressed");
    }
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testVariables()
{
    std::cout << "  testPython_Variables... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-python/samples/variables.py",
        {
            {"MODULE_CONSTANT", SymbolKind::Variable},
            {"debug_mode",      SymbolKind::Variable},
            {"_internal",       SymbolKind::Variable},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testNestedClasses()
{
    std::cout << "  testPython_NestedClasses... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-python/samples/nested.py",
        {
            {"Outer",              SymbolKind::Class},
            {"Outer.x",           SymbolKind::Field},
            {"Outer.Inner",       SymbolKind::Class},
            {"Outer.Inner.__init__", SymbolKind::Constructor},
            {"Outer.Inner.method",  SymbolKind::Method},
            {"Outer.outer_method",  SymbolKind::Method},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testEdgeCases()
{
    std::cout << "  testPython_EdgeCases... ";
    AST tree = parse("test/ast-extractor-python/samples/edge_cases.py");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "Empty", SymbolKind::Class),
                      "Empty class extracted");
    ok &= test::check(test::hasFQN(syms, "outer", SymbolKind::Function),
                      "outer function extracted");
    // Python extractor extracts nested function definitions (no function-scope guard)
    ok &= test::check(test::hasFQN(syms, "outer.inner", SymbolKind::Function),
                      "nested inner function IS extracted");
    // local variable (assignment) inside function IS suppressed
    ok &= test::check(!test::hasFQN(syms, "outer.local", SymbolKind::Variable),
                      "local variable assignment suppressed");
    ok &= test::check(test::hasFQN(syms, "WithStaticMethod", SymbolKind::Class),
                      "WithStaticMethod class");
    ok &= test::check(test::hasFQN(syms, "WithStaticMethod.static_method", SymbolKind::Method),
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
