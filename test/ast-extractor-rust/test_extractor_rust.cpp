#include "test_extractor_rust.h"
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

bool testStructs()
{
    std::cout << "  testRust_Structs... ";
    AST tree = parse("test/ast-extractor-rust/samples/structs.rs");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "Point",   SymbolKind::Struct), "Point struct");
    ok &= test::check(test::hasFQN(syms, "Point::x",SymbolKind::Field),  "Point::x field");
    ok &= test::check(test::hasFQN(syms, "Point::y",SymbolKind::Field),  "Point::y field");
    ok &= test::check(test::hasFQN(syms, "Private", SymbolKind::Struct), "Private struct");
    ok &= test::check(test::hasFQN(syms, "Private::data",  SymbolKind::Field), "Private::data");
    ok &= test::check(test::hasFQN(syms, "Private::count", SymbolKind::Field), "Private::count");

    const auto* p = test::findSymbol(syms, "Point", SymbolKind::Struct);
    ok &= test::check(p && p->access == Access::Public, "Point access=Public");
    const auto* pr = test::findSymbol(syms, "Private", SymbolKind::Struct);
    ok &= test::check(pr && pr->access == Access::Unknown, "Private access=Unknown");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testEnums()
{
    std::cout << "  testRust_Enums... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-rust/samples/enums.rs",
        {
            {"Direction",       SymbolKind::Enum},
            {"Direction::North",SymbolKind::EnumValue},
            {"Direction::South",SymbolKind::EnumValue},
            {"Direction::East", SymbolKind::EnumValue},
            {"Direction::West", SymbolKind::EnumValue},
            {"Status",          SymbolKind::Enum},
            {"Status::Ok",      SymbolKind::EnumValue},
            {"Status::Error",   SymbolKind::EnumValue},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testTraits()
{
    std::cout << "  testRust_Traits... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-rust/samples/traits.rs",
        {
            {"Drawable",        SymbolKind::Class},
            {"Drawable::draw",  SymbolKind::Method},
            {"Drawable::bounds",SymbolKind::Method},
            {"Private",         SymbolKind::Class},
            {"Private::setup",  SymbolKind::Method},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testImpls()
{
    std::cout << "  testRust_Impls... ";
    AST tree = parse("test/ast-extractor-rust/samples/impls.rs");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "Counter",           SymbolKind::Struct),      "Counter struct");
    ok &= test::check(test::hasFQN(syms, "Counter::value",    SymbolKind::Field),       "Counter::value field");
    ok &= test::check(test::hasFQN(syms, "Counter::new",      SymbolKind::Constructor), "Counter::new constructor");
    ok &= test::check(test::hasFQN(syms, "Counter::increment",SymbolKind::Method),      "Counter::increment method");
    ok &= test::check(test::hasFQN(syms, "Counter::get",      SymbolKind::Method),      "Counter::get method");
    ok &= test::check(test::hasFQN(syms, "Counter::reset",    SymbolKind::Method),      "Counter::reset method");

    // new() has no self param → Constructor; increment/get/reset have self → Method
    const auto* newSym = test::findSymbol(syms, "Counter::new", SymbolKind::Constructor);
    ok &= test::check(newSym != nullptr, "Counter::new is Constructor");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testModules()
{
    std::cout << "  testRust_Modules... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-rust/samples/modules.rs",
        {
            {"geometry",               SymbolKind::Namespace},
            {"geometry::Circle",       SymbolKind::Struct},
            {"geometry::Circle::radius",SymbolKind::Field},
            {"geometry::area",         SymbolKind::Function},
            {"math",                   SymbolKind::Namespace},
            {"math::ops",              SymbolKind::Namespace},
            {"math::ops::multiply",    SymbolKind::Function},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testFunctions()
{
    std::cout << "  testRust_Functions... ";
    AST tree = parse("test/ast-extractor-rust/samples/functions.rs");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "add",          SymbolKind::Function), "add function");
    ok &= test::check(test::hasFQN(syms, "private_func", SymbolKind::Function), "private_func function");
    ok &= test::check(test::hasFQN(syms, "MAX_VALUE",    SymbolKind::Variable), "MAX_VALUE const");
    ok &= test::check(test::hasFQN(syms, "COUNTER",      SymbolKind::Variable), "COUNTER static");
    ok &= test::check(test::hasFQN(syms, "Score",        SymbolKind::Typedef),  "Score type alias");

    const auto* maxVal = test::findSymbol(syms, "MAX_VALUE", SymbolKind::Variable);
    ok &= test::check(maxVal && maxVal->isConstexpr, "MAX_VALUE isConstexpr");

    const auto* cnt = test::findSymbol(syms, "COUNTER", SymbolKind::Variable);
    ok &= test::check(cnt && cnt->isStatic, "COUNTER isStatic");

    const auto* addSym = test::findSymbol(syms, "add", SymbolKind::Function);
    ok &= test::check(addSym && addSym->access == Access::Public, "add access=Public");

    const auto* priv = test::findSymbol(syms, "private_func", SymbolKind::Function);
    ok &= test::check(priv && priv->access == Access::Unknown, "private_func access=Unknown");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

} // anonymous namespace

bool run_tests_rust()
{
    std::cout << "=== Rust extractor tests ===" << std::endl;
    bool ok = true;
    ok &= testStructs();
    ok &= testEnums();
    ok &= testTraits();
    ok &= testImpls();
    ok &= testModules();
    ok &= testFunctions();
    std::cout << "=== Rust: " << (ok ? "PASS" : "FAIL") << " ===" << std::endl;
    return ok;
}

} // namespace ast
