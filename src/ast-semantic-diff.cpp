#include "ast-semantic-diff.h"
#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace ast
{

// -----------------------------------------------------------------------
// Matching key: (fqn, kind) is the semantic identity of a declaration.
// Source locations and node indices are deliberately excluded.

namespace
{

struct SymbolKey
{
    std::u8string fqn;
    SymbolKind  kind;

    bool operator==(const SymbolKey& o) const
    {
        return kind == o.kind && fqn == o.fqn;
    }
};

struct SymbolKeyHash
{
    size_t operator()(const SymbolKey& k) const
    {
        size_t h = std::hash<std::u8string>{}(k.fqn);
        // Mix in the kind using Fibonacci hashing to reduce collisions.
        h ^= std::hash<size_t>{}(static_cast<size_t>(k.kind))
             + size_t{0x9e3779b97f4a7c15u} + (h << 6) + (h >> 2);
        return h;
    }
};

using SymbolGroup = std::vector<const WorkspaceSymbol*>;
using SymbolIndex = std::unordered_map<SymbolKey, SymbolGroup, SymbolKeyHash>;

// -----------------------------------------------------------------------
// Symbol indexing

SymbolIndex build_index(const Workspace& ws)
{
    SymbolIndex idx;
    idx.reserve(ws.symbols.size());
    for(const WorkspaceSymbol& sym : ws.symbols)
        idx[{sym.symbol.fqn, sym.symbol.kind}].push_back(&sym);
    return idx;
}

// -----------------------------------------------------------------------
// Property comparison

// Returns true when all currently-comparable semantic properties of @p a and
// @p b are identical.
//
// Excluded intentionally:
//   name      — redundant with fqn when fqn is the match key
//   line      — implementation detail (location, not semantics)
//   column    — implementation detail
//   nodeIndex — implementation detail
//
// Not yet comparable (requires Symbol struct extension):
//   return type    — not stored in Symbol
//   parameter types — not stored in Symbol
bool properties_equal(const Symbol& a, const Symbol& b)
{
    return a.access      == b.access
        && a.isStatic    == b.isStatic
        && a.isConstexpr == b.isConstexpr
        && a.isInline    == b.isInline;
}

// -----------------------------------------------------------------------
// Group-level diff: compare a matched pair of symbol groups.
//
// Both groups share the same (fqn, kind).  Symbols are paired positionally;
// extras in whichever group is longer are treated as pure additions or removals.

void diff_group(const SymbolGroup&     oldGroup,
                const SymbolGroup&     newGroup,
                std::vector<DiffEntry>& out)
{
    const size_t common = std::min(oldGroup.size(), newGroup.size());

    for(size_t i = 0; i < common; ++i) {
        const WorkspaceSymbol* o = oldGroup[i];
        const WorkspaceSymbol* n = newGroup[i];
        if(!properties_equal(o->symbol, n->symbol))
            out.push_back({DiffKind::Modified, o, n});
    }

    // Extra symbols in the old group have been removed.
    for(size_t i = common; i < oldGroup.size(); ++i)
        out.push_back({DiffKind::Removed, oldGroup[i], nullptr});

    // Extra symbols in the new group have been added.
    for(size_t i = common; i < newGroup.size(); ++i)
        out.push_back({DiffKind::Added, nullptr, newGroup[i]});
}

} // namespace

// -----------------------------------------------------------------------
// SemanticDiff

SemanticDiffResult SemanticDiff::compare(const Workspace& oldWorkspace,
                                          const Workspace& newWorkspace) const
{
    SemanticDiffResult result;

    SymbolIndex oldIdx = build_index(oldWorkspace);
    SymbolIndex newIdx = build_index(newWorkspace);

    // Walk every key present in the old index.
    for(const auto& [key, oldGroup] : oldIdx) {
        auto it = newIdx.find(key);
        if(it == newIdx.end()) {
            // All symbols with this identity were removed.
            for(const WorkspaceSymbol* sym : oldGroup)
                result.changes.push_back({DiffKind::Removed, sym, nullptr});
        } else {
            // Identity exists in both workspaces — compare symbol-by-symbol.
            diff_group(oldGroup, it->second, result.changes);
        }
    }

    // Walk every key present only in the new index.
    for(const auto& [key, newGroup] : newIdx) {
        if(oldIdx.find(key) == oldIdx.end()) {
            for(const WorkspaceSymbol* sym : newGroup)
                result.changes.push_back({DiffKind::Added, nullptr, sym});
        }
    }

    return result;
}

} // namespace ast
