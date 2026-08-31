#include "ast-resolver.h"
#include "ast-lookup.h"
#include <algorithm>

namespace ast
{

bool is_callable_symbol(SymbolKind kind)
{
    return kind == SymbolKind::Function || kind == SymbolKind::Method
        || kind == SymbolKind::Constructor || kind == SymbolKind::Destructor;
}

bool same_semantic_symbol(const WorkspaceSymbol& a, const WorkspaceSymbol& b)
{
    if(a.symbol.fqn != b.symbol.fqn) return false;
    const bool aCallable = is_callable_symbol(a.symbol.kind);
    const bool bCallable = is_callable_symbol(b.symbol.kind);
    if(aCallable || bCallable) {
        if(!aCallable || !bCallable) return false;
        if(a.symbol.signature.empty() || b.symbol.signature.empty()) {
            return a.symbol.kind == b.symbol.kind
                && a.sourceFile == b.sourceFile
                && a.symbol.line == b.symbol.line
                && a.symbol.nodeIndex == b.symbol.nodeIndex;
        }
        return a.symbol.signature == b.symbol.signature;
    }
    return a.symbol.kind == b.symbol.kind
        && a.sourceFile == b.sourceFile
        && a.symbol.line == b.symbol.line;
}

const WorkspaceSymbol& canonical_symbol(
    const std::vector<WorkspaceSymbol>& occurrences)
{
    for(const WorkspaceSymbol& occurrence: occurrences) {
        if(occurrence.symbol.isDefinition) return occurrence;
    }
    return occurrences.front();
}

// -----------------------------------------------------------------------
// ResolutionResult factories

ResolutionResult ResolutionResult::make_unresolved()
{
    return {ResolutionStatus::Unresolved, {}};
}

ResolutionResult ResolutionResult::make_resolved(WorkspaceSymbol sym)
{
    return {ResolutionStatus::Resolved, {std::move(sym)}};
}

ResolutionResult ResolutionResult::make_ambiguous(std::vector<WorkspaceSymbol> candidates)
{
    return {ResolutionStatus::Ambiguous, std::move(candidates)};
}

// -----------------------------------------------------------------------
// IdentifierResolver

IdentifierResolver::IdentifierResolver(const ScopeTree& tree,
                                       const std::vector<Symbol>& symbols,
                                       const std::filesystem::path& sourceFile)
    : tree_(tree)
    , symbols_(symbols)
    , sourceFile_(sourceFile)
    , workspace_(nullptr)
{
}

IdentifierResolver::IdentifierResolver(const ScopeTree& tree,
                                       const std::vector<Symbol>& symbols,
                                       const std::filesystem::path& sourceFile,
                                       const Workspace& workspace)
    : tree_(tree)
    , symbols_(symbols)
    , sourceFile_(sourceFile)
    , workspace_(&workspace)
{
}

WorkspaceSymbol IdentifierResolver::to_workspace_symbol(size_t symIdx) const
{
    WorkspaceSymbol ws;
    ws.symbol = symbols_[symIdx];
    ws.sourceFile = sourceFile_;

    // Prefer O(1) reverse map; fall back to O(N) scan when the map was not built.
    uintptr_t scopeId = tree_.getScopeOfSymbol(symIdx);
    if(scopeId == ScopeTree::InvalidId) {
        scopeId = tree_.findBySymbol(symIdx);
    }
    ws.owningScope = (scopeId != ScopeTree::InvalidId)
                         ? tree_[scopeId].kind_
                         : ScopeKind::Unknown;
    return ws;
}

ResolutionResult IdentifierResolver::from_candidates(std::vector<WorkspaceSymbol> c)
{
    if(c.empty())
        return ResolutionResult::make_unresolved();

    std::vector<std::vector<WorkspaceSymbol>> logicalSymbols;
    for(WorkspaceSymbol& candidate: c) {
        auto group = std::find_if(logicalSymbols.begin(), logicalSymbols.end(),
            [&](const std::vector<WorkspaceSymbol>& occurrences) {
                if(!same_semantic_symbol(occurrences.front(), candidate)) return false;
                if(!candidate.symbol.isDefinition) return true;
                return std::none_of(occurrences.begin(), occurrences.end(),
                    [](const WorkspaceSymbol& occurrence) {
                        return occurrence.symbol.isDefinition;
                    });
            });
        if(group == logicalSymbols.end()) {
            logicalSymbols.push_back({std::move(candidate)});
        } else {
            group->push_back(std::move(candidate));
        }
    }
    if(logicalSymbols.size() == 1) {
        return ResolutionResult::make_resolved(
            canonical_symbol(logicalSymbols.front()));
    }
    std::vector<WorkspaceSymbol> canonical;
    canonical.reserve(logicalSymbols.size());
    for(const auto& occurrences: logicalSymbols) {
        canonical.push_back(canonical_symbol(occurrences));
    }
    return ResolutionResult::make_ambiguous(std::move(canonical));
}

ResolutionResult IdentifierResolver::resolve(std::u8string_view name, uintptr_t fromScope) const
{
    std::vector<size_t> indices = lookup(name, fromScope, tree_, symbols_);
    if(!indices.empty()) {
        std::vector<WorkspaceSymbol> candidates;
        candidates.reserve(indices.size());
        for(size_t idx: indices) {
            candidates.push_back(to_workspace_symbol(idx));
        }
        return from_candidates(std::move(candidates));
    }

    // Not found in the local scope tree; try the workspace if available.
    if(workspace_) {
        return resolve_global(name);
    }

    return ResolutionResult::make_unresolved();
}

ResolutionResult IdentifierResolver::resolve_global(std::u8string_view name) const
{
    if(!workspace_) {
        return ResolutionResult::make_unresolved();
    }

    std::vector<WorkspaceSymbol> candidates;
    for(const WorkspaceSymbol& sym: workspace_->symbols) {
        if(sym.symbol.name == name) {
            candidates.push_back(sym);
        }
    }
    return from_candidates(std::move(candidates));
}

} // namespace ast
