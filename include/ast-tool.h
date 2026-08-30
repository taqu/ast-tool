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
#include "ast-ir.h"
#include <array>
#include <charconv>
#include <concepts>
#include <cstdint>
#include <span>
#include <string>
#include <system_error>

namespace ast
{
constexpr size_t BUFFER_SIZE = 128;
template<std::integral T>
std::u8string to_string(T value, int32_t base = 10)
{
    std::array<char8_t, BUFFER_SIZE> buffer{};
    char* buffer_ptr = reinterpret_cast<char*>(buffer.data());

    auto [ptr, ec] = std::to_chars(buffer_ptr, buffer_ptr + buffer.size(), value, base);

    if(ec == std::errc{}) {
        const char8_t* end_ptr = reinterpret_cast<const char8_t*>(ptr);
        return std::u8string(buffer.data(), end_ptr);
    }
    return std::u8string();
}

template<std::floating_point T>
std::u8string to_string(T value, std::chars_format fmt = std::chars_format::general)
{
    std::array<char8_t, BUFFER_SIZE> buffer{};
    char* buffer_ptr = reinterpret_cast<char*>(buffer.data());

    auto [ptr, ec] = std::to_chars(buffer_ptr, buffer_ptr + buffer.size(), value, fmt);

    if(ec == std::errc{}) {
        const char8_t* end_ptr = reinterpret_cast<const char8_t*>(ptr);
        return std::u8string(buffer.data(), end_ptr);
    }
    return std::u8string();
}

template<std::integral T>
const char8_t* to_string_intermediate(char8_t buffer[BUFFER_SIZE], T value, int32_t base = 10)
{
    char* buffer_ptr = reinterpret_cast<char*>(buffer);

    auto [ptr, ec] = std::to_chars(buffer_ptr, buffer_ptr + BUFFER_SIZE, value, base);

    if(ec == std::errc{}) {
        return buffer;
    }
    return u8"";
}

template<std::floating_point T>
const char8_t* to_string_intermediate(char8_t buffer[BUFFER_SIZE], T value, std::chars_format fmt = std::chars_format::general)
{
    char* buffer_ptr = reinterpret_cast<char*>(buffer);

    auto [ptr, ec] = std::to_chars(buffer_ptr, buffer_ptr + BUFFER_SIZE, value, fmt);

    if(ec == std::errc{}) {
        return buffer;
    }
    return u8"";
}

uint32_t get_physical_core_count();

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
    References,
    Callers,
    Callees,
    CacheWarm,
    CacheStatus,
    Setup,
};

/** Arguments for the "help" subcommand. */
struct ArgHelp
{
    const char8_t* topic_; ///< Command name to show help for, or nullptr for top-level help.
};

/** Arguments for the "dump" subcommand. */
struct ArgDump
{
    const char8_t* input_; ///< Path to the input source file to dump.
};

/** Arguments for the "symbols" subcommand. */
struct ArgSymbols
{
    const char8_t* input_; ///< Path to the input source file to extract symbols from.
    bool json_;            ///< Output symbols as JSON when true.
    bool pretty_;          ///< Pretty-print the output when true.
};

/** Arguments for the "outline" subcommand. */
struct ArgOutline
{
    const char8_t* input_; ///< Path to the input source file to outline.
};

/** Arguments for the "children" subcommand. */
struct ArgChildren
{
    const char8_t* input_; ///< Path to the input source file to search.
    uint32_t id_;          ///< Node hash to look up (hexadecimal).
    bool hasId_;
};

/** Arguments for the "parent" subcommand. */
struct ArgParent
{
    const char8_t* input_; ///< Path to the input source file to search.
    uint32_t id_;          ///< Node hash to look up (hexadecimal).
    bool hasId_;
};

/** Arguments for the "range" subcommand. */
struct ArgRange
{
    const char8_t* input_; ///< Path to the input source file to search.
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
    const char8_t* root_;       ///< Workspace root directory to scan.
    const char8_t* name_;       ///< --name filter value, or nullptr if unset.
    const char8_t* fqn_;        ///< --fqn filter value, or nullptr if unset.
    const char8_t* kind_;       ///< --kind filter value (e.g. "function"), or nullptr if unset.
    const char8_t* file_;       ///< --file filter value (substring), or nullptr if unset.
    const char8_t* name_regex_; ///< --name-regex RE2 pattern, or nullptr if unset.
    const char8_t* fqn_regex_;  ///< --fqn-regex RE2 pattern, or nullptr if unset.
    const char8_t* file_regex_; ///< --file-regex RE2 pattern, or nullptr if unset.
    bool json_;                 ///< Output results as JSON when true.
    bool pretty_;               ///< Pretty-print the JSON output when true.
};

/** Arguments for the "callees" subcommand. */
struct ArgCallees
{
    const char8_t* root_;   ///< Workspace root directory to scan.
    const char8_t* symbol_; ///< Symbol name or FQN to find callees of.
    bool json_;             ///< Output results as JSON when true.
    bool pretty_;           ///< Pretty-print the JSON output when true.
};

/** Arguments for the "callers" subcommand. */
struct ArgCallers
{
    const char8_t* root_;   ///< Workspace root directory to scan.
    const char8_t* symbol_; ///< Symbol name or FQN to find callers of.
    bool json_;             ///< Output results as JSON when true.
    bool pretty_;           ///< Pretty-print the JSON output when true.
};

/** Arguments for the "references" subcommand. */
struct ArgReferences
{
    const char8_t* root_;   ///< Workspace root directory to scan.
    const char8_t* symbol_; ///< Symbol name or FQN to find references for.
    bool json_;             ///< Output results as JSON when true.
    bool pretty_;           ///< Pretty-print the JSON output when true.
};

/** Arguments for the "find" subcommand. */
struct ArgFind
{
    const char8_t* input_;   ///< Path to the input source file to search.
    const char8_t* type_;    ///< --type filter value, or nullptr if unset.
    const char8_t* grammar_; ///< --grammar filter value, or nullptr if unset.
    const char8_t* text_;    ///< --text filter value, or nullptr if unset.

    uint32_t id_; ///< --id filter value, if #hasId_.
    bool hasId_;

    uint32_t line_;   ///< --line filter value (1-based), if #hasLine_.
    uint32_t column_; ///< --column filter value (1-based), if #hasColumn_.
    bool hasLine_;
    bool hasColumn_;
};

/** Arguments for the "cache" subcommand (warm / status). */
struct ArgCache
{
    const char8_t* root_;       ///< Workspace root directory.
    bool           warm_;       ///< Run cache warming.
    bool           status_;     ///< Show cache status.
    bool           verbose_;    ///< Verbose output.
    bool           background_; ///< Spawn warming in the background and return immediately.
};

/** Arguments for the "setup" subcommand. */
struct ArgSetup
{
    bool claude_;              ///< Configure Claude Code.
    bool codex_;               ///< Configure Codex.
    bool dry_run_;             ///< Show what would be done without writing.
    bool remove_;              ///< Remove instead of install.
    bool global_;              ///< Install globally.
    bool local_;               ///< Install locally for user.
    const char8_t* claude_config_; ///< Override Claude config path (nullptr = default).
    const char8_t* codex_config_;  ///< Override Codex config path (nullptr = default).
};

/** Parsed command-line arguments, discriminated by #sub_. */
struct Arguments
{
    SubCommand sub_;
    union
    {
        ArgHelp help_;
        ArgDump dump_;
        ArgSymbols symbols_;
        ArgOutline outline_;
        ArgFind find_;
        ArgRange range_;
        ArgParent parent_;
        ArgChildren children_;
        ArgSearch search_;
        ArgReferences references_;
        ArgCallers callers_;
        ArgCallees callees_;
        ArgCache cache_;
        ArgSetup setup_;
    };
};

/**
 * @brief Parses process command-line arguments into an Arguments structure.
 * @param arguments Output; populated with the parsed subcommand and its options.
 * @param argv UTF-8 argument vector produced by the platform entry point.
 * @return true if the arguments were recognized and parsed successfully, false otherwise.
 */
bool parse(Arguments& arguments, std::span<const std::u8string> argv);
std::vector<std::u8string> args_to_utf8(int32_t argc, const char** argv);

/** Executes the subcommand described by @p arguments (as produced by parse(Arguments&, int32_t, const char8_t**)). Returns true on success. */
bool dispatch(const Arguments& arguments);

/**
 * @brief Convert string to uint32_t as decimal
 * @param begin ... null terminated string
 * @param[out] value ...
 * @return
 */
bool from_chars_10(const char8_t* begin, std::uint32_t& value);
/**
 * @brief Convert string to uint32_t as hexadecimal
 * @param begin ... null terminated string
 * @param value ...
 * @return
 */
bool from_chars_16(const char8_t* begin, std::uint32_t& value);
} // namespace ast
#endif // INC_AST_AST_TOOL_H_
