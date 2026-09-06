#include "test_callees.h"
#include "ast-callees.h"
#include "ast-workspace.h"
#include "cli-semantic.h"
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
    // Extractor deduplicates overloads by FQN; the resolver finds the single
    // kept symbol — 1 callee is returned for the caller.

    bool test_overloaded()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);

        // Extractor keeps only the first ovCeTarget overload; callee is found.
        const WorkspaceSymbol* ambigCaller = findByName(ws, u8"ovCeAmbig");
        ok &= check(ambigCaller != nullptr, "ovCeAmbig found");
        if(ambigCaller) {
            Callees callees;
            auto sites = callees.find(ws, *ambigCaller);
            ok &= check(sites.size() == 1,
                        "ovCeAmbig has 1 callee (deduplicated ovCeTarget)");
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

    // -----------------------------------------------------------------------
    // Declaration/definition deduplication:
    // DdCleClass::ddCleMethod is declared (kind=Method) in decl_def_cle.h and
    // defined out-of-line (kind=Function) in decl_def_cle_impl.cpp.  The resolver
    // must return exactly 1 candidate; Callees::find must work end-to-end.

    bool test_decl_def_callees()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"DdCleClass::ddCleMethod");
        ok &= check(candidates.size() == 1,
                    "DdCleClass::ddCleMethod resolves to exactly 1 candidate");
        if(candidates.empty()) return false;

        ok &= check(candidates[0]->symbol.kind != SymbolKind::Function,
                    "resolved kind is Method, not the misclassified Function");

        Callees callees;
        auto sites = callees.find(ws, *candidates[0]);
        bool foundHelper = false;
        for(const auto& s : sites) {
            if(s.callee && s.callee->symbol.name == u8"ddCleHelper") {
                foundHelper = true;
                break;
            }
        }
        ok &= check(foundHelper, "ddCleMethod has ddCleHelper as a callee");
        return ok;
    }

    bool test_unique_suffix_callees()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"inner::suffixCeSource");
        ok &= check(candidates.size() == 1, "partial FQN resolves one callees target");
        if(candidates.size() != 1) return false;
        ok &= check(candidates[0]->symbol.fqn == u8"suffixce::inner::suffixCeSource",
                    "callees selected the canonical suffix target");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Phase 8c — body identity
    // Case A: free function declaration (header) + definition (cpp)
    // Resolver must collapse the two Function entries and select the definition.

    bool test_bi_free_function()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"biFreeFn");
        ok &= check(candidates.size() == 1,
                    "bi/free: biFreeFn resolves to exactly 1 candidate (definition)");
        if(candidates.empty()) return false;

        const TranslationUnit* tu = ws.get_translation_unit(candidates[0]->sourceFile);
        ok &= check(tu != nullptr, "bi/free: TU found");
        if(tu && candidates[0]->symbol.nodeIndex < tu->ast.size()) {
            ok &= check(tu->ast[candidates[0]->symbol.nodeIndex].type_
                        == ASTNodeType::FunctionDefinition,
                        "bi/free: selected symbol is a FunctionDefinition");
        }

        Callees callees;
        auto sites = callees.find(ws, *candidates[0]);
        bool foundHelper = false;
        for(const auto& s : sites)
            if(s.callee && s.callee->symbol.name == u8"biFreeHelper") foundHelper = true;
        ok &= check(foundHelper, "bi/free: biFreeFn callee is biFreeHelper");
        return ok;
    }

    // Case B: class method declaration + out-of-line definition

    bool test_bi_method_outofline()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"BiMethodClass::biMethodFn");
        ok &= check(candidates.size() == 1,
                    "bi/method: BiMethodClass::biMethodFn resolves to 1 candidate");
        if(candidates.empty()) return false;
        ok &= check(candidates[0]->symbol.kind == SymbolKind::Method,
                    "bi/method: resolved kind is Method");

        Callees callees;
        auto sites = callees.find(ws, *candidates[0]);
        bool foundHelper = false;
        for(const auto& s : sites)
            if(s.callee && s.callee->symbol.name == u8"biMethodHelper") foundHelper = true;
        ok &= check(foundHelper, "bi/method: biMethodFn callee is biMethodHelper");
        return ok;
    }

    // Case C: namespace-qualified out-of-line method definition

    bool test_bi_ns_method()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"binsns::BiNsClass::biNsMethodFn");
        ok &= check(candidates.size() == 1,
                    "bi/ns: binsns::BiNsClass::biNsMethodFn resolves to 1 candidate");
        if(candidates.empty()) return false;
        ok &= check(candidates[0]->symbol.kind == SymbolKind::Method,
                    "bi/ns: resolved kind is Method");

        Callees callees;
        auto sites = callees.find(ws, *candidates[0]);
        bool foundHelper = false;
        for(const auto& s : sites)
            if(s.callee && s.callee->symbol.name == u8"biNsMethodHelper") foundHelper = true;
        ok &= check(foundHelper, "bi/ns: biNsMethodFn callee is biNsMethodHelper");
        return ok;
    }

    // Case D: inline method — body already present, no fallback needed

    bool test_bi_inline_body()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"BiInlineClass::biInlineFn");
        ok &= check(candidates.size() == 1,
                    "bi/inline: BiInlineClass::biInlineFn resolves to 1 candidate");
        if(candidates.empty()) return false;

        Callees callees;
        auto sites = callees.find(ws, *candidates[0]);
        bool foundHelper = false;
        for(const auto& s : sites)
            if(s.callee && s.callee->symbol.name == u8"biInlineHelper") foundHelper = true;
        ok &= check(foundHelper, "bi/inline: biInlineFn callee is biInlineHelper (inline body)");
        return ok;
    }

    // Case E: declaration only — no definition exists, must return empty

    bool test_bi_decl_only()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"biDeclOnlyFn");
        ok &= check(candidates.size() == 1,
                    "bi/decl-only: biDeclOnlyFn found");
        if(candidates.empty()) return false;

        Callees callees;
        auto sites = callees.find(ws, *candidates[0]);
        ok &= check(sites.empty(),
                    "bi/decl-only: no callees for declaration-only function");
        return ok;
    }

    // Stage 10 — motivating failure replay:
    // auth::AuthService::refresh was previously empty because the method
    // declaration in the header was selected (no body). Now the out-of-line
    // definition is used and auth::AuthToken::validate is returned.

    bool test_bi_auth_motivating_case()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"auth::AuthService::refresh");
        ok &= check(candidates.size() == 1,
                    "bi/auth: auth::AuthService::refresh resolves to 1 candidate");
        if(candidates.empty()) return false;
        ok &= check(candidates[0]->symbol.kind == SymbolKind::Method,
                    "bi/auth: resolved kind is Method");

        Callees callees;
        auto sites = callees.find(ws, *candidates[0]);
        bool foundValidate = false;
        for(const auto& s : sites) {
            if(s.callee && s.callee->symbol.name == u8"validate") {
                foundValidate = true;
                break;
            }
        }
        ok &= check(foundValidate,
                    "bi/auth: refresh callee is auth::AuthToken::validate");
        return ok;
    }

    // Stage 12 — false-positive guard:
    // bifpa::biFpRun is declared without a body; bifpb::biFpRun has a body.
    // Querying bifpa::biFpRun must NOT use bifpb::biFpRun's body.

    bool test_bi_false_positive_guard()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCeRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"bifpa::biFpRun");
        ok &= check(candidates.size() == 1, "bi/fp-guard: bifpa::biFpRun found");
        if(candidates.empty()) return false;

        Callees callees;
        auto sites = callees.find(ws, *candidates[0]);
        ok &= check(sites.empty(),
                    "bi/fp-guard: bifpa::biFpRun has no callees (must not use bifpb body)");
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
        {"decl/def: callees work across decl+def",          test_decl_def_callees},
        {"resolver: unique suffix works for callees",       test_unique_suffix_callees},
        {"bi/A: free function decl+def body found",         test_bi_free_function},
        {"bi/B: class method out-of-line body found",       test_bi_method_outofline},
        {"bi/C: namespace-qualified method body found",     test_bi_ns_method},
        {"bi/D: inline method body used directly",          test_bi_inline_body},
        {"bi/E: declaration-only stays empty",              test_bi_decl_only},
        {"bi/G: false-positive guard (different namespace)", test_bi_false_positive_guard},
        {"bi/motivating: auth::AuthService::refresh callee found", test_bi_auth_motivating_case},
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
