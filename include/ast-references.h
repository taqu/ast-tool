#ifndef INC_AST_REFERENCES_H_
#define INC_AST_REFERENCES_H_
/**
 * @file ast-references.h
 * @brief Workspace-wide reference discovery built on IdentifierResolver.
 *
 * FindReferences locates every use-site of a given semantic symbol across the
 * entire workspace.  It operates entirely on already-built TranslationUnit
 * objects stored in the Workspace; it never re-parses source files.
 *
 *   Workspace::translationUnits → IdentifierResolver → Reference Match → Reference Set
 *
 * For each TranslationUnit the service:
 *   1. Constructs a workspace-aware IdentifierResolver using the TU's
 *      pre-built ScopeTree and Symbol list.
 *   2. Traverses every identifier node in the TU's AST.
 *   3. Resolves each identifier; if it resolves to the target declaration, the
 *      node is recorded as a reference.
 *
 * Identifier matching is semantic (resolved-symbol comparison), never textual.
 * The declaration site is excluded from results by default.
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
 * @brief A single reference to a semantic symbol within a source file.
 *
 * Describes where an identifier appears and which declaration it resolves to.
 * The reference location (sourceFile, line, column) is the USE site, which
 * differs from the target symbol's declaration location stored in referencedSymbol.
 */
struct ReferenceResult
{
    WorkspaceSymbol referencedSymbol;             ///< The declaration this reference resolves to.
    std::string     sourceFile;                   ///< Path of the file containing this reference.
    uint32_t        line        = 0;              ///< 0-based source line of the reference identifier.
    uint32_t        column      = 0;              ///< 0-based source column of the reference identifier.
    ScopeKind       owningScope = ScopeKind::Unknown; ///< Scope enclosing this reference.
    size_t          nodeIndex   = size_t(-1);     ///< Index of the identifier ASTNode, or size_t(-1).
};

/**
 * @brief Discovers all references to a semantic symbol across a workspace.
 *
 * The service coordinates IdentifierResolver and the existing parsing pipeline.
 * It holds a const reference to the Workspace, which must remain valid for the
 * service's lifetime.
 *
 * Reference discovery is always semantic: identifiers are resolved via
 * IdentifierResolver, and only those resolving to the exact target declaration
 * are returned.  Textual name matching is never used.
 *
 * Declaration site exclusion:
 *   By default (includeDeclaration = false), the identifier at the declaration
 *   site is excluded.  A node is considered the declaration site when it lies
 *   in the same source file and at the same 0-based line as the target symbol's
 *   declared location.
 */
class FindReferences
{
public:
    /**
     * @brief Constructs the service bound to @p workspace.
     *
     * @p workspace must have been produced by analyze_workspace() or
     * analyze_files(); it supplies the file list and the cross-file symbol
     * table used during resolution.
     */
    explicit FindReferences(const Workspace& workspace);

    /**
     * @brief Returns all references to @p target across the workspace.
     *
     * For each workspace file the method parses the file, traverses all
     * identifier nodes, and resolves each via IdentifierResolver.  An
     * identifier is a reference if and only if its resolved declaration
     * matches @p target (compared by FQN + kind + source file + declaration line).
     *
     * Files that fail to parse are silently skipped.
     *
     * @param target              The declaration whose references are sought.
     * @param includeDeclaration  When false (the default) the declaration site
     *                            itself is excluded from the result.  Pass true
     *                            to include it.
     * @return All matching references in workspace-file / AST-node order.
     */
    std::vector<ReferenceResult> find(const WorkspaceSymbol& target,
                                      bool includeDeclaration = false) const;

private:
    const Workspace& workspace_;
};

} // namespace ast
#endif // INC_AST_REFERENCES_H_
