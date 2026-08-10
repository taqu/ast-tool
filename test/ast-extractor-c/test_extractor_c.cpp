#include "test_extractor_c.h"
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

bool testBasicFunctions()
{
    std::cout << "  testC_BasicFunctions... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-c/samples/basic_functions.c",
        {
            {"add",      SymbolKind::Function},
            {"greet",    SymbolKind::Function},
            {"multiply", SymbolKind::Function},
            {"helper",   SymbolKind::Function},
        });
    // helper must be static
    AST tree = parse("test/ast-extractor-c/samples/basic_functions.c");
    if(tree) {
        auto syms = extract_symbols(tree);
        const auto* s = test::findSymbol(syms, "helper", SymbolKind::Function);
        ok &= test::check(s && s->isStatic, "helper isStatic");
    }
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testStructs()
{
    std::cout << "  testC_Structs... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-c/samples/structs.c",
        {
            {"Point",                 SymbolKind::Struct},
            {"Point::x",              SymbolKind::Field},
            {"Point::y",              SymbolKind::Field},
            {"Rectangle",             SymbolKind::Struct},
            {"Rectangle::top_left",   SymbolKind::Field},
            {"Rectangle::bottom_right", SymbolKind::Field},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testEnums()
{
    std::cout << "  testC_Enums... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-c/samples/enums.c",
        {
            {"Direction",       SymbolKind::Enum},
            {"Direction::North", SymbolKind::EnumValue},
            {"Direction::South", SymbolKind::EnumValue},
            {"Direction::East",  SymbolKind::EnumValue},
            {"Direction::West",  SymbolKind::EnumValue},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testUnions()
{
    std::cout << "  testC_Unions... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-c/samples/unions.c",
        {
            {"Data",    SymbolKind::Union},
            {"Data::i", SymbolKind::Field},
            {"Data::f", SymbolKind::Field},
            {"Data::c", SymbolKind::Field},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testTypedefs()
{
    std::cout << "  testC_Typedefs... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-c/samples/typedefs.c",
        {
            {"ErrorCode", SymbolKind::Typedef},
            {"Size",      SymbolKind::Typedef},
            {"Node",      SymbolKind::Typedef},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testMacros()
{
    std::cout << "  testC_Macros... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-c/samples/macros.c",
        {
            {"MAX_SIZE",    SymbolKind::Macro},
            {"PI",          SymbolKind::Macro},
            {"APP_VERSION", SymbolKind::Macro},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testNestedStructs()
{
    std::cout << "  testC_NestedStructs... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-c/samples/nested.c",
        {
            {"Outer",                SymbolKind::Struct},
            {"Outer::outer_value",   SymbolKind::Field},
            {"Outer::inner",         SymbolKind::Field},
            {"Outer::Inner",         SymbolKind::Struct},
            {"Outer::Inner::inner_value", SymbolKind::Field},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testForwardDeclarations()
{
    std::cout << "  testC_ForwardDeclarations... ";
    AST tree = parse("test/ast-extractor-c/samples/forward_decls.c");
    if(!tree) {
        std::cerr << "    FAIL: could not parse\n";
        return false;
    }
    auto syms = extract_symbols(tree);
    bool ok = true;
    // Struct forward declaration is extracted
    ok &= test::check(test::hasFQN(syms, "ForwardNode", SymbolKind::Struct),
                      "ForwardNode struct from forward decl");
    // Function deduplicated (declaration + definition = single entry)
    size_t computeCount = 0;
    for(const auto& s : syms)
        if(s.fqn == "compute" && s.kind == SymbolKind::Function) ++computeCount;
    ok &= test::check(computeCount == 1, "compute appears exactly once (deduplication)");
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testEdgeCases()
{
    std::cout << "  testC_EdgeCases... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-c/samples/edge_cases.c",
        {
            {"ANON_A",  SymbolKind::EnumValue},
            {"ANON_B",  SymbolKind::EnumValue},
            {"Empty",   SymbolKind::Struct},
            {"no_args", SymbolKind::Function},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

} // anonymous namespace

bool run_tests_c()
{
    std::cout << "=== C extractor tests ===" << std::endl;
    bool ok = true;
    ok &= testBasicFunctions();
    ok &= testStructs();
    ok &= testEnums();
    ok &= testUnions();
    ok &= testTypedefs();
    ok &= testMacros();
    ok &= testNestedStructs();
    ok &= testForwardDeclarations();
    ok &= testEdgeCases();
    std::cout << "=== C: " << (ok ? "PASS" : "FAIL") << " ===" << std::endl;
    return ok;
}

} // namespace ast
