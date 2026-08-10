#include "test_workspace.h"
#include "ast-workspace.h"
#include <algorithm>
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

    bool hasFile(const std::vector<std::string>& files, std::string_view suffix)
    {
        for(const auto& f : files) {
            if(f.size() >= suffix.size() &&
               f.substr(f.size() - suffix.size()) == suffix) {
                return true;
            }
        }
        return false;
    }

    bool hasSymbolFqn(const std::vector<WorkspaceSymbol>& syms, std::string_view fqn, SymbolKind kind)
    {
        for(const auto& s : syms) {
            if(s.symbol.fqn == fqn && s.symbol.kind == kind) return true;
        }
        return false;
    }

    bool hasSymbolFqn(const std::vector<WorkspaceSymbol>& syms, std::string_view fqn)
    {
        for(const auto& s : syms) {
            if(s.symbol.fqn == fqn) return true;
        }
        return false;
    }

    bool hasSymbolInFile(const std::vector<WorkspaceSymbol>& syms,
                         std::string_view fqn, std::string_view fileSuffix)
    {
        for(const auto& s : syms) {
            if(s.symbol.fqn != fqn) continue;
            if(s.sourceFile.size() >= fileSuffix.size() &&
               s.sourceFile.substr(s.sourceFile.size() - fileSuffix.size()) == fileSuffix) {
                return true;
            }
        }
        return false;
    }

    bool hasDep(const std::vector<FileDependencies>& deps,
                std::string_view fileSuffix, std::string_view include)
    {
        for(const auto& d : deps) {
            if(d.file.size() < fileSuffix.size()) continue;
            if(d.file.substr(d.file.size() - fileSuffix.size()) != fileSuffix) continue;
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
        auto files = scan_workspace(kWorkspaceRoot);

        ok &= check(files.size() >= 4, "scan finds at least 4 files");
        ok &= check(hasFile(files, "alpha.cpp"),    "scan finds alpha.cpp");
        ok &= check(hasFile(files, "beta.h"),       "scan finds beta.h");
        ok &= check(hasFile(files, "gamma.py"),     "scan finds gamma.py");
        ok &= check(hasFile(files, "delta.cpp"),    "scan finds delta.cpp (recursive)");

        // Result must be sorted
        ok &= check(std::is_sorted(files.begin(), files.end()), "scan result is sorted");

        return ok;
    }

    bool test_scan_null_root()
    {
        bool ok = true;
        auto files = scan_workspace(nullptr);
        ok &= check(files.empty(), "null root returns empty list");
        auto files2 = scan_workspace("nonexistent_path_xyz_99999");
        ok &= check(files2.empty(), "nonexistent root returns empty list");
        return ok;
    }

    bool test_analyze_workspace_counts()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kWorkspaceRoot);

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
        Workspace ws = analyze_workspace(kWorkspaceRoot);

        // AlphaNs::alphaVar must come from alpha.cpp
        ok &= check(hasSymbolInFile(ws.symbols, "AlphaNs::alphaVar", "alpha.cpp"),
                    "AlphaNs::alphaVar has sourceFile=alpha.cpp");

        // DeltaNs::deltaVar must come from sub/delta.cpp
        ok &= check(hasSymbolInFile(ws.symbols, "DeltaNs::deltaVar", "delta.cpp"),
                    "DeltaNs::deltaVar has sourceFile=delta.cpp");

        // BetaStruct from beta.h
        ok &= check(hasSymbolInFile(ws.symbols, "BetaStruct", "beta.h"),
                    "BetaStruct has sourceFile=beta.h");

        return ok;
    }

    bool test_symbols_from_all_files()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kWorkspaceRoot);

        // C++ symbols
        ok &= check(hasSymbolFqn(ws.symbols, "AlphaNs", SymbolKind::Namespace),
                    "AlphaNs namespace found");
        ok &= check(hasSymbolFqn(ws.symbols, "AlphaNs::alphaVar", SymbolKind::Variable),
                    "AlphaNs::alphaVar found");
        ok &= check(hasSymbolFqn(ws.symbols, "BetaStruct", SymbolKind::Struct),
                    "BetaStruct found");
        ok &= check(hasSymbolFqn(ws.symbols, "DeltaNs", SymbolKind::Namespace),
                    "DeltaNs namespace found");

        // Python class from gamma.py
        ok &= check(hasSymbolFqn(ws.symbols, "GammaClass"),
                    "GammaClass from gamma.py found");

        return ok;
    }

    bool test_owning_scope()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kWorkspaceRoot);

        // AlphaNs (namespace) should be in Global scope
        for(const auto& s : ws.symbols) {
            if(s.symbol.fqn == "AlphaNs" && s.symbol.kind == SymbolKind::Namespace) {
                ok &= check(s.owningScope == ScopeKind::Global,
                            "AlphaNs owning scope is Global");
            }
            // alphaVar lives inside AlphaNs → Namespace scope
            if(s.symbol.fqn == "AlphaNs::alphaVar") {
                ok &= check(s.owningScope == ScopeKind::Namespace,
                            "alphaVar owning scope is Namespace");
            }
            // BetaStruct::betaField lives inside a Struct scope
            if(s.symbol.fqn == "BetaStruct::betaField") {
                ok &= check(s.owningScope == ScopeKind::Struct,
                            "betaField owning scope is Struct");
            }
        }

        return ok;
    }

    bool test_dependency_graph()
    {
        bool ok = true;
        Workspace ws = analyze_workspace(kWorkspaceRoot);

        // alpha.cpp includes beta.h
        ok &= check(hasDep(ws.deps, "alpha.cpp", "beta.h"),
                    "alpha.cpp depends on beta.h");

        // gamma.py imports os
        ok &= check(hasDep(ws.deps, "gamma.py", "os"),
                    "gamma.py imports os");

        // delta.cpp has no includes
        for(const auto& d : ws.deps) {
            if(d.file.size() >= 9 &&
               d.file.substr(d.file.size() - 9) == "delta.cpp") {
                ok &= check(d.includes.empty(), "delta.cpp has no includes");
            }
        }

        return ok;
    }

    bool test_analyze_files_explicit()
    {
        bool ok = true;
        std::vector<std::string> files = {
            "test/ast-workspace/workspace/alpha.cpp",
            "test/ast-workspace/workspace/beta.h",
        };
        Workspace ws = analyze_files(files);

        ok &= check(ws.files.size() == 2,    "analyze_files: 2 files");
        ok &= check(ws.parsedCount == 2,     "analyze_files: 2 parsed");
        ok &= check(ws.failedCount == 0,     "analyze_files: 0 failed");
        ok &= check(hasSymbolFqn(ws.symbols, "AlphaNs", SymbolKind::Namespace),
                    "analyze_files: AlphaNs symbol found");
        ok &= check(hasSymbolFqn(ws.symbols, "BetaStruct", SymbolKind::Struct),
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

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_workspace()
{
    static const TestCase cases[] = {
        {"scan workspace",             test_scan_workspace},
        {"scan null/invalid root",     test_scan_null_root},
        {"analyze: file counts",       test_analyze_workspace_counts},
        {"symbol source files",        test_symbol_source_file},
        {"symbols from all files",     test_symbols_from_all_files},
        {"owning scope kind",          test_owning_scope},
        {"dependency graph",           test_dependency_graph},
        {"analyze_files (explicit)",   test_analyze_files_explicit},
        {"analyze empty inputs",       test_analyze_empty},
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
