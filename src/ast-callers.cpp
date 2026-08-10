#include "ast-callers.h"
#include "ast-call-utils.h"
#include "ast-resolver.h"
#include "ast-ir.h"

using namespace ast::call_utils;

namespace ast
{

Callers::Callers(const Workspace& workspace)
    : workspace_(workspace)
{
}

std::vector<CallSite> Callers::find(const WorkspaceSymbol& target) const
{
    std::vector<CallSite> results;

    for(const TranslationUnit& tu : workspace_.translationUnits) {
        IdentifierResolver resolver(tu.scopeTree, tu.symbols, tu.path, workspace_);

        for(size_t i = 0; i < tu.ast.size(); ++i) {
            const ASTNode& node = tu.ast[i];
            if(!is_call_expression(node.type_)) continue;

            size_t calleeIdentIdx = find_callee_identifier(tu.ast, i);
            if(calleeIdentIdx == size_t(-1)) continue;

            const ASTNode& calleeNode = tu.ast[calleeIdentIdx];
            if(calleeNode.text_.empty()) continue;

            std::string text    = calleeNode.text_.getText();
            uintptr_t   scopeId = tu.scopeTree.getNodeScope(i);

            ResolutionResult resolution = resolver.resolve(text, scopeId);
            if(!resolution.resolved()) continue;

            if(!same_declaration(resolution.symbol(), target)) continue;

            CallSite site;
            site.caller     = find_enclosing_caller(tu, workspace_, scopeId);
            site.callee     = &target;
            site.sourceFile = tu.path;
            site.line       = calleeNode.start_.row_;
            site.column     = calleeNode.start_.column_;
            site.nodeIndex  = i;

            results.push_back(std::move(site));
        }
    }

    return results;
}

} // namespace ast
