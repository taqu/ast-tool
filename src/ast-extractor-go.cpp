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
    // ── Node type string constants (tree-sitter-go grammar) ─────────────────
    constexpr const char* k_package_clause      = "package_clause";
    constexpr const char* k_package_identifier  = "package_identifier";
    constexpr const char* k_import_spec         = "import_spec";
    constexpr const char* k_type_spec           = "type_spec";
    constexpr const char* k_type_alias          = "type_alias";
    constexpr const char* k_type_identifier     = "type_identifier";
    constexpr const char* k_struct_type         = "struct_type";
    constexpr const char* k_interface_type      = "interface_type";
    constexpr const char* k_field_declaration   = "field_declaration";
    constexpr const char* k_field_identifier    = "field_identifier";
    constexpr const char* k_method_elem         = "method_elem";
    constexpr const char* k_function_declaration = "function_declaration";
    constexpr const char* k_method_declaration  = "method_declaration";
    constexpr const char* k_parameter_list      = "parameter_list";
    constexpr const char* k_parameter_declaration = "parameter_declaration";
    constexpr const char* k_pointer_type        = "pointer_type";
    constexpr const char* k_qualified_type      = "qualified_type";
    constexpr const char* k_generic_type        = "generic_type";
    constexpr const char* k_const_spec          = "const_spec";
    constexpr const char* k_var_spec            = "var_spec";
    constexpr const char* k_identifier          = "identifier";

    // ── Low-level helpers ─────────────────────────────────────────────────────

    // Go's only visibility rule: an identifier is exported (public) iff its
    // first letter is uppercase. There is no access-modifier keyword to parse.
    Access accessFromName(const std::string& name)
    {
        if(name.empty()) return Access::Unknown;
        char c = name[0];
        if(c >= 'A' && c <= 'Z') return Access::Public;
        if((c >= 'a' && c <= 'z') || c == '_') return Access::Private;
        return Access::Unknown; // non-ASCII leading rune: not classified here
    }

    // Unwraps pointer_type / qualified_type / generic_type down to the base
    // type name, e.g. "*io.Reader" -> "Reader", "List[int]" -> "List".
    std::string baseTypeName(const ast::AST& tree, const ast::ASTNode& node)
    {
        if(node.typeEquals(k_type_identifier)) return node.getText();
        if(node.typeEquals(k_qualified_type)) {
            const ast::ASTNode* ident = findChild(tree, node, k_type_identifier);
            return ident ? ident->getText() : std::string();
        }
        if(node.typeEquals(k_pointer_type) || node.typeEquals(k_generic_type)) {
            for(uintptr_t id : node.children_) {
                if(id == ast::InvalidId) continue;
                const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
                std::string name = baseTypeName(tree, child);
                if(!name.empty()) return name;
            }
        }
        return {};
    }

    // The declared name of a type_spec / type_alias: the first type_identifier
    // direct child. It always precedes the underlying type expression, so the
    // first match is unambiguous (unlike C#, Go never places a return/field
    // type before the name at this level).
    std::string getTypeSpecName(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* ident = findChild(tree, node, k_type_identifier);
        return ident ? ident->getText() : std::string();
    }

    // The receiver type name of a method_declaration, e.g. "Foo" from both
    // "(f Foo)" and "(f *Foo)". The receiver is always the first parameter_list.
    std::string getReceiverTypeName(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* recv = findChild(tree, node, k_parameter_list);
        if(!recv) return {};
        const ast::ASTNode* param = findChild(tree, *recv, k_parameter_declaration);
        if(!param) return {};
        for(uintptr_t id : param->children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            std::string name = baseTypeName(tree, child);
            if(!name.empty()) return name;
        }
        return {};
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_go(const ast::AST& tree)
{
    std::vector<Symbol>             result;
    std::unordered_set<std::string> seen;   // key = fqn + ":" + kind ordinal
    std::vector<ScopeFrame>         scopeStack;

    auto emit = [&](Symbol sym) {
        std::string key = sym.fqn + ":" + std::to_string(static_cast<int>(sym.kind));
        if(!sym.fqn.empty() && seen.insert(key).second) {
            result.push_back(std::move(sym));
        }
    };

    for(uint32_t i = 0; i < tree.size(); ++i) {
        const ast::ASTNode& node = tree[i];

        // Pop scopes whose byte range ended before this node begins.
        // Works because nodes are stored in DFS pre-order.
        while(!scopeStack.empty() && scopeStack.back().endByte <= node.startByte_) {
            scopeStack.pop_back();
        }

        // ── Package clause ──────────────────────────────────────────────────
        // Applies to the whole file; there is no closing delimiter, so the
        // scope is given a byte range that never closes during this traversal.
        if(node.typeEquals(k_package_clause)) {
            const ast::ASTNode* ident = findChild(tree, node, k_package_identifier);
            std::string name = ident ? ident->getText() : std::string();
            if(!name.empty()) {
                emit(makeSymbol(name, name, SymbolKind::Namespace, Access::Unknown,
                                i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Namespace, uint32_t(-1),
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Import alias: `import alias "path"` ─────────────────────────────
        if(node.typeEquals(k_import_spec)) {
            const ast::ASTNode* alias = findChild(tree, node, k_package_identifier);
            if(alias) {
                std::string name = alias->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::UsingAlias, Access::Unknown,
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Type declaration: struct / interface / defined type / alias ─────
        if((node.typeEquals(k_type_spec) || node.typeEquals(k_type_alias))
           && !insideFunctionScope(scopeStack)) {
            std::string name = getTypeSpecName(tree, node);
            if(name.empty()) continue;

            SymbolKind kind;
            bool       isContainer = false;
            if(node.typeEquals(k_type_alias)) {
                kind = SymbolKind::UsingAlias;
            } else if(findChild(tree, node, k_struct_type)) {
                kind = SymbolKind::Struct;
                isContainer = true;
            } else if(findChild(tree, node, k_interface_type)) {
                kind = SymbolKind::Class; // closest available SymbolKind
                isContainer = true;
            } else {
                kind = SymbolKind::Typedef;
            }

            std::string fqn = buildFQN(scopeStack, name, ".");
            emit(makeSymbol(name, fqn, kind, accessFromName(name),
                            i, node.start_.row_, node.start_.column_));
            if(isContainer) {
                scopeStack.push_back({name, kind, node.endByte_, Access::Unknown, false});
            }
            continue;
        }

        // ── Struct fields (named and embedded) ───────────────────────────────
        if(node.typeEquals(k_field_declaration) && !insideFunctionScope(scopeStack)) {
            bool any = false;
            for(uintptr_t id : node.children_) {
                if(id == ast::InvalidId) continue;
                const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
                if(!child.typeEquals(k_field_identifier)) continue;
                any = true;
                std::string name = child.getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Field, accessFromName(name),
                                i, node.start_.row_, node.start_.column_));
            }
            if(!any) {
                // Embedded field: no explicit name, so Go uses the base type
                // name as the implicit field name.
                for(uintptr_t id : node.children_) {
                    if(id == ast::InvalidId) continue;
                    const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
                    std::string name = baseTypeName(tree, child);
                    if(name.empty()) continue;
                    std::string fqn = buildFQN(scopeStack, name, ".");
                    emit(makeSymbol(name, fqn, SymbolKind::Field, accessFromName(name),
                                    i, node.start_.row_, node.start_.column_));
                    break;
                }
            }
            continue;
        }

        // ── Interface method signatures ─────────────────────────────────────
        if(node.typeEquals(k_method_elem)) {
            const ast::ASTNode* ident = findChild(tree, node, k_field_identifier);
            if(ident) {
                std::string name = ident->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Method, accessFromName(name),
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Free function ────────────────────────────────────────────────────
        if(node.typeEquals(k_function_declaration)) {
            const ast::ASTNode* ident = findChild(tree, node, k_identifier);
            if(ident) {
                std::string name = ident->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Function, accessFromName(name),
                                i, node.start_.row_, node.start_.column_));
            }
            // Pushed even without a name so nested var/const declarations in an
            // (malformed) body are still correctly recognized as local.
            scopeStack.push_back({{}, SymbolKind::Function, node.endByte_, Access::Unknown, false});
            continue;
        }

        // ── Method (function with a receiver) ────────────────────────────────
        if(node.typeEquals(k_method_declaration)) {
            const ast::ASTNode* ident = findChild(tree, node, k_field_identifier);
            if(ident) {
                std::string name      = ident->getText();
                std::string recvType  = getReceiverTypeName(tree, node);
                std::string qualified = recvType.empty() ? name : (recvType + "." + name);
                std::string fqn       = buildFQN(scopeStack, qualified, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Method, accessFromName(name),
                                i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({{}, SymbolKind::Method, node.endByte_, Access::Unknown, false});
            continue;
        }

        // ── Package-level const / var ────────────────────────────────────────
        if((node.typeEquals(k_const_spec) || node.typeEquals(k_var_spec))
           && !insideFunctionScope(scopeStack)) {
            bool isConst = node.typeEquals(k_const_spec);
            for(uintptr_t id : node.children_) {
                if(id == ast::InvalidId) continue;
                const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
                if(!child.grammarEquals(k_identifier)) continue;
                std::string name = child.getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Variable, accessFromName(name),
                                       i, node.start_.row_, node.start_.column_);
                sym.isConstexpr = isConst;
                emit(std::move(sym));
            }
            continue;
        }
    }

    return result;
}

} // namespace extractor
} // namespace ast
