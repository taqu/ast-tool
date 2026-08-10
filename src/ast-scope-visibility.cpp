#include "ast-scope-visibility.h"
#include <algorithm>
#include "ast-extractor.h"
#include "ast-scope.h"

namespace ast
{

const std::vector<size_t> ScopeVisibility::kEmpty_;

ScopeVisibility ScopeVisibility::compute(const ScopeTree& tree, const std::vector<Symbol>& symbols)
{
    ScopeVisibility result;
    result.entries_.resize(tree.size());

    // Scopes were inserted in DFS pre-order by build_scope_tree(), so for every
    // scope id, tree[id].parent_ < id.  Processing 0..N-1 guarantees each
    // parent's entry is complete before any child entry is started.
    for(uint32_t id = 0; id < tree.size(); ++id) {
        Entry& entry = result.entries_[id];

        // Inherit parent's visible set (copy-on-write semantics via copy)
        uintptr_t parentId = tree[id].parent_;
        if(parentId != ScopeTree::InvalidId && parentId < static_cast<uintptr_t>(result.entries_.size())) {
            entry.byName = result.entries_[parentId].byName;
        }

        // Layer this scope's own symbols on top; same-name entries shadow the inherited one
        for(size_t symIdx : tree[id].symbols_) {
            if(symIdx < symbols.size()) {
                entry.byName[symbols[symIdx].name] = symIdx;
            }
        }

        // Build the visible indices vector from the name map; sort for determinism
        entry.visible.clear();
        entry.visible.reserve(entry.byName.size());
        for(const auto& [name, idx] : entry.byName) {
            entry.visible.push_back(idx);
        }
        std::sort(entry.visible.begin(), entry.visible.end());
    }

    return result;
}

const std::vector<size_t>& ScopeVisibility::visibleIn(uintptr_t scopeId) const
{
    if(scopeId >= entries_.size()) {
        return kEmpty_;
    }
    return entries_[scopeId].visible;
}

size_t ScopeVisibility::resolve(uintptr_t scopeId, std::string_view name) const
{
    if(scopeId >= entries_.size()) {
        return InvalidSymbol;
    }
    auto it = entries_[scopeId].byName.find(std::string(name));
    if(it == entries_[scopeId].byName.end()) {
        return InvalidSymbol;
    }
    return it->second;
}

} // namespace ast
