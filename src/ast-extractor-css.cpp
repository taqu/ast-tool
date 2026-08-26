#include "ast-tool.h"
#include "ast-extractor-langs.h"
#include "ast-extractor-common.h"
#include "ast-ir.h"
#include <unordered_set>

namespace ast
{
namespace extractor
{
namespace
{
    // Return the bare class name from a class_selector node (.foo → "foo").
    // Prefers the dedicated class_name child; falls back to stripping the
    // leading '.' from the node text for grammar variants that inline it.
    std::u8string getClassName(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* child = findChild(tree, node, ASTNodeType::ClassName);
        if(child) return child->getText();
        std::u8string text = node.getText();
        if(!text.empty() && text[0] == u8'.') return text.substr(1);
        return {};
    }

    // Return the bare id name from an id_selector node (#foo → "foo").
    std::u8string getIdName(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* child = findChild(tree, node, ASTNodeType::IdName);
        if(child) return child->getText();
        std::u8string text = node.getText();
        if(!text.empty() && text[0] == u8'#') return text.substr(1);
        return {};
    }

    // Return the animation name from a keyframes_statement node.
    std::u8string getKeyframesName(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* child = findChild(tree, node, ASTNodeType::KeyframesName);
        return child ? child->getText() : std::u8string();
    }

    // Return the custom property name from a declaration node if its property
    // starts with "--", otherwise return empty. Checks both property_name and
    // custom_property_name child types to cover grammar variants.
    std::u8string getCustomPropertyName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(!child.typeEquals(ASTNodeType::PropertyName) && !child.typeEquals(ASTNodeType::CustomPropertyName))
                continue;
            std::u8string text = child.getText();
            if(text.size() >= 2 && text[0] == u8'-' && text[1] == u8'-') return text;
            return {};   // first property-like child is not a custom property
        }
        return {};
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_css(const ast::AST& tree)
{
    std::vector<Symbol>             result;
    std::unordered_set<std::u8string> seen;   // key = fqn + ":" + kind ordinal

    auto emit = [&](Symbol sym) {
        char8_t buffer[BUFFER_SIZE];
        std::u8string key = sym.fqn + u8":" + ast::to_string_intermediate(buffer, static_cast<int32_t>(sym.kind));
        if(!sym.fqn.empty() && seen.insert(key).second)
            result.push_back(std::move(sym));
    };

    for(uint32_t i = 0; i < tree.size(); ++i) {
        const ast::ASTNode& node = tree[i];

        // ── Class selector (.classname) ──────────────────────────────────────
        // Emitted as SymbolKind::Class; FQN retains the leading '.' so it is
        // unambiguous against identically-named ID selectors and element types.
        if(node.typeEquals(ASTNodeType::ClassSelector)) {
            std::u8string name = getClassName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = u8"." + name;
                emit(makeSymbol(name, fqn, SymbolKind::Class, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── ID selector (#idname) ────────────────────────────────────────────
        // Closest kind is Variable; FQN retains the leading '#'.
        if(node.typeEquals(ASTNodeType::IdSelector)) {
            std::u8string name = getIdName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = u8"#" + name;
                emit(makeSymbol(name, fqn, SymbolKind::Variable, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── @keyframes ───────────────────────────────────────────────────────
        // Named animation sequences are the closest CSS analogue to a reusable
        // callable — Function is the most appropriate SymbolKind.
        if(node.typeEquals(ASTNodeType::KeyframesStatement)) {
            std::u8string name = getKeyframesName(tree, node);
            if(!name.empty())
                emit(makeSymbol(name, name, SymbolKind::Function, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            continue;
        }

        // ── CSS custom properties (--variable: value) ────────────────────────
        // Defined anywhere in the stylesheet; :root-scoped ones are effectively
        // global, but all are meaningful symbols for indexing purposes.
        if(node.typeEquals(ASTNodeType::Declaration)) {
            std::u8string name = getCustomPropertyName(tree, node);
            if(!name.empty())
                emit(makeSymbol(name, name, SymbolKind::Variable, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            continue;
        }
    }

    return result;
}

} // namespace extractor
} // namespace ast
