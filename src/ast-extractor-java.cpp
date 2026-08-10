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
    // ── Node type string constants (tree-sitter-java grammar) ────────────────
    constexpr const char* k_package_declaration                  = "package_declaration";
    constexpr const char* k_class_declaration                    = "class_declaration";
    constexpr const char* k_interface_declaration                = "interface_declaration";
    constexpr const char* k_annotation_type_declaration          = "annotation_type_declaration";
    constexpr const char* k_enum_declaration                     = "enum_declaration";
    constexpr const char* k_enum_constant                        = "enum_constant";
    constexpr const char* k_constructor_declaration              = "constructor_declaration";
    constexpr const char* k_method_declaration                   = "method_declaration";
    constexpr const char* k_field_declaration                    = "field_declaration";
    constexpr const char* k_annotation_type_element_declaration  = "annotation_type_element_declaration";
    constexpr const char* k_modifiers                            = "modifiers";
    constexpr const char* k_identifier                           = "identifier";
    constexpr const char* k_scoped_identifier                    = "scoped_identifier";
    constexpr const char* k_variable_declarator                  = "variable_declarator";

    // ── Low-level helpers ─────────────────────────────────────────────────────

    // Extract the access level declared in the node's modifiers child.
    // Returns Unknown for package-private (no explicit access keyword).
    Access getAccess(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* mods = findChild(tree, node, k_modifiers);
        if(!mods) return Access::Unknown;
        if(childHasText(tree, *mods, "public"))    return Access::Public;
        if(childHasText(tree, *mods, "private"))   return Access::Private;
        if(childHasText(tree, *mods, "protected")) return Access::Protected;
        return Access::Unknown;
    }

    // Returns true if the node's modifiers child contains the given keyword.
    bool hasModifier(const ast::AST& tree, const ast::ASTNode& node, const char* kw)
    {
        const ast::ASTNode* mods = findChild(tree, node, k_modifiers);
        return mods && childHasText(tree, *mods, kw);
    }

    // Extract the package name from a package_declaration node.
    // The name is a scoped_identifier ("com.example.app") or plain identifier.
    std::string getPackageName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(k_scoped_identifier) || child.typeEquals(k_identifier))
                return child.getText();
        }
        return {};
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_java(const ast::AST& tree)
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

        // Skip every node inside a method/constructor body: Java local variables,
        // statements, and local classes are not publicly visible symbols.
        if(insideFunctionScope(scopeStack)) continue;

        // ── Package declaration ───────────────────────────────────────────────
        // Treated as a Namespace. Pushed with an infinite range so every type
        // declaration in the file receives the correct FQN prefix.
        if(node.typeEquals(k_package_declaration)) {
            std::string name = getPackageName(tree, node);
            if(!name.empty()) {
                emit(makeSymbol(name, name, SymbolKind::Namespace, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Namespace, uint32_t(-1),
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Class / Annotation type ───────────────────────────────────────────
        // @interface is an annotation type; Class is the closest SymbolKind.
        if(node.typeEquals(k_class_declaration) ||
           node.typeEquals(k_annotation_type_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, k_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                Access      acc  = getAccess(tree, node);
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Class, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = hasModifier(tree, node, "static");
                emit(std::move(sym));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Interface ─────────────────────────────────────────────────────────
        if(node.typeEquals(k_interface_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, k_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Class, acc,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Enum ──────────────────────────────────────────────────────────────
        if(node.typeEquals(k_enum_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, k_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Enum, acc,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Enum, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Enum constant ─────────────────────────────────────────────────────
        if(node.typeEquals(k_enum_constant)) {
            const ast::ASTNode* id = findChild(tree, node, k_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::EnumValue, Access::Public,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Constructor ───────────────────────────────────────────────────────
        if(node.typeEquals(k_constructor_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, k_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Constructor, acc,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Constructor, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Method ────────────────────────────────────────────────────────────
        // The method name is the first direct `identifier` child. The return type
        // appears before it but has node type "type_identifier" (not "identifier"),
        // so findChild(k_identifier) correctly skips over it to the method name.
        if(node.typeEquals(k_method_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, k_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                Access      acc  = getAccess(tree, node);
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Method, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = hasModifier(tree, node, "static");
                emit(std::move(sym));
                scopeStack.push_back({name, SymbolKind::Method, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Field declaration ─────────────────────────────────────────────────
        // A single field_declaration may declare multiple variables ("int x, y;"),
        // so iterate over all variable_declarator children.
        if(node.typeEquals(k_field_declaration)) {
            Access acc      = getAccess(tree, node);
            bool   isStatic = hasModifier(tree, node, "static");
            bool   isFinal  = hasModifier(tree, node, "final");
            for(uintptr_t cid : node.children_) {
                if(cid == ast::InvalidId) continue;
                const ast::ASTNode& decl = tree[static_cast<uint32_t>(cid)];
                if(!decl.typeEquals(k_variable_declarator)) continue;
                const ast::ASTNode* id = findChild(tree, decl, k_identifier);
                if(!id) continue;
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Field, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic    = isStatic;
                sym.isConstexpr = isStatic && isFinal; // static final ≈ constant
                emit(std::move(sym));
            }
            continue;
        }

        // ── Annotation type element ───────────────────────────────────────────
        // Annotation elements behave like abstract method signatures;
        // Method is the closest SymbolKind.
        if(node.typeEquals(k_annotation_type_element_declaration)) {
            const ast::ASTNode* id = findChild(tree, node, k_identifier);
            if(id) {
                std::string name = id->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Method, Access::Public,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }
    }

    return result;
}

} // namespace extractor
} // namespace ast
