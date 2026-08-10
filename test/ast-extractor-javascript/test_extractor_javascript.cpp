#include "test_extractor_javascript.h"
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

bool testFunctions()
{
    std::cout << "  testJS_Functions... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-javascript/samples/functions.js",
        {
            {"add",      SymbolKind::Function},
            {"generate", SymbolKind::Function},
            {"multiply", SymbolKind::Function},
            {"square",   SymbolKind::Function},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testClasses()
{
    std::cout << "  testJS_Classes... ";
    AST tree = parse("test/ast-extractor-javascript/samples/classes.js");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "Animal",            SymbolKind::Class),       "Animal class");
    ok &= test::check(test::hasFQN(syms, "Animal.species",    SymbolKind::Field),       "Animal.species field");
    ok &= test::check(test::hasFQN(syms, "Animal.constructor",SymbolKind::Constructor), "Animal constructor");
    ok &= test::check(test::hasFQN(syms, "Animal.speak",      SymbolKind::Method),      "Animal.speak method");
    ok &= test::check(test::hasFQN(syms, "Animal.create",     SymbolKind::Method),      "Animal.create method");

    const auto* create = test::findSymbol(syms, "Animal.create", SymbolKind::Method);
    ok &= test::check(create && create->isStatic, "Animal.create isStatic");

    // Private field #name must be extracted
    ok &= test::check(test::hasFQN(syms, "Animal.#name", SymbolKind::Field), "Animal.#name private field");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testVariables()
{
    std::cout << "  testJS_Variables... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-javascript/samples/variables.js",
        {
            {"PI",           SymbolKind::Variable},
            {"counter",      SymbolKind::Variable},
            {"legacyGlobal", SymbolKind::Variable},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testObjectLiterals()
{
    std::cout << "  testJS_ObjectLiterals... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-javascript/samples/objects.js",
        {
            {"utils",         SymbolKind::Variable},
            {"utils.format",  SymbolKind::Function},
            {"utils.parse",   SymbolKind::Function},
            {"utils.version", SymbolKind::Variable},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

} // anonymous namespace

bool run_tests_javascript()
{
    std::cout << "=== JavaScript extractor tests ===" << std::endl;
    bool ok = true;
    ok &= testFunctions();
    ok &= testClasses();
    ok &= testVariables();
    ok &= testObjectLiterals();
    std::cout << "=== JavaScript: " << (ok ? "PASS" : "FAIL") << " ===" << std::endl;
    return ok;
}

} // namespace ast
