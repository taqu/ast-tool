#include "ast-resolver.h"
#include "ast-lookup.h"

namespace ast
{

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

IdentifierResolver::IdentifierResolver(const ScopeTree&           tree,
                                       const std::vector<Symbol>& symbols,
                                       std::string_view            sourceFile)
    : tree_(tree), symbols_(symbols), sourceFile_(sourceFile), workspace_(nullptr)
{
}

IdentifierResolver::IdentifierResolver(const ScopeTree&           tree,
                                       const std::vector<Symbol>& symbols,
                                       std::string_view            sourceFile,
                                       const Workspace&            workspace)
    : tree_(tree), symbols_(symbols), sourceFile_(sourceFile), workspace_(&workspace)
{
}

WorkspaceSymbol IdentifierResolver::to_workspace_symbol(size_t symIdx) const
{
    const Symbol& sym = symbols_[symIdx];
    WorkspaceSymbol ws;
    ws.name        = sym.name;
    ws.fqn         = sym.fqn;
    ws.kind        = sym.kind;
    ws.access      = sym.access;
    ws.isStatic    = sym.isStatic;
    ws.isConstexpr = sym.isConstexpr;
    ws.isInline    = sym.isInline;
    ws.sourceFile  = sourceFile_;
    ws.line        = sym.line;
    ws.column      = sym.column;
    ws.nodeIndex   = sym.nodeIndex;

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
    if(c.empty())     return ResolutionResult::make_unresolved();
    if(c.size() == 1) return ResolutionResult::make_resolved(std::move(c[0]));
    return ResolutionResult::make_ambiguous(std::move(c));
}

ResolutionResult IdentifierResolver::resolve(std::string_view name, uintptr_t fromScope) const
{
    std::vector<size_t> indices = lookup(name, fromScope, tree_, symbols_);
    if(!indices.empty()) {
        std::vector<WorkspaceSymbol> candidates;
        candidates.reserve(indices.size());
        for(size_t idx : indices) {
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

ResolutionResult IdentifierResolver::resolve_global(std::string_view name) const
{
    if(!workspace_) {
        return ResolutionResult::make_unresolved();
    }

    std::vector<WorkspaceSymbol> candidates;
    for(const WorkspaceSymbol& sym : workspace_->symbols) {
        if(sym.name == name) {
            candidates.push_back(sym);
        }
    }
    return from_candidates(std::move(candidates));
}

} // namespace ast
