#include "test_resolver.h"
#include "ast-extractor.h"
#include "ast-resolver.h"
#include "ast-scope.h"
#include "ast-workspace.h"
#include <iostream>
#include <string_view>

namespace ast
{
namespace
{
    static constexpr const char* kWorkspaceRoot = "test/ast-workspace/workspace";

    bool check(bool condition, const char* description)
    {
        if(!condition)
            std::cerr << "    FAIL: " << description << "\n";
        return condition;
    }

    Symbol makeVar(const char* name)
    {
        Symbol s;
        s.name = name;
        s.fqn  = name;
        s.kind = SymbolKind::Variable;
        return s;
    }

    Symbol makeFunc(const char* name)
    {
        Symbol s;
        s.name = name;
        s.fqn  = name;
        s.kind = SymbolKind::Function;
        return s;
    }

    // Wire up the symbol-to-scope reverse map so to_workspace_symbol() can
    // find owningScope without a linear scan.
    void wire_symbol_map(ScopeTree& tree,
                         const std::vector<std::pair<uintptr_t, size_t>>& pairs,
                         size_t symbolCount)
    {
        tree.reserveSymbolMap(symbolCount);
        for(const auto& [scopeId, symIdx] : pairs) {
            tree.setScopeOfSymbol(symIdx, scopeId);
        }
    }

    // -----------------------------------------------------------------------
    // ResolutionResult factory / status tests

    bool test_factories()
    {
        bool ok = true;

        auto u = ResolutionResult::make_unresolved();
        ok &= check(u.unresolved(),        "make_unresolved → unresolved()");
        ok &= check(!u.resolved(),         "make_unresolved → !resolved()");
        ok &= check(!u.ambiguous(),        "make_unresolved → !ambiguous()");
        ok &= check(u.candidates.empty(), "make_unresolved → candidates empty");

        WorkspaceSymbol ws;
        ws.symbol.name = "foo"; ws.symbol.fqn = "foo"; ws.symbol.kind = SymbolKind::Variable;
        auto r = ResolutionResult::make_resolved(ws);
        ok &= check(r.resolved(),                      "make_resolved → resolved()");
        ok &= check(!r.unresolved(),                   "make_resolved → !unresolved()");
        ok &= check(r.candidates.size() == 1,          "make_resolved → 1 candidate");
        ok &= check(r.symbol().symbol.name == "foo",   "make_resolved → symbol name correct");

        WorkspaceSymbol w2; w2.symbol.name = "bar"; w2.symbol.fqn = "bar";
        auto a = ResolutionResult::make_ambiguous({ws, w2});
        ok &= check(a.ambiguous(),              "make_ambiguous → ambiguous()");
        ok &= check(!a.resolved(),              "make_ambiguous → !resolved()");
        ok &= check(a.candidates.size() == 2,   "make_ambiguous → 2 candidates");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Per-file resolution — single symbol

    bool test_resolve_resolved()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 1000);

        std::vector<Symbol> syms = { makeVar("x") };
        tree.addSymbol(global, 0);
        wire_symbol_map(tree, {{global, 0}}, 1);

        IdentifierResolver resolver(tree, syms, "test.cpp");
        auto r = resolver.resolve("x", global);
        ok &= check(r.resolved(),                                        "single match → Resolved");
        ok &= check(r.symbol().symbol.name == "x",                       "resolved symbol name");
        ok &= check(r.symbol().symbol.kind == SymbolKind::Variable,      "resolved symbol kind");
        ok &= check(r.symbol().sourceFile == "test.cpp",                 "resolved symbol sourceFile");
        ok &= check(r.symbol().owningScope == ScopeKind::Global,         "resolved symbol owningScope");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Per-file resolution — no match

    bool test_resolve_unresolved()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 100);
        std::vector<Symbol> syms = { makeVar("x") };
        tree.addSymbol(global, 0);

        IdentifierResolver resolver(tree, syms, "test.cpp");
        auto r = resolver.resolve("nope", global);
        ok &= check(r.unresolved(),          "unknown name → Unresolved");
        ok &= check(r.candidates.empty(),    "Unresolved has no candidates");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Per-file resolution — overload set → Ambiguous

    bool test_resolve_ambiguous_overloads()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 1000);

        std::vector<Symbol> syms = { makeFunc("foo"), makeFunc("foo") };
        tree.addSymbol(global, 0);
        tree.addSymbol(global, 1);
        wire_symbol_map(tree, {{global, 0}, {global, 1}}, 2);

        IdentifierResolver resolver(tree, syms, "test.cpp");
        auto r = resolver.resolve("foo", global);
        ok &= check(r.ambiguous(),              "overload set → Ambiguous");
        ok &= check(r.candidates.size() == 2,   "two overload candidates");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Shadowing — inner declaration hides outer

    bool test_resolve_shadowing()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t block  = tree.add(ScopeKind::Block,  ScopeTree::InvalidNodeIndex, 10, 500, global);

        // sym0 = "z" in global; sym1 = "z" in block (shadows)
        std::vector<Symbol> syms = { makeVar("z"), makeVar("z") };
        tree.addSymbol(global, 0);
        tree.addSymbol(block,  1);
        wire_symbol_map(tree, {{global, 0}, {block, 1}}, 2);

        IdentifierResolver resolver(tree, syms, "test.cpp");

        // From block: sees inner "z" (sym1)
        auto inner = resolver.resolve("z", block);
        ok &= check(inner.resolved(),           "inner scope resolves to Resolved");
        ok &= check(inner.symbol().owningScope == ScopeKind::Block,
                    "inner resolution: owningScope is Block");

        // From global: sees outer "z" (sym0)
        auto outer = resolver.resolve("z", global);
        ok &= check(outer.resolved(),           "outer scope resolves to Resolved");
        ok &= check(outer.symbol().owningScope == ScopeKind::Global,
                    "outer resolution: owningScope is Global");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Nested scopes — symbol found in grandparent

    bool test_resolve_nested_scopes()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global,    ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t ns     = tree.add(ScopeKind::Namespace, ScopeTree::InvalidNodeIndex, 10, 5000, global);
        uintptr_t fn     = tree.add(ScopeKind::Function,  ScopeTree::InvalidNodeIndex, 20, 1000, ns);
        uintptr_t block  = tree.add(ScopeKind::Block,     ScopeTree::InvalidNodeIndex, 30, 500, fn);

        std::vector<Symbol> syms = { makeVar("g") };
        tree.addSymbol(global, 0);
        wire_symbol_map(tree, {{global, 0}}, 1);

        IdentifierResolver resolver(tree, syms, "test.cpp");
        auto r = resolver.resolve("g", block);
        ok &= check(r.resolved(),                      "deep nested block finds 'g' in global");
        ok &= check(r.symbol().owningScope == ScopeKind::Global, "owningScope is Global");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Namespace scope lookup

    bool test_resolve_namespace()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global,    ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t ns     = tree.add(ScopeKind::Namespace, ScopeTree::InvalidNodeIndex, 10, 8000, global);

        std::vector<Symbol> syms = { makeVar("nsVar") };
        tree.addSymbol(ns, 0);
        wire_symbol_map(tree, {{ns, 0}}, 1);

        IdentifierResolver resolver(tree, syms, "test.cpp");

        auto from_ns = resolver.resolve("nsVar", ns);
        ok &= check(from_ns.resolved(), "lookup from inside namespace finds nsVar");
        ok &= check(from_ns.symbol().owningScope == ScopeKind::Namespace,
                    "owningScope is Namespace");

        // Not visible from outside the namespace
        auto from_global = resolver.resolve("nsVar", global);
        ok &= check(from_global.unresolved(), "nsVar not visible from global scope");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Class member from method scope

    bool test_resolve_class_member()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t cls    = tree.add(ScopeKind::Class,  ScopeTree::InvalidNodeIndex, 10, 5000, global);
        uintptr_t method = tree.add(ScopeKind::Method, ScopeTree::InvalidNodeIndex, 20, 400, cls);

        Symbol field; field.name = "m_val"; field.fqn = "MyClass::m_val";
        field.kind = SymbolKind::Field;

        std::vector<Symbol> syms = { field };
        tree.addSymbol(cls, 0);
        wire_symbol_map(tree, {{cls, 0}}, 1);

        IdentifierResolver resolver(tree, syms, "test.cpp");
        auto r = resolver.resolve("m_val", method);
        ok &= check(r.resolved(),                                 "method scope finds class member");
        ok &= check(r.symbol().symbol.kind == SymbolKind::Field,  "kind is Field");
        ok &= check(r.symbol().owningScope == ScopeKind::Class,   "owningScope is Class");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Local variable from nested block (parameter + block scope)

    bool test_resolve_local_variable()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global,   ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t fn     = tree.add(ScopeKind::Function, ScopeTree::InvalidNodeIndex, 10, 500, global);
        uintptr_t block  = tree.add(ScopeKind::Block,    ScopeTree::InvalidNodeIndex, 20, 400, fn);

        // param in function scope, local var in block
        Symbol param; param.name = "n"; param.fqn = "n"; param.kind = SymbolKind::Variable;
        Symbol local; local.name = "tmp"; local.fqn = "tmp"; local.kind = SymbolKind::Variable;

        std::vector<Symbol> syms = { param, local };
        tree.addSymbol(fn,    0);
        tree.addSymbol(block, 1);
        wire_symbol_map(tree, {{fn, 0}, {block, 1}}, 2);

        IdentifierResolver resolver(tree, syms, "test.cpp");

        auto rn   = resolver.resolve("n",   block);
        auto rtmp = resolver.resolve("tmp", block);
        ok &= check(rn.resolved()   && rn.symbol().symbol.name == "n",    "block finds parameter 'n'");
        ok &= check(rtmp.resolved() && rtmp.symbol().symbol.name == "tmp", "block finds local 'tmp'");
        ok &= check(rn.symbol().owningScope   == ScopeKind::Function, "'n' owned by Function");
        ok &= check(rtmp.symbol().owningScope == ScopeKind::Block,    "'tmp' owned by Block");
        return ok;
    }

    // -----------------------------------------------------------------------
    // InvalidId fromScope → skip lexical lookup

    bool test_resolve_invalid_scope()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 100);
        std::vector<Symbol> syms = { makeVar("x") };
        tree.addSymbol(global, 0);

        IdentifierResolver resolver(tree, syms, "test.cpp");
        auto r = resolver.resolve("x", ScopeTree::InvalidId);
        ok &= check(r.unresolved(), "InvalidId fromScope → Unresolved (no workspace)");
        return ok;
    }

    // -----------------------------------------------------------------------
    // resolve_global — workspace symbol table

    bool test_resolve_global_resolved()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kWorkspaceRoot);
        ScopeTree emptyTree;
        emptyTree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 0);
        std::vector<Symbol> noSyms;

        IdentifierResolver resolver(emptyTree, noSyms, "other.cpp", ws);
        auto r = resolver.resolve_global("alphaVar");
        ok &= check(r.resolved(),                                          "alphaVar found globally");
        ok &= check(r.symbol().symbol.name == "alphaVar",                  "global result name correct");
        ok &= check(r.symbol().symbol.kind == SymbolKind::Variable,        "global result kind correct");
        ok &= check(!r.symbol().sourceFile.empty(),                        "global result has sourceFile");
        return ok;
    }

    bool test_resolve_global_unresolved()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kWorkspaceRoot);
        ScopeTree emptyTree;
        emptyTree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 0);
        std::vector<Symbol> noSyms;

        IdentifierResolver resolver(emptyTree, noSyms, "other.cpp", ws);
        auto r = resolver.resolve_global("zzz_no_such_symbol_xyz");
        ok &= check(r.unresolved(), "unknown name → Unresolved in global lookup");
        return ok;
    }

    bool test_resolve_global_no_workspace()
    {
        bool ok = true;
        ScopeTree tree;
        tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 0);
        std::vector<Symbol> noSyms;

        IdentifierResolver resolver(tree, noSyms, "test.cpp");
        auto r = resolver.resolve_global("alphaVar");
        ok &= check(r.unresolved(), "resolve_global without workspace → Unresolved");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Cross-file fallback: not in local scope → falls back to workspace

    bool test_resolve_cross_file_fallback()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kWorkspaceRoot);

        // Local scope tree has no symbols — simulates a different file
        ScopeTree emptyTree;
        uintptr_t global = emptyTree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 0);
        std::vector<Symbol> noSyms;

        IdentifierResolver resolver(emptyTree, noSyms, "other.cpp", ws);

        // "alphaFunc" is not in the local scope, but is in the workspace
        auto r = resolver.resolve("alphaFunc", global);
        ok &= check(r.resolved(),                                         "cross-file fallback resolves alphaFunc");
        ok &= check(r.symbol().symbol.name == "alphaFunc",                "cross-file result name");
        ok &= check(r.symbol().symbol.kind == SymbolKind::Function,       "cross-file result kind");

        // Truly unknown name → Unresolved even with workspace
        auto ru = resolver.resolve("zzz_missing", global);
        ok &= check(ru.unresolved(), "unknown name with workspace → Unresolved");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Cross-file ambiguity: same unqualified name in multiple workspace files

    bool test_resolve_global_ambiguous()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kWorkspaceRoot);

        // Inject a duplicate symbol name to trigger Ambiguous
        WorkspaceSymbol dup;
        dup.symbol.name = "alphaVar"; dup.symbol.fqn = "DupNs::alphaVar";
        dup.symbol.kind = SymbolKind::Variable; dup.sourceFile = "dup.cpp";
        ws.symbols.push_back(dup);

        ScopeTree emptyTree;
        emptyTree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 0);
        std::vector<Symbol> noSyms;

        IdentifierResolver resolver(emptyTree, noSyms, "other.cpp", ws);
        auto r = resolver.resolve_global("alphaVar");
        ok &= check(r.ambiguous(),            "two 'alphaVar' symbols → Ambiguous");
        ok &= check(r.candidates.size() >= 2, "ambiguous has at least 2 candidates");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Metadata correctness for workspace-resolved symbols

    bool test_resolve_metadata()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kWorkspaceRoot);
        ScopeTree emptyTree;
        emptyTree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 0);
        std::vector<Symbol> noSyms;

        IdentifierResolver resolver(emptyTree, noSyms, "other.cpp", ws);
        auto r = resolver.resolve_global("BetaStruct");
        ok &= check(r.resolved(),                            "BetaStruct resolves");
        if(!r.resolved()) return false;
        const WorkspaceSymbol& sym = r.symbol();
        ok &= check(sym.symbol.name == "BetaStruct",         "metadata: name");
        ok &= check(sym.symbol.kind == SymbolKind::Struct,   "metadata: kind=Struct");
        ok &= check(!sym.sourceFile.empty(),                 "metadata: sourceFile set");
        ok &= check(sym.symbol.fqn == "BetaStruct",          "metadata: fqn");
        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_resolver()
{
    static const TestCase cases[] = {
        // ResolutionResult API
        {"result: factories and status",       test_factories},
        // Per-file resolution
        {"resolve: single match → Resolved",   test_resolve_resolved},
        {"resolve: no match → Unresolved",     test_resolve_unresolved},
        {"resolve: overloads → Ambiguous",     test_resolve_ambiguous_overloads},
        {"resolve: shadowing",                 test_resolve_shadowing},
        {"resolve: nested scopes",             test_resolve_nested_scopes},
        {"resolve: namespace scope",           test_resolve_namespace},
        {"resolve: class member",              test_resolve_class_member},
        {"resolve: local variable",            test_resolve_local_variable},
        {"resolve: InvalidId fromScope",       test_resolve_invalid_scope},
        // Workspace / cross-file
        {"global: resolved",                   test_resolve_global_resolved},
        {"global: unresolved",                 test_resolve_global_unresolved},
        {"global: no workspace",               test_resolve_global_no_workspace},
        {"global: cross-file fallback",        test_resolve_cross_file_fallback},
        {"global: ambiguous",                  test_resolve_global_ambiguous},
        {"global: metadata",                   test_resolve_metadata},
    };

    std::cout << "=== identifier resolver tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
