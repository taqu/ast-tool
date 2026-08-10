#include "test_semantic_diff.h"
#include "ast-semantic-diff.h"
#include "ast-workspace.h"
#include <algorithm>
#include <iostream>
#include <string_view>

namespace ast
{
namespace
{
    static constexpr const char* kWs = "test/ast-semantic-diff/workspace/";

    bool check(bool condition, const char* description)
    {
        if(!condition)
            std::cerr << "    FAIL: " << description << "\n";
        return condition;
    }

    std::string ws(const char* file) { return std::string(kWs) + file; }

    // Returns the number of entries with the given kind.
    size_t count_kind(const SemanticDiffResult& r, DiffKind kind)
    {
        return static_cast<size_t>(
            std::count_if(r.changes.begin(), r.changes.end(),
                [kind](const DiffEntry& e) { return e.kind == kind; }));
    }

    // Returns true if any entry matches: kind + after->symbol.name == name.
    bool has_added(const SemanticDiffResult& r, std::string_view name)
    {
        for(const DiffEntry& e : r.changes)
            if(e.kind == DiffKind::Added && e.after && e.after->symbol.name == name)
                return true;
        return false;
    }

    // Returns true if any entry matches: kind + before->symbol.name == name.
    bool has_removed(const SemanticDiffResult& r, std::string_view name)
    {
        for(const DiffEntry& e : r.changes)
            if(e.kind == DiffKind::Removed && e.before && e.before->symbol.name == name)
                return true;
        return false;
    }

    // Returns the first Modified entry for a symbol whose FQN equals @p fqn,
    // or nullptr if none exists.
    const DiffEntry* find_modified(const SemanticDiffResult& r, std::string_view fqn)
    {
        for(const DiffEntry& e : r.changes)
            if(e.kind == DiffKind::Modified && e.before && e.before->symbol.fqn == fqn)
                return &e;
        return nullptr;
    }

    // -----------------------------------------------------------------------
    // Added function

    bool test_added_function()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("added_fn_old.cpp")});
        Workspace newWs = analyze_files({ws("added_fn_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        ok &= check(count_kind(result, DiffKind::Added)    == 1, "exactly 1 Added entry");
        ok &= check(count_kind(result, DiffKind::Removed)  == 0, "0 Removed entries");
        ok &= check(count_kind(result, DiffKind::Modified) == 0, "0 Modified entries");
        ok &= check(has_added(result, "afAdded"), "afAdded is in Added entries");
        // afBase is unchanged → must NOT appear
        ok &= check(!has_added(result, "afBase"),   "afBase not in Added (unchanged)");
        ok &= check(!has_removed(result, "afBase"), "afBase not in Removed (unchanged)");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Removed function

    bool test_removed_function()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("removed_fn_old.cpp")});
        Workspace newWs = analyze_files({ws("removed_fn_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        ok &= check(count_kind(result, DiffKind::Removed)  == 1, "exactly 1 Removed entry");
        ok &= check(count_kind(result, DiffKind::Added)    == 0, "0 Added entries");
        ok &= check(count_kind(result, DiffKind::Modified) == 0, "0 Modified entries");
        ok &= check(has_removed(result, "rfRemoved"), "rfRemoved is in Removed entries");

        // Verify DiffEntry pointer layout for a Removed entry.
        for(const DiffEntry& e : result.changes) {
            if(e.kind == DiffKind::Removed) {
                ok &= check(e.before != nullptr, "Removed entry: before is set");
                ok &= check(e.after  == nullptr, "Removed entry: after is null");
            }
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Renamed function (appears as Removed old name + Added new name)

    bool test_renamed_function()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("renamed_fn_old.cpp")});
        Workspace newWs = analyze_files({ws("renamed_fn_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        ok &= check(count_kind(result, DiffKind::Removed) == 1, "1 Removed (old name gone)");
        ok &= check(count_kind(result, DiffKind::Added)   == 1, "1 Added   (new name)");
        ok &= check(has_removed(result, "rnOldName"), "rnOldName is Removed");
        ok &= check(has_added(result,   "rnNewName"), "rnNewName is Added");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Access changed

    bool test_access_changed()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("access_changed_old.cpp")});
        Workspace newWs = analyze_files({ws("access_changed_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        // AcStruct itself is unchanged.  AcStruct::acMethod changes access.
        const DiffEntry* entry = find_modified(result, "AcStruct::acMethod");
        ok &= check(entry != nullptr, "AcStruct::acMethod is Modified");
        if(entry) {
            ok &= check(entry->before != nullptr, "Modified entry: before is set");
            ok &= check(entry->after  != nullptr, "Modified entry: after is set");
            ok &= check(entry->before->symbol.access == Access::Public,
                        "before: access is Public");
            ok &= check(entry->after->symbol.access  == Access::Private,
                        "after: access is Private");
        }
        ok &= check(count_kind(result, DiffKind::Modified) == 1, "exactly 1 Modified entry");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Static modifier changed

    bool test_static_changed()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("static_changed_old.cpp")});
        Workspace newWs = analyze_files({ws("static_changed_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        const DiffEntry* entry = find_modified(result, "scFunc");
        ok &= check(entry != nullptr, "scFunc is Modified");
        if(entry) {
            ok &= check(!entry->before->symbol.isStatic, "before: isStatic is false");
            ok &= check( entry->after->symbol.isStatic,  "after:  isStatic is true");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Constexpr modifier changed

    bool test_constexpr_changed()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("constexpr_changed_old.cpp")});
        Workspace newWs = analyze_files({ws("constexpr_changed_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        const DiffEntry* entry = find_modified(result, "ccFunc");
        ok &= check(entry != nullptr, "ccFunc is Modified");
        if(entry) {
            ok &= check(!entry->before->symbol.isConstexpr, "before: isConstexpr is false");
            ok &= check( entry->after->symbol.isConstexpr,  "after:  isConstexpr is true");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Inline modifier changed

    bool test_inline_changed()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("inline_changed_old.cpp")});
        Workspace newWs = analyze_files({ws("inline_changed_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        const DiffEntry* entry = find_modified(result, "icFunc");
        ok &= check(entry != nullptr, "icFunc is Modified");
        if(entry) {
            ok &= check(!entry->before->symbol.isInline, "before: isInline is false");
            ok &= check( entry->after->symbol.isInline,  "after:  isInline is true");
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Parameter renamed only — no semantic change expected.
    // The Symbol struct does not capture parameter names; renaming a parameter
    // produces identical WorkspaceSymbol objects on both sides.

    bool test_parameter_renamed_only()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("param_renamed_old.cpp")});
        Workspace newWs = analyze_files({ws("param_renamed_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        ok &= check(result.empty(), "parameter rename: no semantic changes detected");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Whitespace changes — no semantic change expected.

    bool test_whitespace_changes()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("whitespace_old.cpp")});
        Workspace newWs = analyze_files({ws("whitespace_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        ok &= check(result.empty(), "whitespace-only change: no semantic changes detected");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Comment changes — no semantic change expected.

    bool test_comment_changes()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("comment_changed_old.cpp")});
        Workspace newWs = analyze_files({ws("comment_changed_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        ok &= check(result.empty(), "comment-only change: no semantic changes detected");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Namespace moved (FQN changes → Removed old + Added new)

    bool test_namespace_moved()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("ns_moved_old.cpp")});
        Workspace newWs = analyze_files({ws("ns_moved_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        // NsA::nmFunc Removed, NsB::nmFunc Added; NsA and NsB namespaces also change.
        ok &= check(count_kind(result, DiffKind::Removed) >= 1, "at least 1 Removed entry");
        ok &= check(count_kind(result, DiffKind::Added)   >= 1, "at least 1 Added entry");

        // Verify the function specifically (namespace symbols may also appear).
        bool foundRemoved = false, foundAdded = false;
        for(const DiffEntry& e : result.changes) {
            if(e.kind == DiffKind::Removed && e.before &&
               e.before->symbol.fqn == "NsA::nmFunc") foundRemoved = true;
            if(e.kind == DiffKind::Added   && e.after  &&
               e.after->symbol.fqn  == "NsB::nmFunc") foundAdded   = true;
        }
        ok &= check(foundRemoved, "NsA::nmFunc is Removed");
        ok &= check(foundAdded,   "NsB::nmFunc is Added");
        ok &= check(count_kind(result, DiffKind::Modified) == 0, "no Modified entries");
        return ok;
    }

    // -----------------------------------------------------------------------
    // No semantic change

    bool test_no_semantic_change()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("no_change_old.cpp")});
        Workspace newWs = analyze_files({ws("no_change_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        ok &= check(result.empty(), "identical workspaces: no semantic changes");
        return ok;
    }

    // -----------------------------------------------------------------------
    // DiffEntry pointer invariants

    bool test_diff_entry_invariants()
    {
        bool ok = true;
        Workspace oldWs = analyze_files({ws("added_fn_old.cpp")});
        Workspace newWs = analyze_files({ws("added_fn_new.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(oldWs, newWs);

        for(const DiffEntry& e : result.changes) {
            switch(e.kind) {
            case DiffKind::Added:
                ok &= check(e.before == nullptr, "Added:    before must be null");
                ok &= check(e.after  != nullptr, "Added:    after  must be set");
                break;
            case DiffKind::Removed:
                ok &= check(e.before != nullptr, "Removed:  before must be set");
                ok &= check(e.after  == nullptr, "Removed:  after  must be null");
                break;
            case DiffKind::Modified:
                ok &= check(e.before != nullptr, "Modified: before must be set");
                ok &= check(e.after  != nullptr, "Modified: after  must be set");
                break;
            }
        }
        return ok;
    }

    // -----------------------------------------------------------------------
    // Self-comparison always yields empty result

    bool test_self_comparison()
    {
        bool ok = true;
        Workspace ws1 = analyze_files({ws("added_fn_new.cpp"),
                                       ws("no_change_old.cpp")});

        SemanticDiff diff;
        auto result = diff.compare(ws1, ws1);

        ok &= check(result.empty(), "comparing a workspace to itself yields no changes");
        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_semantic_diff()
{
    static const TestCase cases[] = {
        {"added function",                   test_added_function},
        {"removed function",                 test_removed_function},
        {"renamed function",                 test_renamed_function},
        {"access changed",                   test_access_changed},
        {"static changed",                   test_static_changed},
        {"constexpr changed",                test_constexpr_changed},
        {"inline changed",                   test_inline_changed},
        {"parameter renamed only: no change",test_parameter_renamed_only},
        {"whitespace changes: no change",    test_whitespace_changes},
        {"comment changes: no change",       test_comment_changes},
        {"namespace moved",                  test_namespace_moved},
        {"no semantic change",               test_no_semantic_change},
        {"DiffEntry pointer invariants",     test_diff_entry_invariants},
        {"self-comparison: empty result",    test_self_comparison},
    };

    std::cout << "=== semantic diff tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
