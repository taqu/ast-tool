#include "ast-context-export.h"
#include "ast-callees.h"

namespace ast
{

SemanticContext ContextExporter::export_context(const Workspace&       workspace,
                                                 const WorkspaceSymbol& target) const
{
    SemanticContext ctx;
    ctx.symbol = &target;

    ctx.references = FindReferences(workspace).find(target);
    ctx.callers    = Callers(workspace).find(target);
    ctx.callees    = Callees().find(workspace, target);

    return ctx;
}

} // namespace ast
