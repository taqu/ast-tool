#include "test_workspace.h"
#include "ast-workspace.h"
#include <algorithm>
#include <fstream>
#include <iostream>
#include <string_view>
#include <git2/global.h>
#include <git2/repository.h>

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

    bool hasFile(const std::vector<std::filesystem::path>& files, std::u8string_view suffix)
    {
        for(const auto& f : files) {
            std::u8string file = f.u8string();
            if(file.size() >= suffix.size() &&
               file.substr(file.size() - suffix.size()) == suffix) {
                return true;
            }
        }
        return false;
    }

    bool hasSymbolFqn(const std::vector<WorkspaceSymbol>& syms, std::u8string_view fqn, SymbolKind kind)
    {
        for(const auto& s : syms) {
            if(s.symbol.fqn == fqn && s.symbol.kind == kind) return true;
        }
        return false;
    }

    bool hasSymbolFqn(const std::vector<WorkspaceSymbol>& syms, std::u8string_view fqn)
    {
        for(const auto& s : syms) {
            if(s.symbol.fqn == fqn) return true;
        }
        return false;
    }

    bool hasSymbolInFile(const std::vector<WorkspaceSymbol>& syms,
                         std::u8string_view fqn, std::u8string_view fileSuffix)
    {
        for(const auto& s : syms) {
            if(s.symbol.fqn != fqn) continue;
            std::u8string sourceFile = s.sourceFile.u8string();
            if(sourceFile.size() >= fileSuffix.size() &&
               sourceFile.substr(sourceFile.size() - fileSuffix.size()) == fileSuffix) {
                return true;
            }
        }
        return false;
    }

    bool hasDep(const std::vector<FileDependencies>& deps,
                std::u8string_view fileSuffix, std::u8string_view include)
    {
        for(const auto& d : deps) {
            std::u8string file = d.file.u8string();
            if(file.size() < fileSuffix.size()) continue;
            if(file.substr(file.size() - fileSuffix.size()) != fileSuffix) continue;
            for(const auto& inc : d.includes) {
                if(inc == include) return true;
            }
        }
        return false;
    }

    // -----------------------------------------------------------------------

    bool test_scan_workspace()
    {
        bool ok = true;
        auto files = scan_workspace((const char8_t*)kWorkspaceRoot);

        ok &= check(files.size() >= 4, "scan finds at least 4 files");
        ok &= check(hasFile(files, u8"alpha.cpp"),    "scan finds alpha.cpp");
        ok &= check(hasFile(files, u8"beta.h"),       "scan finds beta.h");
        ok &= check(hasFile(files, u8"gamma.py"),     "scan finds gamma.py");
        ok &= check(hasFile(files, u8"delta.cpp"),    "scan finds delta.cpp (recursive)");

        // Result must be sorted
        ok &= check(std::is_sorted(files.begin(), files.end()), "scan result is sorted");

        return ok;
    }

    bool test_scan_null_root()
    {
        bool ok = true;
        auto files = scan_workspace(nullptr);
        ok &= check(files.empty(), "null root returns empty list");
        auto files2 = scan_workspace(u8"nonexistent_path_xyz_99999");
        ok &= check(files2.empty(), "nonexistent root returns empty list");
        return ok;
    }

    bool test_analyze_workspace_counts()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);

        ok &= check(ws.files.size() >= 4, "workspace has at least 4 files");
        ok &= check(ws.parsedCount >= 4,  "at least 4 files parsed successfully");
        ok &= check(ws.failedCount == 0,  "no parse failures");
        ok &= check(!ws.symbols.empty(),  "workspace has symbols");
        ok &= check(ws.deps.size() == ws.parsedCount, "one deps entry per parsed file");

        return ok;
    }

    bool test_symbol_source_file()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);

        // AlphaNs::alphaVar must come from alpha.cpp
        ok &= check(hasSymbolInFile(ws.symbols, u8"AlphaNs::alphaVar", u8"alpha.cpp"),
                    "AlphaNs::alphaVar has sourceFile=alpha.cpp");

        // DeltaNs::deltaVar must come from sub/delta.cpp
        ok &= check(hasSymbolInFile(ws.symbols, u8"DeltaNs::deltaVar", u8"delta.cpp"),
                    "DeltaNs::deltaVar has sourceFile=delta.cpp");

        // BetaStruct from beta.h
        ok &= check(hasSymbolInFile(ws.symbols, u8"BetaStruct", u8"beta.h"),
                    "BetaStruct has sourceFile=beta.h");

        return ok;
    }

    bool test_symbols_from_all_files()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);

        // C++ symbols
        ok &= check(hasSymbolFqn(ws.symbols, u8"AlphaNs", SymbolKind::Namespace),
                    "AlphaNs namespace found");
        ok &= check(hasSymbolFqn(ws.symbols, u8"AlphaNs::alphaVar", SymbolKind::Variable),
                    "AlphaNs::alphaVar found");
        ok &= check(hasSymbolFqn(ws.symbols, u8"BetaStruct", SymbolKind::Struct),
                    "BetaStruct found");
        ok &= check(hasSymbolFqn(ws.symbols, u8"DeltaNs", SymbolKind::Namespace),
                    "DeltaNs namespace found");

        // Python class from gamma.py
        ok &= check(hasSymbolFqn(ws.symbols, u8"GammaClass"),
                    "GammaClass from gamma.py found");

        return ok;
    }

    bool test_owning_scope()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);

        // AlphaNs (namespace) should be in Global scope
        for(const auto& s : ws.symbols) {
            if(s.symbol.fqn == u8"AlphaNs" && s.symbol.kind == SymbolKind::Namespace) {
                ok &= check(s.owningScope == ScopeKind::Global,
                            "AlphaNs owning scope is Global");
            }
            // alphaVar lives inside AlphaNs → Namespace scope
            if(s.symbol.fqn == u8"AlphaNs::alphaVar") {
                ok &= check(s.owningScope == ScopeKind::Namespace,
                            "alphaVar owning scope is Namespace");
            }
            // BetaStruct::betaField lives inside a Struct scope
            if(s.symbol.fqn == u8"BetaStruct::betaField") {
                ok &= check(s.owningScope == ScopeKind::Struct,
                            "betaField owning scope is Struct");
            }
        }

        return ok;
    }

    bool test_dependency_graph()
    {
        bool ok = true;
        Workspace ws = analyze_workspace((const char8_t*)kWorkspaceRoot);

        // alpha.cpp includes beta.h
        ok &= check(hasDep(ws.deps, u8"alpha.cpp", u8"beta.h"),
                    "alpha.cpp depends on beta.h");

        // gamma.py imports os
        ok &= check(hasDep(ws.deps, u8"gamma.py", u8"os"),
                    "gamma.py imports os");

        // delta.cpp has no includes
        for(const auto& d : ws.deps) {
            std::u8string file = d.file.u8string();
            if(file.size() >= 9 &&
               file.substr(file.size() - 9) == u8"delta.cpp") {
                ok &= check(d.includes.empty(), "delta.cpp has no includes");
            }
        }

        return ok;
    }

    bool test_analyze_files_explicit()
    {
        bool ok = true;
        std::vector<std::filesystem::path> files = {
            "test/ast-workspace/workspace/alpha.cpp",
            "test/ast-workspace/workspace/beta.h",
        };
        Workspace ws = analyze_files(files);

        ok &= check(ws.files.size() == 2,    "analyze_files: 2 files");
        ok &= check(ws.parsedCount == 2,     "analyze_files: 2 parsed");
        ok &= check(ws.failedCount == 0,     "analyze_files: 0 failed");
        ok &= check(hasSymbolFqn(ws.symbols, u8"AlphaNs", SymbolKind::Namespace),
                    "analyze_files: AlphaNs symbol found");
        ok &= check(hasSymbolFqn(ws.symbols, u8"BetaStruct", SymbolKind::Struct),
                    "analyze_files: BetaStruct found");

        return ok;
    }

    bool test_analyze_empty()
    {
        bool ok = true;
        Workspace ws = analyze_files({});
        ok &= check(ws.symbols.empty(),   "empty file list → no symbols");
        ok &= check(ws.parsedCount == 0,  "empty file list → parsedCount == 0");
        ok &= check(ws.failedCount == 0,  "empty file list → failedCount == 0");

        Workspace ws2 = analyze_workspace(nullptr);
        ok &= check(ws2.symbols.empty(), "null root → no symbols");

        return ok;
    }

    // -----------------------------------------------------------------------
    // Git ignore tests
    // -----------------------------------------------------------------------

    struct TempRepo
    {
        std::filesystem::path root;
        bool ok = false;

        explicit TempRepo(const char* suffix)
        {
            std::error_code ec;
            root = std::filesystem::temp_directory_path(ec) / suffix;
            if(ec) return;
            std::filesystem::remove_all(root, ec);
            std::filesystem::create_directories(root, ec);
            if(ec) return;

            git_libgit2_init();
            git_repository* repo = nullptr;
            std::u8string rootU8 = root.u8string();
            const char* rootUtf8 = reinterpret_cast<const char*>(rootU8.c_str());
            if(git_repository_init(&repo, rootUtf8, 0) == 0) {
                git_repository_free(repo);
                ok = true;
            }
        }

        ~TempRepo()
        {
            std::error_code ec;
            std::filesystem::remove_all(root, ec);
            git_libgit2_shutdown();
        }

        void write(const std::filesystem::path& relPath, const char* content) const
        {
            auto full = root / relPath;
            std::error_code ec;
            std::filesystem::create_directories(full.parent_path(), ec);
            std::ofstream(full) << content;
        }

        TempRepo(const TempRepo&) = delete;
        TempRepo& operator=(const TempRepo&) = delete;
    };

    bool test_git_ignore_basic()
    {
        TempRepo repo("ast-tool-test-ignore-basic");
        if(!repo.ok) {
            std::cerr << "    SKIP: could not init git repo\n";
            return true;
        }

        repo.write(".gitignore", "build/\n");
        repo.write("src/main.cpp", "int main() {}\n");
        repo.write("build/generated.cpp", "void gen() {}\n");

        std::u8string rootU8 = repo.root.u8string();
        auto files = scan_workspace(rootU8.c_str());

        bool ok = true;
        ok &= check(hasFile(files, u8"main.cpp"),      "src/main.cpp is scanned");
        ok &= check(!hasFile(files, u8"generated.cpp"), "build/generated.cpp is skipped");
        return ok;
    }

    bool test_git_ignore_nested()
    {
        TempRepo repo("ast-tool-test-ignore-nested");
        if(!repo.ok) {
            std::cerr << "    SKIP: could not init git repo\n";
            return true;
        }

        repo.write(".gitignore", "src/generated/\n");
        repo.write("src/main.cpp", "int main() {}\n");
        repo.write("src/generated/gen.cpp", "void gen() {}\n");

        std::u8string rootU8 = repo.root.u8string();
        auto files = scan_workspace(rootU8.c_str());

        bool ok = true;
        ok &= check(hasFile(files, u8"main.cpp"),  "src/main.cpp is scanned");
        ok &= check(!hasFile(files, u8"gen.cpp"),   "src/generated/gen.cpp is skipped");
        return ok;
    }

    bool test_git_ignore_file_pattern()
    {
        TempRepo repo("ast-tool-test-ignore-pattern");
        if(!repo.ok) {
            std::cerr << "    SKIP: could not init git repo\n";
            return true;
        }

        repo.write(".gitignore", "*.generated.cpp\n");
        repo.write("src/main.cpp", "int main() {}\n");
        repo.write("src/foo.generated.cpp", "void foo() {}\n");

        std::u8string rootU8 = repo.root.u8string();
        auto files = scan_workspace(rootU8.c_str());

        bool ok = true;
        ok &= check(hasFile(files, u8"main.cpp"),          "main.cpp is scanned");
        ok &= check(!hasFile(files, u8"foo.generated.cpp"), "foo.generated.cpp is skipped");
        return ok;
    }

    bool test_non_git_workspace()
    {
        std::error_code ec;
        auto tmp = std::filesystem::temp_directory_path(ec) / "ast-tool-test-nongit";
        std::filesystem::remove_all(tmp, ec);
        std::filesystem::create_directories(tmp / "src", ec);
        { std::ofstream(tmp / "src" / "main.cpp") << "int main() {}\n"; }

        std::u8string rootU8 = tmp.u8string();
        auto files = scan_workspace(rootU8.c_str());

        bool ok = check(!files.empty(), "non-git workspace scans successfully");
        ok &= check(hasFile(files, u8"main.cpp"), "main.cpp found in non-git workspace");

        std::filesystem::remove_all(tmp, ec);
        return ok;
    }

    bool test_open_workspace_is_lazy()
    {
        bool ok = true;
        Workspace ws = open_workspace((const char8_t*)kWorkspaceRoot);

        ok &= check(ws.files.size() >= 4, "open_workspace: discovers files");
        ok &= check(ws.parsedCount == 0,  "open_workspace: parsedCount is 0 (no parsing yet)");
        ok &= check(ws.failedCount == 0,  "open_workspace: failedCount is 0");
        ok &= check(ws.translationUnits.empty(), "open_workspace: no TUs loaded yet");
        ok &= check(ws.symbols.empty(),           "open_workspace: no symbols yet");
        ok &= check(std::is_sorted(ws.files.begin(), ws.files.end()),
                    "open_workspace: files are sorted");
        return ok;
    }

    bool test_get_translation_unit_parses_on_miss()
    {
        bool ok = true;
        Workspace ws = open_workspace((const char8_t*)kWorkspaceRoot);

        // Pick the first file that was discovered.
        if(ws.files.empty()) {
            std::cerr << "    SKIP: no files in workspace\n";
            return true;
        }

        const TranslationUnit* tu = ws.get_translation_unit(ws.files[0]);
        ok &= check(tu != nullptr,          "get_translation_unit: returns non-null on valid path");
        ok &= check(ws.parsedCount == 1,    "get_translation_unit: parsedCount is 1 after first access");
        ok &= check(ws.cacheHits_ == 0,     "get_translation_unit: no cache hits yet");
        ok &= check(ws.cacheMisses_ == 1,   "get_translation_unit: one cache miss");
        return ok;
    }

    bool test_get_translation_unit_cache_reuse()
    {
        bool ok = true;
        Workspace ws = open_workspace((const char8_t*)kWorkspaceRoot);

        if(ws.files.empty()) {
            std::cerr << "    SKIP: no files in workspace\n";
            return true;
        }

        const TranslationUnit* tu1 = ws.get_translation_unit(ws.files[0]);
        ok &= check(tu1 != nullptr,         "cache reuse: first access succeeds");
        ok &= check(ws.parsedCount == 1,    "cache reuse: parsedCount == 1 after first access");

        const TranslationUnit* tu2 = ws.get_translation_unit(ws.files[0]);
        ok &= check(tu2 != nullptr,         "cache reuse: second access succeeds");
        ok &= check(tu1 == tu2,             "cache reuse: same pointer returned (no re-parse)");
        ok &= check(ws.parsedCount == 1,    "cache reuse: parsedCount still 1 (no re-parse)");
        ok &= check(ws.cacheHits_ == 1,     "cache reuse: one cache hit recorded");
        return ok;
    }

    bool test_get_translation_unit_independent_paths()
    {
        bool ok = true;
        Workspace ws = open_workspace((const char8_t*)kWorkspaceRoot);

        if(ws.files.size() < 2) {
            std::cerr << "    SKIP: need at least 2 files\n";
            return true;
        }

        ws.get_translation_unit(ws.files[0]);
        ok &= check(ws.parsedCount == 1, "independent paths: first file parses one TU");

        ws.get_translation_unit(ws.files[1]);
        ok &= check(ws.parsedCount == 2, "independent paths: second file parses a second TU");

        // All other files should remain unloaded.
        ok &= check(ws.translationUnits.size() == 2,
                    "independent paths: only 2 TUs loaded");
        return ok;
    }

    bool test_ensure_all_loaded()
    {
        bool ok = true;
        Workspace ws = open_workspace((const char8_t*)kWorkspaceRoot);
        ok &= check(ws.parsedCount == 0, "ensure_all_loaded: starts lazy");

        ws.ensure_all_loaded();
        ok &= check(ws.parsedCount == ws.files.size(),
                    "ensure_all_loaded: parsedCount == files.size() after full load");
        ok &= check(!ws.symbols.empty(), "ensure_all_loaded: symbols populated");
        ok &= check(!ws.translationUnits.empty(), "ensure_all_loaded: TUs populated");
        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_workspace()
{
    static const TestCase cases[] = {
        {"open_workspace is lazy",              test_open_workspace_is_lazy},
        {"get_translation_unit: parse on miss", test_get_translation_unit_parses_on_miss},
        {"get_translation_unit: cache reuse",   test_get_translation_unit_cache_reuse},
        {"get_translation_unit: independent paths", test_get_translation_unit_independent_paths},
        {"ensure_all_loaded",                   test_ensure_all_loaded},
        {"scan workspace",             test_scan_workspace},
        {"scan null/invalid root",     test_scan_null_root},
        {"analyze: file counts",       test_analyze_workspace_counts},
        {"symbol source files",        test_symbol_source_file},
        {"symbols from all files",     test_symbols_from_all_files},
        {"owning scope kind",          test_owning_scope},
        {"dependency graph",           test_dependency_graph},
        {"analyze_files (explicit)",   test_analyze_files_explicit},
        {"analyze empty inputs",       test_analyze_empty},
        {"git ignore: basic",          test_git_ignore_basic},
        {"git ignore: nested dir",     test_git_ignore_nested},
        {"git ignore: file pattern",   test_git_ignore_file_pattern},
        {"non-git workspace",          test_non_git_workspace},
    };

    std::cout << "=== workspace analysis tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
