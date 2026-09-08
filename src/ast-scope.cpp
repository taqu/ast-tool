#include "ast-scope.h"
#include <cassert>

namespace ast
{

const char* getScopeKindName(ScopeKind kind)
{
    switch(kind) {
    case ScopeKind::Global:      return "Global";
    case ScopeKind::Namespace:   return "Namespace";
    case ScopeKind::Module:      return "Module";
    case ScopeKind::Class:       return "Class";
    case ScopeKind::Struct:      return "Struct";
    case ScopeKind::Enum:        return "Enum";
    case ScopeKind::Function:    return "Function";
    case ScopeKind::Method:      return "Method";
    case ScopeKind::Lambda:      return "Lambda";
    case ScopeKind::Block:       return "Block";
    case ScopeKind::Conditional: return "Conditional";
    case ScopeKind::Loop:        return "Loop";
    case ScopeKind::Switch:      return "Switch";
    default:                     return "Unknown";
    }
}

uintptr_t ScopeTree::add(ScopeKind kind, size_t nodeIndex, uint32_t startByte, uint32_t endByte, uintptr_t parentId)
{
    uintptr_t id = static_cast<uintptr_t>(scopes_.size());
    Scope& scope = scopes_.emplace_back();
    scope.id_        = id;
    scope.kind_      = kind;
    scope.nodeIndex_ = nodeIndex;
    scope.startByte_ = startByte;
    scope.endByte_   = endByte;
    scope.parent_    = parentId;

    if(parentId != InvalidId) {
        assert(parentId < scopes_.size() - 1);
        scopes_[parentId].children_.push_back(id);
    }
    return id;
}

void ScopeTree::addSymbol(uintptr_t scopeId, size_t symbolIndex)
{
    assert(scopeId < scopes_.size());
    scopes_[scopeId].symbols_.push_back(symbolIndex);
}

uint32_t ScopeTree::size() const
{
    return static_cast<uint32_t>(scopes_.size());
}

Scope& ScopeTree::operator[](uintptr_t id)
{
    assert(id < scopes_.size());
    return scopes_[id];
}

const Scope& ScopeTree::operator[](uintptr_t id) const
{
    assert(id < scopes_.size());
    return scopes_[id];
}

uintptr_t ScopeTree::findByNodeIndex(size_t nodeIndex) const
{
    for(const Scope& s : scopes_) {
        if(s.nodeIndex_ == nodeIndex) {
            return s.id_;
        }
    }
    return InvalidId;
}

uintptr_t ScopeTree::findBySymbol(size_t symbolIndex) const
{
    for(const Scope& s : scopes_) {
        for(size_t sym : s.symbols_) {
            if(sym == symbolIndex) {
                return s.id_;
            }
        }
    }
    return InvalidId;
}

void ScopeTree::reserveSymbolMap(size_t symbolCount)
{
    symbolScope_.assign(symbolCount, InvalidId);
}

void ScopeTree::setScopeOfSymbol(size_t symbolIndex, uintptr_t scopeId)
{
    if(symbolIndex < symbolScope_.size()) {
        symbolScope_[symbolIndex] = scopeId;
    }
}

uintptr_t ScopeTree::getScopeOfSymbol(size_t symbolIndex) const
{
    if(symbolIndex < symbolScope_.size()) {
        return symbolScope_[symbolIndex];
    }
    return InvalidId;
}

void ScopeTree::reserveNodeMap(size_t nodeCount)
{
    nodeScope_.assign(nodeCount, InvalidId);
}

void ScopeTree::setNodeScope(size_t nodeIndex, uintptr_t scopeId)
{
    if(nodeIndex < nodeScope_.size()) {
        nodeScope_[nodeIndex] = scopeId;
    }
}

uintptr_t ScopeTree::getNodeScope(size_t nodeIndex) const
{
    if(nodeIndex < nodeScope_.size()) {
        return nodeScope_[nodeIndex];
    }
    return InvalidId;
}

} // namespace ast
