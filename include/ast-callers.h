#ifndef INC_AST_CALLERS_H_
#define INC_AST_CALLERS_H_
/**
 * @file ast-callers.h
 * @brief Workspace-wide caller discovery built on IdentifierResolver.
 *
 * Callers locates every function that directly calls a given target function
 * across the entire workspace.  It operates entirely on already-built
 * TranslationUnit objects stored in the Workspace; it never re-parses source files.
 *
 *   Workspace::translationUnits → IdentifierResolver → Call Match → CallSite Set
 *
 * For each TranslationUnit the service:
 *   1. Constructs a workspace-aware IdentifierResolver using the TU's
 *      pre-built ScopeTree and Symbol list.
 *   2. Traverses every call-expression node in the TU's AST.
 *   3. Extracts the callee identifier from each call expression.
 *   4. Resolves the identifier; if it resolves to the target declaration, the
 *      call is recorded as a CallSite with the enclosing function as the caller.
 *
 * Identifier matching is semantic (resolved-symbol comparison), never textual.
 * Calls that cannot be resolved are silently skipped.
 */
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
#include "ast-extractor.h"
#include "ast-scope.h"
#include "ast-workspace.h"

namespace ast
{

/**
 * @brief A single call site where a given target function is invoked.
 *
 * Records the calling function, the called function, and the source location
 * of the call expression.  Both caller and callee are pointers into the
 * workspace symbol table and remain valid as long as the Workspace is alive.
 */
struct CallSite
{
    const WorkspaceSymbol* caller;     ///< The function containing this call, or nullptr if at file scope.
    const WorkspaceSymbol* callee;     ///< The target function being called.
    std::string            sourceFile; ///< Path of the file containing this call.
    uint32_t               line   = 0; ///< 0-based source line of the callee identifier.
    uint32_t               column = 0; ///< 0-based source column of the callee identifier.
    size_t                 nodeIndex = size_t(-1); ///< Index of the call-expression ASTNode.
};

/**
 * @brief Discovers all functions that directly call a given target function.
 *
 * The service coordinates IdentifierResolver and the existing parsing pipeline.
 * It holds a const reference to the Workspace, which must remain valid for the
 * service's lifetime.
 *
 * Only direct calls resolvable by IdentifierResolver are reported.
 * Indirect calls (function pointers, virtual dispatch) are out of scope.
 * Calls whose identifier is ambiguous or unresolved are silently skipped.
 */
class Callers
{
public:
    /**
     * @brief Constructs the service bound to @p workspace.
     *
     * @p workspace must have been produced by analyze_workspace() or
     * analyze_files(); it supplies the file list and the cross-file symbol
     * table used during resolution.
     */
    explicit Callers(const Workspace& workspace);

    /**
     * @brief Returns all direct call sites for @p target across the workspace.
     *
     * For each workspace file the method traverses all call-expression nodes,
     * resolves the callee via IdentifierResolver, and records the call when the
     * resolved declaration matches @p target (compared by FQN + kind +
     * source file + declaration line).
     *
     * Files that fail to parse are silently skipped.
     * Calls with unresolved or ambiguous callees are silently skipped.
     *
     * @param target  The function whose callers are sought.
     * @return All matching call sites in workspace-file / AST-node order.
     */
    std::vector<CallSite> find(const WorkspaceSymbol& target) const;

private:
    const Workspace& workspace_;
};

} // namespace ast
#endif // INC_AST_CALLERS_H_
