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
 *        a. parse using the existing AST IR
 *        b. extract symbols via the existing semantic layer
 *        c. run scope analysis to determine the owning scope of each symbol
 *        d. collect direct include/import references
 *        e. merge results into the returned Workspace
 *
 * No intermediate results are persisted.  The entire workspace is parsed on
 * every invocation.  Parallel parsing, incremental updates, and caching are
 * non-goals of this phase and are left to future layers.
 */
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "ast-extractor.h"
#include "ast-scope.h"

namespace ast
{

/**
 * @brief A symbol extracted from a workspace file, augmented with file-level context.
 *
 * Mirrors ast::Symbol but adds the source file path and the kind of the
 * lexical scope that owns the declaration.
 */
struct WorkspaceSymbol
{
    std::string name;        ///< Unqualified symbol name.
    std::string fqn;         ///< Fully qualified name (language-specific separator).
    SymbolKind  kind     = SymbolKind::Function; ///< Declaration category.
    Access      access   = Access::Unknown;       ///< Access specifier in effect.

    bool isStatic    = false;
    bool isConstexpr = false;
    bool isInline    = false;

    std::string sourceFile; ///< Path of the file that declares this symbol.
    uint32_t    line   = 0; ///< 0-based source line within sourceFile.
    uint32_t    column = 0; ///< 0-based source column within sourceFile.

    size_t    nodeIndex   = size_t(-1); ///< Index in the per-file AST, or size_t(-1) if unavailable.
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
    std::string              file;     ///< Path of the file containing the references.
    std::vector<std::string> includes; ///< Include/import paths, deduplicated, as written.
};

/**
 * @brief In-memory representation of a parsed workspace.
 *
 * Aggregates symbols and dependency information from every source file found
 * under the scanned directory.  This structure is valid only for the duration
 * of the current execution; nothing is persisted.
 */
struct Workspace
{
    std::vector<std::string>      files;        ///< All discovered source files, sorted.
    std::vector<WorkspaceSymbol>  symbols;      ///< All symbols from all files.
    std::vector<FileDependencies> deps;         ///< Per-file direct include/import graph.
    uint32_t parsedCount = 0; ///< Number of files successfully parsed.
    uint32_t failedCount = 0; ///< Number of files that failed to parse.
};

/**
 * @brief Recursively scans @p root for supported source files.
 *
 * A file is "supported" if get_language_type() returns a value other than
 * ASTLanguage::Unknown for its extension.  The returned list is sorted for
 * deterministic ordering.  Returns an empty list if @p root is null, does
 * not exist, or is not a directory.
 */
std::vector<std::string> scan_workspace(const char* root);

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
Workspace analyze_workspace(const char* root);

/**
 * @brief Parses and analyzes the given list of source files.
 *
 * For each file the pipeline is:
 *   parse → extract_symbols → build_scope_tree → associate_symbols →
 *   collect_includes → merge into Workspace.
 *
 * Files that fail to parse contribute to failedCount and are otherwise
 * skipped; the remaining files are processed normally.
 *
 * @param files Paths to source files.  Order is preserved in Workspace::files.
 */
Workspace analyze_files(const std::vector<std::string>& files);

} // namespace ast
#endif // INC_AST_WORKSPACE_H_
