#include "ast-workspace.h"
#include <cstring>
#include <algorithm>
#include <filesystem>
#include <unordered_set>
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

    static std::u8string strip_delimiters(const std::u8string& s)
    {
        if(s.size() >= 2) return s.substr(1, s.size() - 2);
        return s;
    }

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

            if(0 == ::strcmp(t, "use_declaration")) {
                push(node.getText());
                continue;
            }

            if(0 == ::strcmp(t, "import_declaration")) {
                push(node.getText());
                continue;
            }

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
    AnalysisResult analyze_one(const std::filesystem::path& path)
    {
        AnalysisResult result;

        AST ast = parse(path.u8string().c_str());
        if(!ast) return result;

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
    // Merge a single AnalysisResult into the shared Workspace.
    // Call only while holding the workspace mutex.
    // -----------------------------------------------------------------------

    void merge_result(Workspace& ws, AnalysisResult&& r)
    {
        if(!r.parsed) {
            ++ws.failedCount;
            return;
        }
        ++ws.parsedCount;
        ws.deps.push_back(std::move(r.dependencies));
        ws.symbols.insert(ws.symbols.end(),
            std::make_move_iterator(r.symbols.begin()),
            std::make_move_iterator(r.symbols.end()));
        ws.translationUnits.push_back(std::move(r.translationUnit));
    }

    void merge_result(Workspace& ws, AnalysisResult&& r, std::function<bool(const WorkspaceSymbol&)> match, std::mutex& mu)
    {
        if(!r.parsed) {
            std::lock_guard lk(mu);
            ++ws.failedCount;
            return;
        }
        bool found = false;
        for(WorkspaceSymbol& sym : r.symbols) {
            if(match(sym)) {
                std::lock_guard lk(mu);
                ws.symbols.push_back(std::move(sym));
                found = true;
            }
        }

        if(found){
            std::lock_guard lk(mu);
            ++ws.parsedCount;
            ws.deps.push_back(std::move(r.dependencies));
            ws.translationUnits.push_back(std::move(r.translationUnit));
        }
    }

    // -----------------------------------------------------------------------
    // Sort workspace vectors by file path for deterministic output.
    // Called once after all workers have finished, with no lock needed.
    // -----------------------------------------------------------------------

    void sort_workspace(Workspace& ws)
    {
        std::sort(ws.translationUnits.begin(), ws.translationUnits.end(),
            [](const TranslationUnit& a, const TranslationUnit& b) {
                return a.path < b.path;
            });

        std::sort(ws.deps.begin(), ws.deps.end(),
            [](const FileDependencies& a, const FileDependencies& b) {
                return a.file < b.file;
            });

        std::sort(ws.symbols.begin(), ws.symbols.end(),
            [](const WorkspaceSymbol& a, const WorkspaceSymbol& b) {
                if(a.sourceFile != b.sourceFile) return a.sourceFile < b.sourceFile;
                if(a.symbol.line != b.symbol.line) return a.symbol.line < b.symbol.line;
                return a.symbol.column < b.symbol.column;
            });
    }

    // -----------------------------------------------------------------------
    // Recursive directory scanner — emits discovered paths via callback.
    // -----------------------------------------------------------------------

    template<typename Emit>
    void scan_recursive(
        const std::filesystem::path& dir,
        const IgnoreMatcher& matcher,
        Emit&& emit)
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
                scan_recursive(entry.path(), matcher, emit);
            } else if(entry.is_regular_file(typeEc)) {
                auto ext = entry.path().extension();
                if(get_language_type_from_extension(ext.c_str()) != ASTLanguage::Unknown) {
                    emit(entry.path());
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
    scan_recursive(rootPath, matcher, [&](std::filesystem::path p) {
        files.push_back(std::move(p));
    });
    std::sort(files.begin(), files.end());
    return files;
}

Workspace analyze_workspace(const char8_t* root)
{
    if(nullptr == root) return {};

    std::filesystem::path rootPath(root);
    std::error_code ec;
    if(!std::filesystem::is_directory(rootPath, ec) || ec) return {};

    constexpr size_t kQueueCapacity = 256;
    BlockingQueue<std::filesystem::path> queue(kQueueCapacity);

    // All discovered paths, collected by the scan thread.
    // Read by the main thread only after scanThread.join().
    std::vector<std::filesystem::path> allFiles;

    Workspace ws;
    std::mutex wsMu;

    // Producer: scan the directory tree and stream paths into the queue.
    std::thread scanThread([&]() noexcept {
        try {
            IgnoreMatcher matcher(rootPath);
            scan_recursive(rootPath, matcher, [&](std::filesystem::path p) {
                allFiles.push_back(p);
                queue.push(std::move(p));
            });
        } catch(...) {}
        queue.markDone();
    });

    // Workers: consume paths, analyze each file, immediately merge the result.
    const uint32_t hwThreads = ast::get_physical_core_count();
    const uint32_t nWorkers  = std::max(1u, hwThreads);

    std::vector<std::thread> workers;
    workers.reserve(nWorkers);
    for(uint32_t i = 0; i < nWorkers; ++i) {
        workers.emplace_back([&]() noexcept {
            std::filesystem::path path;
            while(queue.pop(path)) {
                try {
                    AnalysisResult r = analyze_one(path);
                    std::lock_guard lk(wsMu);
                    merge_result(ws, std::move(r));
                } catch(...) {
                    std::lock_guard lk(wsMu);
                    ++ws.failedCount;
                }
            }
        });
    }

    scanThread.join();
    for(std::thread& w : workers) w.join();

    std::sort(allFiles.begin(), allFiles.end());
    ws.files = std::move(allFiles);

    sort_workspace(ws);
    return ws;
}

Workspace analyze_files(const std::vector<std::filesystem::path>& files)
{
    const size_t N = files.size();
    if(N == 0) return {};

    constexpr size_t kQueueCapacity = 256;
    BlockingQueue<std::filesystem::path> queue(std::min(kQueueCapacity, N));

    Workspace ws;
    ws.files = files;
    ws.symbols.reserve(N * 8);
    ws.deps.reserve(N);
    ws.translationUnits.reserve(N);
    std::mutex wsMu;

    const uint32_t hwThreads = ast::get_physical_core_count();
    const size_t nWorkers = (hwThreads > 1 && N > 1)
                          ? std::min(static_cast<size_t>(hwThreads), N)
                          : 1;

    // Feed the input file list into the bounded queue.
    std::thread feeder([&]() noexcept {
        for(const auto& p : files) {
            queue.push(p);
        }
        queue.markDone();
    });

    std::vector<std::thread> workers;
    workers.reserve(nWorkers);
    for(size_t i = 0; i < nWorkers; ++i) {
        workers.emplace_back([&]() noexcept {
            std::filesystem::path path;
            while(queue.pop(path)) {
                try {
                    AnalysisResult r = analyze_one(path);
                    std::lock_guard lk(wsMu);
                    merge_result(ws, std::move(r));
                } catch(...) {
                    std::lock_guard lk(wsMu);
                    ++ws.failedCount;
                }
            }
        });
    }

    feeder.join();
    for(std::thread& w : workers) w.join();

    sort_workspace(ws);
    return ws;
}

} // namespace ast
