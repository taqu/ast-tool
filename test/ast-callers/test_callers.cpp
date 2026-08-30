#include "test_callers.h"
#include "ast-callers.h"
#include "ast-workspace.h"
#include "cli-semantic.h"
#include <iostream>
#include <string_view>

namespace ast
{
namespace
{
    static constexpr const char* kCalRoot = "test/ast-callers/workspace";

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
               sourceFile.find(fileHint) != std::u8string::npos)
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
    // CallSite struct — basic field checks

    bool test_callsite_fields()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"calTarget");
        ok &= check(target != nullptr, "calTarget found in workspace");
        if(!target) return false;

        Callers callers(ws);
        auto sites = callers.find(*target);

        ok &= check(!sites.empty(), "calTarget has at least one call site");
        if(sites.empty()) return ok;

        const CallSite& s = sites[0];
        ok &= check(s.callee != nullptr,                    "callee pointer is set");
        ok &= check(!s.sourceFile.empty(),                  "sourceFile is non-empty");
        ok &= check(s.nodeIndex != size_t(-1),              "nodeIndex is populated");
        ok &= check(s.callee->symbol.name == u8"calTarget",   "callee name matches target");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Simple direct function call

    bool test_simple_call()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"calTarget");
        ok &= check(target != nullptr, "calTarget found");
        if(!target) return false;

        Callers callers(ws);
        auto sites = callers.find(*target);

        ok &= check(sites.size() == 1, "calTarget has exactly 1 call site");
        if(!sites.empty()) {
            std::u8string sourceFile = sites[0].sourceFile.u8string();
            ok &= check(sourceFile.find(u8"simple_call") != std::string::npos,
                        "call site is in simple_call.cpp");
            ok &= check(sites[0].caller != nullptr, "caller pointer is set");
            if(sites[0].caller)
                ok &= check(sites[0].caller->symbol.name == u8"callerFn",
                            "enclosing caller is callerFn");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Zero callers

    bool test_zero_callers()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"noCalTarget");
        ok &= check(target != nullptr, "noCalTarget found");
        if(!target) return false;

        Callers callers(ws);
        auto sites = callers.find(*target);

        ok &= check(sites.empty(), "noCalTarget has zero call sites");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Multiple callers

    bool test_multiple_callers()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"mcTarget");
        ok &= check(target != nullptr, "mcTarget found");
        if(!target) return false;

        Callers callers(ws);
        auto sites = callers.find(*target);

        ok &= check(sites.size() == 2, "mcTarget has 2 call sites");

        bool foundFirst  = false;
        bool foundSecond = false;
        for(const auto& s : sites) {
            if(s.caller && s.caller->symbol.name == u8"mcFirst")  foundFirst  = true;
            if(s.caller && s.caller->symbol.name == u8"mcSecond") foundSecond = true;
        }
        ok &= check(foundFirst,  "mcFirst  is a caller of mcTarget");
        ok &= check(foundSecond, "mcSecond is a caller of mcTarget");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Recursive function

    bool test_recursive()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"recTarget");
        ok &= check(target != nullptr, "recTarget found");
        if(!target) return false;

        Callers callers(ws);
        auto sites = callers.find(*target);

        ok &= check(sites.size() == 1, "recTarget has exactly 1 call site (itself)");
        if(!sites.empty()) {
            ok &= check(sites[0].caller != nullptr, "caller pointer is set");
            if(sites[0].caller)
                ok &= check(sites[0].caller->symbol.name == u8"recTarget",
                            "recursive call: caller is recTarget itself");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Namespace-qualified call

    bool test_namespace_call()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        const WorkspaceSymbol* target = findByFQN(ws, u8"NsCal::nsTarget");
        ok &= check(target != nullptr, "NsCal::nsTarget found");
        if(!target) return false;

        Callers callers(ws);
        auto sites = callers.find(*target);

        ok &= check(sites.size() == 1, "nsTarget has 1 call site");
        if(!sites.empty()) {
            std::u8string sourceFile = sites[0].sourceFile.u8string();
            ok &= check(sourceFile.find(u8"ns_call") != std::string::npos,
                        "call site is in ns_call.cpp");
            ok &= check(sites[0].caller != nullptr, "caller pointer is set");
            if(sites[0].caller)
                ok &= check(sites[0].caller->symbol.name == u8"nsCaller",
                            "caller is nsCaller");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Member function call

    bool test_member_call()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        const WorkspaceSymbol* target = findByFQN(ws, u8"MbObj::mbTarget");
        ok &= check(target != nullptr, "MbObj::mbTarget found");
        if(!target) return false;

        Callers callers(ws);
        auto sites = callers.find(*target);

        ok &= check(sites.size() == 1, "mbTarget has 1 call site");
        if(!sites.empty()) {
            std::u8string sourceFile = sites[0].sourceFile.u8string();
            ok &= check(sourceFile.find(u8"member_call") != std::string::npos,
                        "call site is in member_call.cpp");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Overloaded functions
    // The extractor deduplicates overloads by FQN, so only the first ovTarget
    // survives. The resolver finds exactly that one symbol — 1 caller is returned.

    bool test_overloaded()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);

        // Extractor keeps only the first overload (dedup by fqn); caller is found.
        const WorkspaceSymbol* ovAny = findByNameInFile(ws, u8"ovTarget", u8"overload");
        ok &= check(ovAny != nullptr, "at least one ovTarget overload found");
        if(ovAny) {
            Callers callers(ws);
            auto sites = callers.find(*ovAny);
            ok &= check(sites.size() == 1,
                        "ovTarget (deduplicated) has 1 caller");
        }

        // Unique function inside a namespace: resolved via class-scope lexical lookup.
        const WorkspaceSymbol* nsTarget = findByFQN(ws, u8"OvNs::ovNsTarget");
        ok &= check(nsTarget != nullptr, "OvNs::ovNsTarget found");
        if(nsTarget) {
            Callers callers(ws);
            auto sites = callers.find(*nsTarget);
            ok &= check(sites.size() == 1,
                        "OvNs::ovNsTarget has 1 caller (unique within namespace)");
            if(!sites.empty() && sites[0].caller)
                ok &= check(sites[0].caller->symbol.name == u8"ovNsCaller",
                            "OvNs::ovNsTarget caller is ovNsCaller");
        }

        return ok;
    }

    // -----------------------------------------------------------------------
    // Unresolved call — silently skipped, no false positives

    bool test_unresolved_call()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"unresDefined");
        ok &= check(target != nullptr, "unresDefined found");
        if(!target) return false;

        Callers callers(ws);
        auto sites = callers.find(*target);

        ok &= check(sites.empty(),
                    "unresolved calls produce no false positives for unresDefined");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Cross-file call

    bool test_cross_file()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        const WorkspaceSymbol* target = findByNameInFile(ws, u8"xfileTarget", u8"xfile_fn");
        ok &= check(target != nullptr, "xfileTarget found in xfile_fn.cpp");
        if(!target) return false;

        Callers callers(ws);
        auto sites = callers.find(*target);

        ok &= check(!sites.empty(), "xfileTarget has at least one call site");
        bool foundInCaller = false;
        for(const auto& s : sites) {
            std::u8string sourceFile = s.sourceFile.u8string();
            if(sourceFile.find(u8"xfile_caller") != std::string::npos) {
                foundInCaller = true;
                break;
            }
        }
        ok &= check(foundInCaller, "xfileTarget is called from xfile_caller.cpp");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Result ordering: call sites appear in AST-node order within a file

    bool test_result_ordering()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"mcTarget");
        ok &= check(target != nullptr, "mcTarget found");
        if(!target) return false;

        Callers callers(ws);
        auto sites = callers.find(*target);

        ok &= check(sites.size() >= 2, "at least 2 call sites for ordering check");
        for(size_t i = 1; i < sites.size(); ++i) {
            if(sites[i].sourceFile == sites[i - 1].sourceFile) {
                ok &= check(sites[i].nodeIndex > sites[i - 1].nodeIndex,
                            "call sites within a file are in AST-node order");
            }
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Declaration/definition deduplication:
    // DdCalClass::ddCalMethod is declared (kind=Method) in decl_def_class.h and
    // defined out-of-line (kind=Function) in decl_def_impl.cpp.  The resolver
    // must return exactly one candidate (not report ambiguity).

    bool test_decl_def_no_ambiguity()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"DdCalClass::ddCalMethod");
        ok &= check(candidates.size() == 1,
                    "DdCalClass::ddCalMethod resolves to exactly 1 candidate");
        if(!candidates.empty()) {
            ok &= check(candidates[0]->symbol.kind != SymbolKind::Function,
                        "resolved kind is Method, not the misclassified Function");
        }
        return ok;
    }

    bool test_decl_def_callers()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"DdCalClass::ddCalMethod");
        ok &= check(candidates.size() == 1, "DdCalClass::ddCalMethod resolved");
        if(candidates.empty()) return false;

        Callers callers(ws);
        auto sites = callers.find(*candidates[0]);

        ok &= check(sites.size() == 1, "ddCalMethod has exactly 1 caller (ddCalCaller)");
        if(!sites.empty() && sites[0].caller) {
            ok &= check(sites[0].caller->symbol.name == u8"ddCalCaller",
                        "caller is ddCalCaller");
        }
        return ok;
    }

    bool test_decl_def_constructor()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"DdCalClass::DdCalClass");
        ok &= check(candidates.size() == 1,
                    "DdCalClass constructor resolves to exactly 1 candidate");
        if(!candidates.empty()) {
            ok &= check(candidates[0]->symbol.kind == SymbolKind::Constructor,
                        "resolved kind is Constructor");
        }
        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_callers()
{
    static const TestCase cases[] = {
        {"CallSite fields populated correctly",          test_callsite_fields},
        {"simple direct call",                           test_simple_call},
        {"zero callers",                                 test_zero_callers},
        {"multiple callers",                             test_multiple_callers},
        {"recursive function calls itself",              test_recursive},
        {"namespace-qualified call",                     test_namespace_call},
        {"member function call",                         test_member_call},
        {"overloaded functions",                         test_overloaded},
        {"unresolved call: no false positives",          test_unresolved_call},
        {"cross-file call",                              test_cross_file},
        {"result ordering within file",                  test_result_ordering},
        {"decl/def: no ambiguity for out-of-line method", test_decl_def_no_ambiguity},
        {"decl/def: callers work across decl+def",       test_decl_def_callers},
        {"decl/def: constructor resolves to 1 candidate", test_decl_def_constructor},
    };

    std::cout << "=== callers tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
