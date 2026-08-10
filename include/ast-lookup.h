#ifndef INC_AST_LOOKUP_H_
#define INC_AST_LOOKUP_H_
/**
 * @file ast-lookup.h
 * @brief Lexical name lookup over a ScopeTree and Symbol index.
 *
 * Lookup walks the scope ancestor chain from the innermost scope outward.
 * The search stops as soon as at least one symbol with the requested name is
 * found; all matches at that scope level are returned.  This preserves
 * overload sets: if two functions named "foo" are both declared directly in
 * the same scope, both are returned.
 *
 * Lexical shadowing is implicit: a declaration in an inner scope stops the
 * walk before any outer scope with the same name is reached.
 */
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <vector>

namespace ast
{
class ScopeTree;
class ScopeVisibility;
struct Symbol;

/**
 * @brief Resolves @p name by searching the ancestor chain from @p scopeId outward.
 *
 * The walk visits @p scopeId first, then its parent, grandparent, and so on.
 * At each scope, Scope::symbols_ is searched for symbols whose unqualified name
 * equals @p name.  The walk stops at the first scope that yields at least one
 * match; all matches at that scope level are returned (so overload sets are
 * preserved).
 *
 * @param name     The unqualified name to look up.
 * @param scopeId  The scope from which lookup begins.
 * @param tree     The scope tree for the translation unit.
 * @param symbols  The symbol list that was used with associate_symbols().
 * @return         Indices of all matching symbols at the winning scope level,
 *                 or an empty vector if the name is not found in any ancestor.
 */
std::vector<size_t> lookup(std::string_view name,
                           uintptr_t scopeId,
                           const ScopeTree& tree,
                           const std::vector<Symbol>& symbols);

/**
 * @brief Returns true if @p name is visible from @p scopeId.
 *
 * Uses @p vis (precomputed by ScopeVisibility::compute()) for O(1) lookup.
 * Equivalent to `lookup(name, scopeId, ...) not empty` but faster when
 * visibility has already been computed.
 *
 * @param name    The unqualified name to test.
 * @param scopeId The scope from which visibility is queried.
 * @param vis     The visibility analysis result for the same tree.
 * @return        true if at least one symbol with @p name is visible.
 */
bool contains(std::string_view name,
              uintptr_t scopeId,
              const ScopeVisibility& vis);

/**
 * @brief Returns true if @p name is visible from @p scopeId.
 *
 * This overload does not require a precomputed ScopeVisibility; it performs
 * the ancestor walk directly.  Prefer the ScopeVisibility overload when
 * visibility has already been computed.
 */
bool contains(std::string_view name,
              uintptr_t scopeId,
              const ScopeTree& tree,
              const std::vector<Symbol>& symbols);

} // namespace ast
#endif // INC_AST_LOOKUP_H_
