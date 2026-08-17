#ifndef INC_AST_SEARCH_H_
#define INC_AST_SEARCH_H_
/**
 * @file ast-search.h
 * @brief Reusable semantic search engine that filters workspace symbols by query.
 *
 * The engine operates entirely on an already-built Workspace (symbol table).
 * It never re-parses files, rebuilds ASTs, or accesses tree-sitter directly.
 *
 * Typical usage:
 *   auto q = build_search_query(name, fqn, kind_str, file,
 *                                name_regex, fqn_regex, file_regex);
 *   if (!q) { report_error(q.error()); return; }
 *   SemanticSearchEngine engine(workspace);
 *   auto results = engine.search(*q);
 */
#include "ast-extractor.h"
#include "ast-regex.h"
#include "ast-scope.h"
#include "ast-workspace.h"
#include <expected>
#include <optional>
#include <string>
#include <vector>

namespace ast
{

/**
 * @brief An extensible, combinable set of filters for semantic search.
 *
 * Prefer build_search_query() to construct validated queries from raw inputs.
 * Direct field assignment is safe when patterns are already known to be valid.
 *
 * Filter evaluation order inside SemanticSearchEngine::search() (cheapest first):
 *   1. kind        — exact enum comparison
 *   2. name        — exact string equality against unqualified name
 *   3. fqn         — exact string equality against fully qualified name
 *   4. file        — substring containment in source file path
 *   5. name_regex  — RE2 partial match on unqualified name
 *   6. fqn_regex   — RE2 partial match on fully qualified name
 *   7. file_regex  — RE2 partial match on source file path
 *
 * Exact filters always execute before regex filters to reduce the candidate
 * set before the more expensive RE2 evaluation begins.
 *
 * Constraint: at most one matching mode (exact or regex) may be active per
 * field. build_search_query() enforces this invariant; direct assignment bypasses it.
 *
 * An empty query (no fields set) matches every symbol in the workspace.
 */
struct SearchQuery
{
    std::optional<std::u8string> name;      ///< Exact unqualified name match.
    std::optional<std::u8string> fqn;       ///< Exact fully qualified name match.
    std::optional<SymbolKind> kind;         ///< Exact declaration category.
    std::optional<std::u8string> file;      ///< Substring in source file path.
    std::optional<RegexPattern> name_regex; ///< RE2 partial match on unqualified name.
    std::optional<RegexPattern> fqn_regex;  ///< RE2 partial match on fully qualified name.
    std::optional<RegexPattern> file_regex; ///< RE2 partial match on source file path.
};

/**
 * @brief Constructs and validates a SearchQuery from raw string parameters.
 *
 * All parameters are nullable; nullptr means the corresponding filter is not set.
 *
 * Validation performed:
 *   - kind_str, if non-null, must be a recognised SymbolKind name (case-insensitive).
 *   - Each non-null regex string is compiled; an invalid RE2 pattern is rejected.
 *   - A field must not have both an exact value and a regex active simultaneously.
 *
 * Regex patterns are compiled once here and stored in the returned query.
 * The search engine never compiles patterns; it reuses those stored in the query.
 *
 * @return The validated SearchQuery on success, or a human-readable error string
 *         describing why the query is invalid. The caller is responsible for
 *         reporting the error to the user.
 */
std::expected<SearchQuery, std::u8string> build_search_query(
    const char8_t* name,
    const char8_t* fqn,
    const char8_t* kind_str,
    const char8_t* file,
    const char8_t* name_regex,
    const char8_t* fqn_regex,
    const char8_t* file_regex);

/**
 * @brief Evaluates SearchQuery objects against a Workspace symbol table.
 *
 * The engine assumes every query passed to search() is already valid.
 * Use build_search_query() to obtain a validated query before calling search().
 *
 * The engine holds a const reference to the Workspace; the Workspace must
 * remain valid for the engine's lifetime. Searching consists only of filtering
 * already-built symbol data — no parsing or semantic extraction is triggered.
 *
 * This class is the shared backend for semantic services such as identifier
 * resolution, find-references, call-graph analysis, and AI semantic queries.
 */
class SemanticSearchEngine
{
public:
    explicit SemanticSearchEngine(const Workspace& workspace);

    /**
     * @brief Evaluates @p query against all symbols in the workspace.
     *
     * Filters are applied in the order documented on SearchQuery: exact
     * (kind → name → fqn → file) before regex (name_regex → fqn_regex → file_regex).
     *
     * @return Pointers to all matching WorkspaceSymbol objects owned by the
     *         Workspace, in stable order.  The returned pointers are valid as
     *         long as the Workspace is not modified.  No symbol data is copied.
     */
    std::vector<const WorkspaceSymbol*> search(const SearchQuery& query) const;

private:
    const Workspace& workspace_;
};

/**
 * @brief Evaluates SearchQuery objects against a Workspace symbol table.
 *
 * The engine assumes every query passed to search() is already valid.
 * Use build_search_query() to obtain a validated query before calling search().
 *
 * The engine holds a const reference to the Workspace; the Workspace must
 * remain valid for the engine's lifetime. Searching consists only of filtering
 * already-built symbol data — no parsing or semantic extraction is triggered.
 *
 * This class is the shared backend for semantic services such as identifier
 * resolution, find-references, call-graph analysis, and AI semantic queries.
 */
class SemanticSearchEngineOneShot
{
public:
    explicit SemanticSearchEngineOneShot(const SearchQuery& query);

    /**
     * @brief Evaluates @p query against all symbols in the workspace.
     *
     * Filters are applied in the order documented on SearchQuery: exact
     * (kind → name → fqn → file) before regex (name_regex → fqn_regex → file_regex).
     *
     * @return Pointers to all matching WorkspaceSymbol objects owned by the
     *         Workspace, in stable order.  The returned pointers are valid as
     *         long as the Workspace is not modified.  No symbol data is copied.
     */
    bool match(const WorkspaceSymbol& symbol) const;

private:
    const SearchQuery& query_;
};

} // namespace ast
#endif // INC_AST_SEARCH_H_
