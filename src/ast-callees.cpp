#include "ast-callees.h"
#include "ast-call-utils.h"
#include "ast-resolver.h"
#include "ast-ir.h"
#include <vector>
#include <filesystem>

using namespace ast::call_utils;

namespace ast
{

// Iterative pre-order DFS over the subtree rooted at @p rootIndex.
// Invokes @p visit(nodeIndex, ctx) for every call-expression node encountered.
// Preserves AST-node order (leftmost children processed first).
static void visit_call_expressions(const AST& ast, size_t rootIndex,
                                    void (*visit)(size_t, void*), void* ctx)
{
    std::vector<size_t> stack;
    stack.reserve(64);
    stack.push_back(rootIndex);

    while(!stack.empty()) {
        size_t curr = stack.back();
        stack.pop_back();

        const ASTNode& node = ast[curr];
        if(is_call_expression(node.type_)) visit(curr, ctx);

        // Push children in reverse order so the leftmost child is processed first.
        for(int i = static_cast<int>(node.children_.size()) - 1; i >= 0; --i) {
            uintptr_t childId = node.children_[static_cast<size_t>(i)];
            if(childId != InvalidId)
                stack.push_back(static_cast<size_t>(childId));
        }
    }
}

// -----------------------------------------------------------------------
// Callees

std::vector<CallSite> Callees::find(const Workspace&       workspace,
                                     const WorkspaceSymbol& caller) const
{
    std::vector<CallSite> results;

    // Locate (or lazily load) the TranslationUnit that owns the caller's source file.
    const TranslationUnit* tu = workspace.get_translation_unit(caller.sourceFile);
    if(!tu) return results;

    // Validate the AST node index stored in the caller's symbol.
    size_t funcNodeIdx = caller.symbol.nodeIndex;
    if(funcNodeIdx >= tu->ast.size()) return results;

    // Find a stable pointer to the caller in workspace.symbols.
    const WorkspaceSymbol* callerPtr = find_in_workspace(workspace, caller);

    IdentifierResolver resolver(tu->scopeTree, tu->symbols, tu->path, workspace);

    struct Ctx {
        const AST*              ast;
        const ScopeTree*        scopeTree;
        IdentifierResolver*     resolver;
        const Workspace*        workspace;
        const WorkspaceSymbol*  callerPtr;
        const std::filesystem::path*      sourcePath; // tu->path
        std::vector<CallSite>*  results;
    };

    Ctx ctx{ &tu->ast, &tu->scopeTree, &resolver,
             &workspace, callerPtr, &tu->path, &results };

    visit_call_expressions(tu->ast, funcNodeIdx,
        [](size_t callIdx, void* raw) {
            auto& c = *static_cast<Ctx*>(raw);

            size_t calleeIdentIdx = find_callee_identifier(*c.ast, callIdx);
            if(calleeIdentIdx == size_t(-1)) return;

            const ASTNode& calleeNode = (*c.ast)[calleeIdentIdx];
            if(calleeNode.text_.empty()) return;

            std::u8string text    = calleeNode.text_.getText();
            uintptr_t   scopeId = c.scopeTree->getNodeScope(callIdx);

            ResolutionResult res = c.resolver->resolve(text, scopeId);
            if(!res.resolved()) return;

            const WorkspaceSymbol* calleePtr = find_in_workspace(*c.workspace, res.symbol());
            if(!calleePtr) return;

            CallSite site;
            site.caller     = c.callerPtr;
            site.callee     = calleePtr;
            site.sourceFile = *c.sourcePath;
            site.line       = calleeNode.start_.row_;
            site.column     = calleeNode.start_.column_;
            site.nodeIndex  = callIdx;

            c.results->push_back(std::move(site));
        },
        &ctx);

    return results;
}

} // namespace ast
