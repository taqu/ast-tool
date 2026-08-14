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
    // ── Node type string constants (tree-sitter-scala grammar) ────────────────
    constexpr const char* k_package_clause       = "package_clause";
    constexpr const char* k_package_object       = "package_object";
    constexpr const char* k_object_definition    = "object_definition";
    constexpr const char* k_class_definition     = "class_definition";
    constexpr const char* k_trait_definition     = "trait_definition";
    constexpr const char* k_function_definition  = "function_definition";
    constexpr const char* k_function_declaration = "function_declaration";
    constexpr const char* k_val_definition       = "val_definition";
    constexpr const char* k_var_definition       = "var_definition";
    constexpr const char* k_type_definition      = "type_definition";
    constexpr const char* k_class_parameter      = "class_parameter";
    constexpr const char* k_identifier           = "identifier";
    constexpr const char* k_type_identifier      = "type_identifier";
    constexpr const char* k_package_identifier   = "package_identifier";
    constexpr const char* k_modifiers            = "modifiers";
    constexpr const char* k_access_modifier      = "access_modifier";

    // ── Low-level helpers ─────────────────────────────────────────────────────

    // Scala defaults to public; returns Private/Protected only when an explicit
    // access_modifier is present inside a modifiers child.
    Access getAccess(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* mods = findChild(tree, node, (const char8_t*)k_modifiers);
        if(!mods) return Access::Public;
        const ast::ASTNode* acc = findChild(tree, *mods, (const char8_t*)k_access_modifier);
        if(!acc) return Access::Public;
        std::u8string text = acc->getText();
        if(text == u8"private")   return Access::Private;
        if(text == u8"protected") return Access::Protected;
        return Access::Public;
    }

    // True if the class_parameter declares a val or var constructor field.
    bool paramIsField(const ast::AST& tree, const ast::ASTNode& param)
    {
        for(uintptr_t id : param.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            std::u8string t = child.getText();
            if(t == u8"val" || t == u8"var") return true;
        }
        return false;
    }

    // True if the innermost non-function scope is a Class or Struct
    // (i.e. the enclosing body is a class, object, or trait — not a package).
    bool insideClassScope(const std::vector<ScopeFrame>& stack)
    {
        for(int i = static_cast<int>(stack.size()) - 1; i >= 0; --i) {
            SymbolKind k = stack[i].kind;
            if(k == SymbolKind::Function  || k == SymbolKind::Method ||
               k == SymbolKind::Constructor)
                continue;
            return k == SymbolKind::Class || k == SymbolKind::Struct;
        }
        return false;
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_scala(const ast::AST& tree)
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

        // Suppress everything inside method/function/constructor bodies.
        if(insideFunctionScope(scopeStack)) continue;

        // ── Package clause ────────────────────────────────────────────────────
        // Emitted as Namespace. Uses uint32_t(-1) so the scope spans the entire
        // file (the package_clause node itself ends after the package keyword).
        if(node.typeEquals(k_package_clause)) {
            const ast::ASTNode* pkgId = findChild(tree, node, (const char8_t*)k_package_identifier);
            if(pkgId) {
                std::u8string name = pkgId->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Namespace, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Namespace, uint32_t(-1),
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Package object ────────────────────────────────────────────────────
        // "package object utils { … }" — a namespace-level container.
        if(node.typeEquals(k_package_object)) {
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

        // ── Object definition (singleton) ─────────────────────────────────────
        // Both plain objects and case objects are emitted as Class (closest kind).
        if(node.typeEquals(k_object_definition)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Class, Access::Public,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      Access::Public, false});
            }
            continue;
        }

        // ── Class definition (regular, abstract, case) ────────────────────────
        // isAccessAware repurposed: true → case class (all constructor params are fields).
        if(node.typeEquals(k_class_definition)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
            if(id) {
                std::u8string name   = id->getText();
                std::u8string fqn    = buildFQN(scopeStack, name, u8".");
                Access      acc    = getAccess(tree, node);
                bool        isCase = childHasText(tree, node, u8"case");
                emit(makeSymbol(name, fqn, SymbolKind::Class, acc,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      acc, isCase});
            }
            continue;
        }

        // ── Trait definition ──────────────────────────────────────────────────
        if(node.typeEquals(k_trait_definition)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Class, acc,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Constructor parameter → Field ─────────────────────────────────────
        // Emitted when the parameter has a `val`/`var` prefix (regular class) or
        // when the enclosing class is a case class (all params become val fields).
        if(node.typeEquals(k_class_parameter)) {
            bool isCaseClass = !scopeStack.empty() && scopeStack.back().isAccessAware;
            if(!isCaseClass && !paramIsField(tree, node)) continue;
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Field, acc,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Function definition (with body) ───────────────────────────────────
        // Inside a class/object/trait → Method; otherwise → Function.
        if(node.typeEquals(k_function_definition)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                Access      acc  = getAccess(tree, node);
                SymbolKind  kind = insideClassScope(scopeStack)
                                       ? SymbolKind::Method
                                       : SymbolKind::Function;
                emit(makeSymbol(name, fqn, kind, acc,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, kind, node.endByte_, acc, false});
            }
            continue;
        }

        // ── Function declaration (abstract method, no body) ───────────────────
        if(node.typeEquals(k_function_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Method, acc,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Val definition (immutable) ────────────────────────────────────────
        // Field inside a class/object; Variable at package/top-level scope.
        if(node.typeEquals(k_val_definition)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                Access      acc  = getAccess(tree, node);
                SymbolKind  kind = insideClassScope(scopeStack)
                                       ? SymbolKind::Field
                                       : SymbolKind::Variable;
                Symbol sym = makeSymbol(name, fqn, kind, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isConstexpr = true;
                emit(std::move(sym));
            }
            continue;
        }

        // ── Var definition (mutable) ──────────────────────────────────────────
        if(node.typeEquals(k_var_definition)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                Access      acc  = getAccess(tree, node);
                SymbolKind  kind = insideClassScope(scopeStack)
                                       ? SymbolKind::Field
                                       : SymbolKind::Variable;
                emit(makeSymbol(name, fqn, kind, acc,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Type alias ────────────────────────────────────────────────────────
        if(node.typeEquals(k_type_definition)) {
            const ast::ASTNode* id = findChild(tree, node, (const char8_t*)k_type_identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
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
