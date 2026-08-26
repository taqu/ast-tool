#include "ast-tool.h"
#include "ast-extractor-langs.h"
#include "ast-extractor-common.h"
#include "ast-ir.h"
#include <cstdint>
#include <unordered_set>

namespace ast
{
namespace extractor
{
namespace
{
    // ── Low-level helpers ─────────────────────────────────────────────────────

    // Returns Public if the node has a visibility_modifier child, Unknown otherwise.
    Access getAccess(const ast::AST& tree, const ast::ASTNode& node)
    {
        return findChild(tree, node, ASTNodeType::VisibilityModifier)
            ? Access::Public
            : Access::Unknown;
    }

    // True if the function node's parameters list contains a self parameter.
    bool hasSelfParam(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* params = findChild(tree, node, ASTNodeType::Parameters);
        if(!params) return false;
        return findChild(tree, *params, ASTNodeType::SelfParameter) != nullptr;
    }

    // For impl_item: return the implementing type name.
    // "impl Trait for Type" → finds the type_identifier after the "for" keyword.
    // "impl Type"           → finds the first type_identifier.
    std::u8string getImplTypeName(const ast::AST& tree, const ast::ASTNode& node)
    {
        bool seenFor = false;
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.getText() == u8"for") { seenFor = true; continue; }
            if(child.typeEquals(ASTNodeType::TypeIdentifier)) {
                if(seenFor) return child.getText(); // type after "for"
            }
        }
        // No "for" found or no type_identifier after "for": return first type_identifier.
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(ASTNodeType::TypeIdentifier)) return child.getText();
        }
        return {};
    }

    // True if the innermost non-function scope on the stack is a Struct or Enum
    // (used to decide whether a function_item should be emitted as Method).
    bool insideStructOrTraitScope(const std::vector<ScopeFrame>& stack)
    {
        for(int i = static_cast<int>(stack.size()) - 1; i >= 0; --i) {
            SymbolKind k = stack[i].kind;
            if(k == SymbolKind::Function || k == SymbolKind::Method ||
               k == SymbolKind::Constructor || k == SymbolKind::Destructor)
                continue; // keep looking past nested function scopes
            return k == SymbolKind::Struct || k == SymbolKind::Class;
        }
        return false;
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_rust(const ast::AST& tree)
{
    std::vector<Symbol>             result;
    std::unordered_set<std::u8string> seen;   // key = fqn + ":" + kind ordinal
    std::vector<ScopeFrame>         scopeStack;

    auto emit = [&](Symbol sym) {
        char8_t buffer[BUFFER_SIZE];
        std::u8string key = sym.fqn + u8":" + ast::to_string_intermediate(buffer, static_cast<int32_t>(sym.kind));
        if(!sym.fqn.empty() && seen.insert(key).second)
            result.push_back(std::move(sym));
    };

    for(uint32_t i = 0; i < tree.size(); ++i) {
        const ast::ASTNode& node = tree[i];

        while(!scopeStack.empty() && scopeStack.back().endByte <= node.startByte_)
            scopeStack.pop_back();

        // Suppress everything inside function/method/constructor bodies.
        if(insideFunctionScope(scopeStack)) continue;

        // ── Module ────────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::ModItem)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                emit(makeSymbol(name, fqn, SymbolKind::Namespace, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Namespace, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Struct ────────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::StructItem)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::TypeIdentifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Struct, acc,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Struct, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Struct field ──────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::FieldDeclaration)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::FieldIdentifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Field, acc,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Enum ──────────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::EnumItem)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::TypeIdentifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Enum, acc,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Enum, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Enum variant ──────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::EnumVariant)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                emit(makeSymbol(name, fqn, SymbolKind::EnumValue, Access::Public,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Trait ─────────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::TraitItem)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::TypeIdentifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Class, acc,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Impl block ────────────────────────────────────────────────────────
        // No symbol emitted; push a Struct scope so enclosed function_item nodes
        // receive the correct FQN prefix and are classified as methods.
        if(node.typeEquals(ASTNodeType::ImplItem)) {
            std::u8string typeName = getImplTypeName(tree, node);
            if(!typeName.empty()) {
                std::u8string fqn = buildFQN(scopeStack, typeName, u8"::");
                scopeStack.push_back({typeName, SymbolKind::Struct, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Function / method ─────────────────────────────────────────────────
        // Classification:
        //   name=="new" && no self param  → Constructor
        //   inside struct/trait scope     → Method
        //   otherwise                     → Function
        if(node.typeEquals(ASTNodeType::FunctionItem)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                Access      acc  = getAccess(tree, node);
                bool        self = hasSelfParam(tree, node);
                SymbolKind  kind;
                if(name == u8"new" && !self)
                    kind = SymbolKind::Constructor;
                else if(insideStructOrTraitScope(scopeStack))
                    kind = SymbolKind::Method;
                else
                    kind = SymbolKind::Function;
                Symbol sym = makeSymbol(name, fqn, kind, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = !self && insideStructOrTraitScope(scopeStack);
                emit(std::move(sym));
                scopeStack.push_back({name, kind, node.endByte_, acc, false});
            }
            continue;
        }

        // ── Trait abstract method signature (no body) ─────────────────────────
        if(node.typeEquals(ASTNodeType::FunctionSignatureItem)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Method, acc,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Constant ──────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::ConstItem)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                Access      acc  = getAccess(tree, node);
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Variable, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isConstexpr = true;
                emit(std::move(sym));
            }
            continue;
        }

        // ── Static variable ───────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::StaticItem)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                Access      acc  = getAccess(tree, node);
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Variable, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = true;
                emit(std::move(sym));
            }
            continue;
        }

        // ── Type alias ────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::TypeItem)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::TypeIdentifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Typedef, acc,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }
    }

    return result;
}

} // namespace extractor
} // namespace ast
