#ifndef INC_AST_SYMBOL_SCOPE_H_
#define INC_AST_SYMBOL_SCOPE_H_
/**
 * @file ast-symbol-scope.h
 * @brief Associates extracted symbols with their declaring lexical scope.
 *
 * After calling associate_symbols(), each Scope in the ScopeTree holds the
 * indices of the symbols declared directly in it (Scope::symbols_), and each
 * symbol index maps back to its owning scope via ScopeTree::getScopeOfSymbol().
 *
 * The association rule: a symbol belongs to the innermost lexical scope that
 * contains its declaration, with one refinement — when the symbol's AST node
 * is itself the scope-introducing node (e.g., a function_definition or
 * class_specifier), the symbol belongs to the *parent* scope, because the
 * symbol name is declared in the enclosing scope, not inside its own body.
 */
#include <cstddef>
#include <vector>

namespace ast
{
class ScopeTree;
struct Symbol;

/**
 * @brief Associates every symbol in @p symbols with its declaring scope in @p tree.
 *
 * For each symbol:
 * 1. The innermost containing scope is found via ScopeTree::getNodeScope().
 * 2. If that scope's own introducing node equals the symbol's AST node, the
 *    parent scope is used instead (the symbol name lives in the enclosing scope).
 * 3. The symbol index is appended to Scope::symbols_ (scope → symbols direction).
 * 4. The reverse mapping is recorded via ScopeTree::setScopeOfSymbol()
 *    (symbol → scope direction, retrievable as O(1) via getScopeOfSymbol()).
 *
 * Symbols with an invalid node index (ExtractorInvalidId) are silently skipped.
 *
 * @param tree     A ScopeTree built by build_scope_tree() for the same AST.
 * @param symbols  The symbols produced by extract_symbols() for the same AST.
 */
void associate_symbols(ScopeTree& tree, const std::vector<Symbol>& symbols);

} // namespace ast
#endif // INC_AST_SYMBOL_SCOPE_H_
