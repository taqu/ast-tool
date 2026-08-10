#ifndef INC_AST_CONTEXT_EXPORT_H_
#define INC_AST_CONTEXT_EXPORT_H_
/**
 * @file ast-context-export.h
 * @brief High-level façade that aggregates semantic information around a symbol.
 *
 * ContextExporter is an orchestration service.  It delegates to lower-level
 * semantic services — FindReferences, Callers, Callees — and packages their
 * results into a single SemanticContext object.
 *
 * It never parses source files, never accesses tree-sitter, and never
 * duplicates the analysis implemented by the services it calls.
 *
 * Typical usage:
 *
 *   Workspace ws = analyze_workspace("src/");
 *   const WorkspaceSymbol* sym = ...; // from ws.symbols
 *   ContextExporter exporter;
 *   SemanticContext ctx = exporter.export_context(ws, *sym);
 *
 * All pointers inside SemanticContext remain valid for the lifetime of the
 * Workspace that produced them.
 *
 * The structure is designed for extension: additional semantic fields
 * (inheritance, documentation, diagnostics, …) can be added to SemanticContext
 * without changing the ContextExporter API.
 */
#include <vector>
#include "ast-workspace.h"
#include "ast-references.h"
#include "ast-callers.h"

namespace ast
{

/**
 * @brief The semantic context around a symbol.
 *
 * Aggregates the results of multiple semantic services for a single target
 * symbol.  All contained data is produced by existing services; ContextExporter
 * does not generate any new semantic information.
 *
 * Fields are populated on a best-effort basis: if a service produces no
 * results the corresponding vector is simply empty.
 *
 * Future fields that may be added without breaking existing consumers:
 *   - inheritance relationships
 *   - virtual overrides / implementations
 *   - semantic diff entries
 *   - diagnostics
 *   - documentation strings
 *   - type-hierarchy links
 */
struct SemanticContext
{
    const WorkspaceSymbol*      symbol     = nullptr; ///< The target symbol; points into the workspace.
    std::vector<ReferenceResult> references;           ///< All non-declaration references to the symbol.
    std::vector<CallSite>        callers;              ///< All direct callers of the symbol (if it is a function).
    std::vector<CallSite>        callees;              ///< All direct callees within the symbol's body (if it is a function).
};

/**
 * @brief Aggregates semantic information around a target symbol.
 *
 * The service is stateless.  Each call to export_context() is independent.
 * Compose multiple calls if several symbols need to be exported.
 */
class ContextExporter
{
public:
    /**
     * @brief Exports the semantic context for @p target.
     *
     * Gathers information by calling FindReferences, Callers, and Callees
     * with the supplied workspace.  No source files are re-parsed.
     *
     * If any service returns no results for @p target (e.g. a variable has
     * no callers), the corresponding field is left as an empty vector.  The
     * returned context is always at least partially populated with the target
     * symbol pointer.
     *
     * @param workspace  The workspace used by all delegated services.
     * @param target     The symbol whose context is to be exported.
     * @return           A SemanticContext containing all available information.
     */
    SemanticContext export_context(const Workspace&       workspace,
                                   const WorkspaceSymbol& target) const;
};

} // namespace ast
#endif // INC_AST_CONTEXT_EXPORT_H_
