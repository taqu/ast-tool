#include "test_search.h"
#include "ast-regex.h"
#include "ast-search.h"
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

    bool hasResult(const std::vector<const WorkspaceSymbol*>& results, std::u8string_view fqn)
    {
        for(const auto* r : results) {
            if(r->symbol.fqn == fqn) return true;
        }
        return false;
    }

    // -----------------------------------------------------------------------
    // RegexPattern unit tests

    bool test_regex_valid_pattern()
    {
        bool ok = true;
        RegexPattern pat("^get_.*");
        ok &= check(pat.valid(), "valid pattern compiles successfully");
        ok &= check(pat.error().empty(), "valid pattern has no error message");
        return ok;
    }

    bool test_regex_invalid_pattern()
    {
        bool ok = true;
        RegexPattern pat("(unclosed");
        ok &= check(!pat.valid(), "invalid pattern reports not valid");
        ok &= check(!pat.error().empty(), "invalid pattern has non-empty error message");
        return ok;
    }

    bool test_regex_matches()
    {
        bool ok = true;
        RegexPattern pat("^alpha");
        ok &= check(pat.valid(), "pattern is valid");
        ok &= check(pat.matches("alphaVar"),   "matches 'alphaVar' with ^alpha");
        ok &= check(pat.matches("alphaFunc"),  "matches 'alphaFunc' with ^alpha");
        ok &= check(!pat.matches("betaField"), "does not match 'betaField'");
        ok &= check(!pat.matches("BetaStruct"),"does not match 'BetaStruct'");
        return ok;
    }

    bool test_regex_partial_match()
    {
        bool ok = true;
        RegexPattern pat("Ns");
        ok &= check(pat.valid(), "pattern is valid");
        ok &= check(pat.matches("AlphaNs"),   "partial match finds 'Ns' inside 'AlphaNs'");
        ok &= check(!pat.matches("alphaVar"), "no match in 'alphaVar'");
        return ok;
    }

    bool test_regex_unicode()
    {
        bool ok = true;
        RegexPattern pat("\\p{Lu}");
        ok &= check(pat.valid(), "unicode uppercase pattern is valid");
        ok &= check(pat.matches("AlphaNs"),   "unicode pattern matches uppercase letter in 'AlphaNs'");
        ok &= check(!pat.matches("alphavar"), "unicode pattern does not match all-lowercase 'alphavar'");
        return ok;
    }

    // -----------------------------------------------------------------------
    // build_search_query validation tests

    bool test_build_query_valid()
    {
        bool ok = true;
        auto q = build_search_query(nullptr, nullptr, nullptr, nullptr,
                                    nullptr, nullptr, nullptr);
        ok &= check(q.has_value(), "empty build_search_query succeeds");
        return ok;
    }

    bool test_build_query_invalid_kind()
    {
        bool ok = true;
        auto q = build_search_query(nullptr, nullptr, u8"notakind", nullptr,
                                    nullptr, nullptr, nullptr);
        ok &= check(!q.has_value(), "unknown kind string is rejected");
        ok &= check(!q.error().empty(), "unknown kind error message is non-empty");
        return ok;
    }

    bool test_build_query_invalid_name_regex()
    {
        bool ok = true;
        auto q = build_search_query(nullptr, nullptr, nullptr, nullptr,
                                    u8"(bad", nullptr, nullptr);
        ok &= check(!q.has_value(), "invalid name_regex is rejected");
        return ok;
    }

    bool test_build_query_invalid_fqn_regex()
    {
        bool ok = true;
        auto q = build_search_query(nullptr, nullptr, nullptr, nullptr,
                                    nullptr, u8"[bad", nullptr);
        ok &= check(!q.has_value(), "invalid fqn_regex is rejected");
        return ok;
    }

    bool test_build_query_invalid_file_regex()
    {
        bool ok = true;
        auto q = build_search_query(nullptr, nullptr, nullptr, nullptr,
                                    nullptr, nullptr, u8"**bad**");
        ok &= check(!q.has_value(), "invalid file_regex is rejected");
        return ok;
    }

    bool test_build_query_conflict_name()
    {
        bool ok = true;
        auto q = build_search_query(u8"foo", nullptr, nullptr, nullptr,
                                    u8"^foo", nullptr, nullptr);
        ok &= check(!q.has_value(), "name + name_regex conflict is rejected");
        return ok;
    }

    bool test_build_query_conflict_fqn()
    {
        bool ok = true;
        auto q = build_search_query(nullptr, u8"ns::foo", nullptr, nullptr,
                                    nullptr, u8"^ns::", nullptr);
        ok &= check(!q.has_value(), "fqn + fqn_regex conflict is rejected");
        return ok;
    }

    bool test_build_query_conflict_file()
    {
        bool ok = true;
        auto q = build_search_query(nullptr, nullptr, nullptr, u8"src/",
                                    nullptr, nullptr, u8"\\.cpp$");
        ok &= check(!q.has_value(), "file + file_regex conflict is rejected");
        return ok;
    }

    bool test_build_query_kind_case_insensitive()
    {
        bool ok = true;
        auto q1 = build_search_query(nullptr, nullptr, u8"FUNCTION", nullptr,
                                     nullptr, nullptr, nullptr);
        ok &= check(q1.has_value(), "uppercase kind string accepted");
        ok &= check(q1.has_value() && q1->kind == SymbolKind::Function,
                    "uppercase 'FUNCTION' maps to SymbolKind::Function");

        auto q2 = build_search_query(nullptr, nullptr, u8"Struct", nullptr,
                                     nullptr, nullptr, nullptr);
        ok &= check(q2.has_value(), "mixed-case kind string accepted");
        ok &= check(q2.has_value() && q2->kind == SymbolKind::Struct,
                    "mixed-case 'Struct' maps to SymbolKind::Struct");
        return ok;
    }

    // -----------------------------------------------------------------------
    // SearchQuery filter evaluation tests

    bool test_search_empty_query()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);
        SemanticSearchEngine engine(ws);

        SearchQuery q;
        auto results = engine.search(q);
        ok &= check(!results.empty(), "empty query matches all symbols");
        ok &= check(results.size() == ws.symbols.size(),
                    "empty query returns every symbol in workspace");
        return ok;
    }

    bool test_search_name_exact()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);
        SemanticSearchEngine engine(ws);

        auto q = build_search_query(u8"alphaVar", nullptr, nullptr, nullptr,
                                    nullptr, nullptr, nullptr);
        ok &= check(q.has_value(), "query builds successfully");
        if(!q.has_value()) return false;
        auto results = engine.search(*q);
        ok &= check(!results.empty(), "exact name match finds alphaVar");
        ok &= check(hasResult(results, u8"AlphaNs::alphaVar"), "result fqn is AlphaNs::alphaVar");
        for(const auto* r : results) {
            ok &= check(r->symbol.name == u8"alphaVar", "all results have name == alphaVar");
        }
        return ok;
    }

    bool test_search_name_regex()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);
        SemanticSearchEngine engine(ws);

        auto q = build_search_query(nullptr, nullptr, nullptr, nullptr,
                                    u8"^alpha", nullptr, nullptr);
        ok &= check(q.has_value(), "query builds successfully");
        if(!q.has_value()) return false;
        auto results = engine.search(*q);
        ok &= check(!results.empty(), "name_regex ^alpha finds results");
        ok &= check(hasResult(results, u8"AlphaNs::alphaVar"),  "name_regex finds alphaVar");
        ok &= check(hasResult(results, u8"AlphaNs::alphaFunc"), "name_regex finds alphaFunc");
        ok &= check(!hasResult(results, u8"BetaStruct"),        "name_regex ^alpha excludes BetaStruct");
        return ok;
    }

    bool test_search_fqn_regex()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);
        SemanticSearchEngine engine(ws);

        auto q = build_search_query(nullptr, nullptr, nullptr, nullptr,
                                    nullptr, u8"^AlphaNs::", nullptr);
        ok &= check(q.has_value(), "query builds successfully");
        if(!q.has_value()) return false;
        auto results = engine.search(*q);
        ok &= check(!results.empty(),                         "fqn_regex ^AlphaNs:: finds results");
        ok &= check(hasResult(results, u8"AlphaNs::alphaVar"),  "fqn_regex finds alphaVar");
        ok &= check(hasResult(results, u8"AlphaNs::alphaFunc"), "fqn_regex finds alphaFunc");
        ok &= check(!hasResult(results, u8"AlphaNs"),           "fqn_regex excludes AlphaNs namespace itself");
        ok &= check(!hasResult(results, u8"BetaStruct"),        "fqn_regex excludes BetaStruct");
        return ok;
    }

    bool test_search_file_regex()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);
        SemanticSearchEngine engine(ws);

        auto q = build_search_query(nullptr, nullptr, nullptr, nullptr,
                                    nullptr, nullptr, u8"\\.cpp$");
        ok &= check(q.has_value(), "query builds successfully");
        if(!q.has_value()) return false;
        auto results = engine.search(*q);
        ok &= check(!results.empty(), "file_regex \\.cpp$ finds results");
        ok &= check(hasResult(results, u8"AlphaNs::alphaVar"), "file_regex finds symbols in .cpp files");
        for(const auto* r : results) {
            std::u8string sourceFile = r->sourceFile.u8string();
            bool endsCpp = sourceFile.size() >= 4 &&
                           sourceFile.substr(sourceFile.size() - 4) == u8".cpp";
            ok &= check(endsCpp, "every file_regex result comes from a .cpp file");
            if(!endsCpp) break;
        }
        return ok;
    }

    bool test_search_empty_result()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);
        SemanticSearchEngine engine(ws);

        auto q = build_search_query(nullptr, nullptr, nullptr, nullptr,
                                    u8"^zzz_no_such_symbol_xyz$", nullptr, nullptr);
        ok &= check(q.has_value(), "query builds successfully");
        if(!q.has_value()) return false;
        auto results = engine.search(*q);
        ok &= check(results.empty(), "name_regex that matches nothing returns empty results");
        return ok;
    }

    bool test_search_multiple_matches()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);
        SemanticSearchEngine engine(ws);

        auto q = build_search_query(nullptr, nullptr, nullptr, nullptr,
                                    u8".", nullptr, nullptr);
        ok &= check(q.has_value(), "query builds successfully");
        if(!q.has_value()) return false;
        auto results = engine.search(*q);
        ok &= check(results.size() > 1, "permissive name_regex returns multiple matches");
        return ok;
    }

    bool test_search_kind_exact_with_name_regex()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);
        SemanticSearchEngine engine(ws);

        // kind (exact) evaluated before name_regex — only functions reach regex stage
        auto q = build_search_query(nullptr, nullptr, u8"function", nullptr,
                                    u8".", nullptr, nullptr);
        ok &= check(q.has_value(), "query builds successfully");
        if(!q.has_value()) return false;
        auto results = engine.search(*q);
        for(const auto* r : results) {
            ok &= check(r->symbol.kind == SymbolKind::Function,
                        "kind=function + name_regex: every result is a function");
        }
        return ok;
    }

    bool test_search_kind_exact_with_fqn_regex()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);
        SemanticSearchEngine engine(ws);

        auto q = build_search_query(nullptr, nullptr, u8"variable", nullptr,
                                    nullptr, u8"^AlphaNs::", nullptr);
        ok &= check(q.has_value(), "query builds successfully");
        if(!q.has_value()) return false;
        auto results = engine.search(*q);
        ok &= check(!results.empty(), "kind=variable + fqn_regex finds results");
        for(const auto* r : results) {
            ok &= check(r->symbol.kind == SymbolKind::Variable,
                        "kind=variable + fqn_regex: every result is a variable");
        }
        ok &= check(hasResult(results, u8"AlphaNs::alphaVar"),
                    "kind=variable + fqn_regex finds AlphaNs::alphaVar");
        return ok;
    }

    bool test_search_fqn_regex_anchored()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);
        SemanticSearchEngine engine(ws);

        auto q = build_search_query(nullptr, nullptr, nullptr, nullptr,
                                    nullptr, u8"^Beta", nullptr);
        ok &= check(q.has_value(), "query builds successfully");
        if(!q.has_value()) return false;
        auto results = engine.search(*q);
        ok &= check(!results.empty(),                         "fqn_regex ^Beta finds results");
        ok &= check(hasResult(results, u8"BetaStruct"),         "fqn_regex finds BetaStruct");
        ok &= check(!hasResult(results, u8"AlphaNs::alphaVar"), "fqn_regex ^Beta excludes AlphaNs symbols");
        return ok;
    }

    bool test_search_file_exact_substring()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);
        SemanticSearchEngine engine(ws);

        // file exact uses substring containment, not full-path equality
        auto q = build_search_query(nullptr, nullptr, nullptr, u8"alpha.cpp",
                                    nullptr, nullptr, nullptr);
        ok &= check(q.has_value(), "query builds successfully");
        if(!q.has_value()) return false;
        auto results = engine.search(*q);
        ok &= check(!results.empty(), "file substring 'alpha.cpp' finds results");
        ok &= check(hasResult(results, u8"AlphaNs::alphaVar"),  "file substring finds alphaVar");
        ok &= check(hasResult(results, u8"AlphaNs::alphaFunc"), "file substring finds alphaFunc");
        for(const auto* r : results) {
            std::u8string sourceFile = r->sourceFile.u8string();
            ok &= check(sourceFile.find(u8"alpha.cpp") != std::string::npos,
                        "every result's source file contains 'alpha.cpp'");
        }

        // Verify it does not match beta.h symbols
        ok &= check(!hasResult(results, u8"BetaStruct"),
                    "file substring 'alpha.cpp' excludes BetaStruct from beta.h");
        return ok;
    }

    bool test_search_file_exact_no_match()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);
        SemanticSearchEngine engine(ws);

        auto q = build_search_query(nullptr, nullptr, nullptr, u8"no_such_file.cpp",
                                    nullptr, nullptr, nullptr);
        ok &= check(q.has_value(), "query builds successfully");
        if(!q.has_value()) return false;
        auto results = engine.search(*q);
        ok &= check(results.empty(), "file substring that matches no file returns empty results");
        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_search()
{
    static const TestCase cases[] = {
        // RegexPattern wrapper
        {"regex: valid pattern",                test_regex_valid_pattern},
        {"regex: invalid pattern",              test_regex_invalid_pattern},
        {"regex: matches",                      test_regex_matches},
        {"regex: partial match",                test_regex_partial_match},
        {"regex: unicode",                      test_regex_unicode},
        // build_search_query validation
        {"build: empty query valid",            test_build_query_valid},
        {"build: invalid kind",                 test_build_query_invalid_kind},
        {"build: invalid name_regex",           test_build_query_invalid_name_regex},
        {"build: invalid fqn_regex",            test_build_query_invalid_fqn_regex},
        {"build: invalid file_regex",           test_build_query_invalid_file_regex},
        {"build: name + name_regex conflict",   test_build_query_conflict_name},
        {"build: fqn + fqn_regex conflict",     test_build_query_conflict_fqn},
        {"build: file + file_regex conflict",   test_build_query_conflict_file},
        {"build: kind case-insensitive",        test_build_query_kind_case_insensitive},
        // Filter evaluation
        {"search: empty query matches all",     test_search_empty_query},
        {"search: name exact",                  test_search_name_exact},
        {"search: name_regex",                  test_search_name_regex},
        {"search: fqn_regex",                   test_search_fqn_regex},
        {"search: file_regex",                  test_search_file_regex},
        {"search: empty result",                test_search_empty_result},
        {"search: multiple matches",            test_search_multiple_matches},
        {"search: kind exact + name_regex",     test_search_kind_exact_with_name_regex},
        {"search: kind exact + fqn_regex",      test_search_kind_exact_with_fqn_regex},
        {"search: fqn_regex anchored",          test_search_fqn_regex_anchored},
        {"search: file exact substring",        test_search_file_exact_substring},
        {"search: file exact no match",         test_search_file_exact_no_match},
    };

    std::cout << "=== search + regex tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
