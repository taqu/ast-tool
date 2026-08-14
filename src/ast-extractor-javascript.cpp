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
    // ── Node type string constants (tree-sitter-javascript grammar) ──────────
    constexpr const char* k_function_declaration           = "function_declaration";
    constexpr const char* k_generator_function_declaration = "generator_function_declaration";
    constexpr const char* k_class_declaration              = "class_declaration";
    constexpr const char* k_method_definition              = "method_definition";
    constexpr const char* k_field_definition               = "field_definition";
    constexpr const char* k_variable_declarator            = "variable_declarator";
    constexpr const char* k_pair                           = "pair";
    constexpr const char* k_identifier                     = "identifier";
    constexpr const char* k_property_identifier            = "property_identifier";
    constexpr const char* k_private_property_identifier    = "private_property_identifier";
    constexpr const char* k_arrow_function                 = "arrow_function";
    constexpr const char* k_function_expression            = "function_expression";
    constexpr const char* k_generator_function             = "generator_function";
    constexpr const char* k_object                         = "object";

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

    // Return the declared name from a field_definition node.
    // Public fields use property_identifier; private fields (#name) use
    // private_property_identifier.
    std::u8string getFieldName(const ast::AST& tree, const ast::ASTNode& node)
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

    // True if the innermost meaningful scope on the stack is an object-literal
    // scope (SymbolKind::Variable). This distinguishes object literal methods
    // from free-floating pair nodes inside complex expressions.
    // When called, we already know we are not inside a function scope.
    bool insideObjectLiteralScope(const std::vector<ScopeFrame>& stack)
    {
        for(int i = static_cast<int>(stack.size()) - 1; i >= 0; --i) {
            SymbolKind k = stack[i].kind;
            if(k == SymbolKind::Variable)  return true;  // object literal scope
            if(k == SymbolKind::Class)     return false; // class body scope
            if(k == SymbolKind::Namespace) return false; // module scope
        }
        return false;
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_javascript(const ast::AST& tree)
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

        // ── Free function (regular, async, generator) ─────────────────────────
        if(node.typeEquals(k_function_declaration) ||
           node.typeEquals(k_generator_function_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Function, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                // Push a function scope so the body is suppressed on the next
                // iterations. The scope spans the whole declaration node.
                scopeStack.push_back({name, SymbolKind::Function, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Class declaration ─────────────────────────────────────────────────
        if(node.typeEquals(k_class_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
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

        // ── Method / constructor (class body or object literal) ───────────────
        // method_definition covers: regular methods, constructors, getters,
        // setters, static methods, and async methods. The name is always in the
        // first property_identifier child; static/get/set keywords have distinct
        // node types so findChild skips them cleanly.
        if(node.typeEquals(k_method_definition)) {
            const ast::ASTNode* propId = findChild(tree, node, (const char8_t*)k_property_identifier);
            if(propId) {
                std::u8string name = propId->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                SymbolKind kind  = (name == u8"constructor")
                                        ? SymbolKind::Constructor
                                        : SymbolKind::Method;
                Symbol sym = makeSymbol(name, fqn, kind, Access::Unknown,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = childHasText(tree, node, u8"static");
                emit(std::move(sym));
                scopeStack.push_back({name, kind, node.endByte_, Access::Unknown, false});
            }
            continue;
        }

        // ── Class field (public and private) ──────────────────────────────────
        if(node.typeEquals(k_field_definition)) {
            std::u8string name = getFieldName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Field, Access::Unknown,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = childHasText(tree, node, u8"static");
                emit(std::move(sym));
            }
            continue;
        }

        // ── Variable declarator (const/let/var at module or object scope) ─────
        // Distinguishes three value patterns:
        //   • arrow_function / function_expression / generator_function
        //     → emit as Function, push a function scope to suppress the body.
        //   • object literal
        //     → emit as Variable, push an object-literal scope so that inner
        //       method_definition and pair nodes receive the correct FQN prefix.
        //   • anything else (number, string, array, …)
        //     → emit as Variable, no scope push.
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
                // Scope spans the object literal's byte range; method_definition
                // and pair children inside will build FQNs like "utils.format".
                scopeStack.push_back({name, SymbolKind::Variable, val->endByte_,
                                      Access::Unknown, false});
            } else {
                emit(makeSymbol(name, fqn, SymbolKind::Variable, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Object literal key-value pair ─────────────────────────────────────
        // Only processed when the enclosing scope is an object literal (Variable
        // scope), which is set up by the variable_declarator handler above.
        // function-valued pairs are emitted as Function; other values as Variable
        // (they are observable properties of the module-level object).
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
                // Suppress the function body.
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
