#include "help.h"
#include <cstdio>
#include <cstring>
#include <string>

namespace ast
{
namespace
{

// ---------------------------------------------------------------------------
// Command metadata

enum class CommandCategory
{
    ASTInspection,
    SemanticAnalysis,
};

struct CommandEntry
{
    const char8_t*     name;
    const char8_t*     summary;
    CommandCategory category;
    const char8_t*     detail;
};

// ---------------------------------------------------------------------------
// Per-command help texts

static const char8_t kHelpDump[] =
    u8"NAME\n"
    u8"    dump - Print all AST nodes of a source file.\n"
    u8"\n"
    u8"SYNOPSIS\n"
    u8"    ast-tool dump <file>\n"
    u8"\n"
    u8"DESCRIPTION\n"
    u8"    Parse <file> and print every AST node in depth-first order.\n"
    u8"    Output includes all node types, including anonymous punctuation\n"
    u8"    and keyword tokens.\n"
    u8"\n"
    u8"ARGUMENTS\n"
    u8"    <file>    Path to the source file to parse.\n"
    u8"\n"
    u8"OUTPUT\n"
    u8"    One line per AST node:\n"
    u8"\n"
    u8"        <type> <ID>\n"
    u8"\n"
    u8"    <type>    Node type string (e.g. function_definition, identifier).\n"
    u8"    <ID>      32-bit node hash in uppercase hexadecimal (e.g. 9E52E360).\n"
    u8"              Use this value with the find, parent, and children commands.\n"
    u8"\n"
    u8"EXAMPLES\n"
    u8"    Inspect all nodes in a source file:\n"
    u8"\n"
    u8"        ast-tool dump src/main.cpp\n"
    u8"\n"
    u8"    Inspect a header file:\n"
    u8"\n"
    u8"        ast-tool dump include/parser.hpp\n"
    u8"\n"
    u8"    Collect node IDs for further inspection:\n"
    u8"\n"
    u8"        ast-tool dump src/lexer.cpp\n"
    u8"        ast-tool children --id <ID> src/lexer.cpp\n";

static const char8_t kHelpSymbols[] =
    u8"NAME\n"
    u8"    symbols - Extract semantic symbols from a source file.\n"
    u8"\n"
    u8"SYNOPSIS\n"
    u8"    ast-tool symbols [--json [--pretty]] <file>\n"
    u8"\n"
    u8"DESCRIPTION\n"
    u8"    Parse <file> and extract every named semantic symbol, including\n"
    u8"    functions, methods, classes, structs, enums, variables, and\n"
    u8"    namespaces.\n"
    u8"\n"
    u8"    Each symbol is annotated with its kind, fully-qualified name,\n"
    u8"    access modifier, storage qualifiers, and a stable node ID.\n"
    u8"\n"
    u8"ARGUMENTS\n"
    u8"    <file>    Path to the source file to analyze.\n"
    u8"\n"
    u8"OPTIONS\n"
    u8"    --json      Output results as a JSON object.\n"
    u8"    --pretty    Pretty-print the JSON output. Implies --json.\n"
    u8"\n"
    u8"OUTPUT\n"
    u8"    Default (plain text), one symbol per line:\n"
    u8"\n"
    u8"        <fqn> <ID>\n"
    u8"\n"
    u8"    <fqn>    Fully-qualified name of the symbol.\n"
    u8"    <ID>     32-bit node hash in uppercase hexadecimal.\n"
    u8"\n"
    u8"    JSON fields per symbol:\n"
    u8"\n"
    u8"        kind            Symbol kind (function, class, variable, ...).\n"
    u8"        name            Unqualified symbol name.\n"
    u8"        qualified_name  Fully-qualified symbol name.\n"
    u8"        access          Access specifier (public, protected, private, unknown).\n"
    u8"        static          true if the symbol is declared static.\n"
    u8"        constexpr       true if the symbol is declared constexpr.\n"
    u8"        inline          true if the symbol is declared inline.\n"
    u8"        id              32-bit node hash in uppercase hexadecimal.\n"
    u8"\n"
    u8"EXAMPLES\n"
    u8"    List all symbols in a source file:\n"
    u8"\n"
    u8"        ast-tool symbols src/main.cpp\n"
    u8"\n"
    u8"    List symbols from a header file:\n"
    u8"\n"
    u8"        ast-tool symbols include/parser.hpp\n"
    u8"\n"
    u8"    Export symbols as JSON for programmatic consumption:\n"
    u8"\n"
    u8"        ast-tool symbols --json src/lexer.cpp\n"
    u8"\n"
    u8"    Export symbols as readable JSON for inspection:\n"
    u8"\n"
    u8"        ast-tool symbols --json --pretty src/main.cpp\n";

static const char8_t kHelpOutline[] =
    u8"NAME\n"
    u8"    outline - Print a hierarchical outline of named AST nodes.\n"
    u8"\n"
    u8"SYNOPSIS\n"
    u8"    ast-tool outline <file>\n"
    u8"\n"
    u8"DESCRIPTION\n"
    u8"    Parse <file> and print all named AST nodes in a depth-indented\n"
    u8"    tree.  Anonymous punctuation and keyword tokens are omitted.\n"
    u8"\n"
    u8"ARGUMENTS\n"
    u8"    <file>    Path to the source file to outline.\n"
    u8"\n"
    u8"OUTPUT\n"
    u8"    One line per named AST node, indented by tree depth:\n"
    u8"\n"
    u8"        <indent><type>[ \"<text>\"] @<line>:<col>\n"
    u8"\n"
    u8"    <indent>  Two spaces per depth level.\n"
    u8"    <type>    Node type string.\n"
    u8"    <text>    Source text of the node, present only on leaf nodes.\n"
    u8"    <line>    1-based source line.\n"
    u8"    <col>     1-based source column.\n"
    u8"\n"
    u8"EXAMPLES\n"
    u8"    Inspect the structure of a source file:\n"
    u8"\n"
    u8"        ast-tool outline src/main.cpp\n"
    u8"\n"
    u8"    Inspect the public interface of a header file:\n"
    u8"\n"
    u8"        ast-tool outline include/parser.hpp\n"
    u8"\n"
    u8"    Inspect a deeply nested implementation file:\n"
    u8"\n"
    u8"        ast-tool outline project/src/codegen.cpp\n";

static const char8_t kHelpFind[] =
    u8"NAME\n"
    u8"    find - Find AST nodes by type, text, position, or ID.\n"
    u8"\n"
    u8"SYNOPSIS\n"
    u8"    ast-tool find [--type <type>] [--grammar <grammar>] [--text <text>]\n"
    u8"                  [--id <hex>] [--line <n>] [--column <n>] <file>\n"
    u8"\n"
    u8"DESCRIPTION\n"
    u8"    Parse <file> and print every AST node that matches all supplied\n"
    u8"    filters simultaneously.  When no filter is given, all nodes are\n"
    u8"    printed.\n"
    u8"\n"
    u8"    --line and --column must be supplied together to filter by position.\n"
    u8"\n"
    u8"ARGUMENTS\n"
    u8"    <file>    Path to the source file to search.\n"
    u8"\n"
    u8"OPTIONS\n"
    u8"    --type <type>       Match nodes whose type equals <type>\n"
    u8"                        (e.g. function_definition, identifier).\n"
    u8"    --grammar <grammar> Match nodes whose grammar type equals <grammar>.\n"
    u8"    --text <text>       Match nodes whose source text equals <text>.\n"
    u8"    --id <hex>          Match the node with the given hash. Accepts\n"
    u8"                        uppercase or lowercase hex, with or without\n"
    u8"                        a leading 0x prefix (e.g. 9E52E360, 0x9e52e360).\n"
    u8"    --line <n>          Match nodes that start on line <n> (1-based).\n"
    u8"    --column <n>        Match nodes that start at column <n> (1-based).\n"
    u8"\n"
    u8"OUTPUT\n"
    u8"    One line per matching AST node:\n"
    u8"\n"
    u8"        <ID> <type> @<line>:<col>[ \"<text>\"]\n"
    u8"\n"
    u8"    <ID>      32-bit node hash in uppercase hexadecimal.\n"
    u8"    <type>    Node type string.\n"
    u8"    <line>    1-based source line.\n"
    u8"    <col>     1-based source column.\n"
    u8"    <text>    Source text of the node, present when non-empty.\n"
    u8"\n"
    u8"EXAMPLES\n"
    u8"    Find all function definitions in a file:\n"
    u8"\n"
    u8"        ast-tool find --type function_definition src/parser.cpp\n"
    u8"\n"
    u8"    Find the node at a specific cursor position:\n"
    u8"\n"
    u8"        ast-tool find --line 42 --column 17 src/parser.cpp\n"
    u8"\n"
    u8"    Find all uses of a specific identifier:\n"
    u8"\n"
    u8"        ast-tool find --type identifier --text initialize src/main.cpp\n"
    u8"\n"
    u8"    Look up a specific node by ID:\n"
    u8"\n"
    u8"        ast-tool find --id <node-id> src/main.cpp\n";

static const char8_t kHelpRange[] =
    u8"NAME\n"
    u8"    range - Find AST nodes that intersect a source range.\n"
    u8"\n"
    u8"SYNOPSIS\n"
    u8"    ast-tool range [--start-line <n>] [--start-column <n>]\n"
    u8"                   [--end-line <n>]   [--end-column <n>]   <file>\n"
    u8"\n"
    u8"DESCRIPTION\n"
    u8"    Parse <file> and print every AST node whose source span overlaps\n"
    u8"    the specified line and column range.  All options are optional;\n"
    u8"    omitted bounds default to 0.\n"
    u8"\n"
    u8"ARGUMENTS\n"
    u8"    <file>    Path to the source file to search.\n"
    u8"\n"
    u8"OPTIONS\n"
    u8"    --start-line <n>    First line of the source range (1-based).\n"
    u8"    --start-column <n>  First column of the source range (1-based).\n"
    u8"    --end-line <n>      Last line of the source range (1-based).\n"
    u8"    --end-column <n>    Last column of the source range (1-based).\n"
    u8"\n"
    u8"OUTPUT\n"
    u8"    One line per matching AST node:\n"
    u8"\n"
    u8"        <ID> <type> @<line>:<col>[ \"<text>\"]\n"
    u8"\n"
    u8"    <ID>      32-bit node hash in uppercase hexadecimal.\n"
    u8"    <type>    Node type string.\n"
    u8"    <line>    1-based source line.\n"
    u8"    <col>     1-based source column.\n"
    u8"    <text>    Source text of the node, present when non-empty.\n"
    u8"\n"
    u8"EXAMPLES\n"
    u8"    Find all nodes on a single line:\n"
    u8"\n"
    u8"        ast-tool range \\\n"
    u8"            --start-line 42 --start-column 1 \\\n"
    u8"            --end-line   42 --end-column   80 \\\n"
    u8"            src/parser.cpp\n"
    u8"\n"
    u8"    Find all nodes in a block of code:\n"
    u8"\n"
    u8"        ast-tool range \\\n"
    u8"            --start-line 100 --start-column 1 \\\n"
    u8"            --end-line   150 --end-column   1 \\\n"
    u8"            src/codegen.cpp\n"
    u8"\n"
    u8"    Find the node at an exact cursor position:\n"
    u8"\n"
    u8"        ast-tool range \\\n"
    u8"            --start-line 17 --start-column 5 \\\n"
    u8"            --end-line   17 --end-column   5 \\\n"
    u8"            include/parser.hpp\n";

static const char8_t kHelpParent[] =
    u8"NAME\n"
    u8"    parent - Print the parent of an AST node.\n"
    u8"\n"
    u8"SYNOPSIS\n"
    u8"    ast-tool parent --id <hex> <file>\n"
    u8"\n"
    u8"DESCRIPTION\n"
    u8"    Parse <file> and print the parent AST node of the node identified\n"
    u8"    by --id.  Use the dump, find, or outline commands to obtain node IDs.\n"
    u8"\n"
    u8"ARGUMENTS\n"
    u8"    <file>    Path to the source file.\n"
    u8"\n"
    u8"OPTIONS\n"
    u8"    --id <hex>    ID of the target node. Required. Accepts uppercase or\n"
    u8"                  lowercase hex, with or without a leading 0x prefix\n"
    u8"                  (e.g. 9E52E360, 0x9e52e360).\n"
    u8"\n"
    u8"OUTPUT\n"
    u8"    One line describing the parent AST node:\n"
    u8"\n"
    u8"        <ID> <type> @<line>:<col>[ \"<text>\"]\n"
    u8"\n"
    u8"    <ID>      32-bit node hash in uppercase hexadecimal.\n"
    u8"    <type>    Node type string.\n"
    u8"    <line>    1-based source line.\n"
    u8"    <col>     1-based source column.\n"
    u8"    <text>    Source text of the node, present when non-empty.\n"
    u8"\n"
    u8"EXAMPLES\n"
    u8"    Find the node at a cursor position, then walk up to its parent:\n"
    u8"\n"
    u8"        ast-tool find --line 42 --column 17 src/parser.cpp\n"
    u8"        ast-tool parent --id <node-id> src/parser.cpp\n"
    u8"\n"
    u8"    Walk up two levels to reach an enclosing declaration:\n"
    u8"\n"
    u8"        ast-tool parent --id <node-id>   src/main.cpp\n"
    u8"        ast-tool parent --id <parent-id> src/main.cpp\n";

static const char8_t kHelpChildren[] =
    u8"NAME\n"
    u8"    children - Print the children of an AST node.\n"
    u8"\n"
    u8"SYNOPSIS\n"
    u8"    ast-tool children --id <hex> <file>\n"
    u8"\n"
    u8"DESCRIPTION\n"
    u8"    Parse <file> and print all immediate child AST nodes of the node\n"
    u8"    identified by --id.  Use the dump, find, or outline commands to\n"
    u8"    obtain node IDs.\n"
    u8"\n"
    u8"ARGUMENTS\n"
    u8"    <file>    Path to the source file.\n"
    u8"\n"
    u8"OPTIONS\n"
    u8"    --id <hex>    ID of the target node. Required. Accepts uppercase or\n"
    u8"                  lowercase hex, with or without a leading 0x prefix\n"
    u8"                  (e.g. 9E52E360, 0x9e52e360).\n"
    u8"\n"
    u8"OUTPUT\n"
    u8"    One line per child AST node:\n"
    u8"\n"
    u8"        <ID> <type> @<line>:<col>[ \"<text>\"]\n"
    u8"\n"
    u8"    <ID>      32-bit node hash in uppercase hexadecimal.\n"
    u8"    <type>    Node type string.\n"
    u8"    <line>    1-based source line.\n"
    u8"    <col>     1-based source column.\n"
    u8"    <text>    Source text of the node, present when non-empty.\n"
    u8"\n"
    u8"EXAMPLES\n"
    u8"    Find a function definition, then list its child nodes:\n"
    u8"\n"
    u8"        ast-tool find --type function_definition src/parser.cpp\n"
    u8"        ast-tool children --id <node-id> src/parser.cpp\n"
    u8"\n"
    u8"    Inspect the children of the root node:\n"
    u8"\n"
    u8"        ast-tool dump src/main.cpp\n"
    u8"        ast-tool children --id <root-id> src/main.cpp\n";

static const char8_t kHelpSearch[] =
    u8"NAME\n"
    u8"    search - Search for semantic symbols across a workspace.\n"
    u8"\n"
    u8"SYNOPSIS\n"
    u8"    ast-tool search [--name <name>] [--fqn <fqn>] [--kind <kind>]\n"
    u8"                    [--file <path>] [--name-regex <re>] [--fqn-regex <re>]\n"
    u8"                    [--file-regex <re>] [--json [--pretty]] <root>\n"
    u8"\n"
    u8"DESCRIPTION\n"
    u8"    Scan every source file under <root>, extract semantic symbols, and\n"
    u8"    print those that match all supplied filters.  When no filter is\n"
    u8"    given, all symbols in the workspace are printed.\n"
    u8"\n"
    u8"    Exact filters and regex filters for the same field are mutually\n"
    u8"    exclusive (e.g. --name and --name-regex cannot be combined).\n"
    u8"\n"
    u8"ARGUMENTS\n"
    u8"    <root>    Workspace root directory to scan recursively.\n"
    u8"\n"
    u8"OPTIONS\n"
    u8"  Exact filters (substring match):\n"
    u8"    --name <name>       Match symbols whose unqualified name contains <name>.\n"
    u8"    --fqn <fqn>         Match symbols whose fully-qualified name contains\n"
    u8"                        <fqn>.\n"
    u8"    --kind <kind>       Match symbols of the given kind (function, method,\n"
    u8"                        class, struct, enum, variable, ...).\n"
    u8"    --file <path>       Match symbols in files whose path contains <path>.\n"
    u8"\n"
    u8"  Regex filters (RE2 syntax):\n"
    u8"    --name-regex <re>   Match symbols whose unqualified name matches <re>.\n"
    u8"    --fqn-regex  <re>   Match symbols whose fully-qualified name matches\n"
    u8"                        <re>.\n"
    u8"    --file-regex <re>   Match symbols in files whose path matches <re>.\n"
    u8"\n"
    u8"  Output format:\n"
    u8"    --json              Output results as a JSON array.\n"
    u8"    --pretty            Pretty-print the JSON output. Implies --json.\n"
    u8"\n"
    u8"OUTPUT\n"
    u8"    Default (plain text), one symbol per line:\n"
    u8"\n"
    u8"        <kind> <fqn> <file>:<line>:<col>\n"
    u8"\n"
    u8"    JSON fields per symbol:\n"
    u8"\n"
    u8"        kind            Symbol kind (function, class, variable, ...).\n"
    u8"        name            Unqualified symbol name.\n"
    u8"        fqn             Fully-qualified symbol name.\n"
    u8"        file            Path of the file that declares the symbol.\n"
    u8"        line            1-based declaration line.\n"
    u8"        column          1-based declaration column.\n"
    u8"        owning_scope    Lexical scope kind that contains the symbol.\n"
    u8"\n"
    u8"EXAMPLES\n"
    u8"    List all symbols in a workspace:\n"
    u8"\n"
    u8"        ast-tool search src/\n"
    u8"\n"
    u8"    Find all functions in a workspace:\n"
    u8"\n"
    u8"        ast-tool search --kind function src/\n"
    u8"\n"
    u8"    Find all symbols whose name contains 'parse':\n"
    u8"\n"
    u8"        ast-tool search --name parse src/\n"
    u8"\n"
    u8"    Find all classes declared in header files:\n"
    u8"\n"
    u8"        ast-tool search --kind class --file-regex '\\.hpp$' src/\n"
    u8"\n"
    u8"    Find all symbols in a specific namespace:\n"
    u8"\n"
    u8"        ast-tool search --fqn-regex '^ast::' src/\n"
    u8"\n"
    u8"    Export results as JSON for programmatic consumption:\n"
    u8"\n"
    u8"        ast-tool search --kind function --json --pretty src/\n";

// ---------------------------------------------------------------------------
// Command metadata tables

static const CommandEntry kCommands[] = {
    // AST Inspection
    {u8"dump",     u8"Print all AST nodes of a source file.",           CommandCategory::ASTInspection,   kHelpDump},
    {u8"outline",  u8"Print a hierarchical outline of named AST nodes.", CommandCategory::ASTInspection,  kHelpOutline},
    {u8"find",     u8"Find AST nodes by type, text, position, or ID.",  CommandCategory::ASTInspection,   kHelpFind},
    {u8"range",    u8"Find AST nodes that intersect a source range.",   CommandCategory::ASTInspection,   kHelpRange},
    {u8"parent",   u8"Print the parent of an AST node.",                CommandCategory::ASTInspection,   kHelpParent},
    {u8"children", u8"Print the children of an AST node.",              CommandCategory::ASTInspection,   kHelpChildren},
    // Semantic Analysis
    {u8"symbols",  u8"Extract semantic symbols from a source file.",    CommandCategory::SemanticAnalysis, kHelpSymbols},
    {u8"search",   u8"Search for semantic symbols across a workspace.", CommandCategory::SemanticAnalysis, kHelpSearch},
};

static constexpr int kCommandCount = sizeof(kCommands) / sizeof(kCommands[0]);

struct CategoryInfo
{
    CommandCategory id;
    const char*     label;
};

static const CategoryInfo kCategories[] = {
    {CommandCategory::ASTInspection,   "AST Inspection"},
    {CommandCategory::SemanticAnalysis, "Semantic Analysis"},
};

static constexpr int kCategoryCount = sizeof(kCategories) / sizeof(kCategories[0]);

} // namespace

// ---------------------------------------------------------------------------

void print_top_level_help()
{
    fputs(
        "AST analysis toolkit\n"
        "\n"
        "Usage:\n"
        "    ast-tool [options] <command> [command options]\n"
        "\n"
        "Options:\n"
        "    -h, --help    Show this help message.\n"
        "\n"
        "Commands:\n",
        stdout);

    for(int ci = 0; ci < kCategoryCount; ++ci) {
        fprintf(stdout, "\n  %s\n", kCategories[ci].label);
        for(int i = 0; i < kCommandCount; ++i) {
            if(kCommands[i].category != kCategories[ci].id) continue;
            fprintf(stdout, "    %-12s%s\n", kCommands[i].name, kCommands[i].summary);
        }
    }

    fputs(
        "\n"
        "Run:\n"
        "\n"
        "    ast-tool help <command>\n"
        "\n"
        "for detailed documentation of a command.\n",
        stdout);
}

void print_command_help(const char8_t* topic)
{
    if(!topic) {
        print_top_level_help();
        return;
    }
    std::u8string_view topic_view{topic};
    for(int i = 0; i < kCommandCount; ++i) {
        if(topic_view == kCommands[i].name){
            fputs((const char*)kCommands[i].detail, stdout);
            return;
        }
    }
    fprintf(stderr, "error: unknown help topic '%s'\n", topic);
    fprintf(stderr, "Run 'ast-tool help' to see available commands.\n");
}

} // namespace ast
