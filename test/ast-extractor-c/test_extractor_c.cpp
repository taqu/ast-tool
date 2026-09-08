#include "test_extractor_c.h"
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

bool testBasicFunctions()
{
    std::cout << "  testC_BasicFunctions... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-c/samples/basic_functions.c",
        {
            {u8"add",      SymbolKind::Function},
            {u8"greet",    SymbolKind::Function},
            {u8"multiply", SymbolKind::Function},
            {u8"helper",   SymbolKind::Function},
        });
    // helper must be static
    AST tree = parse(u8"test/ast-extractor-c/samples/basic_functions.c");
    if(tree) {
        auto syms = extract_symbols(tree);
        const auto* s = test::findSymbol(syms, u8"helper", SymbolKind::Function);
        ok &= test::check(s && s->isStatic, "helper isStatic");
    }
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testStructs()
{
    std::cout << "  testC_Structs... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-c/samples/structs.c",
        {
            {u8"Point",                 SymbolKind::Struct},
            {u8"Point::x",              SymbolKind::Field},
            {u8"Point::y",              SymbolKind::Field},
            {u8"Rectangle",             SymbolKind::Struct},
            {u8"Rectangle::top_left",   SymbolKind::Field},
            {u8"Rectangle::bottom_right", SymbolKind::Field},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testEnums()
{
    std::cout << "  testC_Enums... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-c/samples/enums.c",
        {
            {u8"Direction",       SymbolKind::Enum},
            {u8"Direction::North", SymbolKind::EnumValue},
            {u8"Direction::South", SymbolKind::EnumValue},
            {u8"Direction::East",  SymbolKind::EnumValue},
            {u8"Direction::West",  SymbolKind::EnumValue},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testUnions()
{
    std::cout << "  testC_Unions... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-c/samples/unions.c",
        {
            {u8"Data",    SymbolKind::Union},
            {u8"Data::i", SymbolKind::Field},
            {u8"Data::f", SymbolKind::Field},
            {u8"Data::c", SymbolKind::Field},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testTypedefs()
{
    std::cout << "  testC_Typedefs... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-c/samples/typedefs.c",
        {
            {u8"ErrorCode", SymbolKind::Typedef},
            {u8"Size",      SymbolKind::Typedef},
            {u8"Node",      SymbolKind::Typedef},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testMacros()
{
    std::cout << "  testC_Macros... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-c/samples/macros.c",
        {
            {u8"MAX_SIZE",    SymbolKind::Macro},
            {u8"PI",          SymbolKind::Macro},
            {u8"APP_VERSION", SymbolKind::Macro},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testNestedStructs()
{
    std::cout << "  testC_NestedStructs... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-c/samples/nested.c",
        {
            {u8"Outer",                SymbolKind::Struct},
            {u8"Outer::outer_value",   SymbolKind::Field},
            {u8"Outer::inner",         SymbolKind::Field},
            {u8"Outer::Inner",         SymbolKind::Struct},
            {u8"Outer::Inner::inner_value", SymbolKind::Field},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testForwardDeclarations()
{
    std::cout << "  testC_ForwardDeclarations... ";
    AST tree = parse(u8"test/ast-extractor-c/samples/forward_decls.c");
    if(!tree) {
        std::cerr << "    FAIL: could not parse\n";
        return false;
    }
    auto syms = extract_symbols(tree);
    bool ok = true;
    // Struct forward declaration is extracted
    ok &= test::check(test::hasFQN(syms, u8"ForwardNode", SymbolKind::Struct),
                      "ForwardNode struct from forward decl");
    // Function deduplicated (declaration + definition = single entry)
    size_t computeCount = 0;
    for(const auto& s : syms)
        if(s.fqn == u8"compute" && s.kind == SymbolKind::Function) ++computeCount;
    ok &= test::check(computeCount == 1, "compute appears exactly once (deduplication)");
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testEdgeCases()
{
    std::cout << "  testC_EdgeCases... ";
    bool ok = parseAndCheck(
        u8"test/ast-extractor-c/samples/edge_cases.c",
        {
            {u8"ANON_A",  SymbolKind::EnumValue},
            {u8"ANON_B",  SymbolKind::EnumValue},
            {u8"Empty",   SymbolKind::Struct},
            {u8"no_args", SymbolKind::Function},
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
