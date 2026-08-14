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
    // ── Node type string constants (tree-sitter-html grammar) ────────────────
    constexpr const char* k_start_tag              = "start_tag";
    constexpr const char* k_self_closing_element   = "self_closing_element";
    constexpr const char* k_tag_name               = "tag_name";
    constexpr const char* k_attribute              = "attribute";
    constexpr const char* k_attribute_name         = "attribute_name";
    constexpr const char* k_attribute_value        = "attribute_value";
    constexpr const char* k_quoted_attribute_value = "quoted_attribute_value";

    // Extract the attribute value text from an attribute node.
    // Handles both unquoted (attribute_value) and quoted (quoted_attribute_value
    // wrapping an inner attribute_value alias) forms.
    std::u8string getAttrValue(const ast::AST& tree, const ast::ASTNode& attrNode)
    {
        for(uintptr_t id : attrNode.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(k_attribute_value))
                return child.getText();
            if(child.typeEquals(k_quoted_attribute_value)) {
                const ast::ASTNode* inner = findChild(tree, child, (const char8_t*)k_attribute_value);
                return inner ? inner->getText() : std::u8string();
            }
        }
        return {};
    }

    // Return true if the tag name represents a custom element.
    // Per the Web Components spec, custom element names must contain a hyphen.
    bool isCustomElement(const std::u8string& tagName)
    {
        for(char8_t c : tagName) {
            if(c == u8'-') return true;
        }
        return false;
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_html(const ast::AST& tree)
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

        // ── id attribute → document-level identifier ─────────────────────────
        // id="foo" can be targeted by CSS (#foo), JavaScript (getElementById),
        // and URL fragments (href="#foo"). FQN uses the '#' prefix to match
        // the canonical CSS/fragment selector syntax and to stay distinct from
        // custom-element names that happen to share the same string.
        if(node.typeEquals(k_attribute)) {
            const ast::ASTNode* nameNode = findChild(tree, node, (const char8_t*)k_attribute_name);
            if(nameNode && nameNode->getText() == u8"id") {
                std::u8string value = getAttrValue(tree, node);
                if(!value.empty()) {
                    std::u8string fqn = u8"#" + value;
                    emit(makeSymbol(value, fqn, SymbolKind::Variable, Access::Unknown,
                                   i, node.start_.row_, node.start_.column_));
                }
            }
            continue;
        }

        // ── Custom element tag → reusable component ───────────────────────────
        // Custom elements are identified by a hyphen in the tag name (Web
        // Components spec). They represent reusable component types; Class is
        // the closest SymbolKind. Deduplicated so each component name appears
        // once even when used many times in the document.
        // Matches both <my-elem>...</my-elem> (start_tag) and <my-elem/>.
        if(node.typeEquals(k_start_tag) || node.typeEquals(k_self_closing_element)) {
            const ast::ASTNode* tagNode = findChild(tree, node, (const char8_t*)k_tag_name);
            if(tagNode) {
                std::u8string name = tagNode->getText();
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
