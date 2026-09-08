#include "test_lookup.h"
#include "ast-extractor.h"
#include "ast-lookup.h"
#include "ast-scope.h"
#include "ast-scope-visibility.h"
#include <algorithm>
#include <iostream>

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

    bool hasIdx(const std::vector<size_t>& v, size_t idx)
    {
        return std::find(v.begin(), v.end(), idx) != v.end();
    }

    /** Builds a Variable Symbol with only name set. */
    Symbol makeVar(const char8_t* name)
    {
        Symbol s;
        s.name = name;
        s.fqn  = name;
        s.kind = SymbolKind::Variable;
        return s;
    }

    /** Builds a Function Symbol with only name set. */
    Symbol makeFunc(const char8_t* name)
    {
        Symbol s;
        s.name = name;
        s.fqn  = name;
        s.kind = SymbolKind::Function;
        return s;
    }

    // -----------------------------------------------------------------------
    // Test 1: lookup finds a symbol declared in the current scope
    // -----------------------------------------------------------------------
    bool test_current_scope()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 100);

        std::vector<Symbol> syms = { makeVar(u8"x"), makeVar(u8"y") };
        tree.addSymbol(global, 0);
        tree.addSymbol(global, 1);

        auto rx = lookup(u8"x", global, tree, syms);
        auto ry = lookup(u8"y", global, tree, syms);
        ok &= check(rx.size() == 1 && rx[0] == 0, "lookup 'x' in current scope returns sym0");
        ok &= check(ry.size() == 1 && ry[0] == 1, "lookup 'y' in current scope returns sym1");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Test 2: lookup walks to parent when name not found in current scope
    // -----------------------------------------------------------------------
    bool test_parent_scope()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t ns     = tree.add(ScopeKind::Namespace, ScopeTree::InvalidNodeIndex, 10, 500, global);
        uintptr_t block  = tree.add(ScopeKind::Block, ScopeTree::InvalidNodeIndex, 20, 200, ns);

        std::vector<Symbol> syms = { makeVar(u8"g") };
        tree.addSymbol(global, 0);  // "g" only in global

        auto r = lookup(u8"g", block, tree, syms);
        ok &= check(r.size() == 1 && r[0] == 0, "lookup 'g' from block finds it in global (grandparent)");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Test 3: shadowing — inner declaration hides outer; outer NOT returned
    // -----------------------------------------------------------------------
    bool test_shadowing()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t block  = tree.add(ScopeKind::Block,  ScopeTree::InvalidNodeIndex, 10, 500, global);

        // sym0 = "z" in global; sym1 = "z" in block (shadows)
        std::vector<Symbol> syms = { makeVar(u8"z"), makeVar(u8"z") };
        tree.addSymbol(global, 0);
        tree.addSymbol(block,  1);

        auto r = lookup(u8"z", block, tree, syms);
        ok &= check(r.size() == 1,       "shadowing: exactly one result");
        ok &= check(hasIdx(r, 1),        "inner 'z' (sym1) is returned");
        ok &= check(!hasIdx(r, 0),       "outer 'z' (sym0) is shadowed, NOT returned");

        // Lookup from outer scope still sees sym0
        auto rg = lookup(u8"z", global, tree, syms);
        ok &= check(rg.size() == 1 && rg[0] == 0, "outer scope still resolves to sym0");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Test 4: missing name returns empty
    // -----------------------------------------------------------------------
    bool test_missing()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 100);

        std::vector<Symbol> syms = { makeVar(u8"x") };
        tree.addSymbol(global, 0);

        auto r = lookup(u8"nope", global, tree, syms);
        ok &= check(r.empty(), "lookup of unknown name returns empty");
        ok &= check(!contains(u8"nope", global, tree, syms), "contains returns false for unknown name");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Test 5: overloaded declarations — all overloads at the winning scope returned
    // -----------------------------------------------------------------------
    bool test_overloads()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t ns     = tree.add(ScopeKind::Namespace, ScopeTree::InvalidNodeIndex, 10, 500, global);

        // Two overloads of "foo" in namespace; "foo" also in global (shadowed by ns overloads)
        std::vector<Symbol> syms = {
            makeFunc(u8"foo"),  // sym0 — global "foo"
            makeFunc(u8"foo"),  // sym1 — ns overload 1
            makeFunc(u8"foo"),  // sym2 — ns overload 2
        };
        tree.addSymbol(global, 0);
        tree.addSymbol(ns,     1);
        tree.addSymbol(ns,     2);

        // Lookup from ns: both overloads in ns are found; global "foo" is shadowed
        auto r = lookup(u8"foo", ns, tree, syms);
        ok &= check(r.size() == 2,    "overload set: two results from namespace");
        ok &= check(hasIdx(r, 1),     "overload sym1 is in result");
        ok &= check(hasIdx(r, 2),     "overload sym2 is in result");
        ok &= check(!hasIdx(r, 0),    "global sym0 shadowed by ns overloads");

        // Lookup from a child block: reaches ns, still finds both overloads
        uintptr_t block = tree.add(ScopeKind::Block, ScopeTree::InvalidNodeIndex, 20, 300, ns);
        auto rb = lookup(u8"foo", block, tree, syms);
        ok &= check(rb.size() == 2,   "overloads visible from nested block");
        ok &= check(!hasIdx(rb, 0),   "global overload still shadowed from block");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Test 6: overloads in global scope (no shadowing involved)
    // -----------------------------------------------------------------------
    bool test_overloads_global()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 9000);

        std::vector<Symbol> syms = { makeFunc(u8"bar"), makeFunc(u8"bar"), makeVar(u8"baz") };
        tree.addSymbol(global, 0);
        tree.addSymbol(global, 1);
        tree.addSymbol(global, 2);

        auto r = lookup(u8"bar", global, tree, syms);
        ok &= check(r.size() == 2, "two overloads of 'bar' in global");
        ok &= check(hasIdx(r, 0) && hasIdx(r, 1), "both overload indices present");

        auto rz = lookup(u8"baz", global, tree, syms);
        ok &= check(rz.size() == 1 && rz[0] == 2, "non-overloaded 'baz' found");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Test 7: namespace lookup — symbol declared in namespace visible from within
    // -----------------------------------------------------------------------
    bool test_namespace_lookup()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t ns     = tree.add(ScopeKind::Namespace, ScopeTree::InvalidNodeIndex, 10, 800, global);

        std::vector<Symbol> syms = { makeVar(u8"nsVar") };
        tree.addSymbol(ns, 0);

        auto r = lookup(u8"nsVar", ns, tree, syms);
        ok &= check(r.size() == 1 && r[0] == 0, "lookup finds nsVar inside namespace");
        ok &= check(contains(u8"nsVar", ns, tree, syms), "contains returns true for nsVar in ns");
        ok &= check(!contains(u8"nsVar", global, tree, syms), "contains returns false for nsVar in global");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Test 8: class lookup — member symbol found in class scope
    // -----------------------------------------------------------------------
    bool test_class_lookup()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global,    ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t cls    = tree.add(ScopeKind::Class,     ScopeTree::InvalidNodeIndex, 10, 500, global);
        uintptr_t method = tree.add(ScopeKind::Method,    ScopeTree::InvalidNodeIndex, 20, 400, cls);

        std::vector<Symbol> syms = { makeVar(u8"field"), makeVar(u8"global_g") };
        tree.addSymbol(cls,    0);  // "field" in class
        tree.addSymbol(global, 1);  // "global_g" in global

        // Lookup "field" from method scope — found in class (parent)
        auto rf = lookup(u8"field", method, tree, syms);
        ok &= check(rf.size() == 1 && rf[0] == 0, "method scope finds class member 'field'");

        // Lookup "global_g" from method — found in global (grandparent)
        auto rg = lookup(u8"global_g", method, tree, syms);
        ok &= check(rg.size() == 1 && rg[0] == 1, "method scope finds global symbol 'global_g'");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Test 9: contains() using ScopeVisibility overload
    // -----------------------------------------------------------------------
    bool test_contains_visibility()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global,    ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t ns     = tree.add(ScopeKind::Namespace, ScopeTree::InvalidNodeIndex, 10, 500, global);

        std::vector<Symbol> syms = { makeVar(u8"alpha"), makeVar(u8"beta") };
        tree.addSymbol(global, 0);
        tree.addSymbol(ns,     1);

        ScopeVisibility vis = ScopeVisibility::compute(tree, syms);

        ok &= check(contains(u8"alpha", global, vis), "global contains 'alpha'");
        ok &= check(!contains(u8"beta", global, vis), "global does not contain 'beta' (child scope)");
        ok &= check(contains(u8"alpha", ns, vis),     "ns inherits 'alpha' from global");
        ok &= check(contains(u8"beta",  ns, vis),     "ns contains its own 'beta'");
        ok &= check(!contains(u8"gamma", ns, vis),    "ns does not contain unknown 'gamma'");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Test 10: nested blocks — each block only sees its own and ancestor symbols
    // -----------------------------------------------------------------------
    bool test_nested_blocks()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t b1     = tree.add(ScopeKind::Block,  ScopeTree::InvalidNodeIndex, 10, 800, global);
        uintptr_t b2     = tree.add(ScopeKind::Block,  ScopeTree::InvalidNodeIndex, 20, 600, b1);
        uintptr_t b3     = tree.add(ScopeKind::Block,  ScopeTree::InvalidNodeIndex, 30, 400, b2);

        std::vector<Symbol> syms = { makeVar(u8"a"), makeVar(u8"b"), makeVar(u8"c"), makeVar(u8"d") };
        tree.addSymbol(global, 0); // "a"
        tree.addSymbol(b1,     1); // "b"
        tree.addSymbol(b2,     2); // "c"
        tree.addSymbol(b3,     3); // "d"

        // b3 can see all four
        ok &= check(lookup(u8"a", b3, tree, syms).size() == 1, "b3 finds 'a' from global");
        ok &= check(lookup(u8"b", b3, tree, syms).size() == 1, "b3 finds 'b' from b1");
        ok &= check(lookup(u8"c", b3, tree, syms).size() == 1, "b3 finds 'c' from b2");
        ok &= check(lookup(u8"d", b3, tree, syms).size() == 1, "b3 finds 'd' in own scope");

        // global can only see "a"
        ok &= check(lookup(u8"b", global, tree, syms).empty(), "global cannot see 'b' (child scope)");
        ok &= check(lookup(u8"d", global, tree, syms).empty(), "global cannot see 'd' (deep child)");

        // b1 can see "a" and "b" but not "c" or "d"
        ok &= check(lookup(u8"a", b1, tree, syms).size() == 1, "b1 finds 'a'");
        ok &= check(lookup(u8"b", b1, tree, syms).size() == 1, "b1 finds its own 'b'");
        ok &= check(lookup(u8"c", b1, tree, syms).empty(), "b1 cannot see 'c' (b2 child)");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Test 11: lookup from InvalidId scope returns empty
    // -----------------------------------------------------------------------
    bool test_invalid_scope()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 100);
        std::vector<Symbol> syms = { makeVar(u8"x") };
        tree.addSymbol(global, 0);

        // Passing InvalidId as scope — the while loop condition fires immediately
        auto r = lookup(u8"x", ScopeTree::InvalidId, tree, syms);
        ok &= check(r.empty(), "lookup from InvalidId scope returns empty");

        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_lookup()
{
    static const TestCase cases[] = {
        {"current scope",             test_current_scope},
        {"parent scope",              test_parent_scope},
        {"shadowing",                 test_shadowing},
        {"missing symbol",            test_missing},
        {"overloads (namespace)",     test_overloads},
        {"overloads (global)",        test_overloads_global},
        {"namespace lookup",          test_namespace_lookup},
        {"class lookup",              test_class_lookup},
        {"contains (visibility)",     test_contains_visibility},
        {"nested blocks",             test_nested_blocks},
        {"invalid scope",             test_invalid_scope},
    };

    std::cout << "=== lexical name lookup tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
