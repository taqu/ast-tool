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
 */
#include "ast-extractor.h"
#include "ast-ir.h"
#include "ast-scope.h"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include <filesystem>

namespace ast
{

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
    std::vector<std::filesystem::path> files;      ///< All discovered source files, sorted.
    std::vector<WorkspaceSymbol> symbols;          ///< Flat symbol index (all files).
    std::vector<FileDependencies> deps;            ///< Per-file direct include/import graph.
    std::vector<TranslationUnit> translationUnits; ///< Primary ownership: one TU per parsed file.
    uint32_t parsedCount = 0;                      ///< Number of files successfully parsed.
    uint32_t failedCount = 0;                      ///< Number of files that failed to parse.
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

} // namespace ast
#endif // INC_AST_WORKSPACE_H_
