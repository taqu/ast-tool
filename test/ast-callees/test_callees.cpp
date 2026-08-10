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

    const WorkspaceSymbol* findByName(const Workspace& ws, std::string_view name)
    {
        for(const auto& sym : ws.symbols) {
            if(sym.symbol.name == name) return &sym;
        }
        return nullptr;
    }

    const WorkspaceSymbol* findByNameInFile(const Workspace& ws,
                                             std::string_view name,
                                             std::string_view fileHint)
    {
        for(const auto& sym : ws.symbols) {
            if(sym.symbol.name == name &&
               sym.sourceFile.find(fileHint) != std::string::npos)
                return &sym;
        }
        return nullptr;
    }

    const WorkspaceSymbol* findByFQN(const Workspace& ws, std::string_view fqn)
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
        Workspace ws = analyze_workspace(kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, "scSource");
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
        ok &= check(s.caller->symbol.name == "scSource",  "caller name is scSource");
        ok &= check(s.callee->symbol.name == "scTarget",  "callee name is scTarget");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Single callee

    bool test_single_callee()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, "scSource");
        ok &= check(caller != nullptr, "scSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 1, "scSource has exactly 1 callee");
        if(!sites.empty())
            ok &= check(sites[0].callee->symbol.name == "scTarget", "callee is scTarget");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Multiple callees

    bool test_multiple_callees()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, "mcSource");
        ok &= check(caller != nullptr, "mcSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 2, "mcSource has 2 callees");

        bool foundAlpha = false, foundBeta = false;
        for(const auto& s : sites) {
            if(s.callee && s.callee->symbol.name == "mcAlpha") foundAlpha = true;
            if(s.callee && s.callee->symbol.name == "mcBeta")  foundBeta  = true;
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
        Workspace ws = analyze_workspace(kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, "ncNestedSource");
        ok &= check(caller != nullptr, "ncNestedSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 2, "ncNestedSource has 2 direct callees (ncPrint and ncGet)");

        bool foundPrint = false, foundGet = false;
        for(const auto& s : sites) {
            if(s.callee && s.callee->symbol.name == "ncPrint") foundPrint = true;
            if(s.callee && s.callee->symbol.name == "ncGet")   foundGet   = true;
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
        Workspace ws = analyze_workspace(kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, "ncTransitive");
        ok &= check(caller != nullptr, "ncTransitive found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 1, "ncTransitive has exactly 1 callee (not transitive)");
        if(!sites.empty())
            ok &= check(sites[0].callee->symbol.name == "ncNestedSource",
                        "callee of ncTransitive is ncNestedSource only");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Function with no callees

    bool test_no_callees()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, "noCallee");
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
        Workspace ws = analyze_workspace(kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, "recCe");
        ok &= check(caller != nullptr, "recCe found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 1, "recCe has exactly 1 callee (itself)");
        if(!sites.empty()) {
            ok &= check(sites[0].callee != nullptr, "callee pointer is set");
            if(sites[0].callee)
                ok &= check(sites[0].callee->symbol.name == "recCe",
                            "recursive: callee is recCe itself");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Namespace-qualified callee

    bool test_namespace_callee()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, "nsCeSource");
        ok &= check(caller != nullptr, "nsCeSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 1, "nsCeSource has 1 callee");
        if(!sites.empty()) {
            ok &= check(sites[0].callee != nullptr, "callee pointer is set");
            if(sites[0].callee)
                ok &= check(sites[0].callee->symbol.fqn == "NsCe::nsTarget",
                            "callee FQN is NsCe::nsTarget");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Member function callee

    bool test_member_callee()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kCeRoot);
        const WorkspaceSymbol* caller = findByFQN(ws, "MbCe::mbSource");
        ok &= check(caller != nullptr, "MbCe::mbSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(sites.size() == 1, "mbSource has 1 callee");
        if(!sites.empty()) {
            ok &= check(sites[0].callee != nullptr, "callee pointer is set");
            if(sites[0].callee)
                ok &= check(sites[0].callee->symbol.fqn == "MbCe::mbTarget",
                            "callee FQN is MbCe::mbTarget");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Overloaded functions
    // Extractor deduplicates overloads by FQN; the resolver finds the single
    // kept symbol — 1 callee is returned for the caller.

    bool test_overloaded()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kCeRoot);

        // Extractor keeps only the first ovCeTarget overload; callee is found.
        const WorkspaceSymbol* ambigCaller = findByName(ws, "ovCeAmbig");
        ok &= check(ambigCaller != nullptr, "ovCeAmbig found");
        if(ambigCaller) {
            Callees callees;
            auto sites = callees.find(ws, *ambigCaller);
            ok &= check(sites.size() == 1,
                        "ovCeAmbig has 1 callee (deduplicated ovCeTarget)");
        }

        // Unique: ovCeNsSource calls ovCeNsTarget (no ambiguity within namespace).
        const WorkspaceSymbol* nsCaller = findByFQN(ws, "OvCeNs::ovCeNsSource");
        ok &= check(nsCaller != nullptr, "OvCeNs::ovCeNsSource found");
        if(nsCaller) {
            Callees callees;
            auto sites = callees.find(ws, *nsCaller);
            ok &= check(sites.size() == 1, "ovCeNsSource has 1 callee");
            if(!sites.empty() && sites[0].callee)
                ok &= check(sites[0].callee->symbol.fqn == "OvCeNs::ovCeNsTarget",
                            "callee is OvCeNs::ovCeNsTarget");
        }

        return ok;
    }

    // -----------------------------------------------------------------------
    // Unresolved call — silently skipped, no false positives

    bool test_unresolved_call()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, "unresCeSource");
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
        Workspace ws = analyze_workspace(kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, "xfCeSource");
        ok &= check(caller != nullptr, "xfCeSource found");
        if(!caller) return false;

        Callees callees;
        auto sites = callees.find(ws, *caller);

        ok &= check(!sites.empty(), "xfCeSource has at least one callee");
        bool foundTarget = false;
        for(const auto& s : sites) {
            if(s.callee && s.callee->symbol.name == "xfCeTarget") {
                foundTarget = true;
                // The callee's definition is in xfile_def.cpp.
                ok &= check(s.callee->sourceFile.find("xfile_def") != std::string::npos,
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
        Workspace ws = analyze_workspace(kCeRoot);
        const WorkspaceSymbol* caller = findByName(ws, "mcSource");
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
