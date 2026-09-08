#ifndef INC_AST_FIND_H_
#define INC_AST_FIND_H_
/**
 * @file ast-find.h
 * @brief Language-agnostic API for searching a parsed AST for nodes matching a set of criteria.
 *        This is the reusable core behind the "find" subcommand; future navigation features
 *        or AI tooling can call find_nodes() directly without going through the CLI.
 */
#include <cstdint>
#include <vector>
#include "ast-node-type.h"

namespace ast
{
class AST;
struct ASTNode;

/**
 * @brief A set of optional search filters, combined with logical AND.
 *
 * An unset filter (ASTNodeType::Unknown, or its associated has*_ flag left false) is not applied.
 * New filters (hash, flags, byte range, parent id, child count, ...) can be added here
 * as additional optional fields without disturbing existing callers.
 */
struct FindCriteria
{
    ASTNodeType type_    = ASTNodeType::Unknown; ///< Match ASTNode::type_ exactly, if set (Unknown = not set).
    ASTNodeType grammar_ = ASTNodeType::Unknown; ///< Match ASTNode::grammar_type_ exactly, if set (Unknown = not set).
    const char8_t* text_ = nullptr; ///< Match if the node's source text contains this substring, if set.

    uint32_t id_ = 0;     ///< Value to match against ASTNode::id_, if #hasId_.
    bool hasId_ = false;

    uint32_t line_   = 0;  ///< Zero-based row a matching node's source range must contain, if #hasPosition_.
    uint32_t column_ = 0;  ///< Zero-based column a matching node's source range must contain, if #hasPosition_.
    bool hasPosition_ = false;
};

/** Returns true if @p node satisfies every filter set in @p criteria. */
bool matches(const ASTNode& node, const FindCriteria& criteria);

/**
 * @brief Performs a single linear O(N) pass over @p ast and returns every node that
 *        satisfies @p criteria, in AST (source) order.
 */
std::vector<const ASTNode*> find_nodes(const AST& ast, const FindCriteria& criteria);
} // namespace ast
#endif // INC_AST_FIND_H_
