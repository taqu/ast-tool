#ifndef INC_AST_AST_TOOL_H_
#define INC_AST_AST_TOOL_H_
/**
 * @file ast-tool.h
 * @brief CLI front end for ast-tool: argument parsing and subcommand dispatch.
 *
 * This header is the top of the ast-tool layered architecture.  It includes
 * ast-ir.h so that code already including ast-tool.h continues to see the
 * full AST IR without changes.
 *
 * New code that only needs the AST IR (semantic services, extractors, tests)
 * should include ast-ir.h directly and omit this header.
 */
#include <cstdint>
#include "ast-ir.h"

namespace ast
{
    /** Initializes global tree-sitter state (installs the mimalloc-backed allocator). Call once before parsing. */
    void initialize();

/** Identifies which command-line subcommand was requested. */
enum class SubCommand
{
    None,
    Help,
    Dump,
    Symbols,
    Outline,
    Find,
    Range,
    Parent,
    Children,
    Search,
};

/** Arguments for the "help" subcommand. */
struct ArgHelp
{
    const char* topic_; ///< Command name to show help for, or nullptr for top-level help.
};

/** Arguments for the "dump" subcommand. */
struct ArgDump
{
    const char* input_; ///< Path to the input source file to dump.
};

/** Arguments for the "symbols" subcommand. */
struct ArgSymbols
{
    const char* input_; ///< Path to the input source file to extract symbols from.
    bool json_;   ///< Output symbols as JSON when true.
    bool pretty_; ///< Pretty-print the output when true.
};

/** Arguments for the "outline" subcommand. */
struct ArgOutline
{
    const char* input_; ///< Path to the input source file to outline.
};

/** Arguments for the "children" subcommand. */
struct ArgChildren
{
    const char* input_; ///< Path to the input source file to search.
    uint32_t id_;       ///< Node hash to look up (hexadecimal).
    bool hasId_;
};

/** Arguments for the "parent" subcommand. */
struct ArgParent
{
    const char* input_; ///< Path to the input source file to search.
    uint32_t id_;       ///< Node hash to look up (hexadecimal).
    bool hasId_;
};

/** Arguments for the "range" subcommand. */
struct ArgRange
{
    const char* input_;    ///< Path to the input source file to search.
    uint32_t startLine_;   ///< --start-line value (1-based), if #hasStart_.
    uint32_t startColumn_; ///< --start-column value (1-based), if #hasStart_.
    uint32_t endLine_;     ///< --end-line value (1-based), if #hasEnd_.
    uint32_t endColumn_;   ///< --end-column value (1-based), if #hasEnd_.
    bool hasStart_;
    bool hasEnd_;
};

/** Arguments for the "search" subcommand. */
struct ArgSearch
{
    const char* root_;       ///< Workspace root directory to scan.
    const char* name_;       ///< --name filter value, or nullptr if unset.
    const char* fqn_;        ///< --fqn filter value, or nullptr if unset.
    const char* kind_;       ///< --kind filter value (e.g. "function"), or nullptr if unset.
    const char* file_;       ///< --file filter value (substring), or nullptr if unset.
    const char* name_regex_; ///< --name-regex RE2 pattern, or nullptr if unset.
    const char* fqn_regex_;  ///< --fqn-regex RE2 pattern, or nullptr if unset.
    const char* file_regex_; ///< --file-regex RE2 pattern, or nullptr if unset.
    bool json_;              ///< Output results as JSON when true.
    bool pretty_;            ///< Pretty-print the JSON output when true.
};

/** Arguments for the "find" subcommand. */
struct ArgFind
{
    const char* input_;   ///< Path to the input source file to search.
    const char* type_;    ///< --type filter value, or nullptr if unset.
    const char* grammar_; ///< --grammar filter value, or nullptr if unset.
    const char* text_;    ///< --text filter value, or nullptr if unset.

    uint32_t id_; ///< --id filter value, if #hasId_.
    bool hasId_;

    uint32_t line_;   ///< --line filter value (1-based), if #hasLine_.
    uint32_t column_; ///< --column filter value (1-based), if #hasColumn_.
    bool hasLine_;
    bool hasColumn_;
};

/** Parsed command-line arguments, discriminated by #sub_. */
struct Arguments
{
    SubCommand sub_;
    union
    {
        ArgHelp     help_;
        ArgDump     dump_;
        ArgSymbols  symbols_;
        ArgOutline  outline_;
        ArgFind     find_;
        ArgRange    range_;
        ArgParent   parent_;
        ArgChildren children_;
        ArgSearch   search_;
    };
};

/**
 * @brief Parses process command-line arguments into an Arguments structure.
 * @param arguments Output; populated with the parsed subcommand and its options.
 * @param argc Argument count, as passed to main().
 * @param argv Argument vector, as passed to main().
 * @return true if the arguments were recognized and parsed successfully, false otherwise.
 */
bool parse(Arguments& arguments, int32_t argc, const char** argv);

/** Executes the subcommand described by @p arguments (as produced by parse(Arguments&, int32_t, const char**)). Returns true on success. */
bool dispatch(const Arguments& arguments);

} // namespace ast
#endif // INC_AST_AST_TOOL_H_
