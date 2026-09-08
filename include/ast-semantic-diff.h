#ifndef INC_AST_SEMANTIC_DIFF_H_
#define INC_AST_SEMANTIC_DIFF_H_
/**
 * @file ast-semantic-diff.h
 * @brief Semantic comparison of two workspace snapshots.
 *
 * SemanticDiff operates exclusively on WorkspaceSymbol objects produced by
 * the existing analysis pipeline.  It never parses source files, never accesses
 * tree-sitter, and never compares source text.
 *
 * Symbols are matched by semantic identity (FQN + kind).  Source locations,
 * AST node indices, comments, whitespace, and formatting are completely ignored.
 * Only semantic properties (access, storage modifiers, linkage modifiers)
 * influence the result.
 *
 * Detected changes:
 *   - Added   : symbol present in new workspace but not in old
 *   - Removed : symbol present in old workspace but not in new
 *   - Modified: symbol present in both but with different semantic properties
 *               (access, isStatic, isConstexpr, isInline)
 *
 * Architectural note: return-type and parameter-type changes are not detected
 * by the current implementation because the Symbol struct does not carry that
 * information.  When the Symbol model is extended to include signatures, the
 * properties_equal() comparison in the implementation should be updated.
 */
#include <cstddef>
#include <vector>
#include "ast-workspace.h"

namespace ast
{

/** The category of semantic change between an old and a new workspace. */
enum class DiffKind
{
    Added,    ///< Symbol is new in the new workspace.
    Removed,  ///< Symbol existed in the old workspace and is no longer present.
    Modified, ///< Symbol exists in both workspaces but with different semantic properties.
};

/**
 * @brief A single semantic change between two workspace snapshots.
 *
 * For Added entries   : before == nullptr, after  != nullptr.
 * For Removed entries : before != nullptr, after  == nullptr.
 * For Modified entries: before != nullptr, after  != nullptr.
 *
 * Pointers remain valid as long as the corresponding Workspace objects are alive.
 */
struct DiffEntry
{
    DiffKind               kind;
    const WorkspaceSymbol* before = nullptr; ///< Symbol in the old workspace.
    const WorkspaceSymbol* after  = nullptr; ///< Symbol in the new workspace.
};

/**
 * @brief The complete set of semantic changes between two workspace snapshots.
 *
 * Entries appear in no guaranteed order.  Callers that need a stable order
 * should sort by (kind, fqn, sourceFile) after receiving the result.
 */
struct SemanticDiffResult
{
    std::vector<DiffEntry> changes; ///< All detected semantic changes.

    /** Returns true when no semantic changes were detected. */
    bool empty() const { return changes.empty(); }
};

/**
 * @brief Compares two semantic workspace snapshots and reports the differences.
 *
 * The service is stateless; each call to compare() is independent.
 *
 * Matching algorithm:
 *   1. Build a hash index keyed by (fqn, kind) for each workspace.
 *   2. For each key that exists in the old index: if absent from new → Removed;
 *      if present in new → pairwise compare each symbol group.
 *   3. For each key present only in the new index → Added.
 *
 * When multiple symbols share the same (fqn, kind) (e.g. overloaded functions
 * whose FQN does not encode parameter types), they are matched positionally
 * within the group; extras become Added or Removed.
 */
class SemanticDiff
{
public:
    /**
     * @brief Compares @p oldWorkspace against @p newWorkspace and returns all
     *        detected semantic changes.
     *
     * Both workspaces must have been constructed by analyze_workspace() or
     * analyze_files() prior to this call.  Neither workspace is modified.
     */
    SemanticDiffResult compare(const Workspace& oldWorkspace,
                                const Workspace& newWorkspace) const;
};

} // namespace ast
#endif // INC_AST_SEMANTIC_DIFF_H_
