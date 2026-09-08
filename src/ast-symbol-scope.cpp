#include "ast-symbol-scope.h"
#include "ast-extractor.h"
#include "ast-scope.h"

namespace ast
{

void associate_symbols(ScopeTree& tree, const std::vector<Symbol>& symbols)
{
    tree.reserveSymbolMap(symbols.size());

    for(size_t i = 0; i < symbols.size(); ++i) {
        const Symbol& sym = symbols[i];

        if(sym.nodeIndex == ExtractorInvalidId) {
            continue;
        }

        uintptr_t scopeId = tree.getNodeScope(sym.nodeIndex);
        if(scopeId == ScopeTree::InvalidId) {
            continue;
        }

        // When the symbol's AST node is the scope-introducing node itself
        // (e.g., function_definition, class_specifier), the symbol name is
        // declared in the *enclosing* scope, not inside the scope it introduces.
        if(tree[scopeId].nodeIndex_ == sym.nodeIndex) {
            uintptr_t parent = tree[scopeId].parent_;
            if(parent != ScopeTree::InvalidId) {
                scopeId = parent;
            }
        }

        tree.addSymbol(scopeId, i);
        tree.setScopeOfSymbol(i, scopeId);
    }
}

} // namespace ast
