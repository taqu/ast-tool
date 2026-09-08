#include "test_context_export.h"
#include "ast-context-export.h"
#include "ast-workspace.h"
#include <iostream>
#include <string_view>

namespace ast
{
namespace
{
    // Reuse existing test workspaces rather than introducing a new one.
    static constexpr const char* kRefsRoot    = "test/ast-references/workspace";
    static constexpr const char* kCallersRoot = "test/ast-callers/workspace";
    static constexpr const char* kCalleesRoot = "test/ast-callees/workspace";

    bool check(bool condition, const char* description)
    {
        if(!condition)
            std::cerr << "    FAIL: " << description << "\n";
        return condition;
    }

    const WorkspaceSymbol* findByName(const Workspace& ws, std::u8string_view name)
    {
        for(const auto& s : ws.symbols)
            if(s.symbol.name == name) return &s;
        return nullptr;
    }

    const WorkspaceSymbol* findByNameInFile(const Workspace& ws,
                                             std::u8string_view name,
                                             std::u8string_view fileHint)
    {
        for(const auto& s : ws.symbols){
            std::u8string sourceFile = s.sourceFile.u8string();
            if(s.symbol.name == name &&
               sourceFile.find(fileHint) != std::string::npos) return &s;
        }
        return nullptr;
    }

    const WorkspaceSymbol* findByFQN(const Workspace& ws, std::u8string_view fqn)
    {
        for(const auto& s : ws.symbols)
            if(s.symbol.fqn == fqn) return &s;
        return nullptr;
    }

    // -----------------------------------------------------------------------
    // symbol pointer is always set

    bool test_symbol_pointer_always_set()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefsRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"mrCounter");
        ok &= check(target != nullptr, "mrCounter found");
        if(!target) return false;

        ContextExporter exporter;
        SemanticContext ctx = exporter.export_context(ws, *target);

        ok &= check(ctx.symbol != nullptr,       "symbol pointer is set");
        ok &= check(ctx.symbol == target,        "symbol pointer matches target");
        ok &= check(ctx.symbol->symbol.name == u8"mrCounter", "symbol name is correct");
        return ok;
    }

    // -----------------------------------------------------------------------
    // function with references

    bool test_function_with_references()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefsRoot);
        // mrCounter is referenced in mrInc and mrDec (4 non-declaration references).
        const WorkspaceSymbol* target = findByName(ws, u8"mrCounter");
        ok &= check(target != nullptr, "mrCounter found");
        if(!target) return false;

        ContextExporter exporter;
        SemanticContext ctx = exporter.export_context(ws, *target);

        ok &= check(!ctx.references.empty(), "mrCounter has references");
        ok &= check(ctx.symbol != nullptr,   "symbol pointer is set");
        return ok;
    }

    // -----------------------------------------------------------------------
    // function with callers

    bool test_function_with_callers()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCallersRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"calTarget");
        ok &= check(target != nullptr, "calTarget found");
        if(!target) return false;

        ContextExporter exporter;
        SemanticContext ctx = exporter.export_context(ws, *target);

        ok &= check(!ctx.callers.empty(), "calTarget has callers");
        ok &= check(ctx.symbol  != nullptr, "symbol pointer is set");

        // The caller should be callerFn.
        bool found = false;
        for(const auto& site : ctx.callers)
            if(site.caller && site.caller->symbol.name == u8"callerFn") { found = true; break; }
        ok &= check(found, "callerFn is among calTarget's callers");
        return ok;
    }

    // -----------------------------------------------------------------------
    // function with callees

    bool test_function_with_callees()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalleesRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"scSource");
        ok &= check(target != nullptr, "scSource found");
        if(!target) return false;

        ContextExporter exporter;
        SemanticContext ctx = exporter.export_context(ws, *target);

        ok &= check(!ctx.callees.empty(), "scSource has callees");
        ok &= check(ctx.symbol  != nullptr, "symbol pointer is set");

        bool found = false;
        for(const auto& site : ctx.callees)
            if(site.callee && site.callee->symbol.name == u8"scTarget") { found = true; break; }
        ok &= check(found, "scTarget is a callee of scSource");
        return ok;
    }

    // -----------------------------------------------------------------------
    // symbol with no references

    bool test_symbol_no_references()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefsRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"zeroRefVar");
        ok &= check(target != nullptr, "zeroRefVar found");
        if(!target) return false;

        ContextExporter exporter;
        SemanticContext ctx = exporter.export_context(ws, *target);

        ok &= check(ctx.references.empty(), "zeroRefVar has no references");
        ok &= check(ctx.symbol != nullptr,  "symbol pointer still set");
        return ok;
    }

    // -----------------------------------------------------------------------
    // symbol with no callers

    bool test_symbol_no_callers()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCallersRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"noCalTarget");
        ok &= check(target != nullptr, "noCalTarget found");
        if(!target) return false;

        ContextExporter exporter;
        SemanticContext ctx = exporter.export_context(ws, *target);

        ok &= check(ctx.callers.empty(), "noCalTarget has no callers");
        ok &= check(ctx.symbol != nullptr, "symbol pointer still set");
        return ok;
    }

    // -----------------------------------------------------------------------
    // symbol with no callees

    bool test_symbol_no_callees()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCalleesRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"noCallee");
        ok &= check(target != nullptr, "noCallee found");
        if(!target) return false;

        ContextExporter exporter;
        SemanticContext ctx = exporter.export_context(ws, *target);

        ok &= check(ctx.callees.empty(), "noCallee has no callees");
        ok &= check(ctx.symbol != nullptr, "symbol pointer still set");
        return ok;
    }

    // -----------------------------------------------------------------------
    // cross-file symbol

    bool test_cross_file_symbol()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCallersRoot);
        const WorkspaceSymbol* target = findByNameInFile(ws, u8"xfileTarget", u8"xfile_fn");
        ok &= check(target != nullptr, "xfileTarget found in xfile_fn");
        if(!target) return false;

        ContextExporter exporter;
        SemanticContext ctx = exporter.export_context(ws, *target);

        ok &= check(ctx.symbol != nullptr, "symbol pointer is set");
        // xfileCaller calls xfileTarget from a different file.
        bool foundCaller = false;
        for(const auto& site : ctx.callers){
            std::u8string sourceFile = site.sourceFile.u8string();
            if(sourceFile.find(u8"xfile_caller") != std::string::npos)
                { foundCaller = true; break; }
        }
        ok &= check(foundCaller, "cross-file caller found in context");
        return ok;
    }

    // -----------------------------------------------------------------------
    // namespace member

    bool test_namespace_member()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCallersRoot);
        const WorkspaceSymbol* target = findByFQN(ws, u8"NsCal::nsTarget");
        ok &= check(target != nullptr, "NsCal::nsTarget found");
        if(!target) return false;

        ContextExporter exporter;
        SemanticContext ctx = exporter.export_context(ws, *target);

        ok &= check(ctx.symbol != nullptr, "symbol pointer is set");
        ok &= check(!ctx.callers.empty(),  "namespace member has at least one caller");
        return ok;
    }

    // -----------------------------------------------------------------------
    // class member

    bool test_class_member()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCallersRoot);
        const WorkspaceSymbol* target = findByFQN(ws, u8"MbObj::mbTarget");
        ok &= check(target != nullptr, "MbObj::mbTarget found");
        if(!target) return false;

        ContextExporter exporter;
        SemanticContext ctx = exporter.export_context(ws, *target);

        ok &= check(ctx.symbol != nullptr, "symbol pointer is set");
        ok &= check(!ctx.callers.empty(),  "class member has at least one caller");
        return ok;
    }

    // -----------------------------------------------------------------------
    // partially populated context: variable has references but no callers/callees

    bool test_partially_populated_context()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefsRoot);
        // mrCounter is a variable: it has references, but cannot be "called"
        // and has no function body to traverse for callees.
        const WorkspaceSymbol* target = findByName(ws, u8"mrCounter");
        ok &= check(target != nullptr, "mrCounter found");
        if(!target) return false;

        ContextExporter exporter;
        SemanticContext ctx = exporter.export_context(ws, *target);

        ok &= check(ctx.symbol != nullptr,       "symbol pointer is set");
        ok &= check(!ctx.references.empty(),     "references are populated");
        ok &= check(ctx.callers.empty(),         "callers is empty (variable, not a call target)");
        ok &= check(ctx.callees.empty(),         "callees is empty (variable has no body)");
        return ok;
    }

    // -----------------------------------------------------------------------
    // all context fields are independent (no shared state between calls)

    bool test_independent_calls()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kCallersRoot);

        const WorkspaceSymbol* t1 = findByName(ws, u8"calTarget");
        const WorkspaceSymbol* t2 = findByName(ws, u8"noCalTarget");
        ok &= check(t1 != nullptr && t2 != nullptr, "both symbols found");
        if(!t1 || !t2) return false;

        ContextExporter exporter;
        SemanticContext ctx1 = exporter.export_context(ws, *t1);
        SemanticContext ctx2 = exporter.export_context(ws, *t2);

        ok &= check(!ctx1.callers.empty(), "calTarget has callers");
        ok &= check(ctx2.callers.empty(),  "noCalTarget has no callers");
        ok &= check(ctx1.symbol == t1,     "ctx1 symbol pointer correct");
        ok &= check(ctx2.symbol == t2,     "ctx2 symbol pointer correct");
        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_context_export()
{
    static const TestCase cases[] = {
        {"symbol pointer always set",                   test_symbol_pointer_always_set},
        {"function with references",                    test_function_with_references},
        {"function with callers",                       test_function_with_callers},
        {"function with callees",                       test_function_with_callees},
        {"symbol with no references",                   test_symbol_no_references},
        {"symbol with no callers",                      test_symbol_no_callers},
        {"symbol with no callees",                      test_symbol_no_callees},
        {"cross-file symbol",                           test_cross_file_symbol},
        {"namespace member",                            test_namespace_member},
        {"class member",                                test_class_member},
        {"partially populated context",                 test_partially_populated_context},
        {"independent calls produce independent results", test_independent_calls},
    };

    std::cout << "=== context export tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
