#include "ast-references.h"
#include "ast-ir.h"
#include "ast-resolver.h"
#include "ast-member-resolution.h"

namespace ast
{

// Returns true if @p type names an identifier-like AST node across supported languages.
static bool is_identifier_type(ASTNodeType type)
{
    return type == ASTNodeType::Identifier
           || type == ASTNodeType::TypeIdentifier
           || type == ASTNodeType::FieldIdentifier
           || type == ASTNodeType::NamespaceIdentifier
           || type == ASTNodeType::PropertyIdentifier;
}

// Returns true when @p a and @p b refer to the same declaration.
static bool same_declaration(const WorkspaceSymbol& a, const WorkspaceSymbol& b)
{
    return a.symbol.fqn == b.symbol.fqn
           && a.symbol.kind == b.symbol.kind
           && a.sourceFile == b.sourceFile
           && a.symbol.line == b.symbol.line;
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

    for(const TranslationUnit& tu: workspace_.translationUnits) {
        IdentifierResolver resolver(tu.scopeTree, tu.symbols, tu.path, workspace_);

        for(size_t i = 0; i < tu.ast.size(); ++i) {
            const ASTNode& node = tu.ast[i];
            if(!is_identifier_type(node.type_))
                continue;
            if(node.text_.empty())
                continue;

            uintptr_t scopeId = tu.scopeTree.getNodeScope(i);

            ResolutionResult resolution = resolve_relationship_identifier(
                tu, workspace_, resolver, i, scopeId);
            if(!resolution.resolved())
                continue;

            if(!same_declaration(resolution.symbol(), target))
                continue;

            ReferenceResult ref;
            ref.referencedSymbol = target;
            ref.sourceFile = tu.path;
            ref.line = node.start_.row_;
            ref.column = node.start_.column_;
            ref.nodeIndex = i;
            ref.owningScope = (scopeId != ScopeTree::InvalidId)
                                  ? tu.scopeTree[scopeId].kind_
                                  : ScopeKind::Unknown;

            // ref.line is 0-based (ASTNode.start_.row_); target.symbol.line is 1-based.
            if(!includeDeclaration
               && ref.sourceFile == target.sourceFile
               && ref.line + 1 == target.symbol.line) {
                continue;
            }

            results.push_back(std::move(ref));
        }
    }

    return results;
}

} // namespace ast
