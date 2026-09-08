#include "test.h"
#include <iostream>
#include <fstream>
#include <string_view>
#include "ast-ir.h"
#include "ast-extractor.h"
#include "helper.h"

namespace ast
{
namespace
{
    bool hasFQN(const std::vector<ast::Symbol>& syms,
                std::u8string_view fqn, ast::SymbolKind kind)
    {
        for(const auto& s : syms) {
            if(s.fqn == fqn && s.kind == kind) return true;
        }
        return false;
    }

    bool check(bool condition, const char* description)
    {
        if(!condition) {
            std::cerr << "  FAIL: " << description << std::endl;
        }
        return condition;
    }
} // anonymous namespace

namespace
{
    void print_indent(std::ostream& os, uint32_t indent)
    {
        for(uint32_t i = 0; i < indent; ++i) {
            os << ' ';
        }
    }
    void dump(const AST& ast, const ASTNode& node, std::ostream& os, uint32_t indent = 0)
    {
        print_indent(os, indent);
        os << ast_node_type_to_string(node.type_) << std::endl;
        for(uintptr_t c : node.children_) {
            const ASTNode& child = ast[c];
            dump(ast, child, os, indent+2);
        }
    }
}

bool testConversionOperators()
{
    std::cout << "-----------------------------------------------------" << std::endl;
    std::cout << "testConversionOperators" << std::endl;

    ast::AST tree = ast::parse(u8"data/test_operators.cpp");
    if(!tree) {
        std::cerr << "  FAIL: could not parse data/test_operators.cpp" << std::endl;
        return false;
    }

    std::vector<ast::Symbol> syms = ast::extract_symbols(tree);

    bool ok = true;

    // Case 1: basic conversion operator (declaration)
    ok &= check(hasFQN(syms, u8"Info::operator bool", ast::SymbolKind::Method),
                "Info::operator bool detected as Method");

    // Case 2: multiple conversion operators on the same struct
    ok &= check(hasFQN(syms, u8"Value::operator bool", ast::SymbolKind::Method),
                "Value::operator bool detected as Method");
    ok &= check(hasFQN(syms, u8"Value::operator int", ast::SymbolKind::Method),
                "Value::operator int detected as Method");

    // Case 3: fully qualified through namespace and class
    ok &= check(hasFQN(syms, u8"A::B::operator bool", ast::SymbolKind::Method),
                "A::B::operator bool detected as Method");

    // Case 4: inline definition (must not be omitted or duplicated)
    ok &= check(hasFQN(syms, u8"Inline::operator bool", ast::SymbolKind::Method),
                "Inline::operator bool (inline definition) detected as Method");

    // Sanity: regular methods and fields must still be extracted alongside operators
    ok &= check(hasFQN(syms, u8"Info",   ast::SymbolKind::Struct), "Info struct present");
    ok &= check(hasFQN(syms, u8"Value",  ast::SymbolKind::Struct), "Value struct present");
    ok &= check(hasFQN(syms, u8"A::B",   ast::SymbolKind::Struct), "A::B struct present");
    ok &= check(hasFQN(syms, u8"Inline", ast::SymbolKind::Struct), "Inline struct present");

    std::cout << (ok ? "  PASS" : "  FAIL") << std::endl;
    return ok;
}
bool testMemberOperators()
{
    std::cout << "-----------------------------------------------------" << std::endl;
    std::cout << "testMemberOperators" << std::endl;

    ast::AST tree = ast::parse(u8"data/test_member_operators.cpp");
    if(!tree) {
        std::cerr << "  FAIL: could not parse data/test_member_operators.cpp" << std::endl;
        return false;
    }
    std::vector<ast::Symbol> syms = ast::extract_symbols(tree);

    bool ok = true;

    // Member arithmetic operator (declaration only)
    ok &= check(hasFQN(syms, u8"Counter::operator+=", ast::SymbolKind::Method),
                "Counter::operator+= member operator as Method");
    ok &= check(hasFQN(syms, u8"Counter::operator-=", ast::SymbolKind::Method),
                "Counter::operator-= member operator as Method");
    ok &= check(hasFQN(syms, u8"Counter::operator*=", ast::SymbolKind::Method),
                "Counter::operator*= member operator as Method");
    ok &= check(hasFQN(syms, u8"Counter::operator/=", ast::SymbolKind::Method),
                "Counter::operator/= member operator as Method");

    // Validation case: all three operator kinds inside a namespace
    ok &= check(hasFQN(syms, u8"test::Info::operator bool", ast::SymbolKind::Method),
                "test::Info::operator bool conversion operator");
    ok &= check(hasFQN(syms, u8"test::Info::operator int", ast::SymbolKind::Method),
                "test::Info::operator int conversion operator");
    ok &= check(hasFQN(syms, u8"test::Info::operator int*", ast::SymbolKind::Method),
                "test::Info::operator int* conversion operator");
    ok &= check(hasFQN(syms, u8"test::Info::operator int&", ast::SymbolKind::Method),
                "test::Info::operator int& conversion operator");
    ok &= check(hasFQN(syms, u8"test::Info::operator int**", ast::SymbolKind::Method),
                "test::Info::operator int** conversion operator");
    ok &= check(hasFQN(syms, u8"test::Info::operator Info*", ast::SymbolKind::Method),
                "test::Info::operator Info* conversion operator");
    ok &= check(hasFQN(syms, u8"test::Info::operator Info&", ast::SymbolKind::Method),
                "test::Info::operator Info& conversion operator");


    // Free operator at namespace scope must be Function, not Method
    ok &= check(hasFQN(syms, u8"test::operator+", ast::SymbolKind::Function),
                "test::operator+ free operator as Function");

    // Nested namespace scope
    ok &= check(hasFQN(syms, u8"A::B::Value::operator bool", ast::SymbolKind::Method),
                "A::B::Value::operator bool through nested namespaces");

    // Struct symbols still present
    ok &= check(hasFQN(syms, u8"Counter",    ast::SymbolKind::Struct), "Counter struct present");
    ok &= check(hasFQN(syms, u8"test::Info", ast::SymbolKind::Struct), "test::Info struct present");
    ok &= check(hasFQN(syms, u8"A::B::Value", ast::SymbolKind::Struct), "A::B::Value struct present");

    std::cout << (ok ? "  PASS" : "  FAIL") << std::endl;
    return ok;
}
} // namespace ast
