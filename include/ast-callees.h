#ifndef INC_AST_CALLEES_H_
#define INC_AST_CALLEES_H_
/**
 * @file ast-callees.h
 * @brief Function-body callee discovery built on IdentifierResolver.
 *
 * Callees locates every function directly called from within the body of a
 * given caller function.  It operates on a single TranslationUnit whose AST
 * subtree is already available in the Workspace; it never re-parses source files.
 *
 *   Workspace → TranslationUnit → function subtree → IdentifierResolver → CallSite Set
 *
 * For the target TranslationUnit the service:
 *   1. Locates the AST node corresponding to the caller's function definition.
 *   2. Traverses only that subtree (not the entire workspace).
 *   3. Extracts the callee identifier from each call-expression node.
 *   4. Resolves each identifier; if resolution succeeds, the call is recorded
 *      as a CallSite with the given function as the caller.
 *
 * Identifier matching is semantic (resolved-symbol comparison), never textual.
 * Calls that cannot be resolved are silently skipped.
 * Only the target function body is visited, making this O(AST nodes in that body).
 */
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "ast-extractor.h"
#include "ast-scope.h"
#include "ast-workspace.h"
#include "ast-callers.h"

namespace ast
{

/**
 * @brief Discovers all functions directly called from within a given function body.
 *
 * Only direct calls resolvable by IdentifierResolver are reported.
 * Indirect calls (function pointers, virtual dispatch) are out of scope.
 * Calls with unresolved or ambiguous callees are silently skipped.
 * The traversal is restricted to the caller's own function body; calls made
 * by functions it invokes are not reported.
 */
class Callees
{
public:
    /**
     * @brief Returns all direct call sites within @p caller's function body.
     *
     * Locates the TranslationUnit for @p caller, traverses only the AST subtree
     * of @p caller's function definition, and resolves each call-expression
     * via IdentifierResolver.
     *
     * If @p caller's TranslationUnit cannot be found, or if @p caller's AST
     * node is out of range, an empty vector is returned.
     *
     * @param workspace  The workspace supplying TranslationUnits and the
     *                   cross-file symbol table for resolution.
     * @param caller     The function whose direct callees are sought.
     * @return All matching call sites in AST-node order.
     */
    std::vector<CallSite> find(const Workspace&       workspace,
                               const WorkspaceSymbol& caller) const;
};

} // namespace ast
#endif // INC_AST_CALLEES_H_
