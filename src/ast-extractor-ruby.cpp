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
    // ── Node type string constants (tree-sitter-ruby grammar) ────────────────
    constexpr const char* k_class            = "class";
    constexpr const char* k_module           = "module";
    constexpr const char* k_method           = "method";
    constexpr const char* k_singleton_method = "singleton_method";
    constexpr const char* k_assignment       = "assignment";
    constexpr const char* k_call             = "call";
    constexpr const char* k_identifier       = "identifier";
    constexpr const char* k_constant         = "constant";
    constexpr const char* k_global_variable  = "global_variable";
    constexpr const char* k_class_variable   = "class_variable";
    constexpr const char* k_argument_list    = "argument_list";
    constexpr const char* k_simple_symbol    = "simple_symbol";

    // ── Low-level helpers ─────────────────────────────────────────────────────

    // Strip the leading ':' from a Ruby symbol literal (":foo" → "foo").
    std::u8string stripSymbolColon(const std::u8string& s)
    {
        if(!s.empty() && s[0] == ':') return s.substr(1);
        return s;
    }

    // True for the three attr_* helpers that synthesise accessor methods.
    bool isAttrMethod(const std::u8string& name)
    {
        return name == u8"attr_reader"
            || name == u8"attr_writer"
            || name == u8"attr_accessor";
    }

    // Set the currentAccess of the innermost access-aware scope (i.e. a class
    // body). Module bodies are not access-aware.
    void setTopAccess(std::vector<ScopeFrame>& stack, Access acc)
    {
        for(int i = static_cast<int>(stack.size()) - 1; i >= 0; --i) {
            if(stack[i].isAccessAware) {
                stack[i].currentAccess = acc;
                return;
            }
        }
    }

    // Return the first direct child whose type matches `type`, or nullptr.
    // (Thin wrapper kept local so this file needs no extra #includes.)
    const ast::ASTNode* firstChildOfType(const ast::AST& tree,
                                          const ast::ASTNode& node,
                                          const char8_t* type)
    {
        return findChild(tree, node, type);
    }

    // Return the LHS node of an assignment (its very first child).
    const ast::ASTNode* assignmentLhs(const ast::AST& tree,
                                       const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            return &tree[static_cast<uint32_t>(id)];
        }
        return nullptr;
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_ruby(const ast::AST& tree)
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

        // Suppress everything inside method/constructor bodies.
        if(insideFunctionScope(scopeStack)) continue;

        // ── Class definition ──────────────────────────────────────────────────
        // Ruby classes default to public access; the isAccessAware flag enables
        // tracking of subsequent private/protected declarations in this scope.
        if(node.typeEquals(k_class)) {
            const ast::ASTNode* name_node = firstChildOfType(tree, node, (const char8_t*)k_constant);
            if(name_node) {
                std::u8string name = name_node->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                emit(makeSymbol(name, fqn, SymbolKind::Class, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      Access::Public, /*isAccessAware=*/true});
            }
            continue;
        }

        // ── Module definition ─────────────────────────────────────────────────
        if(node.typeEquals(k_module)) {
            const ast::ASTNode* name_node = firstChildOfType(tree, node, (const char8_t*)k_constant);
            if(name_node) {
                std::u8string name = name_node->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                emit(makeSymbol(name, fqn, SymbolKind::Namespace, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Namespace, node.endByte_,
                                      Access::Unknown, /*isAccessAware=*/false});
            }
            continue;
        }

        // ── Instance method (def name … end) ─────────────────────────────────
        // `initialize` is the Ruby constructor.
        if(node.typeEquals(k_method)) {
            const ast::ASTNode* name_node = firstChildOfType(tree, node, (const char8_t*)k_identifier);
            if(name_node) {
                std::u8string name = name_node->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                SymbolKind  kind = (name == u8"initialize")
                                        ? SymbolKind::Constructor
                                        : SymbolKind::Method;
                Access acc = topAccess(scopeStack);
                emit(makeSymbol(name, fqn, kind, acc,
                               i, node.start_.row_, node.start_.column_));
                // Push a function scope so the body is suppressed.
                scopeStack.push_back({name, kind, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Singleton method (def self.name … end) ────────────────────────────
        // These are class-level methods (analogous to static methods).
        if(node.typeEquals(k_singleton_method)) {
            const ast::ASTNode* name_node = firstChildOfType(tree, node, (const char8_t*)k_identifier);
            if(name_node) {
                std::u8string name = name_node->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Method, Access::Public,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = true;
                emit(std::move(sym));
                scopeStack.push_back({name, SymbolKind::Method, node.endByte_,
                                      Access::Public, false});
            }
            continue;
        }

        // ── Assignment: constant, global, or class variable ───────────────────
        // Emits constants (ALL_CAPS or CamelCase LHS), global variables ($foo),
        // and class variables (@@foo) defined at module/class scope.
        // Instance variables (@foo) are skipped — they are not file-scope symbols.
        if(node.typeEquals(k_assignment)) {
            const ast::ASTNode* lhs = assignmentLhs(tree, node);
            if(!lhs) continue;
            if(lhs->typeEquals(k_constant)) {
                std::u8string name = lhs->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                emit(makeSymbol(name, fqn, SymbolKind::Variable, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            } else if(lhs->typeEquals(k_global_variable)) {
                std::u8string name = lhs->getText(); // includes the '$' sigil
                emit(makeSymbol(name, name, SymbolKind::Variable, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            } else if(lhs->typeEquals(k_class_variable)
                      && inNamedClassScope(scopeStack)) {
                std::u8string name = lhs->getText(); // includes the '@@' sigil
                std::u8string fqn  = buildFQN(scopeStack, name, u8"::");
                emit(makeSymbol(name, fqn, SymbolKind::Field, Access::Private,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Access modifier (bare `private` / `protected` / `public`) ─────────
        // In tree-sitter-ruby, a bare access modifier keyword inside a class body
        // appears as an identifier node.  It affects all subsequently defined
        // methods in that class scope.
        if(node.typeEquals(k_identifier)) {
            std::u8string text = node.getText();
            if(text == u8"private")   { setTopAccess(scopeStack, Access::Private);   continue; }
            if(text == u8"protected") { setTopAccess(scopeStack, Access::Protected); continue; }
            if(text == u8"public")    { setTopAccess(scopeStack, Access::Public);    continue; }
            // Other identifiers are not symbols we emit.
            continue;
        }

        // ── attr_reader / attr_writer / attr_accessor ─────────────────────────
        // These Ruby metaprogramming helpers synthesise getter/setter methods.
        // Emit one Method symbol per named attribute.
        if(node.typeEquals(k_call)) {
            const ast::ASTNode* callee = firstChildOfType(tree, node, (const char8_t*)k_identifier);
            if(!callee || !isAttrMethod(callee->getText())) continue;
            const ast::ASTNode* args = firstChildOfType(tree, node, (const char8_t*)k_argument_list);
            if(!args) continue;
            Access acc = topAccess(scopeStack);
            for(uintptr_t cid : args->children_) {
                if(cid == ast::InvalidId) continue;
                const ast::ASTNode& sym_node = tree[static_cast<uint32_t>(cid)];
                if(!sym_node.typeEquals(k_simple_symbol)) continue;
                std::u8string name = stripSymbolColon(sym_node.getText());
                if(name.empty()) continue;
                std::u8string fqn = buildFQN(scopeStack, name, u8"::");
                emit(makeSymbol(name, fqn, SymbolKind::Method, acc,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }
    }

    return result;
}

} // namespace extractor
} // namespace ast
