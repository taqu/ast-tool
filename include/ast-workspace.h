#ifndef INC_AST_WORKSPACE_H_
#define INC_AST_WORKSPACE_H_
/**
 * @file ast-workspace.h
 * @brief Workspace-level analysis: recursive file scan, multi-file parsing,
 *        symbol aggregation, include/import collection, and dependency graph.
 *
 * The analysis pipeline for a workspace is:
 *   1. scan_workspace()  — discover all supported source files under a root.
 *   2. analyze_workspace() / analyze_files() — for each file:
 *        a. parse → build_scope_tree → extract_symbols → associate_symbols
 *        b. collect direct include/import references
 *        c. store a TranslationUnit (owns AST, ScopeTree, Symbol list)
 *        d. merge results into the returned Workspace
 *
 * Each source file is parsed exactly once during workspace construction.
 * Semantic services receive the already-built TranslationUnit objects and
 * never re-parse files.
 *
 * ASTs are now loaded lazily per path via get_translation_unit(); use
 * ensure_all_loaded() to force a full workspace load.
 */
#include "ast-extractor.h"
#include "ast-ir.h"
#include "ast-scope.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>
#include <functional>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <thread>

namespace ast
{
// -----------------------------------------------------------------------
// Bounded blocking queue — producer blocks when full, consumer blocks when empty.
// -----------------------------------------------------------------------

template<typename T>
class BlockingQueue
{
    std::deque<T> data_;
    std::mutex mu_;
    std::condition_variable cvNotFull_;
    std::condition_variable cvNotEmpty_;
    size_t capacity_;
    bool done_ = false;

public:
    explicit BlockingQueue(size_t capacity)
        : capacity_(capacity)
    {
    }

    void push(T v)
    {
        std::unique_lock lk(mu_);
        cvNotFull_.wait(lk, [&] {
            return data_.size() < capacity_ || done_;
        });
        if(done_)
            return;
        data_.push_back(std::move(v));
        cvNotEmpty_.notify_one();
    }

    void markDone()
    {
        {
            std::lock_guard lk(mu_);
            done_ = true;
        }
        cvNotEmpty_.notify_all();
        cvNotFull_.notify_all();
    }

    bool pop(T& v)
    {
        std::unique_lock lk(mu_);
        cvNotEmpty_.wait(lk, [&] {
            return !data_.empty() || done_;
        });
        if(data_.empty())
            return false;
        v = std::move(data_.front());
        data_.pop_front();
        cvNotFull_.notify_one();
        return true;
    }
};

/**
 * @brief Encapsulates Git ignore matching for workspace scans.
 *
 * Constructed from a search path; internally opens the nearest Git repository
 * (if one exists) and exposes ignore queries via isIgnored().  libgit2 types
 * are not exposed through this interface.
 *
 * Non-copyable; move-only.
 */
class IgnoreMatcher
{
public:
    /** Opens the repository containing @p searchPath (searches upward). */
    explicit IgnoreMatcher(const std::filesystem::path& searchPath);
    ~IgnoreMatcher();
    IgnoreMatcher(IgnoreMatcher&&) noexcept;
    IgnoreMatcher& operator=(IgnoreMatcher&&) noexcept;
    IgnoreMatcher(const IgnoreMatcher&) = delete;
    IgnoreMatcher& operator=(const IgnoreMatcher&) = delete;

    /** Returns true when a Git repository was found and is usable. */
    bool valid() const noexcept;

    /**
     * @brief Returns the Git working-directory root path.
     *
     * Used by the scanner to compute repository-relative paths before calling
     * isIgnored().  Returns an empty path when valid() is false.
     */
    const std::filesystem::path& workdir() const noexcept;

    /**
     * @brief Returns true if @p relativePath is ignored by Git ignore rules.
     *
     * @p relativePath must be relative to workdir().  Always returns false
     * when valid() is false.
     */
    bool isIgnored(const std::filesystem::path& relativePath) const;

private:
    void* repo_;                   ///< git_repository*, nullptr when not in a repo.
    std::filesystem::path workdir_; ///< Git working-directory root.
};


/**
 * @brief All semantic objects produced from one source file.
 *
 * Owns the AST, ScopeTree, and Symbol list for a single translation unit.
 * Lifetime is managed by Workspace.  TranslationUnit is move-only because
 * AST is move-only.
 */
struct TranslationUnit
{
    AST ast;
    ScopeTree scopeTree;
    std::vector<Symbol> symbols;
    std::filesystem::path path;
};

/**
 * @brief A symbol extracted from a workspace file, augmented with file-level context.
 *
 * Embeds a Symbol (name, fqn, kind, access, modifiers, line, column, nodeIndex)
 * and adds the source file path and the kind of the lexical scope that owns
 * the declaration.
 */
struct WorkspaceSymbol
{
    Symbol symbol;                              ///< Symbol data (name, fqn, kind, etc.).
    std::filesystem::path sourceFile;           ///< Path of the file that declares this symbol.
    ScopeKind owningScope = ScopeKind::Unknown; ///< Kind of the scope that declares this symbol.

    bool operator==(const WorkspaceSymbol& other) const
    {
        return symbol == other.symbol &&
               sourceFile == other.sourceFile &&
               owningScope == other.owningScope;
    }
};

/**
 * @brief Direct include/import relationships for one source file.
 *
 * Include paths are recorded exactly as they appear in the source
 * (e.g., "foo.h", <vector>, "numpy").  No path resolution is performed.
 */
struct FileDependencies
{
    std::filesystem::path file;        ///< Path of the file containing the references.
    std::vector<std::u8string> includes; ///< Include/import paths, deduplicated, as written.
};

struct AnalysisResult
{
    TranslationUnit translationUnit;
    FileDependencies dependencies;
    std::vector<WorkspaceSymbol> symbols;
    bool parsed = false;
};

/**
 * @brief In-memory representation of a parsed workspace.
 *
 * Owns one TranslationUnit per parsed source file (primary storage).
 * Also maintains a flat symbol index (symbols) for O(N) queries that span
 * all files, and a dependency graph (deps) for include/import analysis.
 *
 * This structure is valid only for the duration of the current execution;
 * nothing is persisted.
 */
struct Workspace
{
    std::vector<std::filesystem::path> files;             ///< All discovered source files, sorted.
    mutable std::vector<WorkspaceSymbol> symbols;         ///< Flat symbol index (all files).
    mutable std::vector<FileDependencies> deps;           ///< Per-file direct include/import graph.
    mutable std::vector<TranslationUnit> translationUnits;///< Primary ownership: one TU per parsed file.
    mutable uint32_t parsedCount = 0;                     ///< Number of files successfully parsed.
    mutable uint32_t failedCount = 0;                     ///< Number of files that failed to parse.

    // -----------------------------------------------------------------------
    // Lazy AST cache — keyed by normalised path string.
    // Fields are mutable so that get_translation_unit() and ensure_all_loaded()
    // can be called on a const Workspace& while updating the in-memory cache.
    mutable std::unordered_map<std::u8string, size_t> tuIndex_; ///< path key -> index in translationUnits.
    mutable uint32_t cacheHits_   = 0; ///< AST cache hits (instrumentation).
    mutable uint32_t cacheMisses_ = 0; ///< AST cache misses (instrumentation).

    /**
     * @brief Returns the TranslationUnit for @p path, parsing lazily on first access.
     *
     * On the first call for a given path the file is parsed, the resulting TU is
     * stored in translationUnits, and its symbols/deps are merged into the workspace.
     * Subsequent calls for the same path return the cached TU without re-parsing.
     *
     * Returns nullptr when the path cannot be parsed.
     */
    const TranslationUnit* get_translation_unit(const std::filesystem::path& path) const;

    /**
     * @brief Ensures every file in @p files has been parsed and cached.
     *
     * Calls get_translation_unit() for each path that has not yet been loaded.
     * After this call every entry in translationUnits, symbols, and deps is populated.
     */
    void ensure_all_loaded() const;
};

/**
 * @brief Recursively scans @p root for supported source files.
 *
 * A file is "supported" if get_language_type() returns a value other than
 * ASTLanguage::Unknown for its extension.  The returned list is sorted for
 * deterministic ordering.  Returns an empty list if @p root is null, does
 * not exist, or is not a directory.
 */
std::vector<std::filesystem::path> scan_workspace(const char8_t* root);

/**
 * @brief Parses and analyzes every source file under @p root.
 *
 * Equivalent to:
 * @code
 *   analyze_files(scan_workspace(root))
 * @endcode
 *
 * @return The aggregated Workspace.  Returns an empty workspace if @p root
 *         is null or no supported files are found.
 */
Workspace analyze_workspace(const char8_t* root);

/**
 * @brief Parses and analyzes the given list of source files.
 *
 * For each file the pipeline is:
 *   parse → build_scope_tree → extract_symbols → associate_symbols →
 *   collect_includes → store TranslationUnit → merge into Workspace.
 *
 * Files that fail to parse contribute to failedCount and are otherwise
 * skipped; the remaining files are processed normally.
 *
 * @param files Paths to source files.  Order is preserved in Workspace::files.
 */
Workspace analyze_files(const std::vector<std::filesystem::path>& files);

/**
 * @brief Opens a workspace by scanning @p root for source files without parsing any of them.
 *
 * Returns a Workspace with @p files populated and @p parsedCount == 0.
 * Call Workspace::get_translation_unit() or Workspace::ensure_all_loaded() to parse files
 * on demand.  Returns an empty workspace if @p root is null or is not a directory.
 */
Workspace open_workspace(const char8_t* root);
} // namespace ast
#endif // INC_AST_WORKSPACE_H_
