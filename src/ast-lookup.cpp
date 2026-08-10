#include "ast-lookup.h"
#include "ast-extractor.h"
#include "ast-scope.h"
#include "ast-scope-visibility.h"

namespace ast
{

std::vector<size_t> lookup(std::string_view name,
                           uintptr_t scopeId,
                           const ScopeTree& tree,
                           const std::vector<Symbol>& symbols)
{
    std::vector<size_t> result;
    uintptr_t current = scopeId;

    while(current != ScopeTree::InvalidId) {
        // Collect every symbol in this scope whose unqualified name matches
        for(size_t symIdx : tree[current].symbols_) {
            if(symIdx < symbols.size() && symbols[symIdx].name == name) {
                result.push_back(symIdx);
            }
        }

        if(!result.empty()) {
            // Found at this scope level; stop here (shadowing: don't continue outward)
            return result;
        }

        current = tree[current].parent_;
    }

    return result; // empty — name not found in any ancestor scope
}

bool contains(std::string_view name,
              uintptr_t scopeId,
              const ScopeVisibility& vis)
{
    return vis.resolve(scopeId, name) != ScopeVisibility::InvalidSymbol;
}

bool contains(std::string_view name,
              uintptr_t scopeId,
              const ScopeTree& tree,
              const std::vector<Symbol>& symbols)
{
    return !lookup(name, scopeId, tree, symbols).empty();
}

} // namespace ast
