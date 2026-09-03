#include "test_error_ux.h"
#include "ast-tool.h"
#include "ast-workspace.h"
#include "cli-semantic.h"
#include <iostream>
#include <string_view>
#include <vector>

namespace ast
{
namespace
{
    static constexpr const char* kEuxRoot      = "test/ast-error-ux/workspace";
    static constexpr const char* kEuxEmptyRoot = "test/ast-error-ux/workspace_empty";
    static constexpr const char* kCalRoot      = "test/ast-callers/workspace";

    bool check(bool condition, const char* description)
    {
        if(!condition)
            std::cerr << "    FAIL: " << description << "\n";
        return condition;
    }

    bool contains(const std::string& haystack, std::string_view needle)
    {
        return haystack.find(needle) != std::string::npos;
    }

    size_t count_occurrences(const std::string& haystack, std::string_view needle)
    {
        size_t n = 0, pos = 0;
        while((pos = haystack.find(needle, pos)) != std::string::npos) {
            ++n;
            pos += needle.size();
        }
        return n;
    }

    bool braces_balanced(const std::string& s)
    {
        int depth = 0;
        for(char c : s) {
            if(c == '{') ++depth;
            else if(c == '}') --depth;
            if(depth < 0) return false;
        }
        return depth == 0;
    }

    bool dispatch_command(std::vector<const char*> argv)
    {
        Arguments arguments;
        std::vector<std::u8string> args = args_to_utf8(static_cast<int32_t>(argv.size()), argv.data());
        if(!parse(arguments, args)) return false;
        return dispatch(arguments);
    }

    // -----------------------------------------------------------------------
    // make_*() builders — structured field checks

    bool test_symbol_not_found_builder()
    {
        bool ok = true;
        cli::RecoveryError err = cli::make_symbol_not_found(u8"auth::AuthToken::validate", u8"test/ast-error-ux/workspace");
        ok &= check(err.category == cli::ErrorCategory::SymbolNotFound, "category is SymbolNotFound");
        ok &= check(err.query == u8"auth::AuthToken::validate", "query is preserved verbatim");
        ok &= check(err.candidates.empty(), "symbol-not-found carries no candidates");
        // An over-qualified query suggests re-searching just the last component.
        ok &= check(err.next == u8"search validate test/ast-error-ux/workspace",
                    "next suggests searching the short (unqualified) name");
        return ok;
    }

    bool test_symbol_not_found_builder_unqualified()
    {
        bool ok = true;
        cli::RecoveryError err = cli::make_symbol_not_found(u8"lonelyThing", u8".");
        ok &= check(err.next == u8"search lonelyThing .", "an already-short query is suggested as-is");
        return ok;
    }

    bool test_unknown_option_builder()
    {
        bool ok = true;
        cli::RecoveryError err = cli::make_unknown_option(u8"--foo", {u8"--json", u8"--pretty"});
        ok &= check(err.category == cli::ErrorCategory::UnknownOption, "category is UnknownOption");
        ok &= check(err.query == u8"--foo", "query holds the offending option");
        ok &= check(err.validOptions.size() == 2, "validOptions carries the small relevant option set");
        return ok;
    }

    bool test_invalid_arguments_builder()
    {
        bool ok = true;
        cli::RecoveryError err = cli::make_invalid_arguments(u8"missing PATH", u8"ast-tool callers SYMBOL PATH");
        ok &= check(err.category == cli::ErrorCategory::InvalidArguments, "category is InvalidArguments");
        ok &= check(err.usage == u8"ast-tool callers SYMBOL PATH", "usage carries the minimal invocation shape");
        return ok;
    }

    bool test_invalid_query_builder()
    {
        bool ok = true;
        cli::RecoveryError err = cli::make_invalid_query(u8"unknown kind 'bogus'");
        ok &= check(err.category == cli::ErrorCategory::InvalidQuery, "category is InvalidQuery");
        ok &= check(err.message == u8"unknown kind 'bogus'", "message is preserved");
        return ok;
    }

    bool test_unsupported_query_form_builder()
    {
        bool ok = true;
        cli::RecoveryError err = cli::make_unsupported_query_form(
            u8"'NsCal' is a Namespace — callers requires a function, method, constructor, or destructor",
            u8"inspect with search or symbols");
        ok &= check(err.category == cli::ErrorCategory::UnsupportedQueryForm, "category is UnsupportedQueryForm");
        ok &= check(err.next == u8"inspect with search or symbols", "next carries the single recommended action");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Ambiguous symbol — workspace-driven, real candidate data

    bool test_ambiguous_unqualified()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kEuxRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"validate");
        ok &= check(candidates.size() == 2, "unqualified 'validate' resolves to exactly 2 candidates");
        if(candidates.size() < 2) return false;

        cli::RecoveryError err = cli::make_ambiguous_symbol(u8"validate", candidates);
        ok &= check(err.category == cli::ErrorCategory::AmbiguousSymbol, "category is AmbiguousSymbol");
        ok &= check(err.candidates.size() == 2, "both candidates are included (under the bound)");
        ok &= check(err.totalCandidates == 2, "totalCandidates matches the full count");
        ok &= check(err.next == u8"retry with a fully-qualified name",
                    "an unqualified ambiguous query recommends qualifying it");
        return ok;
    }

    bool test_ambiguous_already_qualified()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kEuxRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"euxns::dup");
        ok &= check(candidates.size() == 2, "qualified 'euxns::dup' still resolves to 2 genuine candidates");
        if(candidates.size() < 2) return false;

        cli::RecoveryError err = cli::make_ambiguous_symbol(u8"euxns::dup", candidates);
        ok &= check(err.next == u8"inspect candidates with search or symbols",
                    "an already-qualified ambiguous query recommends inspecting candidates, not re-qualifying");
        return ok;
    }

    bool test_ambiguous_candidates_bounded()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kEuxRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"manyDup");
        ok &= check(candidates.size() == 6, "'manyDup' resolves to 6 real candidates");
        if(candidates.size() < 6) return false;

        cli::RecoveryError err = cli::make_ambiguous_symbol(u8"manyDup", candidates);
        ok &= check(err.candidates.size() == cli::kMaxErrorCandidates,
                    "candidate list is bounded to kMaxErrorCandidates");
        ok &= check(err.totalCandidates == 6, "totalCandidates still reports the true count");

        std::string text = cli::render_error_text(err);
        ok &= check(contains(text, "showing 5 of 6 candidates"), "text output reports the bound and true total");
        ok &= check(count_occurrences(text, "Function") == cli::kMaxErrorCandidates,
                    "exactly the bounded number of candidate lines are printed");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Text rendering — compact, actionable, no oversized dumps

    bool test_render_text_symbol_not_found()
    {
        bool ok = true;
        cli::RecoveryError err = cli::make_symbol_not_found(u8"AuthToken::validate", u8".");
        std::string text = cli::render_error_text(err);
        ok &= check(contains(text, "error: symbol not found: AuthToken::validate"), "category and query are stated plainly");
        ok &= check(contains(text, "next: search validate ."), "a single actionable next step is included");
        ok &= check(text.size() < 200, "output stays compact (no help dump)");
        return ok;
    }

    bool test_render_text_unknown_option_no_help_dump()
    {
        bool ok = true;
        cli::RecoveryError err = cli::make_unknown_option(u8"--foo", {u8"--json", u8"--pretty"});
        std::string text = cli::render_error_text(err);
        ok &= check(contains(text, "unknown option: --foo"), "offending option is named");
        ok &= check(contains(text, "available: --json, --pretty"), "a small relevant option set is shown");
        ok &= check(!contains(text, "SYNOPSIS") && !contains(text, "DESCRIPTION") && !contains(text, "EXAMPLES"),
                    "no full command help is dumped");
        ok &= check(text.size() < 200, "output stays compact");
        return ok;
    }

    bool test_render_text_invalid_arguments_usage_only()
    {
        bool ok = true;
        cli::RecoveryError err = cli::make_invalid_arguments(u8"missing PATH", u8"ast-tool callers SYMBOL PATH");
        std::string text = cli::render_error_text(err);
        ok &= check(contains(text, "error: missing PATH"), "the specific problem is stated");
        ok &= check(contains(text, "usage: ast-tool callers SYMBOL PATH"), "the minimal usage shape is shown");
        ok &= check(!contains(text, "OPTIONS") && !contains(text, "EXAMPLES"), "no full CLI documentation is dumped");
        return ok;
    }

    // -----------------------------------------------------------------------
    // JSON rendering — valid, compact, structured

    bool test_render_json_ambiguous_compact()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kEuxRoot);
        auto candidates = cli::resolve_symbol_query(ws, u8"manyDup");
        cli::RecoveryError err = cli::make_ambiguous_symbol(u8"manyDup", candidates);

        std::string json = cli::render_error_json(err, false);
        ok &= check(braces_balanced(json), "compact JSON braces are balanced");
        ok &= check(json.find('\n') == json.size() - 1, "compact JSON is single-line (only the trailing newline)");
        ok &= check(contains(json, "\"error\":\"ambiguous_symbol\""), "machine-readable error token is present");
        ok &= check(contains(json, "\"query\":\"manyDup\""), "query field is present");
        ok &= check(contains(json, "\"total_candidates\":6"), "total_candidates reflects the true count");
        ok &= check(count_occurrences(json, "\"fqn\":") == cli::kMaxErrorCandidates,
                    "JSON candidate list is bounded the same as text output");
        return ok;
    }

    bool test_render_json_pretty_still_balanced()
    {
        bool ok = true;
        cli::RecoveryError err = cli::make_symbol_not_found(u8"validate", u8".");
        std::string json = cli::render_error_json(err, true);
        ok &= check(braces_balanced(json), "pretty JSON braces are balanced");
        ok &= check(contains(json, "\n"), "pretty JSON is multi-line");
        ok &= check(contains(json, "\"error\": \"symbol_not_found\""), "pretty JSON still carries the error token");
        return ok;
    }

    bool test_render_json_backwards_compatible_fields_omitted_when_unset()
    {
        bool ok = true;
        cli::RecoveryError err = cli::make_invalid_arguments(u8"missing PATH", u8"ast-tool callers SYMBOL PATH");
        std::string json = cli::render_error_json(err, false);
        ok &= check(!contains(json, "\"candidates\""), "unset optional fields are omitted, not emitted empty");
        ok &= check(!contains(json, "\"query\""), "unset query field is omitted");
        return ok;
    }

    // -----------------------------------------------------------------------
    // Argument parsing — unrecognized flags never become positionals

    bool test_parse_callers_captures_bad_option()
    {
        bool ok = true;
        Arguments arguments;
        const char* argv[] = {"ast-tool", "callers", "--bogus", "calTarget", kCalRoot};
        std::vector<std::u8string> args = args_to_utf8(5, argv);
        ok &= check(parse(arguments, args), "parse_callers always accepts a recognized subcommand");
        ok &= check(arguments.callers_.badOption_ != nullptr, "the unrecognized flag is captured");
        if(arguments.callers_.badOption_) {
            ok &= check(std::u8string_view(arguments.callers_.badOption_) == u8"--bogus",
                        "badOption_ holds the exact offending token");
        }
        ok &= check(arguments.callers_.symbol_ != nullptr && arguments.callers_.root_ != nullptr,
                    "symbol/root positionals are still recovered around the bad flag");
        return ok;
    }

    bool test_parse_search_captures_bad_option()
    {
        bool ok = true;
        Arguments arguments;
        const char* argv[] = {"ast-tool", "search", "--bogus", kCalRoot};
        std::vector<std::u8string> args = args_to_utf8(4, argv);
        ok &= check(parse(arguments, args), "parse_search always accepts a recognized subcommand");
        ok &= check(arguments.search_.badOption_ != nullptr, "the unrecognized flag is captured");
        ok &= check(arguments.search_.root_ != nullptr, "root positional is still recovered");
        return ok;
    }

    bool test_parse_find_captures_bad_option()
    {
        bool ok = true;
        Arguments arguments;
        const char* argv[] = {"ast-tool", "find", "--bogus", "data/test00.cpp"};
        std::vector<std::u8string> args = args_to_utf8(4, argv);
        ok &= check(parse(arguments, args), "parse_find always accepts a recognized subcommand");
        ok &= check(arguments.find_.badOption_ != nullptr, "the unrecognized flag is captured");
        ok &= check(arguments.find_.input_ != nullptr, "file positional is still recovered");
        return ok;
    }

    // -----------------------------------------------------------------------
    // End-to-end dispatch — real CLI trajectories, exit-code level

    bool test_e2e_missing_args_fails_cleanly()
    {
        bool ok = true;
        ok &= check(dispatch_command({"ast-tool", "callers"}) == false, "callers with no args fails");
        ok &= check(dispatch_command({"ast-tool", "references"}) == false, "references with no args fails");
        ok &= check(dispatch_command({"ast-tool", "callees"}) == false, "callees with no args fails");
        ok &= check(dispatch_command({"ast-tool", "search"}) == false, "search with no root fails");
        ok &= check(dispatch_command({"ast-tool", "find"}) == false, "find with no file fails");
        return ok;
    }

    bool test_e2e_unknown_option_fails_cleanly()
    {
        bool ok = true;
        ok &= check(dispatch_command({"ast-tool", "callers", "--bogus", "calTarget", kCalRoot}) == false,
                    "callers with an unrecognized flag fails");
        ok &= check(dispatch_command({"ast-tool", "search", "--bogus", kCalRoot}) == false,
                    "search with an unrecognized flag fails");
        return ok;
    }

    bool test_e2e_symbol_not_found_fails_cleanly()
    {
        return check(dispatch_command({"ast-tool", "callers", "totallyMissingSymbolXyz", kEuxRoot}) == false,
                     "an unresolved symbol fails");
    }

    bool test_e2e_ambiguous_fails_cleanly()
    {
        bool ok = true;
        ok &= check(dispatch_command({"ast-tool", "callers", "validate", kEuxRoot}) == false,
                    "an ambiguous unqualified symbol fails");
        ok &= check(dispatch_command({"ast-tool", "callers", "--json", "manyDup", kEuxRoot}) == false,
                    "an ambiguous symbol fails under --json too");
        return ok;
    }

    bool test_e2e_unsupported_query_form_fails_cleanly()
    {
        bool ok = true;
        ok &= check(dispatch_command({"ast-tool", "callers", "euxNsOnly", kEuxRoot}) == false,
                    "callers on a non-callable target fails");
        ok &= check(dispatch_command({"ast-tool", "callees", "euxNsOnly", kEuxRoot}) == false,
                    "callees on a non-callable target fails");
        return ok;
    }

    bool test_e2e_empty_result_succeeds()
    {
        bool ok = true;
        ok &= check(dispatch_command({"ast-tool", "callers", "euxLonely", kEuxRoot}) == true,
                    "a resolved, callable symbol with zero call sites is a success, not a failure");
        ok &= check(dispatch_command({"ast-tool", "references", "euxLonely", kEuxRoot}) == true,
                    "a resolved symbol with zero references is a success, not a failure");
        return ok;
    }

    bool test_e2e_empty_workspace_fails_cleanly()
    {
        return check(dispatch_command({"ast-tool", "callers", "foo", kEuxEmptyRoot}) == false,
                     "an empty/unanalyzable workspace root fails");
    }

    bool test_e2e_invalid_query_fails_cleanly()
    {
        return check(dispatch_command({"ast-tool", "search", "--kind", "bogus", kCalRoot}) == false,
                     "an invalid --kind value fails");
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_error_ux()
{
    static const TestCase cases[] = {
        {"make_symbol_not_found: over-qualified query",     test_symbol_not_found_builder},
        {"make_symbol_not_found: already-short query",      test_symbol_not_found_builder_unqualified},
        {"make_unknown_option builder",                     test_unknown_option_builder},
        {"make_invalid_arguments builder",                  test_invalid_arguments_builder},
        {"make_invalid_query builder",                      test_invalid_query_builder},
        {"make_unsupported_query_form builder",              test_unsupported_query_form_builder},
        {"ambiguous: unqualified query",                    test_ambiguous_unqualified},
        {"ambiguous: already-qualified query",              test_ambiguous_already_qualified},
        {"ambiguous: candidate list is bounded",            test_ambiguous_candidates_bounded},
        {"text render: symbol not found",                   test_render_text_symbol_not_found},
        {"text render: unknown option, no help dump",       test_render_text_unknown_option_no_help_dump},
        {"text render: invalid arguments, usage only",      test_render_text_invalid_arguments_usage_only},
        {"json render: ambiguous is compact and bounded",   test_render_json_ambiguous_compact},
        {"json render: pretty stays balanced",              test_render_json_pretty_still_balanced},
        {"json render: unset fields omitted",               test_render_json_backwards_compatible_fields_omitted_when_unset},
        {"parse: callers captures bad option",              test_parse_callers_captures_bad_option},
        {"parse: search captures bad option",               test_parse_search_captures_bad_option},
        {"parse: find captures bad option",                 test_parse_find_captures_bad_option},
        {"e2e: missing arguments fails cleanly",            test_e2e_missing_args_fails_cleanly},
        {"e2e: unknown option fails cleanly",                test_e2e_unknown_option_fails_cleanly},
        {"e2e: symbol not found fails cleanly",             test_e2e_symbol_not_found_fails_cleanly},
        {"e2e: ambiguous symbol fails cleanly",             test_e2e_ambiguous_fails_cleanly},
        {"e2e: unsupported query form fails cleanly",       test_e2e_unsupported_query_form_fails_cleanly},
        {"e2e: empty result is a success",                  test_e2e_empty_result_succeeds},
        {"e2e: empty workspace fails cleanly",              test_e2e_empty_workspace_fails_cleanly},
        {"e2e: invalid query fails cleanly",                test_e2e_invalid_query_fails_cleanly},
    };

    std::cout << "=== error UX tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
