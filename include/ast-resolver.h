#ifndef INC_AST_RESOLVER_H_
#define INC_AST_RESOLVER_H_
/**
 * @file ast-resolver.h
 * @brief Semantic identifier resolution over a ScopeTree and Workspace symbol table.
 *
 * Resolution algorithm:
 *   1. lexical lookup (lookup() from ast-lookup.h) walks the scope ancestor chain
 *      from the reference site outward.  The first scope containing a matching
 *      symbol produces the initial candidate set.
 *   2. If no candidates are found locally and a Workspace was provided, the
 *      resolver falls back to the Workspace symbol table for cross-file matches.
 *   3. Exactly one candidate → Resolved.  Zero → Unresolved.  Two or more → Ambiguous.
 *
 * Interaction with SemanticSearchEngine:
 *   SemanticSearchEngine answers broad queries (kind, file, regex) against the
 *   entire Workspace.  IdentifierResolver answers contextual reference queries:
 *   "what does this identifier refer to at this scope?"  The two are complementary;
 *   neither is a subset of the other.
 *
 * Suitable for direct reuse by:
 *   - Find References   (resolve each use site; compare to a known declaration)
 *   - Rename            (collect all sites that resolve to the same symbol)
 *   - Call Graph        (resolve each call expression to a function symbol)
 *   - Semantic Diff     (compare resolution results across two revisions)
 *   - AI navigation     (answer "what does X refer to?" for any identifier)
 */
#include "ast-extractor.h"
#include "ast-scope.h"
#include "ast-workspace.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace ast
{

// -----------------------------------------------------------------------
// ResolutionStatus

/**
 * @brief Describes how an identifier lookup concluded.
 *
 * Resolved:   Exactly one symbol matched.  ResolutionResult::symbol() is valid.
 * Unresolved: No symbol matched.  The identifier likely refers to an external,
 *             incomplete, unsupported, or generated declaration.
 * Ambiguous:  Multiple candidates survived scope filtering.  This occurs for
 *             overload sets, or when the same unqualified name appears in
 *             multiple workspace files without a more-local declaration to
 *             disambiguate.
 */
enum class ResolutionStatus : uint8_t
{
    Resolved,
    Unresolved,
    Ambiguous,
};

// -----------------------------------------------------------------------
// ResolutionResult

/**
 * @brief The output of IdentifierResolver::resolve().
 *
 * status is always set.  candidates is populated for Resolved (one entry) and
 * Ambiguous (two or more entries); it is empty for Unresolved.
 *
 * Resolution metadata preserved in each candidate:
 *   - name, fqn, kind, access  — symbol identity and declaration category
 *   - sourceFile, line, column — declaration site (for navigation and diagnostics)
 *   - owningScope              — lexical scope kind that declares the symbol
 *   - nodeIndex                — AST index for future cross-referencing services
 *
 * Use the make_* factory functions to construct results; direct construction
 * is reserved for internal use.
 */
struct ResolutionResult
{
    ResolutionStatus status = ResolutionStatus::Unresolved;
    std::vector<WorkspaceSymbol> candidates; ///< 1 entry (Resolved), 2+ (Ambiguous), 0 (Unresolved).

    bool resolved() const
    {
        return status == ResolutionStatus::Resolved;
    }
    bool unresolved() const
    {
        return status == ResolutionStatus::Unresolved;
    }
    bool ambiguous() const
    {
        return status == ResolutionStatus::Ambiguous;
    }

    /**
     * @brief Returns the single resolved symbol.
     * @pre resolved() == true
     */
    const WorkspaceSymbol& symbol() const
    {
        return candidates[0];
    }

    /** @name Factory functions */
    ///@{
    static ResolutionResult make_unresolved();
    static ResolutionResult make_resolved(WorkspaceSymbol sym);
    static ResolutionResult make_ambiguous(std::vector<WorkspaceSymbol> candidates);
    ///@}
};

// -----------------------------------------------------------------------
// IdentifierResolver

/**
 * @brief Resolves identifier references to symbols using lexical scope rules.
 *
 * Per-file resolution (resolve):
 *   Walks the scope ancestor chain from a reference site using the existing
 *   lexical lookup algorithm.  This respects shadowing: an inner declaration
 *   hides outer ones with the same name.
 *
 * Cross-file fallback:
 *   When a Workspace is provided, resolve() falls back to the workspace symbol
 *   table when the identifier is not found in the local scope tree.
 *   resolve_global() always searches the entire workspace (no scope context).
 *
 * Filter precedence:
 *   1. Lexical scope (ancestor walk) — applied first, most precise
 *   2. Workspace symbol table        — fallback, cross-file
 *
 * The resolver holds references to the ScopeTree, symbol list, and optional
 * Workspace.  All referenced objects must remain valid for the resolver's lifetime.
 * The resolver never re-parses source files, builds ASTs, or accesses tree-sitter.
 */
class IdentifierResolver
{
public:
    /**
     * @brief Constructs a per-file resolver without workspace fallback.
     * @param tree       Scope tree for the translation unit.
     * @param symbols    Symbol list associated with @p tree via associate_symbols().
     * @param sourceFile Path of the source file (recorded in all results).
     */
    IdentifierResolver(const ScopeTree& tree,
                       const std::vector<Symbol>& symbols,
                       const std::filesystem::path& sourceFile);

    /**
     * @brief Constructs a workspace-aware resolver.
     *
     * In addition to per-file lexical lookup, resolve() falls back to
     * @p workspace when the identifier is not found in the local scope tree.
     */
    IdentifierResolver(const ScopeTree& tree,
                       const std::vector<Symbol>& symbols,
                       const std::filesystem::path& sourceFile,
                       const Workspace& workspace);

    /**
     * @brief Resolves @p name from the given scope reference site.
     *
     * Resolution steps:
     *   1. Walk the ancestor chain from @p fromScope via lexical lookup.
     *   2. If not found locally and a Workspace was provided, fall back to
     *      resolve_global(name).
     *   3. 0 candidates → Unresolved; 1 → Resolved; 2+ → Ambiguous.
     *
     * Passing ScopeTree::InvalidId as @p fromScope skips lexical lookup and
     * returns the workspace result (or Unresolved if no workspace is set).
     *
     * @param name      The unqualified identifier to resolve.
     * @param fromScope The scope ID of the reference site in the local tree.
     */
    ResolutionResult resolve(std::u8string_view name, uintptr_t fromScope) const;

    /**
     * @brief Resolves @p name across the entire workspace symbol table.
     *
     * Ignores lexical scope; matches any workspace symbol whose unqualified
     * name equals @p name.  Returns Unresolved if no Workspace was provided.
     *
     * Prefer resolve() when a scope context is available — it applies lexical
     * shadowing and produces more precise results.
     *
     * @param name The unqualified name to search for.
     */
    ResolutionResult resolve_global(std::u8string_view name) const;

private:
    WorkspaceSymbol to_workspace_symbol(size_t symIdx) const;
    static ResolutionResult from_candidates(std::vector<WorkspaceSymbol> c);

    const ScopeTree& tree_;
    const std::vector<Symbol>& symbols_;
    std::filesystem::path sourceFile_;
    const Workspace* workspace_; ///< nullptr when no workspace was provided.
};

} // namespace ast
#endif // INC_AST_RESOLVER_H_
