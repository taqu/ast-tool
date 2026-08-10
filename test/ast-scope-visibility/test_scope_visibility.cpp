#include "test_scope_visibility.h"
#include "ast-extractor.h"
#include "ast-scope-builder.h"
#include "ast-scope-visibility.h"
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

    bool isVisible(const ScopeVisibility& vis, uintptr_t scopeId, size_t symIdx)
    {
        const auto& v = vis.visibleIn(scopeId);
        return std::find(v.begin(), v.end(), symIdx) != v.end();
    }

    size_t symbolIndex(const std::vector<Symbol>& syms, std::string_view fqn, SymbolKind kind)
    {
        for(size_t i = 0; i < syms.size(); ++i) {
            if(syms[i].fqn == fqn && syms[i].kind == kind) return i;
        }
        return ScopeVisibility::InvalidSymbol;
    }

    uintptr_t nthScopeOfKind(const ScopeTree& tree, ScopeKind kind, uint32_t n = 0)
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

    // ------------------------------------------------------------------
    // Pure unit tests — no file parsing; manually constructed ScopeTree
    // ------------------------------------------------------------------

    bool test_basic_inheritance()
    {
        // Global {sym0("a"), sym1("b")}  →  Namespace {sym2("c")}
        // Namespace should see a, b, c
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global,    ScopeTree::InvalidNodeIndex, 0, 9999);
        uintptr_t ns     = tree.add(ScopeKind::Namespace, ScopeTree::InvalidNodeIndex, 10, 500, global);

        std::vector<Symbol> syms(3);
        syms[0].name = "a"; syms[0].kind = SymbolKind::Variable;
        syms[1].name = "b"; syms[1].kind = SymbolKind::Variable;
        syms[2].name = "c"; syms[2].kind = SymbolKind::Variable;

        tree.addSymbol(global, 0);
        tree.addSymbol(global, 1);
        tree.addSymbol(ns,     2);

        ScopeVisibility vis = ScopeVisibility::compute(tree, syms);

        ok &= check(isVisible(vis, global, 0), "global sees sym0 (a)");
        ok &= check(isVisible(vis, global, 1), "global sees sym1 (b)");
        ok &= check(!isVisible(vis, global, 2), "global does NOT see sym2 (c, declared in child)");

        ok &= check(isVisible(vis, ns, 0), "namespace inherits sym0 (a) from global");
        ok &= check(isVisible(vis, ns, 1), "namespace inherits sym1 (b) from global");
        ok &= check(isVisible(vis, ns, 2), "namespace sees its own sym2 (c)");

        ok &= check(vis.visibleIn(global).size() == 2, "global has 2 visible symbols");
        ok &= check(vis.visibleIn(ns).size()     == 3, "namespace has 3 visible symbols");

        return ok;
    }

    bool test_shadowing()
    {
        // Global {sym0("x"), sym1("y")}
        //   Block {sym2("x")}   ← shadows global "x"
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 9999);
        uintptr_t block  = tree.add(ScopeKind::Block,  ScopeTree::InvalidNodeIndex, 10, 200, global);

        std::vector<Symbol> syms(3);
        syms[0].name = "x"; syms[0].kind = SymbolKind::Variable;
        syms[1].name = "y"; syms[1].kind = SymbolKind::Variable;
        syms[2].name = "x"; syms[2].kind = SymbolKind::Variable; // shadows sym0

        tree.addSymbol(global, 0);
        tree.addSymbol(global, 1);
        tree.addSymbol(block,  2);

        ScopeVisibility vis = ScopeVisibility::compute(tree, syms);

        // Global: "x" → sym0, "y" → sym1
        ok &= check(vis.resolve(global, "x") == 0, "global resolves 'x' to sym0");
        ok &= check(vis.resolve(global, "y") == 1, "global resolves 'y' to sym1");

        // Block: "x" → sym2 (shadowed), "y" → sym1 (inherited)
        ok &= check(vis.resolve(block, "x") == 2, "block resolves 'x' to sym2 (shadows global)");
        ok &= check(vis.resolve(block, "y") == 1, "block inherits 'y' from global");

        // sym0 is NOT visible in block under name "x" — the shadow hides it
        ok &= check(!isVisible(vis, block, 0), "global 'x' (sym0) hidden in block by shadow");
        ok &= check(isVisible(vis, block, 2),  "shadow 'x' (sym2) is visible in block");

        ok &= check(vis.visibleIn(block).size() == 2, "block has 2 visible symbols (not 3)");

        return ok;
    }

    bool test_sibling_isolation_pure()
    {
        // Global
        //   NsA {sym0("a")}
        //   NsB {sym1("b")}
        // NsA must not see sym1; NsB must not see sym0
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global,    ScopeTree::InvalidNodeIndex, 0, 9999);
        uintptr_t nsA    = tree.add(ScopeKind::Namespace, ScopeTree::InvalidNodeIndex, 10, 200, global);
        uintptr_t nsB    = tree.add(ScopeKind::Namespace, ScopeTree::InvalidNodeIndex, 300, 500, global);

        std::vector<Symbol> syms(2);
        syms[0].name = "a"; syms[0].kind = SymbolKind::Variable;
        syms[1].name = "b"; syms[1].kind = SymbolKind::Variable;

        tree.addSymbol(nsA, 0);
        tree.addSymbol(nsB, 1);

        ScopeVisibility vis = ScopeVisibility::compute(tree, syms);

        ok &= check(!isVisible(vis, global, 0), "global does not see NsA's sym");
        ok &= check(!isVisible(vis, global, 1), "global does not see NsB's sym");
        ok &= check(isVisible(vis, nsA, 0),     "NsA sees its own sym");
        ok &= check(!isVisible(vis, nsA, 1),    "NsA does NOT see NsB's sym");
        ok &= check(isVisible(vis, nsB, 1),     "NsB sees its own sym");
        ok &= check(!isVisible(vis, nsB, 0),    "NsB does NOT see NsA's sym");

        return ok;
    }

    bool test_deep_nesting_pure()
    {
        // Global {sym0("g")}
        //   Class {sym1("m")}
        //     Method {sym2("p")}
        //       Block {sym3("l")}
        // Block should see all 4 symbols
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global,   ScopeTree::InvalidNodeIndex, 0, 9999);
        uintptr_t cls    = tree.add(ScopeKind::Class,    ScopeTree::InvalidNodeIndex, 10, 800, global);
        uintptr_t method = tree.add(ScopeKind::Method,   ScopeTree::InvalidNodeIndex, 20, 700, cls);
        uintptr_t block  = tree.add(ScopeKind::Block,    ScopeTree::InvalidNodeIndex, 30, 600, method);

        std::vector<Symbol> syms(4);
        syms[0].name = "g"; syms[0].kind = SymbolKind::Variable;
        syms[1].name = "m"; syms[1].kind = SymbolKind::Variable;
        syms[2].name = "p"; syms[2].kind = SymbolKind::Variable;
        syms[3].name = "l"; syms[3].kind = SymbolKind::Variable;

        tree.addSymbol(global, 0);
        tree.addSymbol(cls,    1);
        tree.addSymbol(method, 2);
        tree.addSymbol(block,  3);

        ScopeVisibility vis = ScopeVisibility::compute(tree, syms);

        ok &= check(vis.visibleIn(global).size() == 1, "global sees 1 symbol");
        ok &= check(vis.visibleIn(cls).size()    == 2, "class sees 2 symbols");
        ok &= check(vis.visibleIn(method).size() == 3, "method sees 3 symbols");
        ok &= check(vis.visibleIn(block).size()  == 4, "block sees all 4 symbols");

        ok &= check(vis.resolve(block, "g") == 0, "block resolves global symbol");
        ok &= check(vis.resolve(block, "m") == 1, "block resolves class member");
        ok &= check(vis.resolve(block, "p") == 2, "block resolves method parameter");
        ok &= check(vis.resolve(block, "l") == 3, "block resolves local variable");

        ok &= check(vis.resolve(global, "m") == ScopeVisibility::InvalidSymbol, "global cannot see class member");
        ok &= check(vis.resolve(cls,    "l") == ScopeVisibility::InvalidSymbol, "class cannot see local variable");

        return ok;
    }

    bool test_resolve_unknown_name()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 100);
        std::vector<Symbol> syms;

        ScopeVisibility vis = ScopeVisibility::compute(tree, syms);

        ok &= check(vis.resolve(global, "unknown") == ScopeVisibility::InvalidSymbol,
                    "resolving unknown name returns InvalidSymbol");
        ok &= check(vis.resolve(ScopeTree::InvalidId, "x") == ScopeVisibility::InvalidSymbol,
                    "resolving in invalid scope returns InvalidSymbol");
        ok &= check(vis.visibleIn(ScopeTree::InvalidId).empty(),
                    "visibleIn invalid scope returns empty");

        return ok;
    }

    // ------------------------------------------------------------------
    // Integration tests — real parsed C++ files
    // ------------------------------------------------------------------

    bool test_nested_ns_inherits()
    {
        bool ok = true;
        AST ast = parse("test/ast-scope-visibility/samples/nested_ns_visibility.cpp");
        ok &= check(!!ast, "nested_ns_visibility.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);
        std::vector<Symbol> syms = extract_symbols(ast);
        associate_symbols(tree, syms);
        ScopeVisibility vis = ScopeVisibility::compute(tree, syms);

        size_t outerSymIdx = symbolIndex(syms, "Outer::outerSym", SymbolKind::Variable);
        size_t innerSymIdx = symbolIndex(syms, "Outer::Inner::innerSym", SymbolKind::Variable);
        ok &= check(outerSymIdx != ScopeVisibility::InvalidSymbol, "outerSym exists");
        ok &= check(innerSymIdx != ScopeVisibility::InvalidSymbol, "innerSym exists");

        // Outer namespace: sees outerSym; does NOT see innerSym (child scope)
        uintptr_t outerNs = nthScopeOfKind(tree, ScopeKind::Namespace, 0);
        if(outerSymIdx != ScopeVisibility::InvalidSymbol) {
            ok &= check(isVisible(vis, outerNs, outerSymIdx), "Outer sees outerSym");
        }
        if(innerSymIdx != ScopeVisibility::InvalidSymbol) {
            ok &= check(!isVisible(vis, outerNs, innerSymIdx), "Outer does NOT see innerSym (child scope)");
        }

        // Inner namespace: sees BOTH outerSym (inherited) and innerSym (own)
        uintptr_t innerNs = nthScopeOfKind(tree, ScopeKind::Namespace, 1);
        if(outerSymIdx != ScopeVisibility::InvalidSymbol) {
            ok &= check(isVisible(vis, innerNs, outerSymIdx), "Inner inherits outerSym from Outer");
        }
        if(innerSymIdx != ScopeVisibility::InvalidSymbol) {
            ok &= check(isVisible(vis, innerNs, innerSymIdx), "Inner sees its own innerSym");
        }

        return ok;
    }

    bool test_shadowing_integration()
    {
        bool ok = true;
        AST ast = parse("test/ast-scope-visibility/samples/shadowing.cpp");
        ok &= check(!!ast, "shadowing.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);
        std::vector<Symbol> syms = extract_symbols(ast);
        associate_symbols(tree, syms);
        ScopeVisibility vis = ScopeVisibility::compute(tree, syms);

        // Global "value" and Ns "value" — same unqualified name
        size_t globalValIdx = symbolIndex(syms, "value",    SymbolKind::Variable);
        size_t nsValIdx     = symbolIndex(syms, "Ns::value", SymbolKind::Variable);
        ok &= check(globalValIdx != ScopeVisibility::InvalidSymbol, "global value exists");
        ok &= check(nsValIdx     != ScopeVisibility::InvalidSymbol, "Ns::value exists");

        uintptr_t globalScope = nthScopeOfKind(tree, ScopeKind::Global);
        uintptr_t nsScope     = nthScopeOfKind(tree, ScopeKind::Namespace);

        if(globalValIdx != ScopeVisibility::InvalidSymbol) {
            ok &= check(vis.resolve(globalScope, "value") == globalValIdx,
                        "global scope resolves 'value' to global symbol");
        }
        if(nsValIdx != ScopeVisibility::InvalidSymbol && globalValIdx != ScopeVisibility::InvalidSymbol) {
            ok &= check(vis.resolve(nsScope, "value") == nsValIdx,
                        "Ns scope resolves 'value' to its own (shadows global)");
            ok &= check(!isVisible(vis, nsScope, globalValIdx),
                        "global 'value' is hidden in Ns scope");
        }

        return ok;
    }

    bool test_sibling_isolation_integration()
    {
        bool ok = true;
        AST ast = parse("test/ast-scope-visibility/samples/sibling_isolation.cpp");
        ok &= check(!!ast, "sibling_isolation.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);
        std::vector<Symbol> syms = extract_symbols(ast);
        associate_symbols(tree, syms);
        ScopeVisibility vis = ScopeVisibility::compute(tree, syms);

        size_t globalIdx = symbolIndex(syms, "globalSym", SymbolKind::Variable);
        size_t symAIdx   = symbolIndex(syms, "NsA::symA", SymbolKind::Variable);
        size_t symBIdx   = symbolIndex(syms, "NsB::symB", SymbolKind::Variable);
        ok &= check(globalIdx != ScopeVisibility::InvalidSymbol, "globalSym exists");
        ok &= check(symAIdx   != ScopeVisibility::InvalidSymbol, "symA exists");
        ok &= check(symBIdx   != ScopeVisibility::InvalidSymbol, "symB exists");

        uintptr_t nsA = nthScopeOfKind(tree, ScopeKind::Namespace, 0);
        uintptr_t nsB = nthScopeOfKind(tree, ScopeKind::Namespace, 1);

        // Both namespaces inherit globalSym from Global
        if(globalIdx != ScopeVisibility::InvalidSymbol) {
            ok &= check(isVisible(vis, nsA, globalIdx), "NsA inherits globalSym");
            ok &= check(isVisible(vis, nsB, globalIdx), "NsB inherits globalSym");
        }

        // Sibling isolation: NsA cannot see NsB's symB and vice versa
        if(symAIdx != ScopeVisibility::InvalidSymbol) {
            ok &= check(isVisible(vis, nsA, symAIdx),  "NsA sees its own symA");
            ok &= check(!isVisible(vis, nsB, symAIdx), "NsB does NOT see NsA::symA");
        }
        if(symBIdx != ScopeVisibility::InvalidSymbol) {
            ok &= check(isVisible(vis, nsB, symBIdx),  "NsB sees its own symB");
            ok &= check(!isVisible(vis, nsA, symBIdx), "NsA does NOT see NsB::symB");
        }

        return ok;
    }

    bool test_visible_set_determinism()
    {
        // Calling compute twice on the same inputs must yield identical results
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global,    ScopeTree::InvalidNodeIndex, 0, 9999);
        uintptr_t ns     = tree.add(ScopeKind::Namespace, ScopeTree::InvalidNodeIndex, 10, 500, global);

        std::vector<Symbol> syms(2);
        syms[0].name = "x"; syms[0].kind = SymbolKind::Variable;
        syms[1].name = "y"; syms[1].kind = SymbolKind::Variable;
        tree.addSymbol(global, 0);
        tree.addSymbol(ns,     1);

        ScopeVisibility vis1 = ScopeVisibility::compute(tree, syms);
        ScopeVisibility vis2 = ScopeVisibility::compute(tree, syms);

        ok &= check(vis1.visibleIn(ns) == vis2.visibleIn(ns), "visible set is deterministic");

        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_scope_visibility()
{
    static const TestCase cases[] = {
        {"basic inheritance (pure)",      test_basic_inheritance},
        {"shadowing (pure)",              test_shadowing},
        {"sibling isolation (pure)",      test_sibling_isolation_pure},
        {"deep nesting (pure)",           test_deep_nesting_pure},
        {"resolve unknown name",          test_resolve_unknown_name},
        {"nested ns inherits (file)",     test_nested_ns_inherits},
        {"shadowing (file)",              test_shadowing_integration},
        {"sibling isolation (file)",      test_sibling_isolation_integration},
        {"visible set is deterministic",  test_visible_set_determinism},
    };

    std::cout << "=== scope visibility tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
