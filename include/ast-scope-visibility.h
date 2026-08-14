#ifndef INC_AST_SCOPE_VISIBILITY_H_
#define INC_AST_SCOPE_VISIBILITY_H_
/**
 * @file ast-scope-visibility.h
 * @brief Lexical visibility analysis over a ScopeTree.
 *
 * Computes, for every scope, the complete set of symbols that are lexically
 * visible from within that scope.  Visibility follows lexical scoping rules:
 *
 *   - A scope inherits all symbols visible in its parent scope.
 *   - When a scope declares a symbol whose name already appears in an outer
 *     scope, the inner declaration shadows (hides) the outer one.
 *   - Sibling scopes do not share visibility — each inherits only from its
 *     direct ancestor chain.
 *
 * Call ScopeVisibility::compute() once after associate_symbols() has been
 * called on the same ScopeTree.
 */
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace ast
{
class ScopeTree;
struct Symbol;

/**
 * @brief Immutable result of lexical visibility analysis for an entire ScopeTree.
 *
 * Constructed via compute(); all accessors are O(1) after construction.
 */
class ScopeVisibility
{
public:
    /** Sentinel returned by resolve() when the name is not visible. */
    static constexpr size_t InvalidSymbol = size_t(-1);

    /**
     * @brief Computes the visible symbol set for every scope in @p tree.
     *
     * Requires that @p tree was populated by build_scope_tree() and
     * associate_symbols() (so that Scope::symbols_ is populated).
     * Scopes are processed in insertion order; because build_scope_tree()
     * inserts parent scopes before children, each parent's visible set is
     * fully computed before any child's set is derived.
     *
     * @param tree     The scope tree for a single translation unit.
     * @param symbols  The symbol list used when associate_symbols() was called.
     * @return A ScopeVisibility holding the visible set for every scope.
     */
    static ScopeVisibility compute(const ScopeTree& tree, const std::vector<Symbol>& symbols);

    /**
     * @brief Returns the indices of all symbols visible from @p scopeId.
     *
     * The returned vector contains exactly one entry per unique symbol name
     * (the innermost-scope declaration wins when names conflict).
     * Entries are sorted by symbol index for determinism.
     *
     * @return An empty vector if @p scopeId is out of range.
     */
    const std::vector<size_t>& visibleIn(uintptr_t scopeId) const;

    /**
     * @brief Returns the symbol index visible under @p name in @p scopeId.
     *
     * Applies lexical shadowing: if a symbol named @p name exists in multiple
     * ancestor scopes, the one declared in the innermost scope is returned.
     *
     * @return The winning symbol's index, or InvalidSymbol if @p name is not
     *         visible from @p scopeId.
     */
    size_t resolve(uintptr_t scopeId, std::u8string_view name) const;

private:
    struct Entry
    {
        std::vector<size_t>                    visible; ///< Winning symbol indices, sorted.
        std::unordered_map<std::u8string, size_t> byName; ///< name → winning symbol index.
    };

    std::vector<Entry> entries_; ///< One entry per scope, indexed by scope ID.
    static const std::vector<size_t> kEmpty_;
};

} // namespace ast
#endif // INC_AST_SCOPE_VISIBILITY_H_
