#include "ast-scope-builder.h"
#include <cstring>
#include "ast-ir.h"

namespace ast
{
namespace
{
    /**
     * Maps a tree-sitter node type name to a ScopeKind.
     * Returns ScopeKind::Unknown when the node type does not introduce a new scope.
     */
    ScopeKind classify_node(const char* type)
    {
        // Global / translation-unit roots
        if(0 == ::strcmp(type, "translation_unit")  // C, C++
        || 0 == ::strcmp(type, "source_file")        // Rust, Go, TypeScript, JavaScript
        || 0 == ::strcmp(type, "program")            // Java, Ruby, bash
        || 0 == ::strcmp(type, "compilation_unit")   // C#
        || 0 == ::strcmp(type, "module")) {          // Python
            return ScopeKind::Global;
        }

        // Namespaces / packages / modules
        if(0 == ::strcmp(type, "namespace_definition")   // C++
        || 0 == ::strcmp(type, "namespace_declaration")  // C#
        || 0 == ::strcmp(type, "package_declaration")    // Java, Go
        || 0 == ::strcmp(type, "package_clause")) {      // Go
            return ScopeKind::Namespace;
        }

        // Module items (Rust mod, etc.)
        if(0 == ::strcmp(type, "mod_item")           // Rust
        || 0 == ::strcmp(type, "module_declaration")) { // C++20
            return ScopeKind::Module;
        }

        // Classes
        if(0 == ::strcmp(type, "class_specifier")    // C++
        || 0 == ::strcmp(type, "class_declaration")  // C#, Java, TypeScript, JavaScript
        || 0 == ::strcmp(type, "class_definition")   // Python
        || 0 == ::strcmp(type, "trait_item")         // Rust
        || 0 == ::strcmp(type, "impl_item")) {       // Rust
            return ScopeKind::Class;
        }

        // Structs
        if(0 == ::strcmp(type, "struct_specifier")   // C, C++
        || 0 == ::strcmp(type, "union_specifier")    // C, C++ (union shares scope semantics)
        || 0 == ::strcmp(type, "struct_item")        // Rust
        || 0 == ::strcmp(type, "struct_declaration") // Go, C#
        || 0 == ::strcmp(type, "struct_type")) {     // Go
            return ScopeKind::Struct;
        }

        // Enums
        if(0 == ::strcmp(type, "enum_specifier")     // C, C++
        || 0 == ::strcmp(type, "enum_declaration")   // C#, Java, TypeScript
        || 0 == ::strcmp(type, "enum_item")) {       // Rust
            return ScopeKind::Enum;
        }

        // Methods (member functions)
        if(0 == ::strcmp(type, "method_declaration")  // C#, Java
        || 0 == ::strcmp(type, "method_definition")   // Python (in class), JavaScript
        || 0 == ::strcmp(type, "constructor_declaration") // C#, Java
        || 0 == ::strcmp(type, "destructor_declaration")) { // C#
            return ScopeKind::Method;
        }

        // Free functions
        if(0 == ::strcmp(type, "function_definition")  // C, C++, Python
        || 0 == ::strcmp(type, "function_declaration") // TypeScript, C#
        || 0 == ::strcmp(type, "function_item")        // Rust
        || 0 == ::strcmp(type, "function_declaration_statement") // Go
        || 0 == ::strcmp(type, "function_expression")  // JavaScript
        || 0 == ::strcmp(type, "func_declaration")) {  // Go
            return ScopeKind::Function;
        }

        // Lambdas / closures / anonymous functions
        if(0 == ::strcmp(type, "lambda_expression")   // C++, Java
        || 0 == ::strcmp(type, "closure_expression")  // Rust
        || 0 == ::strcmp(type, "arrow_function")      // JavaScript, TypeScript
        || 0 == ::strcmp(type, "func_literal")) {     // Go
            return ScopeKind::Lambda;
        }

        // Bare blocks / compound statements
        if(0 == ::strcmp(type, "compound_statement")  // C, C++
        || 0 == ::strcmp(type, "block")               // Rust, Go, JavaScript
        || 0 == ::strcmp(type, "statement_block")) {  // JavaScript
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
