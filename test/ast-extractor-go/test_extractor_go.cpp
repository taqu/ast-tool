#include "test_extractor_go.h"
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
    std::cout << "  testGo_Structs... ";
    AST tree = parse(u8"test/ast-extractor-go/samples/structs.go");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, u8"shapes",          SymbolKind::Namespace), "shapes package");
    ok &= test::check(test::hasFQN(syms, u8"shapes.Point",    SymbolKind::Struct),    "shapes.Point struct");
    ok &= test::check(test::hasFQN(syms, u8"shapes.Point.X",  SymbolKind::Field),     "shapes.Point.X field");
    ok &= test::check(test::hasFQN(syms, u8"shapes.Point.Y",  SymbolKind::Field),     "shapes.Point.Y field");
    ok &= test::check(test::hasFQN(syms, u8"shapes.circle",   SymbolKind::Struct),    "shapes.circle struct");
    ok &= test::check(test::hasFQN(syms, u8"shapes.circle.radius", SymbolKind::Field),"shapes.circle.radius field");
    ok &= test::check(test::hasFQN(syms, u8"shapes.circle.center", SymbolKind::Field),"shapes.circle.center field");

    const auto* point = test::findSymbol(syms, u8"shapes.Point", SymbolKind::Struct);
    ok &= test::check(point && point->access == Access::Public, "Point access=Public (uppercase)");

    const auto* circ = test::findSymbol(syms, u8"shapes.circle", SymbolKind::Struct);
    ok &= test::check(circ && circ->access == Access::Private, "circle access=Private (lowercase)");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testInterfaces()
{
    std::cout << "  testGo_Interfaces... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-go/samples/interfaces.go",
        {
            {u8"io",               SymbolKind::Namespace},
            {u8"io.Reader",        SymbolKind::Class},
            {u8"io.Reader.Read",   SymbolKind::Method},
            {u8"io.ReadWriter",    SymbolKind::Class},
            {u8"io.ReadWriter.Read",  SymbolKind::Method},
            {u8"io.ReadWriter.Write", SymbolKind::Method},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testFunctions()
{
    std::cout << "  testGo_Functions... ";
    AST tree = parse(u8"test/ast-extractor-go/samples/functions.go");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, u8"math",              SymbolKind::Namespace), "math package");
    ok &= test::check(test::hasFQN(syms, u8"math.Add",          SymbolKind::Function),  "math.Add function");
    ok &= test::check(test::hasFQN(syms, u8"math.subtract",     SymbolKind::Function),  "math.subtract function");
    ok &= test::check(test::hasFQN(syms, u8"math.Pi",           SymbolKind::Variable),  "math.Pi const");
    ok &= test::check(test::hasFQN(syms, u8"math.maxSize",      SymbolKind::Variable),  "math.maxSize const");
    ok &= test::check(test::hasFQN(syms, u8"math.globalCounter",SymbolKind::Variable),  "math.globalCounter var");

    const auto* addSym = test::findSymbol(syms, u8"math.Add", SymbolKind::Function);
    ok &= test::check(addSym && addSym->access == Access::Public, "Add access=Public (uppercase)");

    const auto* subSym = test::findSymbol(syms, u8"math.subtract", SymbolKind::Function);
    ok &= test::check(subSym && subSym->access == Access::Private, "subtract access=Private (lowercase)");

    const auto* pi = test::findSymbol(syms, u8"math.Pi", SymbolKind::Variable);
    ok &= test::check(pi && pi->isConstexpr, "Pi isConstexpr (const)");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testMethods()
{
    std::cout << "  testGo_Methods... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-go/samples/methods.go",
        {
            {u8"shapes",                   SymbolKind::Namespace},
            {u8"shapes.Rectangle",         SymbolKind::Struct},
            {u8"shapes.Rectangle.Width",   SymbolKind::Field},
            {u8"shapes.Rectangle.Height",  SymbolKind::Field},
            {u8"shapes.Rectangle.Area",    SymbolKind::Method},
            {u8"shapes.Rectangle.Scale",   SymbolKind::Method},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testConsts()
{
    std::cout << "  testGo_Consts... ";
    AST tree = parse(u8"test/ast-extractor-go/samples/consts.go");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, u8"config",             SymbolKind::Namespace), "config package");
    ok &= test::check(test::hasFQN(syms, u8"config.MaxRetries",  SymbolKind::Variable),  "config.MaxRetries");
    ok &= test::check(test::hasFQN(syms, u8"config.Timeout",     SymbolKind::Variable),  "config.Timeout");
    ok &= test::check(test::hasFQN(syms, u8"config.debug",       SymbolKind::Variable),  "config.debug");
    ok &= test::check(test::hasFQN(syms, u8"config.verbose",     SymbolKind::Variable),  "config.verbose");

    const auto* mr = test::findSymbol(syms, u8"config.MaxRetries", SymbolKind::Variable);
    ok &= test::check(mr && mr->isConstexpr, "MaxRetries isConstexpr");

    const auto* dbg = test::findSymbol(syms, u8"config.debug", SymbolKind::Variable);
    ok &= test::check(dbg && !dbg->isConstexpr, "debug is not constexpr (var)");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

} // anonymous namespace

bool run_tests_go()
{
    std::cout << "=== Go extractor tests ===" << std::endl;
    bool ok = true;
    ok &= testStructs();
    ok &= testInterfaces();
    ok &= testFunctions();
    ok &= testMethods();
    ok &= testConsts();
    std::cout << "=== Go: " << (ok ? "PASS" : "FAIL") << " ===" << std::endl;
    return ok;
}

} // namespace ast
