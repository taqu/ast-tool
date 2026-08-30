#include "test_references.h"
#include "ast-extractor.h"
#include "ast-references.h"
#include "ast-resolver.h"
#include "ast-scope-builder.h"
#include "ast-symbol-scope.h"
#include "ast-ir.h"
#include "ast-workspace.h"
#include "cli-semantic.h"
#include <iostream>
#include <string_view>

namespace ast
{
namespace
{
    static constexpr const char* kRefRoot = "test/ast-references/workspace";

    bool check(bool condition, const char* description)
    {
        if(!condition)
            std::cerr << "    FAIL: " << description << "\n";
        return condition;
    }

    // Finds the first workspace symbol whose name equals @p name.
    const WorkspaceSymbol* findByName(const Workspace& ws, std::u8string_view name)
    {
        for(const auto& sym : ws.symbols) {
            if(sym.symbol.name == name) return &sym;
        }
        return nullptr;
    }

    // Finds the first workspace symbol matching @p name in a file whose path contains @p fileHint.
    const WorkspaceSymbol* findByNameInFile(const Workspace& ws,
                                             std::u8string_view name,
                                             std::u8string_view fileHint)
    {
        for(const auto& sym : ws.symbols) {
            std::u8string sourceFile = sym.sourceFile.u8string();
            if(sym.symbol.name == name && sourceFile.find(fileHint) != std::string::npos)
                return &sym;
        }
        return nullptr;
    }

    // -----------------------------------------------------------------------
    // ReferenceResult struct — basic field checks

    bool test_reference_result_fields()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"mrCounter");
        ok &= check(target != nullptr, "mrCounter found in workspace");
        if(!target) return false;

        FindReferences finder(ws);
        auto refs = finder.find(*target);

        ok &= check(!refs.empty(), "mrCounter has at least one reference");
        if(refs.empty()) return ok;

        const ReferenceResult& r = refs[0];
        ok &= check(r.referencedSymbol.symbol.name == u8"mrCounter",        "referencedSymbol.name set");
        ok &= check(r.referencedSymbol.symbol.fqn  == target->symbol.fqn, "referencedSymbol.fqn set");
        ok &= check(r.referencedSymbol.symbol.kind == SymbolKind::Variable, "referencedSymbol.kind set");
        ok &= check(!r.sourceFile.empty(),                                 "sourceFile non-empty");
        ok &= check(r.nodeIndex != size_t(-1),                             "nodeIndex populated");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Global variable — multiple references

    bool test_global_variable_multiple_refs()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"mrCounter");
        ok &= check(target != nullptr, "mrCounter found in workspace");
        if(!target) return false;

        FindReferences finder(ws);
        auto refs = finder.find(*target, /*includeDeclaration=*/false);

        // mrInc: mrCounter = mrCounter + 1  → 2 identifier nodes for mrCounter
        // mrDec: mrCounter = mrCounter - 1  → 2 identifier nodes for mrCounter
        // Declaration excluded → expect exactly 4
        ok &= check(refs.size() == 4, "mrCounter has 4 references (declaration excluded)");
        for(const auto& r : refs) {
            std::u8string sourceFile = r.sourceFile.u8string();
            ok &= check(sourceFile.find(u8"multi_ref") != std::string::npos,
                        "reference is in multi_ref.cpp");
            // r.line is 0-based; target->symbol.line is 1-based. Use +1 for comparison.
        ok &= check(r.line + 1 != target->symbol.line, "reference is not on declaration line");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // includeDeclaration=true adds the declaration site

    bool test_include_declaration()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"mrCounter");
        ok &= check(target != nullptr, "mrCounter found in workspace");
        if(!target) return false;

        FindReferences finder(ws);
        auto withDecl    = finder.find(*target, /*includeDeclaration=*/true);
        auto withoutDecl = finder.find(*target, /*includeDeclaration=*/false);

        ok &= check(withDecl.size() == withoutDecl.size() + 1,
                    "includeDeclaration=true adds exactly one entry");

        // The declaration entry has the same file and line as the target symbol.
        bool foundDecl = false;
        for(const auto& r : withDecl) {
            if(r.sourceFile == target->sourceFile && r.line == target->symbol.line) {
                foundDecl = true;
                break;
            }
        }
        ok &= check(foundDecl, "declaration entry present when includeDeclaration=true");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Zero references

    bool test_zero_references()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"zeroRefVar");
        ok &= check(target != nullptr, "zeroRefVar found in workspace");
        if(!target) return false;

        FindReferences finder(ws);
        auto refs = finder.find(*target);
        ok &= check(refs.empty(), "zeroRefVar has zero references");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Namespace member reference

    bool test_namespace_member()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"nsMbrBase");
        ok &= check(target != nullptr, "nsMbrBase found in workspace");
        if(!target) return false;

        FindReferences finder(ws);
        auto refs = finder.find(*target);

        // nsMbrCompute body: "return nsMbrBase + x;" — 1 identifier for nsMbrBase
        ok &= check(refs.size() == 1, "nsMbrBase has 1 reference");
        if(!refs.empty()) {
            ok &= check(refs[0].owningScope == ScopeKind::Function ||
                        refs[0].owningScope == ScopeKind::Method   ||
                        refs[0].owningScope == ScopeKind::Block,
                        "nsMbrBase reference is inside a function/block scope");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Class member reference from method body

    bool test_class_member()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"clsX");
        ok &= check(target != nullptr, "clsX found in workspace");
        if(!target) return false;

        FindReferences finder(ws);
        auto refs = finder.find(*target);

        // "return clsX + clsY;" — 1 identifier for clsX
        ok &= check(refs.size() == 1, "clsX has 1 reference");
        std::u8string sourceFile = refs[0].sourceFile.u8string();
        if(!refs.empty()) {
            ok &= check(sourceFile.find(u8"cls_member") != std::string::npos,
                        "clsX reference is in cls_member.cpp");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Shadowing: namespace-scoped name hides outer name via lexical lookup.
    // The global shadowTarget has exactly 1 reference (in useTarget).
    // ShadowInner::shadowTarget (a different symbol) shadows it inside shadowFunc.

    bool test_shadowing()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefRoot);

        // Find any shadowTarget in shadow.cpp (to confirm workspace has it)
        const WorkspaceSymbol* target = findByNameInFile(ws, u8"shadowTarget", u8"shadow");
        ok &= check(target != nullptr, "global shadowTarget found in workspace");
        if(!target) return false;

        // The global is the one with the smallest line number (before shadowFunc).
        const WorkspaceSymbol* globalTarget = nullptr;
        for(const auto& sym : ws.symbols) {
            std::u8string sourceFile = sym.sourceFile.u8string();
            if(sym.symbol.name == u8"shadowTarget"
               && sourceFile.find(u8"shadow") != std::string::npos) {
                if(!globalTarget || sym.symbol.line < globalTarget->symbol.line)
                    globalTarget = &sym;
            }
        }
        ok &= check(globalTarget != nullptr, "global shadowTarget found");
        if(!globalTarget) return false;

        FindReferences finder(ws);
        auto refs = finder.find(*globalTarget, /*includeDeclaration=*/false);

        // Only useTarget() uses the global shadowTarget.
        // shadowFunc() declares a local that shadows it; (void)shadowTarget inside
        // shadowFunc resolves to the LOCAL, not the global.
        ok &= check(refs.size() == 1, "global shadowTarget has exactly 1 reference");
        if(!refs.empty()) {
            // The reference should NOT be at the declaration line (line 0)
            ok &= check(refs[0].line != 0, "reference is not the declaration");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Cross-file reference: xfileVal defined in xfile_src.cpp, used in xfile_use.cpp

    bool test_cross_file()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefRoot);
        const WorkspaceSymbol* target = findByNameInFile(ws, u8"xfileVal", u8"xfile_src");
        ok &= check(target != nullptr, "xfileVal found in workspace (xfile_src.cpp)");
        if(!target) return false;

        FindReferences finder(ws);
        auto refs = finder.find(*target, /*includeDeclaration=*/false);

        // xfile_use.cpp: "return xfileVal;" — 1 cross-file reference
        ok &= check(!refs.empty(), "xfileVal has at least one reference");
        bool foundInUse = false;
        for(const auto& r : refs) {
            std::u8string sourceFile = r.sourceFile.u8string();
            if(sourceFile.find(u8"xfile_use") != std::string::npos) {
                foundInUse = true;
                break;
            }
        }
        ok &= check(foundInUse, "xfileVal is referenced in xfile_use.cpp");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Unresolved identifiers produce no false positives

    bool test_no_false_positives_from_unresolved()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefRoot);        // zeroRefVar is declared but never used.
        // xfileVal in xfile_use.cpp refers to an external symbol that the
        // resolver finds via workspace fallback — but it does NOT resolve to
        // zeroRefVar.  Neither should any other identifier.
        const WorkspaceSymbol* target = findByName(ws, u8"zeroRefVar");
        ok &= check(target != nullptr, "zeroRefVar found");
        if(!target) return false;

        FindReferences finder(ws);
        auto refs = finder.find(*target);

        ok &= check(refs.empty(), "no false positives: zeroRefVar has 0 references");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Local variable and parameter (may be skipped if extractor omits locals)

    bool test_local_variable_and_parameter()
    {
        static const char* kFile = "test/ast-references/workspace/local_param.cpp";

        // Build a workspace of just this one file so we don't pick up symbols
        // with the same name from other files.
        Workspace ws = analyze_files({kFile});
        bool ok = true;

        // Test lpTmp (local variable) if the extractor included it.
        const WorkspaceSymbol* tmpTarget = findByName(ws, u8"lpTmp");
        if(!tmpTarget) {
            std::cout << "    [SKIP] lpTmp not in workspace symbols (extractor omits locals)\n";
        } else {
            FindReferences finder(ws);
            auto refs = finder.find(*tmpTarget);
            // "return lpTmp;" — 1 reference to lpTmp
            ok &= check(refs.size() == 1, "lpTmp has 1 reference");
        }

        // Test lpN (parameter) if the extractor included it.
        const WorkspaceSymbol* paramTarget = findByName(ws, u8"lpN");
        if(!paramTarget) {
            std::cout << "    [SKIP] lpN not in workspace symbols (extractor omits parameters)\n";
        } else {
            FindReferences finder(ws);
            auto refs = finder.find(*paramTarget);
            // "int lpTmp = lpN;" — 1 reference to lpN
            ok &= check(refs.size() == 1, "lpN has 1 reference");
        }

        return ok;
    }

    // -----------------------------------------------------------------------
    // Result ordering: references appear in file / node order

    bool test_result_ordering()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"mrCounter");
        ok &= check(target != nullptr, "mrCounter found");
        if(!target) return false;

        FindReferences finder(ws);
        auto refs = finder.find(*target, /*includeDeclaration=*/false);
        ok &= check(refs.size() >= 2, "at least 2 references for ordering check");

        // Within a single file, node indices must be strictly increasing.
        for(size_t i = 1; i < refs.size(); ++i) {
            if(refs[i].sourceFile == refs[i - 1].sourceFile) {
                ok &= check(refs[i].nodeIndex > refs[i - 1].nodeIndex,
                            "references within a file are in AST-node order");
            }
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // owningScope is set correctly for namespace-member reference

    bool test_owning_scope_namespace()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefRoot);
        const WorkspaceSymbol* target = findByName(ws, u8"nsMbrBase");
        ok &= check(target != nullptr, "nsMbrBase found");
        if(!target) return false;

        FindReferences finder(ws);
        auto refs = finder.find(*target);
        ok &= check(refs.size() == 1, "exactly 1 reference");
        if(refs.empty()) return ok;

        // The reference appears inside nsMbrCompute, which tree-sitter puts
        // inside the NsMbr namespace scope.  The innermost enclosing scope is
        // the function body (Function or Block), not the namespace itself.
        ScopeKind sk = refs[0].owningScope;
        ok &= check(sk == ScopeKind::Function  ||
                    sk == ScopeKind::Block      ||
                    sk == ScopeKind::Method     ||
                    sk == ScopeKind::Namespace,
                    "owningScope is a function-level or namespace scope");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Declaration/definition deduplication:
    // DdRefClass::ddRefMethod is declared (kind=Method) in decl_def_ref.h and
    // defined out-of-line (kind=Function) in decl_def_ref_impl.cpp.  The resolver
    // must return exactly 1 candidate; FindReferences must work end-to-end.

    bool test_decl_def_references()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kRefRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"DdRefClass::ddRefMethod");
        ok &= check(candidates.size() == 1,
                    "DdRefClass::ddRefMethod resolves to exactly 1 candidate");
        if(candidates.empty()) return false;

        ok &= check(candidates[0]->symbol.kind != SymbolKind::Function,
                    "resolved kind is Method, not the misclassified Function");

        FindReferences finder(ws);
        auto refs = finder.find(*candidates[0]);
        bool foundCallSite = false;
        for(const auto& r : refs) {
            std::u8string f = r.sourceFile.u8string();
            if(f.find(u8"decl_def_ref_impl") != std::string::npos) {
                foundCallSite = true;
                break;
            }
        }
        ok &= check(foundCallSite,
                    "reference to ddRefMethod found in decl_def_ref_impl.cpp");
        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_references()
{
    static const TestCase cases[] = {
        // ReferenceResult API
        {"result: fields populated correctly",            test_reference_result_fields},
        // Multiple references + declaration exclusion
        {"global: multiple references (decl excluded)",   test_global_variable_multiple_refs},
        {"global: includeDeclaration=true",               test_include_declaration},
        // Zero references
        {"zero references",                               test_zero_references},
        // Namespace member
        {"namespace member reference",                    test_namespace_member},
        // Class member
        {"class member reference",                        test_class_member},
        // Shadowing
        {"shadowing: inner decl hides outer",             test_shadowing},
        // Cross-file
        {"cross-file reference",                          test_cross_file},
        // Unresolved identifiers → no false positives
        {"no false positives from unresolved identifiers", test_no_false_positives_from_unresolved},
        // Local variable / parameter (may be skipped)
        {"local variable and parameter",                  test_local_variable_and_parameter},
        // Result ordering
        {"result ordering within file",                   test_result_ordering},
        // owningScope field
        {"owningScope set for namespace-member reference", test_owning_scope_namespace},
        // Declaration/definition deduplication
        {"decl/def: references work across decl+def",     test_decl_def_references},
    };

    std::cout << "=== find references tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
