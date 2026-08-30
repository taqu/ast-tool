#ifndef INC_AST_CLI_SEMANTIC_H_
#define INC_AST_CLI_SEMANTIC_H_
/**
 * @file cli-semantic.h
 * @brief Shared CLI infrastructure for semantic symbol-targeting commands.
 *
 * Provides workspace lifecycle, symbol resolution, callable validation, argument
 * parsing, and JSON array output helpers shared by the references, callers, and
 * callees commands.
 */
#include "ast-extractor.h"
#include "ast-workspace.h"
#include <cstdint>
#include <functional>
#include <print>
#include <string>
#include <string_view>
#include <vector>
#include <filesystem>

namespace ast
{
namespace cli
{

/**
 * @brief Parse the common --json/--pretty/<symbol> <root> arguments.
 *
 * Used by references, callers, and callees.  Writes directly into the
 * caller-supplied output variables; does not touch SubCommand.
 *
 * @return true when both symbol and root were supplied.
 */
bool parse_symbol_root_args(
    const char8_t*& symbol,
    const char8_t*& root,
    bool& json,
    bool& pretty,
    int32_t argc,
    const char8_t** argv);

/**
 * @brief Return workspace symbols matching @p query.
 *
 * Matches against FQN when query contains "::"; otherwise against the
 * unqualified name.
 */
std::vector<const WorkspaceSymbol*> resolve_symbol_query(
    const Workspace& ws,
    const char8_t* query);

/**
 * @brief True when @p kind is a callable (Function, Method, Constructor, or Destructor).
 */
bool is_callable(SymbolKind kind);

/**
 * @brief Construct workspace, resolve symbol, and invoke @p op(workspace, target).
 *
 * Prints a diagnostic and returns false on workspace failure, symbol not found,
 * or ambiguous symbol.  On success the Workspace remains alive for the entire
 * duration of @p op.
 */
bool with_resolved_symbol(
    const char8_t* root,
    const char8_t* symbol,
    std::function<bool(Workspace&, const WorkspaceSymbol&)> op);

/**
 * @brief Return a JSON-safe copy of @p s with backslashes and double-quotes escaped.
 *
 * Used for file paths (which contain backslashes on Windows) and any other
 * string values emitted inside JSON string literals.  Only the two characters
 * that appear in practice for file paths and symbol names are escaped here;
 * extend the table if other control characters are ever encountered.
 */
inline std::string json_escape(std::string_view s)
{
    std::string out;
    out.reserve(s.size());
    for(char c : s) {
        if(c == '\\') { out += "\\\\"; }
        else if(c == '"')  { out += "\\\""; }
        else               { out += c; }
    }
    return out;
}

inline std::string json_escape(std::u8string_view s)
{
    return json_escape(std::string_view{
        reinterpret_cast<const char*>(s.data()), s.size()
    });
}

/**
 * @brief Print a JSON array of @p items, one element per call to the supplied printers.
 *
 * @param items          Result collection to print.
 * @param pretty         When true, use @p print_pretty; otherwise use @p print_compact.
 * @param print_compact  Printer for compact (single-line) JSON.
 * @param print_pretty   Printer for indented (multi-line) JSON.
 */
template<typename T>
void print_json_array(
    const std::vector<T>& items,
    bool pretty,
    void(*print_compact)(const T&, bool),
    void(*print_pretty)(const T&, bool))
{
    std::print("[");
    if(pretty) {
        std::print("\n");
    }
    for(size_t i = 0; i < items.size(); ++i) {
        bool last = (i == items.size() - 1);
        if(pretty) {
            print_pretty(items[i], last);
        } else {
            print_compact(items[i], last);
        }
    }
    std::print("]\n");
}

} // namespace cli
} // namespace ast
#endif // INC_AST_CLI_SEMANTIC_H_
