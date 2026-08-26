#include "ast-extractor-common.h"
#include "ast-ir.h"

namespace ast
{
namespace extractor
{
    const ast::ASTNode* findChild(const ast::AST& tree, const ast::ASTNode& node, ASTNodeType type)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(type)) return &child;
        }
        return nullptr;
    }

    bool childHasText(const ast::AST& tree, const ast::ASTNode& node, const char8_t* text)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            if(tree[static_cast<uint32_t>(id)].getText() == text) return true;
        }
        return false;
    }

    std::u8string buildFQN(const std::vector<ScopeFrame>& stack, const std::u8string& symName, const char8_t* sep)
    {
        std::u8string fqn;
        for(const auto& f : stack) {
            if(f.name.empty()) continue;
            if(!fqn.empty()) fqn += sep;
            fqn += f.name;
        }
        if(!symName.empty()) {
            if(!fqn.empty()) fqn += sep;
            fqn += symName;
        }
        return fqn;
    }

    Access topAccess(const std::vector<ScopeFrame>& stack)
    {
        for(int i = (int)stack.size() - 1; i >= 0; --i) {
            if(stack[i].isAccessAware) return stack[i].currentAccess;
        }
        return Access::Unknown;
    }

    bool inNamedClassScope(const std::vector<ScopeFrame>& stack)
    {
        for(const auto& f : stack) {
            if((f.kind == SymbolKind::Class || f.kind == SymbolKind::Struct
                || f.kind == SymbolKind::Union) && !f.name.empty()) {
                return true;
            }
        }
        return false;
    }

    bool insideFunctionScope(const std::vector<ScopeFrame>& stack)
    {
        for(const auto& f : stack) {
            auto k = f.kind;
            if(k == SymbolKind::Function || k == SymbolKind::Method
               || k == SymbolKind::Constructor || k == SymbolKind::Destructor) {
                return true;
            }
        }
        return false;
    }

    Symbol makeSymbol(std::u8string name, std::u8string fqn, SymbolKind kind,
                       Access access, uint32_t idx, uint32_t row, uint32_t col)
    {
        Symbol s;
        s.name      = std::move(name);
        s.fqn       = std::move(fqn);
        s.kind      = kind;
        s.access    = access;
        s.nodeIndex = idx;
        s.line      = row + 1;  // 0-based row → 1-based line
        s.column    = col;
        return s;
    }

} // namespace extractor
} // namespace ast
