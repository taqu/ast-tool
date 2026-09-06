#include "ast-workspace.h"
#include "ast-cache-db.h"
#include "ast-cache.h"
#include "ast-extractor.h"
#include "ast-ir.h"
#include "ast-scope-builder.h"
#include "ast-symbol-scope.h"
#include "ast-tool.h"
#include "xxhash.h"
#include <algorithm>
#include <filesystem>
#include <git2/global.h>
#include <git2/ignore.h>
#include <git2/repository.h>
#include <unordered_set>
#if defined(_WIN32) || defined(_WIN64)
#    include <sys/stat.h>
#    include <sys/types.h>
#    define WS_STAT_STRUCT struct _stat64
#    define WS_STAT_FUNC(path, buf) _stat64(path, buf)
#else
#    include <sys/stat.h>
#    include <sys/types.h>
#    define WS_STAT_STRUCT struct stat
#    define WS_STAT_FUNC(path, buf) stat(path, buf)
#endif

namespace ast
{

// ── Workspace special members ─────────────────────────────────────────────────

Workspace::Workspace() = default;

Workspace::~Workspace()
{
    delete persistentCache_;
}

Workspace::Workspace(Workspace&& other) noexcept
    : files(std::move(other.files))
    , symbols(std::move(other.symbols))
    , deps(std::move(other.deps))
    , translationUnits(std::move(other.translationUnits))
    , parsedCount(other.parsedCount)
    , failedCount(other.failedCount)
    , tuIndex_(std::move(other.tuIndex_))
    , cacheHits_(other.cacheHits_)
    , cacheMisses_(other.cacheMisses_)
    , persistentCacheHits_(other.persistentCacheHits_)
    , persistentCacheMisses_(other.persistentCacheMisses_)
    , workspaceRoot_(std::move(other.workspaceRoot_))
    , persistentCache_(other.persistentCache_)
{
    other.persistentCache_ = nullptr;
}

Workspace& Workspace::operator=(Workspace&& other) noexcept
{
    if(this != &other) {
        delete persistentCache_;
        files = std::move(other.files);
        symbols = std::move(other.symbols);
        deps = std::move(other.deps);
        translationUnits = std::move(other.translationUnits);
        parsedCount = other.parsedCount;
        failedCount = other.failedCount;
        tuIndex_ = std::move(other.tuIndex_);
        cacheHits_ = other.cacheHits_;
        cacheMisses_ = other.cacheMisses_;
        persistentCacheHits_ = other.persistentCacheHits_;
        persistentCacheMisses_ = other.persistentCacheMisses_;
        workspaceRoot_ = std::move(other.workspaceRoot_);
        persistentCache_ = other.persistentCache_;
        other.persistentCache_ = nullptr;
    }
    return *this;
}

namespace
{
    // -----------------------------------------------------------------------
    // Include / import collection
    // -----------------------------------------------------------------------

    static std::u8string strip_delimiters(const std::u8string& s)
    {
        if(s.size() >= 2) {
            return s.substr(1, s.size() - 2);
        }
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
            ASTNodeType t = node.type_;

            if(t == ASTNodeType::PreprocInclude) {
                for(uintptr_t childIdx: node.children_) {
                    if(childIdx == InvalidId) {
                        continue;
                    }
                    const ASTNode& child = ast[childIdx];
                    ASTNodeType ct = child.type_;
                    if(ct == ASTNodeType::StringLiteral
                       || ct == ASTNodeType::SystemLibString) {
                        push(strip_delimiters(child.getText()));
                        break;
                    }
                }
                continue;
            }

            if(t == ASTNodeType::ImportStatement
               || t == ASTNodeType::ImportFromStatement) {
                for(uintptr_t childIdx: node.children_) {
                    if(childIdx == InvalidId) {
                        continue;
                    }
                    const ASTNode& child = ast[childIdx];
                    ASTNodeType ct = child.type_;
                    if(ct == ASTNodeType::DottedName
                       || ct == ASTNodeType::RelativeImport) {
                        push(child.getText());
                        break;
                    }
                }
                continue;
            }

            if(t == ASTNodeType::ImportStatement) {
                for(uintptr_t childIdx: node.children_) {
                    if(childIdx == InvalidId) {
                        continue;
                    }
                    const ASTNode& child = ast[childIdx];
                    if(child.type_ == ASTNodeType::String) {
                        push(strip_delimiters(child.getText()));
                        break;
                    }
                }
                continue;
            }

            if(t == ASTNodeType::UseDeclaration) {
                push(node.getText());
                continue;
            }

            if(t == ASTNodeType::ImportDeclaration) {
                push(node.getText());
                continue;
            }

            if(t == ASTNodeType::ImportSpec) {
                for(uintptr_t childIdx: node.children_) {
                    if(childIdx == InvalidId) {
                        continue;
                    }
                    const ASTNode& child = ast[childIdx];
                    if(child.type_ == ASTNodeType::InterpretedStringLiteral) {
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
    // Per-file analysis — builds scope/symbol layers from an already-parsed AST.
    // -----------------------------------------------------------------------
    AnalysisResult analyze_from_ast(AST&& ast, const std::filesystem::path& path)
    {
        AnalysisResult result;
        if(!ast)
            return result;
        result.parsed = true;

        std::vector<Symbol> syms = extract_symbols(ast);
        ScopeTree tree = build_scope_tree(ast);
        associate_symbols(tree, syms);

        result.dependencies.file = path;
        result.dependencies.includes = collect_includes(ast);

        result.symbols.reserve(syms.size());
        for(size_t i = 0; i < syms.size(); ++i) {
            uintptr_t scopeId = tree.getScopeOfSymbol(i);
            ScopeKind owning = (scopeId != ScopeTree::InvalidId)
                                   ? tree[scopeId].kind_
                                   : ScopeKind::Unknown;
            WorkspaceSymbol wsym;
            wsym.symbol = syms[i];
            wsym.sourceFile = path;
            wsym.owningScope = owning;
            result.symbols.push_back(std::move(wsym));
        }

        result.translationUnit = {std::move(ast), std::move(tree), std::move(syms), path};
        return result;
    }

    AnalysisResult analyze_one(const std::filesystem::path& path)
    {
        return analyze_from_ast(parse(path.u8string().c_str()), path);
    }

    // -----------------------------------------------------------------------
    // File metadata helpers for cache invalidation.
    // -----------------------------------------------------------------------

    bool get_file_stat_ws(const std::filesystem::path& path, int64_t& outSize, int64_t& outMtime)
    {
        WS_STAT_STRUCT st;
        std::string p = path.string();
        if(WS_STAT_FUNC(p.c_str(), &st) != 0)
            return false;
        outSize = (int64_t)st.st_size;
        outMtime = (int64_t)st.st_mtime;
        return true;
    }

    // Compute XXH64 of file content. Returns 0 on failure.
    uint64_t hash_file(const std::filesystem::path& path)
    {
        std::string p = path.string();
        FILE* file = nullptr;
        errno_t err = fopen_s(&file, p.c_str(), "rb");
        if(0 != err) {
            return 0;
        }
        XXH64_state_t* state = XXH64_createState();
        XXH64_reset(state, 0);
        char buf[65536];
        size_t n;
        while((n = fread(buf, 1, sizeof(buf), file)) > 0) {
            XXH64_update(state, buf, n);
        }
        fclose(file);
        uint64_t h = XXH64_digest(state);
        XXH64_freeState(state);
        return h;
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
        std::u8string key = r.translationUnit.path.lexically_normal().u8string();
        ws.tuIndex_[key] = ws.translationUnits.size();
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
        for(WorkspaceSymbol& sym: r.symbols) {
            if(match(sym)) {
                std::lock_guard lk(mu);
                ws.symbols.push_back(std::move(sym));
                found = true;
            }
        }

        if(found) {
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
                      if(a.sourceFile != b.sourceFile)
                          return a.sourceFile < b.sourceFile;
                      if(a.symbol.line != b.symbol.line)
                          return a.symbol.line < b.symbol.line;
                      return a.symbol.column < b.symbol.column;
                  });

        // Rebuild tuIndex_ after sorting so get_translation_unit() stays consistent.
        ws.tuIndex_.clear();
        for(size_t i = 0; i < ws.translationUnits.size(); ++i) {
            std::u8string key = ws.translationUnits[i].path.lexically_normal().u8string();
            ws.tuIndex_[key] = i;
        }
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
        for(const auto& entry: std::filesystem::directory_iterator(dir, ec)) {
            if(ec)
                break;

            if(matcher.valid()) {
                std::error_code relEc;
                std::filesystem::path rel = std::filesystem::relative(entry.path(), matcher.workdir(), relEc);
                if(!relEc && matcher.isIgnored(rel))
                    continue;
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
        if(repo_)
            git_repository_free(static_cast<git_repository*>(repo_));
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
    if(!repo_)
        return false;

    std::string relStr = relativePath.string();
    for(char& c: relStr) {
        if(c == '\\')
            c = '/';
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
    if(nullptr == root)
        return files;

    std::filesystem::path rootPath(root);
    std::error_code ec;
    if(!std::filesystem::is_directory(rootPath, ec) || ec)
        return files;

    IgnoreMatcher matcher(rootPath);
    scan_recursive(rootPath, matcher, [&](std::filesystem::path p) {
        files.push_back(std::move(p));
    });
    std::sort(files.begin(), files.end());
    return files;
}

void scan_workspace_stream(const std::filesystem::path& root,
                           std::function<void(std::filesystem::path)> emit)
{
    std::error_code ec;
    if(!std::filesystem::is_directory(root, ec) || ec)
        return;
    IgnoreMatcher matcher(root);
    scan_recursive(root, matcher, std::move(emit));
}

Workspace analyze_workspace(const char8_t* root)
{
    if(nullptr == root)
        return {};

    std::filesystem::path rootPath(root);
    std::error_code ec;
    if(!std::filesystem::is_directory(rootPath, ec) || ec)
        return {};

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
        } catch(...) {
        }
        queue.markDone();
    });

    // Workers: consume paths, analyze each file, immediately merge the result.
    const uint32_t hwThreads = ast::get_physical_core_count();
    const uint32_t nWorkers = std::max(1u, hwThreads);

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
    for(std::thread& w: workers) w.join();

    std::sort(allFiles.begin(), allFiles.end());
    ws.files = std::move(allFiles);

    sort_workspace(ws);
    return ws;
}

Workspace analyze_files(const std::vector<std::filesystem::path>& files)
{
    const size_t N = files.size();
    if(N == 0)
        return {};

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
        for(const auto& p: files) {
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
    for(std::thread& w: workers) w.join();

    sort_workspace(ws);
    return ws;
}

Workspace open_workspace(const char8_t* root)
{
    Workspace ws;
    if(root) {
        ws.workspaceRoot_ = std::filesystem::path(root);
        std::filesystem::path cacheDir = ws.workspaceRoot_ / ".ast-tool";
        std::error_code ec;
        std::filesystem::create_directories(cacheDir, ec);
        if(!ec) {
            ws.persistentCache_ = new ASTCacheDatabase();
            if(!ws.persistentCache_->open(cacheDir / "ast-cache.db")) {
                delete ws.persistentCache_;
                ws.persistentCache_ = nullptr;
            }
        }
    }
    ws.files = scan_workspace(root);
    ws.translationUnits.reserve(ws.files.size());
    ws.symbols.reserve(ws.files.size() * 8);
    ws.deps.reserve(ws.files.size());
    return ws;
}

// Store an AnalysisResult into the workspace's lazy caches (memory + persistent).
// All modified fields are mutable, so const Workspace& is valid here.
static void commit_result(const Workspace& ws, AnalysisResult&& r,
                          const std::u8string& key, const std::filesystem::path& path)
{
    // Persistent cache store.
    if(ws.persistentCache_ && ws.persistentCache_->is_open()) {
        const AST& ast = r.translationUnit.ast;
        std::vector<uint8_t> rawBytes = ast_serialize(ast);
        if(!rawBytes.empty()) {
            std::vector<uint8_t> compressed = ast_lz4_compress(rawBytes.data(), rawBytes.size());
            ASTCacheDatabase::Entry e;
            // Compute source hash from source text.
            if(ast.text() && ast.text_size() > 0) {
                e.source_hash = XXH64(ast.text(), (size_t)ast.text_size(), 0);
            }
            int64_t fsize = 0, fmtime = 0;
            get_file_stat_ws(path, fsize, fmtime);
            e.source_size = fsize;
            e.source_mtime = fmtime;
            e.language = static_cast<uint32_t>(ast.language());
            e.format_version = kAstCacheFormatVersion;
            e.uncompressed_size = (int64_t)rawBytes.size();
            if(!compressed.empty() && compressed.size() < rawBytes.size()) {
                e.compression = AstCompressionMode::LZ4;
                e.blob = std::move(compressed);
            } else {
                e.compression = AstCompressionMode::None;
                e.blob = std::move(rawBytes);
            }
            ws.persistentCache_->store(path.string(), e);
        }
    }

    size_t idx = ws.translationUnits.size();
    ws.tuIndex_[key] = idx;
    ++ws.parsedCount;
    ws.deps.push_back(std::move(r.dependencies));
    ws.symbols.insert(ws.symbols.end(),
                      std::make_move_iterator(r.symbols.begin()),
                      std::make_move_iterator(r.symbols.end()));
    ws.translationUnits.push_back(std::move(r.translationUnit));
}

const TranslationUnit* Workspace::get_translation_unit(
    const std::filesystem::path& path) const
{
    std::u8string key = path.lexically_normal().u8string();

    // 1. Memory cache.
    auto it = tuIndex_.find(key);
    if(it != tuIndex_.end()) {
        ++cacheHits_;
        return &translationUnits[it->second];
    }
    ++cacheMisses_;

    // 2. Persistent SQLite cache.
    if(persistentCache_ && persistentCache_->is_open()) {
        ASTCacheDatabase::Entry e;
        if(persistentCache_->lookup(path.string(), e)) {
            bool valid = (e.format_version == kAstCacheFormatVersion);
            if(valid) {
                // Fast path: mtime + size match.
                int64_t curSize = 0, curMtime = 0;
                bool statOk = get_file_stat_ws(path, curSize, curMtime);
                if(statOk && curSize == e.source_size && curMtime == e.source_mtime) {
                    // Trusted hit — decompress and deserialize.
                } else if(statOk) {
                    // Slow path: content hash check.
                    uint64_t curHash = hash_file(path);
                    valid = (curHash == e.source_hash && curHash != 0);
                    if(valid) {
                        // Update mtime/size in DB to restore fast path next time.
                        e.source_size = curSize;
                        e.source_mtime = curMtime;
                        persistentCache_->store(path.string(), e);
                    }
                } else {
                    valid = false;
                }
            }
            if(valid && !e.blob.empty()) {
                // Decompress if needed.
                std::vector<uint8_t> raw;
                if(e.compression == AstCompressionMode::LZ4) {
                    raw = ast_lz4_decompress(e.blob.data(), e.blob.size(),
                                             (size_t)e.uncompressed_size);
                } else {
                    raw = std::move(e.blob);
                }
                if(!raw.empty()) {
                    AST deserialized = ast_deserialize(raw.data(), raw.size());
                    if(deserialized) {
                        ++persistentCacheHits_;
                        AnalysisResult r = analyze_from_ast(std::move(deserialized), path);
                        if(r.parsed) {
                            size_t idx = translationUnits.size();
                            tuIndex_[key] = idx;
                            ++parsedCount;
                            deps.push_back(std::move(r.dependencies));
                            symbols.insert(symbols.end(),
                                           std::make_move_iterator(r.symbols.begin()),
                                           std::make_move_iterator(r.symbols.end()));
                            translationUnits.push_back(std::move(r.translationUnit));
                            return &translationUnits[idx];
                        }
                    }
                }
                // Deserialization failed: evict the corrupt entry.
                persistentCache_->remove(path.string());
            } else if(!valid) {
                persistentCache_->remove(path.string());
            }
        }
        ++persistentCacheMisses_;
    }

    // 3. Parse fresh with Tree-sitter.
    AnalysisResult r = analyze_one(path);
    if(!r.parsed) {
        ++failedCount;
        return nullptr;
    }

    // Store in persistent cache + memory cache.
    commit_result(*this, std::move(r), key, path);
    return &translationUnits[tuIndex_[key]];
}

void Workspace::ensure_all_loaded() const
{
    for(const std::filesystem::path& p: files) {
        get_translation_unit(p);
    }
}

} // namespace ast
