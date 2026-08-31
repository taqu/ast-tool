#include "ast-references.h"
#include "ast-ir.h"
#include "ast-resolver.h"
#include "ast-call-utils.h"

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

            std::u8string text = node.text_.getText();
            uintptr_t scopeId = tu.scopeTree.getNodeScope(i);

            ResolutionResult resolution = resolver.resolve(text, scopeId);
            if(!resolution.resolved())
                continue;

            if(!same_semantic_symbol(resolution.symbol(), target))
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
            if(!includeDeclaration) {
                bool declarationSite = false;
                for(const WorkspaceSymbol& occurrence: workspace_.symbols) {
                    if(same_semantic_symbol(occurrence, target)
                       && ref.sourceFile == occurrence.sourceFile
                       && ref.line + 1 == occurrence.symbol.line) {
                        declarationSite = true;
                        break;
                    }
                }
                if(declarationSite) continue;
            }

            results.push_back(std::move(ref));
        }
    }

    return results;
}

} // namespace ast
