#include "test_callees.h"
#include "ast-callees.h"
#include "ast-workspace.h"
#include <iostream>
#include <string_view>

namespace ast
{
namespace
{
    static constexpr const char* kCeRoot = "test/ast-callees/workspace";

    bool check(bool condition, const char* description)
    {
        if(!condition)
            std::cerr << "    FAIL: " << description << "\n";
        return condition;
    }

    const WorkspaceSymbol* findByName(const Workspace& ws, std::u8string_view name)
    {
        for(const auto& sym : ws.symbols) {
            if(sym.symbol.name == name) return &sym;
        }
        return nullptr;
    }

    const WorkspaceSymbol* findByNameInFile(const Workspace& ws,
                                             std::u8string_view name,
                                             std::u8string_view fileHint)
    {
        for(const auto& sym : ws.symbols) {
            std::u8string sourceFile = sym.sourceFile.u8string();
            if(sym.symbol.name == name &&
               sourceFile.find(fileHint) != std::string::npos)
                return &sym;
        }
        return nullptr;
    }

    const WorkspaceSymbol* findByFQN(const Workspace& ws, std::u8string_view fqn)
    {
        for(const auto& sym : ws.symbols) {
            if(sym.symbol.fqn == fqn) return &sym;
        }
        return nullptr;
    }

    // -----------------------------------------------------------------------
    // CallSite fields — basic validation

    bool test_callsite_fields()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, u8"scSource");
        ok &= check(caller != nullptr, "scSource found in workspace");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(!sites.empty(), "scSource has at least one call site");
        if(sites.empty()) return ok;

        const CallSite& s = sites[0];
        ok &= check(s.caller != nullptr,                  "caller pointer is set");
        ok &= check(s.callee != nullptr,                  "callee pointer is set");
        ok &= check(!s.sourceFile.empty(),                "sourceFile is non-empty");
        ok &= check(s.nodeIndex != size_t(-1),            "nodeIndex is populated");
        ok &= check(s.caller->symbol.name == u8"scSource",  "caller name is scSource");
        ok &= check(s.callee->symbol.name == u8"scTarget",  "callee name is scTarget");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Single callee

    bool test_single_callee()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, u8"scSource");
        ok &= check(caller != nullptr, "scSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 1, "scSource has exactly 1 callee");
        if(!sites.empty())
            ok &= check(sites[0].callee->symbol.name == u8"scTarget", "callee is scTarget");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Multiple callees

    bool test_multiple_callees()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, u8"mcSource");
        ok &= check(caller != nullptr, "mcSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 2, "mcSource has 2 callees");

        bool foundAlpha = false, foundBeta = false;
        for(const auto& s : sites) {
            if(s.callee && s.callee->symbol.name == u8"mcAlpha") foundAlpha = true;
            if(s.callee && s.callee->symbol.name == u8"mcBeta")  foundBeta  = true;
        }
        ok &= check(foundAlpha, "mcAlpha is a callee of mcSource");
        ok &= check(foundBeta,  "mcBeta  is a callee of mcSource");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Nested call expressions: foo(bar()) — both foo and bar are direct callees

    bool test_nested_calls()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, u8"ncNestedSource");
        ok &= check(caller != nullptr, "ncNestedSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 2, "ncNestedSource has 2 direct callees (ncPrint and ncGet)");

        bool foundPrint = false, foundGet = false;
        for(const auto& s : sites) {
            if(s.callee && s.callee->symbol.name == u8"ncPrint") foundPrint = true;
            if(s.callee && s.callee->symbol.name == u8"ncGet")   foundGet   = true;
        }
        ok &= check(foundPrint, "ncPrint is a callee of ncNestedSource");
        ok &= check(foundGet,   "ncGet   is a callee of ncNestedSource");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Traversal scope: only the target function body is visited
    // ncTransitive calls ncNestedSource; ncNestedSource calls ncPrint and ncGet.
    // The callees of ncTransitive should be {ncNestedSource} only, not ncPrint/ncGet.

    bool test_traversal_scope()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, u8"ncTransitive");
        ok &= check(caller != nullptr, "ncTransitive found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 1, "ncTransitive has exactly 1 callee (not transitive)");
        if(!sites.empty())
            ok &= check(sites[0].callee->symbol.name == u8"ncNestedSource",
                        "callee of ncTransitive is ncNestedSource only");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Function with no callees

    bool test_no_callees()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, u8"noCallee");
        ok &= check(caller != nullptr, "noCallee found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.empty(), "noCallee has zero callees");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Recursive function: callee is the function itself

    bool test_recursive()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, u8"recCe");
        ok &= check(caller != nullptr, "recCe found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 1, "recCe has exactly 1 callee (itself)");
        if(!sites.empty()) {
            ok &= check(sites[0].callee != nullptr, "callee pointer is set");
            if(sites[0].callee)
                ok &= check(sites[0].callee->symbol.name == u8"recCe",
                            "recursive: callee is recCe itself");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Namespace-qualified callee

    bool test_namespace_callee()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, u8"nsCeSource");
        ok &= check(caller != nullptr, "nsCeSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 1, "nsCeSource has 1 callee");
        if(!sites.empty()) {
            ok &= check(sites[0].callee != nullptr, "callee pointer is set");
            if(sites[0].callee)
                ok &= check(sites[0].callee->symbol.fqn == u8"NsCe::nsTarget",
                            "callee FQN is NsCe::nsTarget");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Member function callee

    bool test_member_callee()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        const WorkspaceSymbol* caller = findByFQN(ws, u8"MbCe::mbSource");
        ok &= check(caller != nullptr, "MbCe::mbSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 1, "mbSource has 1 callee");
        if(!sites.empty()) {
            ok &= check(sites[0].callee != nullptr, "callee pointer is set");
            if(sites[0].callee)
                ok &= check(sites[0].callee->symbol.fqn == u8"MbCe::mbTarget",
                            "callee FQN is MbCe::mbTarget");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Overloaded functions
    // Overloads remain separate. A call that cannot be disambiguated by the
    // current name-only resolver is omitted rather than assigned arbitrarily.

    bool test_overloaded()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);

        // Both overloads are retained, so the name-only call is ambiguous.
        const WorkspaceSymbol* ambigCaller = findByName(ws, u8"ovCeAmbig");
        ok &= check(ambigCaller != nullptr, "ovCeAmbig found");
        if(ambigCaller) {
            Callees callees;
            auto sites = callees.find(ws, *ambigCaller);
            ok &= check(sites.empty(),
                        "ovCeAmbig does not select an arbitrary overload");
        }

        // Unique: ovCeNsSource calls ovCeNsTarget (no ambiguity within namespace).
        const WorkspaceSymbol* nsCaller = findByFQN(ws, u8"OvCeNs::ovCeNsSource");
        ok &= check(nsCaller != nullptr, "OvCeNs::ovCeNsSource found");
        if(nsCaller) {
            Callees callees;
            auto sites = callees.find(ws, *nsCaller);
            ok &= check(sites.size() == 1, "ovCeNsSource has 1 callee");
            if(!sites.empty() && sites[0].callee)
                ok &= check(sites[0].callee->symbol.fqn == u8"OvCeNs::ovCeNsTarget",
                            "callee is OvCeNs::ovCeNsTarget");
        }

        return ok;
    }

    // -----------------------------------------------------------------------
    // Declaration/definition merge (Phase 3 — Semantic Symbol Resolution)
    // ddCeSource is declared in decl_def_callee.h and defined out-of-line in
    // decl_def_callee.cpp. Querying with the *declaration* occurrence (whose
    // own nodeIndex has no body) must still find the callee resolved from the
    // definition's body.

    bool test_decl_def_merge()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);

        const WorkspaceSymbol* declSite = findByNameInFile(ws, u8"ddCeSource", u8"decl_def_callee.h");
        ok &= check(declSite != nullptr, "ddCeSource header declaration found");
        if(declSite)
            ok &= check(!declSite->symbol.isDefinition, "ddCeSource header occurrence is a declaration");
        if(!declSite) return false;

        Callees callees;
        auto sites = callees.find(ws, *declSite);
        ok &= check(sites.size() == 1,
                    "querying via the header declaration finds the def-resolved callee");
        if(!sites.empty() && sites[0].callee)
            ok &= check(sites[0].callee->symbol.name == u8"ddCeTarget",
                        "ddCeSource callee is ddCeTarget");
        return ok;
    }

    // -----------------------------------------------------------------------
    // const/non-const method overloads must remain distinct logical symbols.

    bool test_const_overload_distinct()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);

        std::vector<const WorkspaceSymbol*> coRuns;
        for(const auto& sym : ws.symbols) {
            if(sym.symbol.fqn == u8"CoCeObj::coCeRun") coRuns.push_back(&sym);
        }
        ok &= check(coRuns.size() == 2,
                    "CoCeObj::coCeRun() and coCeRun() const remain 2 distinct symbols");
        if(coRuns.size() == 2)
            ok &= check(coRuns[0]->symbol.signature != coRuns[1]->symbol.signature,
                        "const and non-const overloads have different signatures");

        const WorkspaceSymbol* caller = findByFQN(ws, u8"coCeCaller");
        ok &= check(caller != nullptr, "coCeCaller found");
        if(caller) {
            Callees callees;
            auto sites = callees.find(ws, *caller);
            ok &= check(sites.empty(),
                        "ambiguous coCeRun call is not arbitrarily attributed to either overload");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Unresolved call — silently skipped, no false positives

    bool test_unresolved_call()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, u8"unresCeSource");
        ok &= check(caller != nullptr, "unresCeSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.empty(), "unresolved call: 0 callees found");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Cross-file callee

    bool test_cross_file_callee()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, u8"xfCeSource");
        ok &= check(caller != nullptr, "xfCeSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(!sites.empty(), "xfCeSource has at least one callee");
        bool foundTarget = false;
        for(const auto& s : sites) {
            if(s.callee && s.callee->symbol.name == u8"xfCeTarget") {
                foundTarget = true;
                std::u8string sourceFile = s.callee->sourceFile.u8string();
                // The callee's definition is in xfile_def.cpp.
                ok &= check(sourceFile.find(u8"xfile_def") != std::string::npos,
                            "cross-file callee source is xfile_def.cpp");
                break;
            }
        }
        ok &= check(foundTarget, "xfCeTarget is a callee of xfCeSource");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Result ordering: call sites appear in AST-node order within a function

    bool test_result_ordering()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, u8"mcSource");
        ok &= check(caller != nullptr, "mcSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() >= 2, "at least 2 call sites for ordering check");
        for(size_t i = 1; i < sites.size(); ++i)
            ok &= check(sites[i].nodeIndex > sites[i - 1].nodeIndex,
                        "call sites are in AST-node order");
        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_callees()
{
    static const TestCase cases[] = {
        {"CallSite fields populated correctly",             test_callsite_fields},
        {"single callee",                                   test_single_callee},
        {"multiple callees",                                test_multiple_callees},
        {"nested call expressions (foo(bar()))",            test_nested_calls},
        {"traversal scope: only target body visited",       test_traversal_scope},
        {"function with no callees",                        test_no_callees},
        {"recursive function (callee is self)",             test_recursive},
        {"namespace-qualified callee",                      test_namespace_callee},
        {"member function callee",                          test_member_callee},
        {"overloaded functions",                            test_overloaded},
        {"decl/def merge: header decl finds def-resolved callee", test_decl_def_merge},
        {"const/non-const overloads remain distinct",       test_const_overload_distinct},
        {"unresolved call: silently skipped",               test_unresolved_call},
        {"cross-file callee",                               test_cross_file_callee},
        {"result ordering within function",                 test_result_ordering},
    };

    std::cout << "=== callees tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
