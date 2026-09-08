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

    bool isFunctionLike(const ast::ASTNode& node)
    {
        return node.typeEquals(ASTNodeType::ArrowFunction)
            || node.typeEquals(ASTNodeType::FunctionExpression)
            || node.typeEquals(ASTNodeType::GeneratorFunction);
    }

    const ast::ASTNode* getDeclValue(const ast::AST& tree, const ast::ASTNode& decl)
    {
        bool seenEq = false;
        for(uintptr_t id : decl.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(seenEq) return &child;
            if(child.getText() == u8"=") seenEq = true;
        }
        return nullptr;
    }

    const ast::ASTNode* getPairValue(const ast::AST& tree, const ast::ASTNode& pair)
    {
        bool seenColon = false;
        for(uintptr_t id : pair.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(seenColon) return &child;
            if(child.getText() == u8":") seenColon = true;
        }
        return nullptr;
    }

    // TypeScript/TSX class members default to public when no modifier is present.
    // A private_property_identifier (#name) implies Private.
    Access getMemberAccess(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* acc = findChild(tree, node, ASTNodeType::AccessibilityModifier);
        if(!acc) {
            if(findChild(tree, node, ASTNodeType::PrivatePropertyIdentifier))
                return Access::Private;
            return Access::Public;
        }
        std::u8string text = acc->getText();
        if(text == u8"private")   return Access::Private;
        if(text == u8"protected") return Access::Protected;
        return Access::Public;
    }

    // Scan for the first property_identifier or private_property_identifier child.
    std::u8string getMemberName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(ASTNodeType::PropertyIdentifier) ||
               child.typeEquals(ASTNodeType::PrivatePropertyIdentifier))
                return child.getText();
        }
        return {};
    }

    bool insideEnumScope(const std::vector<ScopeFrame>& stack)
    {
        for(int i = static_cast<int>(stack.size()) - 1; i >= 0; --i) {
            SymbolKind k = stack[i].kind;
            if(k == SymbolKind::Function  || k == SymbolKind::Method ||
               k == SymbolKind::Constructor)
                continue;
            return k == SymbolKind::Enum;
        }
        return false;
    }

    bool insideObjectLiteralScope(const std::vector<ScopeFrame>& stack)
    {
        for(int i = static_cast<int>(stack.size()) - 1; i >= 0; --i) {
            SymbolKind k = stack[i].kind;
            if(k == SymbolKind::Variable)  return true;
            if(k == SymbolKind::Class)     return false;
            if(k == SymbolKind::Namespace) return false;
            if(k == SymbolKind::Enum)      return false;
        }
        return false;
    }

    // Return the tag-name identifier from a jsx_opening_element or
    // jsx_self_closing_element node (the first identifier child after '<').
    const ast::ASTNode* jsxTagName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(ASTNodeType::Identifier)) return &child;
        }
        return nullptr;
    }

    // Per the Web Components spec a custom element name must contain a hyphen.
    bool isCustomElement(const std::u8string& name)
    {
        for(char c : name) {
            if(c == '-') return true;
        }
        return false;
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_tsx(const ast::AST& tree)
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

        if(insideFunctionScope(scopeStack)) continue;

        // ── Ambient declaration (declare …) ───────────────────────────────────
        if(node.typeEquals(ASTNodeType::AmbientDeclaration)) {
            scopeStack.push_back({u8"", SymbolKind::Function, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Namespace ─────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::InternalModule)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Namespace, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Namespace, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Enum ──────────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::EnumDeclaration)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Enum, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Enum, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Enum value ────────────────────────────────────────────────────────
        // Both bare (property_identifier) and assigned (enum_assignment child)
        // values appear as property_identifier in DFS order inside an enum scope.
        if(node.typeEquals(ASTNodeType::PropertyIdentifier) && insideEnumScope(scopeStack)) {
            std::u8string name = node.getText();
            std::u8string fqn  = buildFQN(scopeStack, name, u8".");
            emit(makeSymbol(name, fqn, SymbolKind::EnumValue, Access::Public,
                           i, node.start_.row_, node.start_.column_));
            continue;
        }

        // ── Interface ─────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::InterfaceDeclaration)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::TypeIdentifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Class, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Interface method signature (abstract) ─────────────────────────────
        if(node.typeEquals(ASTNodeType::MethodSignature)) {
            std::u8string name = getMemberName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Method, Access::Public,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Interface property signature ──────────────────────────────────────
        if(node.typeEquals(ASTNodeType::PropertySignature)) {
            std::u8string name = getMemberName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Field, Access::Public,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Class (regular and abstract) ──────────────────────────────────────
        if(node.typeEquals(ASTNodeType::ClassDeclaration) ||
           node.typeEquals(ASTNodeType::AbstractClassDeclaration)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::TypeIdentifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Class, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Class field ───────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::PublicFieldDefinition)) {
            std::u8string name = getMemberName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                Access acc = getMemberAccess(tree, node);
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Field, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic    = childHasText(tree, node, u8"static");
                sym.isConstexpr = childHasText(tree, node, u8"readonly");
                emit(std::move(sym));
            }
            continue;
        }

        // ── Method / constructor ──────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::MethodDefinition)) {
            std::u8string name = getMemberName(tree, node);
            if(!name.empty()) {
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                Access      acc  = getMemberAccess(tree, node);
                SymbolKind  kind = (name == u8"constructor")
                                       ? SymbolKind::Constructor
                                       : SymbolKind::Method;
                Symbol sym = makeSymbol(name, fqn, kind, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = childHasText(tree, node, u8"static");
                emit(std::move(sym));
                scopeStack.push_back({name, kind, node.endByte_, acc, false});
            }
            continue;
        }

        // ── Abstract method signature (no body) ───────────────────────────────
        if(node.typeEquals(ASTNodeType::AbstractMethodSignature)) {
            std::u8string name = getMemberName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                Access      acc = getMemberAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Method, acc,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Type alias ────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::TypeAliasDeclaration)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::TypeIdentifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Typedef, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Free function (regular, async, generator) ─────────────────────────
        if(node.typeEquals(ASTNodeType::FunctionDeclaration) ||
           node.typeEquals(ASTNodeType::GeneratorFunctionDeclaration)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Function, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Function, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Variable / lexical declaration ────────────────────────────────────
        if(node.typeEquals(ASTNodeType::VariableDeclaration) ||
           node.typeEquals(ASTNodeType::LexicalDeclaration)) {
            continue;
        }

        // ── Variable declarator ───────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::VariableDeclarator)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(!id) continue;
            std::u8string name = id->getText();
            std::u8string fqn  = buildFQN(scopeStack, name, u8".");

            const ast::ASTNode* val = getDeclValue(tree, node);
            if(val && isFunctionLike(*val)) {
                emit(makeSymbol(name, fqn, SymbolKind::Function, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Function, node.endByte_,
                                      Access::Unknown, false});
            } else if(val && val->typeEquals(ASTNodeType::Object)) {
                emit(makeSymbol(name, fqn, SymbolKind::Variable, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Variable, val->endByte_,
                                      Access::Unknown, false});
            } else {
                emit(makeSymbol(name, fqn, SymbolKind::Variable, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Object literal key-value pair ─────────────────────────────────────
        if(node.typeEquals(ASTNodeType::Pair)) {
            if(!insideObjectLiteralScope(scopeStack)) continue;
            const ast::ASTNode* key = findChild(tree, node, ASTNodeType::PropertyIdentifier);
            if(!key) continue;
            std::u8string name = key->getText();
            std::u8string fqn  = buildFQN(scopeStack, name, u8".");

            const ast::ASTNode* val = getPairValue(tree, node);
            if(val && isFunctionLike(*val)) {
                emit(makeSymbol(name, fqn, SymbolKind::Function, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Function, node.endByte_,
                                      Access::Unknown, false});
            } else {
                emit(makeSymbol(name, fqn, SymbolKind::Variable, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── JSX custom element (web component) ────────────────────────────────
        // Custom element names contain a hyphen (Web Components spec). They are
        // emitted as Class — the closest SymbolKind for a reusable component type.
        // Only processed outside function bodies, where JSX appears at module scope
        // (e.g. assigned to a top-level const). JSX inside component render methods
        // is already suppressed by insideFunctionScope above.
        if(node.typeEquals(ASTNodeType::JsxOpeningElement) ||
           node.typeEquals(ASTNodeType::JsxSelfClosingElement)) {
            const ast::ASTNode* tag = jsxTagName(tree, node);
            if(tag) {
                std::u8string name = tag->getText();
                if(isCustomElement(name))
                    emit(makeSymbol(name, name, SymbolKind::Class, Access::Unknown,
                                   i, node.start_.row_, node.start_.column_));
            }
            continue;
        }
    }

    return result;
}

} // namespace extractor
} // namespace ast
