#include "ast-references.h"
#include "ast-resolver.h"
#include "ast-scope-builder.h"
#include "ast-symbol-scope.h"
#include "ast-ir.h"
#include <cstring>

namespace ast
{

// Returns true if @p type names an identifier-like AST node across supported languages.
static bool is_identifier_type(const char* type)
{
    if(!type) return false;
    // Common identifier node types in C/C++, Python, Rust, Go, Java, JavaScript
    return strcmp(type, "identifier")           == 0
        || strcmp(type, "type_identifier")      == 0
        || strcmp(type, "field_identifier")     == 0
        || strcmp(type, "namespace_identifier") == 0
        || strcmp(type, "property_identifier")  == 0;
}

// Returns true when @p a and @p b refer to the same declaration.
// Comparison uses FQN + kind + source file + declaration line; this uniquely
// identifies a symbol even when multiple files contain declarations with the
// same unqualified name.
static bool same_declaration(const WorkspaceSymbol& a, const WorkspaceSymbol& b)
{
    return a.fqn        == b.fqn
        && a.kind       == b.kind
        && a.sourceFile == b.sourceFile
        && a.line       == b.line;
}

// -----------------------------------------------------------------------
// FindReferences

FindReferences::FindReferences(const Workspace& workspace)
    : workspace_(workspace)
{
}

std::vector<ReferenceResult> FindReferences::find(
    const WorkspaceSymbol& target,
    bool includeDeclaration) const
{
    std::vector<ReferenceResult> results;

    for(const std::string& file : workspace_.files) {
        AST ast = parse(file.c_str());
        if(!ast) continue;

        ScopeTree            tree    = build_scope_tree(ast);
        std::vector<Symbol>  symbols = extract_symbols(ast);
        associate_symbols(tree, symbols);

        IdentifierResolver resolver(tree, symbols, file, workspace_);

        for(size_t i = 0; i < ast.size(); ++i) {
            const ASTNode& node = ast[i];
            if(!is_identifier_type(node.type_)) continue;
            if(node.text_.empty()) continue;

            std::string text    = node.text_.getText();
            uintptr_t   scopeId = tree.getNodeScope(i);

            ResolutionResult resolution = resolver.resolve(text, scopeId);
            if(!resolution.resolved()) continue;

            if(!same_declaration(resolution.symbol(), target)) continue;

            ReferenceResult ref;
            ref.referencedSymbol = target;
            ref.sourceFile       = file;
            ref.line             = node.start_.row_;
            ref.column           = node.start_.column_;
            ref.nodeIndex        = i;
            ref.owningScope      = (scopeId != ScopeTree::InvalidId)
                                   ? tree[scopeId].kind_
                                   : ScopeKind::Unknown;

            // Exclude the declaration site: same file and same 0-based line.
            if(!includeDeclaration
               && ref.sourceFile == target.sourceFile
               && ref.line       == target.line) {
                continue;
            }

            results.push_back(std::move(ref));
        }
    }

    return results;
}

} // namespace ast
