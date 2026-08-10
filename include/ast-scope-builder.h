#ifndef INC_AST_SCOPE_BUILDER_H_
#define INC_AST_SCOPE_BUILDER_H_
/**
 * @file ast-scope-builder.h
 * @brief Constructs a ScopeTree from a parsed AST using a depth-first traversal.
 *
 * The builder identifies scope-introducing nodes by their tree-sitter type name
 * and builds the parent/child scope hierarchy without accessing Tree-sitter APIs.
 * Every AST node is associated with its innermost containing scope via
 * ScopeTree::getNodeScope().
 */
#include "ast-scope.h"

namespace ast
{
class AST;

/**
 * @brief Traverses @p ast depth-first and constructs a complete ScopeTree.
 *
 * Scope-introducing node types (e.g. translation_unit, namespace_definition,
 * class_specifier, function_definition, compound_statement) are recognized by
 * their type_ string; the mapping is language-independent in the sense that
 * multiple languages' equivalent constructs are covered.
 *
 * After the call:
 * - tree.size() reflects the total number of scopes found.
 * - tree.getNodeScope(i) returns the innermost scope ID for AST node i (O(1)).
 * - Parent/child relationships are fully wired.
 *
 * @return The constructed ScopeTree. Returns an empty tree if @p ast has no nodes.
 */
ScopeTree build_scope_tree(const AST& ast);

} // namespace ast
#endif // INC_AST_SCOPE_BUILDER_H_
