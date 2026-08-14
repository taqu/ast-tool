#include "ast-workspace.h"
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <unordered_set>
#include <atomic>
#include <thread>
#include "ast-tool.h"
#include "ast-extractor.h"
#include "ast-scope-builder.h"
#include "ast-symbol-scope.h"
#include "ast-ir.h"
#include <git2/global.h>
#include <git2/repository.h>
#include <git2/ignore.h>

namespace ast
{
namespace
{
    // -----------------------------------------------------------------------
    // Include / import collection
    // -----------------------------------------------------------------------

    /** Strips one leading and one trailing character (quotes or angle brackets). */
    static std::u8string strip_delimiters(const std::u8string& s)
    {
        if(s.size() >= 2) return s.substr(1, s.size() - 2);
        return s;
    }

    /**
     * Collects direct include/import paths from @p ast.
     * Paths are returned deduplicated and in source order.
     */
    std::vector<std::u8string> collect_includes(const AST& ast)
    {
        std::vector<std::u8string> result;
        std::unordered_set<std::u8string> seen;

        auto push = [&](std::u8string path) {
            if(!path.empty() && seen.insert(path).second) {
                result.push_back(std::move(path));
            }
        };

        for(uint32_t i = 0; i < ast.size(); ++i) {
            const ASTNode& node = ast[i];
            const char* t = node.type_;

            // C / C++: #include "foo.h" or #include <foo.h>
            if(0 == ::strcmp(t, "preproc_include")) {
                for(uintptr_t childIdx : node.children_) {
                    if(childIdx == InvalidId) continue;
                    const ASTNode& child = ast[childIdx];
                    const char* ct = child.type_;
                    if(0 == ::strcmp(ct, "string_literal")
                    || 0 == ::strcmp(ct, "system_lib_string")) {
                        push(strip_delimiters(child.getText()));
                        break;
                    }
                }
                continue;
            }

            // Python: import numpy  /  from os import path
            if(0 == ::strcmp(t, "import_statement")
            || 0 == ::strcmp(t, "import_from_statement")) {
                for(uintptr_t childIdx : node.children_) {
                    if(childIdx == InvalidId) continue;
                    const ASTNode& child = ast[childIdx];
                    const char* ct = child.type_;
                    if(0 == ::strcmp(ct, "dotted_name")
                    || 0 == ::strcmp(ct, "relative_import")) {
                        push(child.getText());
                        break;
                    }
                }
                continue;
            }

            // JavaScript / TypeScript: import ... from "module"
            if(0 == ::strcmp(t, "import_statement")) {
                for(uintptr_t childIdx : node.children_) {
                    if(childIdx == InvalidId) continue;
                    const ASTNode& child = ast[childIdx];
                    if(0 == ::strcmp(child.type_, "string")) {
                        push(strip_delimiters(child.getText()));
                        break;
                    }
                }
                continue;
            }

            // Rust: use std::collections::HashMap;
            if(0 == ::strcmp(t, "use_declaration")) {
                push(node.getText());
                continue;
            }

            // Java / Go: import java.util.List  /  import "fmt"
            if(0 == ::strcmp(t, "import_declaration")) {
                push(node.getText());
                continue;
            }

            // Go: individual path inside import block: import_spec → interpreted_string_literal
            if(0 == ::strcmp(t, "import_spec")) {
                for(uintptr_t childIdx : node.children_) {
                    if(childIdx == InvalidId) continue;
                    const ASTNode& child = ast[childIdx];
                    if(0 == ::strcmp(child.type_, "interpreted_string_literal")) {
                        push(strip_delimiters(child.getText()));
                        break;
                    }
                }
                continue;
            }
        }

        return result;
    }

    // -----------------------------------------------------------------------
    // Per-file analysis — returns an owned result; safe to call concurrently.
    // -----------------------------------------------------------------------

    struct AnalysisResult
    {
        TranslationUnit translationUnit;
        FileDependencies dependencies;
        std::vector<WorkspaceSymbol> symbols;
        bool parsed = false;
    };

    AnalysisResult analyze_one(const std::filesystem::path& path)
    {
        AnalysisResult result;

        AST ast = parse(path.u8string().c_str());
        if(!ast) return result;  // parsed remains false

        result.parsed = true;

        std::vector<Symbol> syms = extract_symbols(ast);
        ScopeTree tree = build_scope_tree(ast);
        associate_symbols(tree, syms);

        result.dependencies.file     = path;
        result.dependencies.includes = collect_includes(ast);

        result.symbols.reserve(syms.size());
        for(size_t i = 0; i < syms.size(); ++i) {
            uintptr_t scopeId = tree.getScopeOfSymbol(i);
            ScopeKind owning  = (scopeId != ScopeTree::InvalidId)
                              ? tree[scopeId].kind_
                              : ScopeKind::Unknown;
            WorkspaceSymbol wsym;
            wsym.symbol      = syms[i];
            wsym.sourceFile  = path;
            wsym.owningScope = owning;
            result.symbols.push_back(std::move(wsym));
        }

        result.translationUnit = {std::move(ast), std::move(tree), std::move(syms), path};
        return result;
    }

    // -----------------------------------------------------------------------
    // Recursive directory scanner
    // -----------------------------------------------------------------------

    void scan_recursive(
        const std::filesystem::path& dir,
        const IgnoreMatcher& matcher,
        std::vector<std::filesystem::path>& files)
    {
        std::error_code ec;
        for(const auto& entry : std::filesystem::directory_iterator(dir, ec)) {
            if(ec) break;

            if(matcher.valid()) {
                std::error_code relEc;
                std::filesystem::path rel = std::filesystem::relative(entry.path(), matcher.workdir(), relEc);
                if(!relEc && matcher.isIgnored(rel)) continue;
            }

            std::error_code typeEc;
            if(entry.is_directory(typeEc)) {
                scan_recursive(entry.path(), matcher, files);
            } else if(entry.is_regular_file(typeEc)) {
                auto ext = entry.path().extension();
                if(get_language_type_from_extension(ext.c_str()) != ASTLanguage::Unknown) {
                    files.push_back(entry.path());
                }
            }
        }
    }

} // anonymous namespace

// -----------------------------------------------------------------------
// IgnoreMatcher
// -----------------------------------------------------------------------

IgnoreMatcher::IgnoreMatcher(const std::filesystem::path& searchPath)
    : repo_(nullptr)
{
    git_libgit2_init();

    git_repository* repo = nullptr;
    std::u8string u8path = searchPath.u8string();
    const char* pathUtf8 = reinterpret_cast<const char*>(u8path.c_str());
    if(git_repository_open_ext(&repo, pathUtf8, 0, nullptr) == 0) {
        const char* wd = git_repository_workdir(repo);
        if(wd) {
            repo_ = repo;
            workdir_ = std::filesystem::path(reinterpret_cast<const char8_t*>(wd));
        } else {
            // Bare repository — no working directory; treat as non-git.
            git_repository_free(repo);
        }
    }
}

IgnoreMatcher::~IgnoreMatcher()
{
    if(repo_) {
        git_repository_free(static_cast<git_repository*>(repo_));
        repo_ = nullptr;
    }
    git_libgit2_shutdown();
}

IgnoreMatcher::IgnoreMatcher(IgnoreMatcher&& other) noexcept
    : repo_(other.repo_)
    , workdir_(std::move(other.workdir_))
{
    other.repo_ = nullptr;
}

IgnoreMatcher& IgnoreMatcher::operator=(IgnoreMatcher&& other) noexcept
{
    if(this != &other) {
        if(repo_) git_repository_free(static_cast<git_repository*>(repo_));
        repo_ = other.repo_;
        workdir_ = std::move(other.workdir_);
        other.repo_ = nullptr;
    }
    return *this;
}

bool IgnoreMatcher::valid() const noexcept
{
    return repo_ != nullptr;
}

const std::filesystem::path& IgnoreMatcher::workdir() const noexcept
{
    return workdir_;
}

bool IgnoreMatcher::isIgnored(const std::filesystem::path& relativePath) const
{
    if(!repo_) return false;

    std::string relStr = relativePath.string();
    for(char& c : relStr) {
        if(c == '\\') c = '/';
    }

    int ignored = 0;
    git_ignore_path_is_ignored(&ignored, static_cast<git_repository*>(repo_), relStr.c_str());
    return ignored != 0;
}

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

std::vector<std::filesystem::path> scan_workspace(const char8_t* root)
{
    std::vector<std::filesystem::path> files;
    if(nullptr == root) return files;

    std::filesystem::path rootPath(root);
    std::error_code ec;
    if(!std::filesystem::is_directory(rootPath, ec) || ec) return files;

    IgnoreMatcher matcher(rootPath);
    scan_recursive(rootPath, matcher, files);
    std::sort(files.begin(), files.end());
    return files;
}

Workspace analyze_workspace(const char8_t* root)
{
    return analyze_files(scan_workspace(root));
}

Workspace analyze_files(const std::vector<std::filesystem::path>& files)
{
    const size_t N = files.size();

    // Pre-allocate one result slot per file so workers write to distinct indices.
    std::vector<AnalysisResult> results(N);

    const uint32_t hwThreads = ast::get_physical_core_count();
    const size_t nThreads = (hwThreads > 1 && N > 1)
                          ? std::min(static_cast<size_t>(hwThreads), N)
                          : 0;

    if(nThreads > 0) {
        // Atomic counter drives dynamic load balancing: each worker grabs the
        // next unprocessed file index until the queue is exhausted.
        std::atomic<size_t> nextIdx{0};

        auto worker = [&]() noexcept {
            for(;;) {
                const size_t i = nextIdx.fetch_add(1, std::memory_order_relaxed);
                if(i >= N) break;
                try {
                    results[i] = analyze_one(files[i]);
                } catch(...) {
                    // Parse failure recorded via results[i].parsed == false.
                }
            }
        };

        std::vector<std::thread> threads;
        threads.reserve(nThreads);
        for(size_t t = 0; t < nThreads; ++t) threads.emplace_back(worker);
        for(std::thread& t : threads) t.join();
    } else {
        for(size_t i = 0; i < N; ++i) {
            results[i] = analyze_one(files[i]);
        }
    }

    // Single-threaded merge: combine per-file results into the shared Workspace.
    // Processing in file-index order preserves deterministic output ordering.
    Workspace ws;
    ws.files = files;
    ws.symbols.reserve(N * 8);
    ws.deps.reserve(N);
    ws.translationUnits.reserve(N);

    for(size_t i = 0; i < N; ++i) {
        AnalysisResult& r = results[i];
        if(!r.parsed) {
            ++ws.failedCount;
            continue;
        }
        ++ws.parsedCount;
        ws.deps.push_back(std::move(r.dependencies));
        ws.symbols.insert(ws.symbols.end(),
            std::make_move_iterator(r.symbols.begin()),
            std::make_move_iterator(r.symbols.end()));
        ws.translationUnits.push_back(std::move(r.translationUnit));
    }

    return ws;
}

} // namespace ast
