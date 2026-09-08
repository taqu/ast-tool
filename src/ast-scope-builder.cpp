#include "ast-scope-builder.h"
#include "ast-ir.h"

namespace ast
{
namespace
{
    /**
     * Maps a tree-sitter node type enum to a ScopeKind.
     * Returns ScopeKind::Unknown when the node type does not introduce a new scope.
     */
    ScopeKind classify_node(ASTNodeType type)
    {
        // Global / translation-unit roots
        if(type == ASTNodeType::TranslationUnit  // C, C++
        || type == ASTNodeType::SourceFile        // Rust, Go, TypeScript, JavaScript
        || type == ASTNodeType::Program           // Java, Ruby, bash
        || type == ASTNodeType::CompilationUnit   // C#
        || type == ASTNodeType::Module) {         // Python
            return ScopeKind::Global;
        }

        // Namespaces / packages / modules
        if(type == ASTNodeType::NamespaceDefinition   // C++
        || type == ASTNodeType::NamespaceDeclaration  // C#
        || type == ASTNodeType::PackageDeclaration    // Java, Go
        || type == ASTNodeType::PackageClause) {      // Go
            return ScopeKind::Namespace;
        }

        // Module items (Rust mod, etc.)
        if(type == ASTNodeType::ModItem           // Rust
        || type == ASTNodeType::ModuleDeclaration) { // C++20
            return ScopeKind::Module;
        }

        // Classes
        if(type == ASTNodeType::ClassSpecifier    // C++
        || type == ASTNodeType::ClassDeclaration  // C#, Java, TypeScript, JavaScript
        || type == ASTNodeType::ClassDefinition   // Python
        || type == ASTNodeType::TraitItem         // Rust
        || type == ASTNodeType::ImplItem) {       // Rust
            return ScopeKind::Class;
        }

        // Structs
        if(type == ASTNodeType::StructSpecifier   // C, C++
        || type == ASTNodeType::UnionSpecifier    // C, C++ (union shares scope semantics)
        || type == ASTNodeType::StructItem        // Rust
        || type == ASTNodeType::StructDeclaration // Go, C#
        || type == ASTNodeType::StructType) {     // Go
            return ScopeKind::Struct;
        }

        // Enums
        if(type == ASTNodeType::EnumSpecifier     // C, C++
        || type == ASTNodeType::EnumDeclaration   // C#, Java, TypeScript
        || type == ASTNodeType::EnumItem) {       // Rust
            return ScopeKind::Enum;
        }

        // Methods (member functions)
        if(type == ASTNodeType::MethodDeclaration  // C#, Java
        || type == ASTNodeType::MethodDefinition   // Python (in class), JavaScript
        || type == ASTNodeType::ConstructorDeclaration // C#, Java
        || type == ASTNodeType::DestructorDeclaration) { // C#
            return ScopeKind::Method;
        }

        // Free functions
        if(type == ASTNodeType::FunctionDefinition  // C, C++, Python
        || type == ASTNodeType::FunctionDeclaration // TypeScript, C#
        || type == ASTNodeType::FunctionItem        // Rust
        || type == ASTNodeType::FunctionDeclarationStatement // Go
        || type == ASTNodeType::FunctionExpression  // JavaScript
        || type == ASTNodeType::FuncDeclaration) {  // Go
            return ScopeKind::Function;
        }

        // Lambdas / closures / anonymous functions
        if(type == ASTNodeType::LambdaExpression   // C++, Java
        || type == ASTNodeType::ClosureExpression  // Rust
        || type == ASTNodeType::ArrowFunction      // JavaScript, TypeScript
        || type == ASTNodeType::FuncLiteral) {     // Go
            return ScopeKind::Lambda;
        }

        // Bare blocks / compound statements
        if(type == ASTNodeType::CompoundStatement  // C, C++
        || type == ASTNodeType::Block              // Rust, Go, JavaScript
        || type == ASTNodeType::StatementBlock) {  // JavaScript
            return ScopeKind::Block;
        }

        return ScopeKind::Unknown; // does not introduce a scope
    }

    void dfs(const AST& ast, size_t nodeIndex, uintptr_t currentScope, ScopeTree& tree)
    {
        const ASTNode& node = ast[nodeIndex];
        uintptr_t myScope = currentScope;

        ScopeKind kind = classify_node(node.type_);
        if(kind != ScopeKind::Unknown) {
            myScope = tree.add(kind, nodeIndex, node.startByte_, node.endByte_, currentScope);
        }

        tree.setNodeScope(nodeIndex, myScope);

        for(uintptr_t childIdx : node.children_) {
            if(childIdx != InvalidId) {
                dfs(ast, childIdx, myScope, tree);
            }
        }
    }
} // namespace

ScopeTree build_scope_tree(const AST& ast)
{
    ScopeTree tree;
    if(ast.size() == 0) {
        return tree;
    }

    tree.reserveNodeMap(ast.size());
    // Node 0 is the root (translation_unit / source_file / etc.)
    dfs(ast, 0, ScopeTree::InvalidId, tree);
    return tree;
}

} // namespace ast
