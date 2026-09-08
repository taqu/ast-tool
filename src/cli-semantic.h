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
 * @brief True when @p arg looks like an option token (e.g. "--foo"), not a positional.
 *
 * Shared by every command's argument parser so that an unrecognized flag is
 * never silently swallowed as a positional argument (symbol, root, file, ...).
 */
bool looks_like_option(const char8_t* arg);

/**
 * @brief Parse the common --json/--pretty/<symbol> <root> arguments.
 *
 * Used by references, callers, and callees.  Writes directly into the
 * caller-supplied output variables; does not touch SubCommand.
 *
 * Any `--flag`-shaped token that is not `--json`/`--pretty` is not consumed
 * as the symbol or root positional; the first one encountered is written to
 * @p badOption (nullptr if none was seen), letting the exec function report
 * an unknown-option error instead of silently misinterpreting the flag as a
 * positional argument.
 *
 * @return true when both symbol and root were supplied.
 */
bool parse_symbol_root_args(
    const char8_t*& symbol,
    const char8_t*& root,
    bool& json,
    bool& pretty,
    const char8_t*& badOption,
    int32_t argc,
    const char8_t** argv);

/**
 * @brief Return workspace symbols matching @p query.
 *
 * When query contains "::", exact FQN matches take precedence. If no exact
 * match exists, returns FQNs ending at a qualified-name boundary with the
 * query; callers preserve ambiguity unless this leaves one canonical symbol.
 * Queries without "::" retain unqualified-name matching.
 */
std::vector<const WorkspaceSymbol*> resolve_symbol_query(
    const Workspace& ws,
    const char8_t* query);

/**
 * @brief True when @p kind is a callable (Function, Method, Constructor, or Destructor).
 */
bool is_callable(SymbolKind kind);

/**
 * @brief True when @p ws contains at least one parsed or attempted file.
 *
 * Otherwise prints an actionable "empty workspace" diagnostic (root likely
 * wrong or has no supported source files) and returns false.  Shared by
 * every workspace-scanning command (search, callers, references, callees)
 * so the diagnostic stays consistent.
 */
bool workspace_has_content(const Workspace& ws, const char8_t* root, bool json, bool pretty);

/**
 * @brief Construct workspace, resolve symbol, and invoke @p op(workspace, target).
 *
 * Prints a structured, recoverable diagnostic (see print_error()) and returns
 * false on workspace failure, symbol not found, or ambiguous symbol.  On
 * success the Workspace remains alive for the entire duration of @p op.
 *
 * @param json    Render diagnostics as JSON instead of plain text.
 * @param pretty  Pretty-print JSON diagnostics. Ignored when @p json is false.
 */
bool with_resolved_symbol(
    const char8_t* root,
    const char8_t* symbol,
    bool json,
    bool pretty,
    std::function<bool(Workspace&, const WorkspaceSymbol&)> op);

// ---------------------------------------------------------------------------
// Error Recovery UX (Phase 5)
//
// Centralized error classification and rendering shared by callers,
// references, callees, search, and find.  Every recoverable failure is
// reduced to a RecoveryError describing what failed, what is already known,
// and (when one exists) the single cheapest reasonable next action.  This
// keeps recovery-message logic out of each individual command.

/** Upper bound on candidates listed for an ambiguous-symbol or large-result error. */
constexpr size_t kMaxErrorCandidates = 5;

/** Broad classification of a recoverable command failure. */
enum class ErrorCategory
{
    SymbolNotFound,
    AmbiguousSymbol,
    UnknownOption,
    InvalidArguments,
    InvalidQuery,
    UnsupportedQueryForm,
    InternalError,
};

/** One bounded candidate entry for an ambiguous-symbol error. */
struct ErrorCandidate
{
    std::u8string kind;
    std::u8string fqn;
    std::u8string file;
    uint32_t line = 0; ///< 1-based.
};

/**
 * @brief A structured, recoverable command failure.
 *
 * Built by the make_*() helpers below and rendered by print_error() /
 * render_error_text() / render_error_json().  Fields left at their default
 * are simply omitted from the rendered output.
 */
struct RecoveryError
{
    ErrorCategory category = ErrorCategory::InternalError;
    std::u8string query;                     ///< Subject of the failure (symbol, option, ...), if any.
    std::u8string message;                   ///< Short human-readable detail.
    std::vector<ErrorCandidate> candidates;   ///< Bounded candidate list (ambiguous symbol only).
    size_t totalCandidates = 0;               ///< Full candidate count before bounding.
    std::u8string next;                       ///< Recommended next action, plain-text and human-readable.
    std::u8string usage;                      ///< Minimal valid invocation shape (invalid arguments only).
    std::vector<std::u8string> validOptions;  ///< Small relevant option set (unknown option only).
};

/** Machine-readable snake_case token for @p category (the JSON "error" field). */
const char* error_category_token(ErrorCategory category);

/** Human-readable label for @p category (plain-text header, e.g. "symbol not found"). */
const char* error_category_label(ErrorCategory category);

/** @p query was not found anywhere in the workspace. Suggests a search fallback. */
RecoveryError make_symbol_not_found(std::u8string_view query, std::u8string_view root);

/**
 * @brief @p query matched more than one symbol.
 *
 * Candidates are bounded to kMaxErrorCandidates. The recommendation differs
 * for an already-qualified query (inspect candidates) versus an unqualified
 * one (retry qualified).
 */
RecoveryError make_ambiguous_symbol(std::u8string_view query, const std::vector<const WorkspaceSymbol*>& candidates);

/** An unrecognized `--option` was supplied. @p validOptions should be small and relevant. */
RecoveryError make_unknown_option(std::u8string_view option, std::vector<std::u8string> validOptions);

/** Required positional arguments are missing or malformed. */
RecoveryError make_invalid_arguments(std::u8string_view message, std::u8string_view usage);

/** A query option combination is invalid (e.g. bad --kind, invalid regex, conflicting filters). */
RecoveryError make_invalid_query(std::u8string_view message);

/** The resolved target cannot serve this command's query form (e.g. callers on a non-callable). */
RecoveryError make_unsupported_query_form(std::u8string_view message, std::u8string_view next);

/** A genuine internal failure, distinct from user/input mistakes. */
RecoveryError make_internal_error(std::u8string_view message);

/** Renders @p err as compact, agent-facing plain text (includes a trailing newline). */
std::string render_error_text(const RecoveryError& err);

/** Renders @p err as a single compact or pretty-printed JSON object (includes a trailing newline). */
std::string render_error_json(const RecoveryError& err, bool pretty);

/** Prints @p err to stderr, selecting the JSON or plain-text renderer per @p json. */
void print_error(const RecoveryError& err, bool json, bool pretty);

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
