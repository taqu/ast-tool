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
    // ── Low-level helpers ─────────────────────────────────────────────────────

    // Extract the access level declared in the node's modifiers child.
    // Returns Unknown for package-private (no explicit access keyword).
    Access getAccess(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* mods = findChild(tree, node, ASTNodeType::Modifiers);
        if(!mods) return Access::Unknown;
        if(childHasText(tree, *mods, u8"public"))    return Access::Public;
        if(childHasText(tree, *mods, u8"private"))   return Access::Private;
        if(childHasText(tree, *mods, u8"protected")) return Access::Protected;
        return Access::Unknown;
    }

    // Returns true if the node's modifiers child contains the given keyword.
    bool hasModifier(const ast::AST& tree, const ast::ASTNode& node, const char8_t* kw)
    {
        const ast::ASTNode* mods = findChild(tree, node, ASTNodeType::Modifiers);
        return mods && childHasText(tree, *mods, kw);
    }

    // Extract the package name from a package_declaration node.
    // The name is a scoped_identifier ("com.example.app") or plain identifier.
    std::u8string getPackageName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(ASTNodeType::ScopedIdentifier) || child.typeEquals(ASTNodeType::Identifier))
                return child.getText();
        }
        return {};
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_java(const ast::AST& tree)
{
    std::vector<Symbol>             result;
    std::unordered_set<std::u8string> seen;   // key = fqn + ":" + kind ordinal
    std::vector<ScopeFrame>         scopeStack;

    auto emit = [&](Symbol sym) {
        char8_t buffer[BUFFER_SIZE];
        std::u8string key = sym.fqn + u8":" + ast::to_string_intermediate(buffer, static_cast<int>(sym.kind));
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
        if(node.typeEquals(ASTNodeType::PackageDeclaration)) {
            std::u8string name = getPackageName(tree, node);
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
        if(node.typeEquals(ASTNodeType::ClassDeclaration) ||
           node.typeEquals(ASTNodeType::AnnotationTypeDeclaration)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                Access      acc  = getAccess(tree, node);
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Class, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = hasModifier(tree, node, u8"static");
                emit(std::move(sym));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Interface ─────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::InterfaceDeclaration)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
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

        // ── Enum ──────────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::EnumDeclaration)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                Access      acc  = getAccess(tree, node);
                emit(makeSymbol(name, fqn, SymbolKind::Enum, acc,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Enum, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Enum constant ─────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::EnumConstant)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::EnumValue, Access::Public,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Constructor ───────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::ConstructorDeclaration)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
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
        // so findChild(Identifier) correctly skips over it to the method name.
        if(node.typeEquals(ASTNodeType::MethodDeclaration)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                Access      acc  = getAccess(tree, node);
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Method, acc,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = hasModifier(tree, node, u8"static");
                emit(std::move(sym));
                scopeStack.push_back({name, SymbolKind::Method, node.endByte_,
                                      acc, false});
            }
            continue;
        }

        // ── Field declaration ─────────────────────────────────────────────────
        // A single field_declaration may declare multiple variables ("int x, y;"),
        // so iterate over all variable_declarator children.
        if(node.typeEquals(ASTNodeType::FieldDeclaration)) {
            Access acc      = getAccess(tree, node);
            bool   isStatic = hasModifier(tree, node, u8"static");
            bool   isFinal  = hasModifier(tree, node, u8"final");
            for(uintptr_t cid : node.children_) {
                if(cid == ast::InvalidId) continue;
                const ast::ASTNode& decl = tree[static_cast<uint32_t>(cid)];
                if(!decl.typeEquals(ASTNodeType::VariableDeclarator)) continue;
                const ast::ASTNode* id = findChild(tree, decl, ASTNodeType::Identifier);
                if(!id) continue;
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
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
        if(node.typeEquals(ASTNodeType::AnnotationTypeElementDeclaration)) {
            const ast::ASTNode* id = findChild(tree, node, ASTNodeType::Identifier);
            if(id) {
                std::u8string name = id->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
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
