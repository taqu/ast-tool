#include "test_extractor_java.h"
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
    std::cout << "  testJava_Classes... ";
    AST tree = parse("test/ast-extractor-java/samples/classes.java");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "com.example",              SymbolKind::Namespace),   "com.example package");
    ok &= test::check(test::hasFQN(syms, "com.example.Person",       SymbolKind::Class),       "Person class");
    ok &= test::check(test::hasFQN(syms, "com.example.Person.name",  SymbolKind::Field),       "Person.name field");
    ok &= test::check(test::hasFQN(syms, "com.example.Person.age",   SymbolKind::Field),       "Person.age field");
    ok &= test::check(test::hasFQN(syms, "com.example.Person.Person",SymbolKind::Constructor), "Person constructor");
    ok &= test::check(test::hasFQN(syms, "com.example.Person.getName",  SymbolKind::Method),  "Person.getName");
    ok &= test::check(test::hasFQN(syms, "com.example.Person.setName",  SymbolKind::Method),  "Person.setName");
    ok &= test::check(test::hasFQN(syms, "com.example.Person.validate", SymbolKind::Method),  "Person.validate");

    const auto* cls = test::findSymbol(syms, "com.example.Person", SymbolKind::Class);
    ok &= test::check(cls && cls->access == Access::Public, "Person access=Public");

    const auto* nm = test::findSymbol(syms, "com.example.Person.name", SymbolKind::Field);
    ok &= test::check(nm && nm->access == Access::Private, "Person.name access=Private");

    const auto* get = test::findSymbol(syms, "com.example.Person.getName", SymbolKind::Method);
    ok &= test::check(get && get->access == Access::Public, "getName access=Public");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testInterfaces()
{
    std::cout << "  testJava_Interfaces... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-java/samples/interfaces.java",
        {
            {"com.example",                    SymbolKind::Namespace},
            {"com.example.Printable",          SymbolKind::Class},
            {"com.example.Printable.print",    SymbolKind::Method},
            {"com.example.Printable.format",   SymbolKind::Method},
            {"com.example.Comparable",         SymbolKind::Class},
            {"com.example.Comparable.compareTo",SymbolKind::Method},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testEnums()
{
    std::cout << "  testJava_Enums... ";
    bool ok = parseAndCheck(
        "test/ast-extractor-java/samples/enums.java",
        {
            {"com.example",                  SymbolKind::Namespace},
            {"com.example.Status",           SymbolKind::Enum},
            {"com.example.Status.ACTIVE",    SymbolKind::EnumValue},
            {"com.example.Status.INACTIVE",  SymbolKind::EnumValue},
            {"com.example.Status.PENDING",   SymbolKind::EnumValue},
            {"com.example.Status.isActive",  SymbolKind::Method},
        });
    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

bool testNestedClasses()
{
    std::cout << "  testJava_NestedClasses... ";
    AST tree = parse("test/ast-extractor-java/samples/nested.java");
    if(!tree) { std::cerr << "    FAIL: could not parse\n"; return false; }
    auto syms = extract_symbols(tree);
    bool ok = true;
    ok &= test::check(test::hasFQN(syms, "com.example.Outer",             SymbolKind::Class),  "Outer class");
    ok &= test::check(test::hasFQN(syms, "com.example.Outer.value",       SymbolKind::Field),  "Outer.value field");
    ok &= test::check(test::hasFQN(syms, "com.example.Outer.Inner",       SymbolKind::Class),  "Outer.Inner class");
    ok &= test::check(test::hasFQN(syms, "com.example.Outer.Inner.process",SymbolKind::Method),"Outer.Inner.process");
    ok &= test::check(test::hasFQN(syms, "com.example.Outer.doWork",      SymbolKind::Method), "Outer.doWork");

    const auto* inner = test::findSymbol(syms, "com.example.Outer.Inner", SymbolKind::Class);
    ok &= test::check(inner && inner->isStatic, "Outer.Inner isStatic");

    std::cout << (ok ? "PASS" : "FAIL") << "\n";
    return ok;
}

} // anonymous namespace

bool run_tests_java()
{
    std::cout << "=== Java extractor tests ===" << std::endl;
    bool ok = true;
    ok &= testClasses();
    ok &= testInterfaces();
    ok &= testEnums();
    ok &= testNestedClasses();
    std::cout << "=== Java: " << (ok ? "PASS" : "FAIL") << " ===" << std::endl;
    return ok;
}

} // namespace ast
