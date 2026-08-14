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
    // ── Node type string constants (tree-sitter-typescript grammar) ───────────
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

    // ── Low-level helpers ─────────────────────────────────────────────────────

    bool isFunctionLike(const ast::ASTNode& node)
    {
        return node.typeEquals(k_arrow_function)
            || node.typeEquals(k_function_expression)
            || node.typeEquals(k_generator_function);
    }

    // Return the value node of a variable_declarator (the child after '=').
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

    // Return the value node of a pair (the child after ':').
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

    // Extract the accessibility-modifier access for class members.
    // TypeScript defaults to public when no modifier is present.
    // A private_property_identifier (#name) implies Private.
    Access getMemberAccess(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* acc = findChild(tree, node, (const char8_t*)k_accessibility_modifier);
        if(!acc) {
            if(findChild(tree, node, (const char8_t*)k_private_property_identifier))
                return Access::Private;
            return Access::Public;
        }
        std::u8string text = acc->getText();
        if(text == u8"private")   return Access::Private;
        if(text == u8"protected") return Access::Protected;
        return Access::Public;
    }

    // Return the declared name from a class field or method node.
    // Public members use property_identifier; private fields use
    // private_property_identifier (#name).
    std::u8string getMemberName(const ast::AST& tree, const ast::ASTNode& node)
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

    // True if the innermost non-function scope is an Enum.
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

    // True if the innermost meaningful scope is an object-literal scope
    // (Variable kind), set up by the variable_declarator handler.
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

} // anonymous namespace

std::vector<Symbol> extract_symbols_typescript(const ast::AST& tree)
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

        // Skip everything inside function/method/constructor bodies.
        if(insideFunctionScope(scopeStack)) continue;

        // ── Ambient declaration (declare …) ───────────────────────────────────
        // Describes types from external modules; not symbols defined in this file.
        // Push a Function scope so that all children are suppressed.
        if(node.typeEquals(k_ambient_declaration)) {
            scopeStack.push_back({u8"", SymbolKind::Function, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Namespace / module ────────────────────────────────────────────────
        // "namespace Foo { … }" appears as internal_module in tree-sitter.
        if(node.typeEquals(k_internal_module)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
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
        if(node.typeEquals(k_enum_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
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
        // Bare enum member or left-hand side of enum_assignment, both of which
        // appear as property_identifier nodes inside the enum_body.
        if(node.typeEquals(k_property_identifier) && insideEnumScope(scopeStack)) {
            std::u8string name = node.getText();
            std::u8string fqn  = buildFQN(scopeStack, name, u8".");
            emit(makeSymbol(name, fqn, SymbolKind::EnumValue, Access::Public,
                           i, node.start_.row_, node.start_.column_));
            continue;
        }

        // ── Interface ─────────────────────────────────────────────────────────
        // Modelled as Class (closest SymbolKind). Members are visited via DFS
        // and handled by the method_signature / property_signature cases below.
        if(node.typeEquals(k_interface_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_type_identifier);
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
        if(node.typeEquals(k_method_signature)) {
            std::u8string name = getMemberName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Method, Access::Public,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Interface property signature ──────────────────────────────────────
        if(node.typeEquals(k_property_signature)) {
            std::u8string name = getMemberName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Field, Access::Public,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Class declaration (regular and abstract) ──────────────────────────
        if(node.typeEquals(k_class_declaration) ||
           node.typeEquals(k_abstract_class_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_type_identifier);
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
        if(node.typeEquals(k_public_field_definition)) {
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
        // Covers: regular methods, static methods, constructors, getters, setters,
        // async methods. Name is always the property_identifier child; get/set
        // keyword children are skipped by getMemberName.
        if(node.typeEquals(k_method_definition)) {
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
        if(node.typeEquals(k_abstract_method_signature)) {
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
        if(node.typeEquals(k_type_alias_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_type_identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Typedef, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Free function (regular, async, generator) ─────────────────────────
        if(node.typeEquals(k_function_declaration) ||
           node.typeEquals(k_generator_function_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
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
        // Both "var" and "const/let" forms; the variable_declarator children
        // carry the actual names and values.
        if(node.typeEquals(k_variable_declaration) ||
           node.typeEquals(k_lexical_declaration)) {
            // Children are visited normally in DFS; the variable_declarator
            // handler below does the real work.
            continue;
        }

        // ── Variable declarator ───────────────────────────────────────────────
        // Distinguishes three value patterns (same logic as the JS extractor):
        //   • function-like (arrow/function_expression/generator) → Function
        //   • object literal                                       → Variable + object scope
        //   • anything else                                        → Variable
        if(node.typeEquals(k_variable_declarator)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
            if(!id) continue;
            std::u8string name = id->getText();
            std::u8string fqn  = buildFQN(scopeStack, name, u8".");

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
        // Only meaningful when the enclosing scope is an object-literal (Variable).
        if(node.typeEquals(k_pair)) {
            if(!insideObjectLiteralScope(scopeStack)) continue;
            const ast::ASTNode* key = findChild(tree, node, (const char8_t*)k_property_identifier);
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
    }

    return result;
}

} // namespace extractor
} // namespace ast
