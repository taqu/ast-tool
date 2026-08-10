#include "test_extractor_go.h"
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
    std::cout << "  testGo_Structs... ";
    AST tree = parse("test/ast-extractor-go/samples/structs.go");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "shapes",          SymbolKind::Namespace), "shapes package");
    ok &= test::check(test::hasFQN(syms, "shapes.Point",    SymbolKind::Struct),    "shapes.Point struct");
    ok &= test::check(test::hasFQN(syms, "shapes.Point.X",  SymbolKind::Field),     "shapes.Point.X field");
    ok &= test::check(test::hasFQN(syms, "shapes.Point.Y",  SymbolKind::Field),     "shapes.Point.Y field");
    ok &= test::check(test::hasFQN(syms, "shapes.circle",   SymbolKind::Struct),    "shapes.circle struct");
    ok &= test::check(test::hasFQN(syms, "shapes.circle.radius", SymbolKind::Field),"shapes.circle.radius field");
    ok &= test::check(test::hasFQN(syms, "shapes.circle.center", SymbolKind::Field),"shapes.circle.center field");

    const auto* point = test::findSymbol(syms, "shapes.Point", SymbolKind::Struct);
    ok &= test::check(point && point->access == Access::Public, "Point access=Public (uppercase)");

    const auto* circ = test::findSymbol(syms, "shapes.circle", SymbolKind::Struct);
    ok &= test::check(circ && circ->access == Access::Private, "circle access=Private (lowercase)");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testInterfaces()
{
    std::cout << "  testGo_Interfaces... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-go/samples/interfaces.go",
        {
            {"io",               SymbolKind::Namespace},
            {"io.Reader",        SymbolKind::Class},
            {"io.Reader.Read",   SymbolKind::Method},
            {"io.ReadWriter",    SymbolKind::Class},
            {"io.ReadWriter.Read",  SymbolKind::Method},
            {"io.ReadWriter.Write", SymbolKind::Method},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testFunctions()
{
    std::cout << "  testGo_Functions... ";
    AST tree = parse("test/ast-extractor-go/samples/functions.go");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "math",              SymbolKind::Namespace), "math package");
    ok &= test::check(test::hasFQN(syms, "math.Add",          SymbolKind::Function),  "math.Add function");
    ok &= test::check(test::hasFQN(syms, "math.subtract",     SymbolKind::Function),  "math.subtract function");
    ok &= test::check(test::hasFQN(syms, "math.Pi",           SymbolKind::Variable),  "math.Pi const");
    ok &= test::check(test::hasFQN(syms, "math.maxSize",      SymbolKind::Variable),  "math.maxSize const");
    ok &= test::check(test::hasFQN(syms, "math.globalCounter",SymbolKind::Variable),  "math.globalCounter var");

    const auto* addSym = test::findSymbol(syms, "math.Add", SymbolKind::Function);
    ok &= test::check(addSym && addSym->access == Access::Public, "Add access=Public (uppercase)");

    const auto* subSym = test::findSymbol(syms, "math.subtract", SymbolKind::Function);
    ok &= test::check(subSym && subSym->access == Access::Private, "subtract access=Private (lowercase)");

    const auto* pi = test::findSymbol(syms, "math.Pi", SymbolKind::Variable);
    ok &= test::check(pi && pi->isConstexpr, "Pi isConstexpr (const)");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testMethods()
{
    std::cout << "  testGo_Methods... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-go/samples/methods.go",
        {
            {"shapes",                   SymbolKind::Namespace},
            {"shapes.Rectangle",         SymbolKind::Struct},
            {"shapes.Rectangle.Width",   SymbolKind::Field},
            {"shapes.Rectangle.Height",  SymbolKind::Field},
            {"shapes.Rectangle.Area",    SymbolKind::Method},
            {"shapes.Rectangle.Scale",   SymbolKind::Method},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testConsts()
{
    std::cout << "  testGo_Consts... ";
    AST tree = parse("test/ast-extractor-go/samples/consts.go");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "config",             SymbolKind::Namespace), "config package");
    ok &= test::check(test::hasFQN(syms, "config.MaxRetries",  SymbolKind::Variable),  "config.MaxRetries");
    ok &= test::check(test::hasFQN(syms, "config.Timeout",     SymbolKind::Variable),  "config.Timeout");
    ok &= test::check(test::hasFQN(syms, "config.debug",       SymbolKind::Variable),  "config.debug");
    ok &= test::check(test::hasFQN(syms, "config.verbose",     SymbolKind::Variable),  "config.verbose");

    const auto* mr = test::findSymbol(syms, "config.MaxRetries", SymbolKind::Variable);
    ok &= test::check(mr && mr->isConstexpr, "MaxRetries isConstexpr");

    const auto* dbg = test::findSymbol(syms, "config.debug", SymbolKind::Variable);
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
