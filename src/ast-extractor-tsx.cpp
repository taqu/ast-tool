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
    // ── Node type string constants (tree-sitter-tsx grammar) ──────────────────
    // TSX is a superset of TypeScript; these constants are identical to the TS
    // extractor, with JSX-specific additions at the bottom.
    constexpr const char* k_internal_module               = "internal_module";
    constexpr const char* k_ambient_declaration           = "ambient_declaration";
    constexpr const char* k_enum_declaration              = "enum_declaration";
    constexpr const char* k_interface_declaration         = "interface_declaration";
    constexpr const char* k_class_declaration             = "class_declaration";
    constexpr const char* k_abstract_class_declaration    = "abstract_class_declaration";
    constexpr const char* k_public_field_definition       = "public_field_definition";
    constexpr const char* k_method_definition             = "method_definition";
    constexpr const char* k_abstract_method_signature     = "abstract_method_signature";
    constexpr const char* k_method_signature              = "method_signature";
    constexpr const char* k_property_signature            = "property_signature";
    constexpr const char* k_type_alias_declaration        = "type_alias_declaration";
    constexpr const char* k_function_declaration          = "function_declaration";
    constexpr const char* k_generator_function_declaration= "generator_function_declaration";
    constexpr const char* k_variable_declaration          = "variable_declaration";
    constexpr const char* k_lexical_declaration           = "lexical_declaration";
    constexpr const char* k_variable_declarator           = "variable_declarator";
    constexpr const char* k_pair                          = "pair";
    constexpr const char* k_arrow_function                = "arrow_function";
    constexpr const char* k_function_expression           = "function_expression";
    constexpr const char* k_generator_function            = "generator_function";
    constexpr const char* k_object                        = "object";
    constexpr const char* k_identifier                    = "identifier";
    constexpr const char* k_type_identifier               = "type_identifier";
    constexpr const char* k_property_identifier           = "property_identifier";
    constexpr const char* k_private_property_identifier   = "private_property_identifier";
    constexpr const char* k_accessibility_modifier        = "accessibility_modifier";
    // ── JSX additions ─────────────────────────────────────────────────────────
    constexpr const char* k_jsx_opening_element           = "jsx_opening_element";
    constexpr const char* k_jsx_self_closing_element      = "jsx_self_closing_element";

    // ── Low-level helpers ─────────────────────────────────────────────────────

    bool isFunctionLike(const ast::ASTNode& node)
    {
        return node.typeEquals(k_arrow_function)
            || node.typeEquals(k_function_expression)
            || node.typeEquals(k_generator_function);
    }

    const ast::ASTNode* getDeclValue(const ast::AST& tree, const ast::ASTNode& decl)
    {
        bool seenEq = false;
        for(uintptr_t id : decl.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(seenEq) return &child;
            if(child.typeEquals("=")) seenEq = true;
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
            if(child.typeEquals(":")) seenColon = true;
        }
        return nullptr;
    }

    // TypeScript/TSX class members default to public when no modifier is present.
    // A private_property_identifier (#name) implies Private.
    Access getMemberAccess(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* acc = findChild(tree, node, k_accessibility_modifier);
        if(!acc) {
            if(findChild(tree, node, k_private_property_identifier))
                return Access::Private;
            return Access::Public;
        }
        std::string text = acc->getText();
        if(text == "private")   return Access::Private;
        if(text == "protected") return Access::Protected;
        return Access::Public;
    }

    // Scan for the first property_identifier or private_property_identifier child.
    std::string getMemberName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(k_property_identifier) ||
               child.typeEquals(k_private_property_identifier))
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
            if(child.typeEquals(k_identifier)) return &child;
        }
        return nullptr;
    }

    // Per the Web Components spec a custom element name must contain a hyphen.
    bool isCustomElement(const std::string& name)
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
    std::unordered_set<std::string> seen;   // key = fqn + ":" + kind ordinal
    std::vector<ScopeFrame>         scopeStack;

    auto emit = [&](Symbol sym) {
        std::string key = sym.fqn + ":" + std::to_string(static_cast<int>(sym.kind));
        if(!sym.fqn.empty() && seen.insert(key).second)
            result.push_back(std::move(sym));
    };

    for(uint32_t i = 0; i < tree.size(); ++i) {
        const ast::ASTNode& node = tree[i];

        while(!scopeStack.empty() && scopeStack.back().endByte <= node.startByte_)
            scopeStack.pop_back();

        if(insideFunctionScope(scopeStack)) continue;

        // ── Ambient declaration (declare …) ───────────────────────────────────
        if(node.typeEquals(k_ambient_declaration)) {
            scopeStack.push_back({"", SymbolKind::Function, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Namespace ─────────────────────────────────────────────────────────
        if(node.typeEquals(k_internal_module)) {
            const ast::ASTNode* id = findChild(tree, node, k_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Namespace, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Namespace, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Enum ──────────────────────────────────────────────────────────────
        if(node.typeEquals(k_enum_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, k_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
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
        if(node.typeEquals(k_property_identifier) && insideEnumScope(scopeStack)) {
            std::string name = node.getText();
            std::string fqn  = buildFQN(scopeStack, name, ".");
            emit(makeSymbol(name, fqn, SymbolKind::EnumValue, Access::Public,
                           i, node.start_.row_, node.start_.column_));
            continue;
        }

        // ── Interface ─────────────────────────────────────────────────────────
        if(node.typeEquals(k_interface_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, k_type_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Class, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Interface method signature (abstract) ─────────────────────────────
        if(node.typeEquals(k_method_signature)) {
            std::string name = getMemberName(tree, node);
            if(!name.empty()) {
                std::string fqn = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Method, Access::Public,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Interface property signature ──────────────────────────────────────
        if(node.typeEquals(k_property_signature)) {
            std::string name = getMemberName(tree, node);
            if(!name.empty()) {
                std::string fqn = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Field, Access::Public,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Class (regular and abstract) ──────────────────────────────────────
        if(node.typeEquals(k_class_declaration) ||
           node.typeEquals(k_abstract_class_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, k_type_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Class, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Class field ───────────────────────────────────────────────────────
        if(node.typeEquals(k_public_field_definition)) {
            std::string name = getMemberName(tree, node);
            if(!name.empty()) {
                std::string fqn = buildFQN(scopeStack, name, ".");
                Access acc = getMemberAccess(tree, node);
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Field, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic    = childHasText(tree, node, "static");
                sym.isConstexpr = childHasText(tree, node, "readonly");
                emit(std::move(sym));
            }
            continue;
        }

        // ── Method / constructor ──────────────────────────────────────────────
        if(node.typeEquals(k_method_definition)) {
            std::string name = getMemberName(tree, node);
            if(!name.empty()) {
                std::string fqn  = buildFQN(scopeStack, name, ".");
                Access      acc  = getMemberAccess(tree, node);
                SymbolKind  kind = (name == "constructor")
                                       ? SymbolKind::Constructor
                                       : SymbolKind::Method;
                Symbol sym = makeSymbol(name, fqn, kind, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = childHasText(tree, node, "static");
                emit(std::move(sym));
                scopeStack.push_back({name, kind, node.endByte_, acc, false});
            }
            continue;
        }

        // ── Abstract method signature (no body) ───────────────────────────────
        if(node.typeEquals(k_abstract_method_signature)) {
            std::string name = getMemberName(tree, node);
            if(!name.empty()) {
                std::string fqn = buildFQN(scopeStack, name, ".");
                Access      acc = getMemberAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Method, acc,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Type alias ────────────────────────────────────────────────────────
        if(node.typeEquals(k_type_alias_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, k_type_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Typedef, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Free function (regular, async, generator) ─────────────────────────
        if(node.typeEquals(k_function_declaration) ||
           node.typeEquals(k_generator_function_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, k_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Function, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Function, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Variable / lexical declaration ────────────────────────────────────
        if(node.typeEquals(k_variable_declaration) ||
           node.typeEquals(k_lexical_declaration)) {
            continue;
        }

        // ── Variable declarator ───────────────────────────────────────────────
        if(node.typeEquals(k_variable_declarator)) {
            const ast::ASTNode* id = findChild(tree, node, k_identifier);
            if(!id) continue;
            std::string name = id->getText();
            std::string fqn  = buildFQN(scopeStack, name, ".");

            const ast::ASTNode* val = getDeclValue(tree, node);
            if(val && isFunctionLike(*val)) {
                emit(makeSymbol(name, fqn, SymbolKind::Function, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Function, node.endByte_,
                                      Access::Unknown, false});
            } else if(val && val->typeEquals(k_object)) {
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
        if(node.typeEquals(k_pair)) {
            if(!insideObjectLiteralScope(scopeStack)) continue;
            const ast::ASTNode* key = findChild(tree, node, k_property_identifier);
            if(!key) continue;
            std::string name = key->getText();
            std::string fqn  = buildFQN(scopeStack, name, ".");

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
        if(node.typeEquals(k_jsx_opening_element) ||
           node.typeEquals(k_jsx_self_closing_element)) {
            const ast::ASTNode* tag = jsxTagName(tree, node);
            if(tag) {
                std::string name = tag->getText();
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
