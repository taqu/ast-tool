#include "test_extractor_rust.h"
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

bool testStructs()
{
    std::cout << "  testRust_Structs... ";
    AST tree = parse(u8"test/ast-extractor-rust/samples/structs.rs");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, u8"Point",   SymbolKind::Struct), "Point struct");
    ok &= test::check(test::hasFQN(syms, u8"Point::x",SymbolKind::Field),  "Point::x field");
    ok &= test::check(test::hasFQN(syms, u8"Point::y",SymbolKind::Field),  "Point::y field");
    ok &= test::check(test::hasFQN(syms, u8"Private", SymbolKind::Struct), "Private struct");
    ok &= test::check(test::hasFQN(syms, u8"Private::data",  SymbolKind::Field), "Private::data");
    ok &= test::check(test::hasFQN(syms, u8"Private::count", SymbolKind::Field), "Private::count");

    const auto* p = test::findSymbol(syms, u8"Point", SymbolKind::Struct);
    ok &= test::check(p && p->access == Access::Public, "Point access=Public");
    const auto* pr = test::findSymbol(syms, u8"Private", SymbolKind::Struct);
    ok &= test::check(pr && pr->access == Access::Unknown, "Private access=Unknown");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testEnums()
{
    std::cout << "  testRust_Enums... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-rust/samples/enums.rs",
        {
            {u8"Direction",       SymbolKind::Enum},
            {u8"Direction::North",SymbolKind::EnumValue},
            {u8"Direction::South",SymbolKind::EnumValue},
            {u8"Direction::East", SymbolKind::EnumValue},
            {u8"Direction::West", SymbolKind::EnumValue},
            {u8"Status",          SymbolKind::Enum},
            {u8"Status::Ok",      SymbolKind::EnumValue},
            {u8"Status::Error",   SymbolKind::EnumValue},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testTraits()
{
    std::cout << "  testRust_Traits... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-rust/samples/traits.rs",
        {
            {u8"Drawable",        SymbolKind::Class},
            {u8"Drawable::draw",  SymbolKind::Method},
            {u8"Drawable::bounds",SymbolKind::Method},
            {u8"Private",         SymbolKind::Class},
            {u8"Private::setup",  SymbolKind::Method},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testImpls()
{
    std::cout << "  testRust_Impls... ";
    AST tree = parse(u8"test/ast-extractor-rust/samples/impls.rs");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, u8"Counter",           SymbolKind::Struct),      "Counter struct");
    ok &= test::check(test::hasFQN(syms, u8"Counter::value",    SymbolKind::Field),       "Counter::value field");
    ok &= test::check(test::hasFQN(syms, u8"Counter::new",      SymbolKind::Constructor), "Counter::new constructor");
    ok &= test::check(test::hasFQN(syms, u8"Counter::increment",SymbolKind::Method),      "Counter::increment method");
    ok &= test::check(test::hasFQN(syms, u8"Counter::get",      SymbolKind::Method),      "Counter::get method");
    ok &= test::check(test::hasFQN(syms, u8"Counter::reset",    SymbolKind::Method),      "Counter::reset method");

    // new() has no self param → Constructor; increment/get/reset have self → Method
    const auto* newSym = test::findSymbol(syms, u8"Counter::new", SymbolKind::Constructor);
    ok &= test::check(newSym != nullptr, "Counter::new is Constructor");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testModules()
{
    std::cout << "  testRust_Modules... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-rust/samples/modules.rs",
        {
            {u8"geometry",               SymbolKind::Namespace},
            {u8"geometry::Circle",       SymbolKind::Struct},
            {u8"geometry::Circle::radius",SymbolKind::Field},
            {u8"geometry::area",         SymbolKind::Function},
            {u8"math",                   SymbolKind::Namespace},
            {u8"math::ops",              SymbolKind::Namespace},
            {u8"math::ops::multiply",    SymbolKind::Function},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testFunctions()
{
    std::cout << "  testRust_Functions... ";
    AST tree = parse(u8"test/ast-extractor-rust/samples/functions.rs");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, u8"add",          SymbolKind::Function), "add function");
    ok &= test::check(test::hasFQN(syms, u8"private_func", SymbolKind::Function), "private_func function");
    ok &= test::check(test::hasFQN(syms, u8"MAX_VALUE",    SymbolKind::Variable), "MAX_VALUE const");
    ok &= test::check(test::hasFQN(syms, u8"COUNTER",      SymbolKind::Variable), "COUNTER static");
    ok &= test::check(test::hasFQN(syms, u8"Score",        SymbolKind::Typedef),  "Score type alias");

    const auto* maxVal = test::findSymbol(syms, u8"MAX_VALUE", SymbolKind::Variable);
    ok &= test::check(maxVal && maxVal->isConstexpr, "MAX_VALUE isConstexpr");

    const auto* cnt = test::findSymbol(syms, u8"COUNTER", SymbolKind::Variable);
    ok &= test::check(cnt && cnt->isStatic, "COUNTER isStatic");

    const auto* addSym = test::findSymbol(syms, u8"add", SymbolKind::Function);
    ok &= test::check(addSym && addSym->access == Access::Public, "add access=Public");

    const auto* priv = test::findSymbol(syms, u8"private_func", SymbolKind::Function);
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
