#include "test_symbol_scope.h"
#include "ast-extractor.h"
#include "ast-scope-builder.h"
#include "ast-symbol-scope.h"
#include "ast-ir.h"
#include <algorithm>
#include <iostream>
#include <string_view>

namespace ast
{
namespace
{
    bool check(bool condition, const char* description)
    {
        if(!condition)
            std::cerr << "    FAIL: " << description << "\n";
        return condition;
    }

    /** Returns the index of the first symbol matching @p fqn and @p kind, or ExtractorInvalidId. */
    size_t symbolIndex(const std::vector<Symbol>& syms, std::string_view fqn, SymbolKind kind)
    {
        for(size_t i = 0; i < syms.size(); ++i) {
            if(syms[i].fqn == fqn && syms[i].kind == kind) return i;
        }
        return ExtractorInvalidId;
    }

    /** Returns true when @p scopeId's symbols_ list contains @p symIdx. */
    bool scopeContains(const ScopeTree& tree, uintptr_t scopeId, size_t symIdx)
    {
        if(scopeId == ScopeTree::InvalidId) return false;
        const auto& syms = tree[scopeId].symbols_;
        return std::find(syms.begin(), syms.end(), symIdx) != syms.end();
    }

    /** Finds the first scope of @p kind; returns InvalidId if none. */
    uintptr_t firstScopeOfKind(const ScopeTree& tree, ScopeKind kind)
    {
        for(uint32_t i = 0; i < tree.size(); ++i) {
            if(tree[i].kind_ == kind) return static_cast<uintptr_t>(i);
        }
        return ScopeTree::InvalidId;
    }

    /** Finds the Nth scope (0-based) of @p kind; returns InvalidId if not enough. */
    uintptr_t nthScopeOfKind(const ScopeTree& tree, ScopeKind kind, uint32_t n)
    {
        uint32_t count = 0;
        for(uint32_t i = 0; i < tree.size(); ++i) {
            if(tree[i].kind_ == kind) {
                if(count == n) return static_cast<uintptr_t>(i);
                ++count;
            }
        }
        return ScopeTree::InvalidId;
    }

    // -----------------------------------------------------------------------

    bool test_ns_symbols()
    {
        bool ok = true;
        AST ast = parse("test/ast-symbol-scope/samples/ns_symbols.cpp");
        ok &= check(!!ast, "ns_symbols.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);
        std::vector<Symbol> syms = extract_symbols(ast);
        associate_symbols(tree, syms);

        // MyNs (Namespace) → declared in Global scope
        size_t nsIdx = symbolIndex(syms, "MyNs", SymbolKind::Namespace);
        ok &= check(nsIdx != ExtractorInvalidId, "MyNs symbol exists");
        if(nsIdx != ExtractorInvalidId) {
            uintptr_t ownerScope = tree.getScopeOfSymbol(nsIdx);
            ok &= check(ownerScope != ScopeTree::InvalidId, "MyNs has an owner scope");
            ok &= check(tree[ownerScope].kind_ == ScopeKind::Global, "MyNs belongs to Global scope");
            ok &= check(scopeContains(tree, ownerScope, nsIdx), "Global scope lists MyNs (scope→symbol)");
        }

        // nsVar (Variable) → declared in MyNs namespace scope
        size_t varIdx = symbolIndex(syms, "MyNs::nsVar", SymbolKind::Variable);
        ok &= check(varIdx != ExtractorInvalidId, "nsVar symbol exists");
        if(varIdx != ExtractorInvalidId) {
            uintptr_t ownerScope = tree.getScopeOfSymbol(varIdx);
            ok &= check(ownerScope != ScopeTree::InvalidId, "nsVar has an owner scope");
            ok &= check(tree[ownerScope].kind_ == ScopeKind::Namespace, "nsVar belongs to Namespace scope");
            ok &= check(scopeContains(tree, ownerScope, varIdx), "Namespace scope lists nsVar (scope→symbol)");
        }

        // nsFunc (Function) → declared in MyNs namespace scope
        size_t fnIdx = symbolIndex(syms, "MyNs::nsFunc", SymbolKind::Function);
        ok &= check(fnIdx != ExtractorInvalidId, "nsFunc symbol exists");
        if(fnIdx != ExtractorInvalidId) {
            uintptr_t ownerScope = tree.getScopeOfSymbol(fnIdx);
            ok &= check(ownerScope != ScopeTree::InvalidId, "nsFunc has an owner scope");
            ok &= check(tree[ownerScope].kind_ == ScopeKind::Namespace, "nsFunc belongs to Namespace scope");
            ok &= check(scopeContains(tree, ownerScope, fnIdx), "Namespace scope lists nsFunc (scope→symbol)");
        }

        return ok;
    }

    bool test_class_members()
    {
        bool ok = true;
        AST ast = parse("test/ast-symbol-scope/samples/class_members.cpp");
        ok &= check(!!ast, "class_members.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);
        std::vector<Symbol> syms = extract_symbols(ast);
        associate_symbols(tree, syms);

        // MyClass (Class) → declared in Global scope
        size_t clsIdx = symbolIndex(syms, "MyClass", SymbolKind::Class);
        ok &= check(clsIdx != ExtractorInvalidId, "MyClass symbol exists");
        if(clsIdx != ExtractorInvalidId) {
            uintptr_t ownerScope = tree.getScopeOfSymbol(clsIdx);
            ok &= check(tree[ownerScope].kind_ == ScopeKind::Global, "MyClass belongs to Global scope");
            ok &= check(scopeContains(tree, ownerScope, clsIdx), "Global scope lists MyClass (scope→symbol)");
        }

        // field1 and field2 (Field) → declared in Class scope
        size_t f1Idx = symbolIndex(syms, "MyClass::field1", SymbolKind::Field);
        size_t f2Idx = symbolIndex(syms, "MyClass::field2", SymbolKind::Field);
        ok &= check(f1Idx != ExtractorInvalidId, "field1 symbol exists");
        ok &= check(f2Idx != ExtractorInvalidId, "field2 symbol exists");

        uintptr_t classScope = firstScopeOfKind(tree, ScopeKind::Class);
        ok &= check(classScope != ScopeTree::InvalidId, "Class scope exists");

        if(f1Idx != ExtractorInvalidId && classScope != ScopeTree::InvalidId) {
            ok &= check(tree.getScopeOfSymbol(f1Idx) == classScope, "field1 belongs to Class scope");
            ok &= check(scopeContains(tree, classScope, f1Idx), "Class scope lists field1 (scope→symbol)");
        }
        if(f2Idx != ExtractorInvalidId && classScope != ScopeTree::InvalidId) {
            ok &= check(tree.getScopeOfSymbol(f2Idx) == classScope, "field2 belongs to Class scope");
            ok &= check(scopeContains(tree, classScope, f2Idx), "Class scope lists field2 (scope→symbol)");
        }

        return ok;
    }

    bool test_sibling_scopes()
    {
        bool ok = true;
        AST ast = parse("test/ast-symbol-scope/samples/sibling_ns.cpp");
        ok &= check(!!ast, "sibling_ns.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);
        std::vector<Symbol> syms = extract_symbols(ast);
        associate_symbols(tree, syms);

        // varA → NsA scope  (not NsB)
        size_t varAIdx = symbolIndex(syms, "NsA::varA", SymbolKind::Variable);
        size_t varBIdx = symbolIndex(syms, "NsB::varB", SymbolKind::Variable);
        ok &= check(varAIdx != ExtractorInvalidId, "varA symbol exists");
        ok &= check(varBIdx != ExtractorInvalidId, "varB symbol exists");

        uintptr_t nsA = nthScopeOfKind(tree, ScopeKind::Namespace, 0);
        uintptr_t nsB = nthScopeOfKind(tree, ScopeKind::Namespace, 1);
        ok &= check(nsA != ScopeTree::InvalidId, "NsA scope exists");
        ok &= check(nsB != ScopeTree::InvalidId, "NsB scope exists");
        ok &= check(nsA != nsB, "NsA and NsB are distinct scopes");

        if(varAIdx != ExtractorInvalidId) {
            uintptr_t ownerA = tree.getScopeOfSymbol(varAIdx);
            ok &= check(ownerA == nsA, "varA belongs to NsA (not NsB)");
            ok &= check(!scopeContains(tree, nsB, varAIdx), "NsB does not list varA");
        }
        if(varBIdx != ExtractorInvalidId) {
            uintptr_t ownerB = tree.getScopeOfSymbol(varBIdx);
            ok &= check(ownerB == nsB, "varB belongs to NsB (not NsA)");
            ok &= check(!scopeContains(tree, nsA, varBIdx), "NsA does not list varB");
        }

        return ok;
    }

    bool test_nested_ns_symbols()
    {
        bool ok = true;
        AST ast = parse("test/ast-symbol-scope/samples/nested_ns.cpp");
        ok &= check(!!ast, "nested_ns.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);
        std::vector<Symbol> syms = extract_symbols(ast);
        associate_symbols(tree, syms);

        // outerVar (Variable in Outer) → Outer scope
        size_t outerVarIdx = symbolIndex(syms, "Outer::outerVar", SymbolKind::Variable);
        ok &= check(outerVarIdx != ExtractorInvalidId, "outerVar symbol exists");

        // innerVar (Variable in Outer::Inner) → Inner scope
        size_t innerVarIdx = symbolIndex(syms, "Outer::Inner::innerVar", SymbolKind::Variable);
        ok &= check(innerVarIdx != ExtractorInvalidId, "innerVar symbol exists");

        uintptr_t outerNs = nthScopeOfKind(tree, ScopeKind::Namespace, 0); // Outer
        uintptr_t innerNs = nthScopeOfKind(tree, ScopeKind::Namespace, 1); // Inner

        if(outerVarIdx != ExtractorInvalidId && outerNs != ScopeTree::InvalidId) {
            ok &= check(tree.getScopeOfSymbol(outerVarIdx) == outerNs, "outerVar → Outer scope");
            ok &= check(scopeContains(tree, outerNs, outerVarIdx), "Outer scope lists outerVar");
        }
        if(innerVarIdx != ExtractorInvalidId && innerNs != ScopeTree::InvalidId) {
            ok &= check(tree.getScopeOfSymbol(innerVarIdx) == innerNs, "innerVar → Inner scope");
            ok &= check(scopeContains(tree, innerNs, innerVarIdx), "Inner scope lists innerVar");
            // outerVar must NOT appear in Inner scope
            if(outerVarIdx != ExtractorInvalidId) {
                ok &= check(!scopeContains(tree, innerNs, outerVarIdx), "Inner scope does not contain outerVar");
            }
        }

        return ok;
    }

    bool test_bidirectional_consistency()
    {
        // For every symbol: if symbol→scope gives scopeId, then scope→symbols must include that symbol.
        // For every scope:  if scope→symbols lists symIdx, then symbol→scope must give that scope.
        bool ok = true;
        AST ast = parse("test/ast-symbol-scope/samples/ns_symbols.cpp");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);
        std::vector<Symbol> syms = extract_symbols(ast);
        associate_symbols(tree, syms);

        // symbol → scope → must contain symbol
        for(size_t i = 0; i < syms.size(); ++i) {
            uintptr_t scopeId = tree.getScopeOfSymbol(i);
            if(scopeId == ScopeTree::InvalidId) continue;
            ok &= check(scopeContains(tree, scopeId, i), "scope→symbols contains symbol that mapped to it");
        }

        // scope → symbol → must map back to scope
        for(uint32_t s = 0; s < tree.size(); ++s) {
            for(size_t symIdx : tree[s].symbols_) {
                ok &= check(tree.getScopeOfSymbol(symIdx) == static_cast<uintptr_t>(s),
                            "symbol→scope maps back to the scope that listed it");
            }
        }

        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_symbol_scope()
{
    static const TestCase cases[] = {
        {"namespace symbols",         test_ns_symbols},
        {"class members",             test_class_members},
        {"sibling scopes independent",test_sibling_scopes},
        {"nested namespace symbols",  test_nested_ns_symbols},
        {"bidirectional consistency", test_bidirectional_consistency},
    };

    std::cout << "=== symbol-scope association tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
