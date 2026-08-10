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
    // ── Node type string constants (tree-sitter-css grammar) ────────────────
    constexpr const char* k_class_selector       = "class_selector";
    constexpr const char* k_id_selector          = "id_selector";
    constexpr const char* k_class_name           = "class_name";
    constexpr const char* k_id_name              = "id_name";
    constexpr const char* k_keyframes_statement  = "keyframes_statement";
    constexpr const char* k_keyframes_name       = "keyframes_name";
    constexpr const char* k_declaration          = "declaration";
    constexpr const char* k_property_name        = "property_name";
    constexpr const char* k_custom_property_name = "custom_property_name";

    // Return the bare class name from a class_selector node (.foo → "foo").
    // Prefers the dedicated class_name child; falls back to stripping the
    // leading '.' from the node text for grammar variants that inline it.
    std::string getClassName(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* child = findChild(tree, node, k_class_name);
        if(child) return child->getText();
        std::string text = node.getText();
        if(!text.empty() && text[0] == '.') return text.substr(1);
        return {};
    }

    // Return the bare id name from an id_selector node (#foo → "foo").
    std::string getIdName(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* child = findChild(tree, node, k_id_name);
        if(child) return child->getText();
        std::string text = node.getText();
        if(!text.empty() && text[0] == '#') return text.substr(1);
        return {};
    }

    // Return the animation name from a keyframes_statement node.
    std::string getKeyframesName(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* child = findChild(tree, node, k_keyframes_name);
        return child ? child->getText() : std::string();
    }

    // Return the custom property name from a declaration node if its property
    // starts with "--", otherwise return empty. Checks both property_name and
    // custom_property_name child types to cover grammar variants.
    std::string getCustomPropertyName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(!child.typeEquals(k_property_name) && !child.typeEquals(k_custom_property_name))
                continue;
            std::string text = child.getText();
            if(text.size() >= 2 && text[0] == '-' && text[1] == '-') return text;
            return {};   // first property-like child is not a custom property
        }
        return {};
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_css(const ast::AST& tree)
{
    std::vector<Symbol>             result;
    std::unordered_set<std::string> seen;   // key = fqn + ":" + kind ordinal

    auto emit = [&](Symbol sym) {
        std::string key = sym.fqn + ":" + std::to_string(static_cast<int>(sym.kind));
        if(!sym.fqn.empty() && seen.insert(key).second)
            result.push_back(std::move(sym));
    };

    for(uint32_t i = 0; i < tree.size(); ++i) {
        const ast::ASTNode& node = tree[i];

        // ── Class selector (.classname) ──────────────────────────────────────
        // Emitted as SymbolKind::Class; FQN retains the leading '.' so it is
        // unambiguous against identically-named ID selectors and element types.
        if(node.typeEquals(k_class_selector)) {
            std::string name = getClassName(tree, node);
            if(!name.empty()) {
                std::string fqn = "." + name;
                emit(makeSymbol(name, fqn, SymbolKind::Class, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── ID selector (#idname) ────────────────────────────────────────────
        // Closest kind is Variable; FQN retains the leading '#'.
        if(node.typeEquals(k_id_selector)) {
            std::string name = getIdName(tree, node);
            if(!name.empty()) {
                std::string fqn = "#" + name;
                emit(makeSymbol(name, fqn, SymbolKind::Variable, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── @keyframes ───────────────────────────────────────────────────────
        // Named animation sequences are the closest CSS analogue to a reusable
        // callable — Function is the most appropriate SymbolKind.
        if(node.typeEquals(k_keyframes_statement)) {
            std::string name = getKeyframesName(tree, node);
            if(!name.empty())
                emit(makeSymbol(name, name, SymbolKind::Function, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            continue;
        }

        // ── CSS custom properties (--variable: value) ────────────────────────
        // Defined anywhere in the stylesheet; :root-scoped ones are effectively
        // global, but all are meaningful symbols for indexing purposes.
        if(node.typeEquals(k_declaration)) {
            std::string name = getCustomPropertyName(tree, node);
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
