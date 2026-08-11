#include "help.h"
#include <cstdio>
#include <cstring>

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
    const char*     name;
    const char*     summary;
    CommandCategory category;
    const char*     detail;
};

// ---------------------------------------------------------------------------
// Per-command help texts

static const char kHelpDump[] =
    "NAME\n"
    "    dump - Print all AST nodes of a source file.\n"
    "\n"
    "SYNOPSIS\n"
    "    ast-tool dump <file>\n"
    "\n"
    "DESCRIPTION\n"
    "    Parse <file> and print every AST node in depth-first order.\n"
    "    Output includes all node types, including anonymous punctuation\n"
    "    and keyword tokens.\n"
    "\n"
    "ARGUMENTS\n"
    "    <file>    Path to the source file to parse.\n"
    "\n"
    "OUTPUT\n"
    "    One line per AST node:\n"
    "\n"
    "        <type> <ID>\n"
    "\n"
    "    <type>    Node type string (e.g. function_definition, identifier).\n"
    "    <ID>      32-bit node hash in uppercase hexadecimal (e.g. 9E52E360).\n"
    "              Use this value with the find, parent, and children commands.\n"
    "\n"
    "EXAMPLES\n"
    "    Inspect all nodes in a source file:\n"
    "\n"
    "        ast-tool dump src/main.cpp\n"
    "\n"
    "    Inspect a header file:\n"
    "\n"
    "        ast-tool dump include/parser.hpp\n"
    "\n"
    "    Collect node IDs for further inspection:\n"
    "\n"
    "        ast-tool dump src/lexer.cpp\n"
    "        ast-tool children --id <ID> src/lexer.cpp\n";

static const char kHelpSymbols[] =
    "NAME\n"
    "    symbols - Extract semantic symbols from a source file.\n"
    "\n"
    "SYNOPSIS\n"
    "    ast-tool symbols [--json [--pretty]] <file>\n"
    "\n"
    "DESCRIPTION\n"
    "    Parse <file> and extract every named semantic symbol, including\n"
    "    functions, methods, classes, structs, enums, variables, and\n"
    "    namespaces.\n"
    "\n"
    "    Each symbol is annotated with its kind, fully-qualified name,\n"
    "    access modifier, storage qualifiers, and a stable node ID.\n"
    "\n"
    "ARGUMENTS\n"
    "    <file>    Path to the source file to analyze.\n"
    "\n"
    "OPTIONS\n"
    "    --json      Output results as a JSON object.\n"
    "    --pretty    Pretty-print the JSON output. Implies --json.\n"
    "\n"
    "OUTPUT\n"
    "    Default (plain text), one symbol per line:\n"
    "\n"
    "        <fqn> <ID>\n"
    "\n"
    "    <fqn>    Fully-qualified name of the symbol.\n"
    "    <ID>     32-bit node hash in uppercase hexadecimal.\n"
    "\n"
    "    JSON fields per symbol:\n"
    "\n"
    "        kind            Symbol kind (function, class, variable, ...).\n"
    "        name            Unqualified symbol name.\n"
    "        qualified_name  Fully-qualified symbol name.\n"
    "        access          Access specifier (public, protected, private, unknown).\n"
    "        static          true if the symbol is declared static.\n"
    "        constexpr       true if the symbol is declared constexpr.\n"
    "        inline          true if the symbol is declared inline.\n"
    "        id              32-bit node hash in uppercase hexadecimal.\n"
    "\n"
    "EXAMPLES\n"
    "    List all symbols in a source file:\n"
    "\n"
    "        ast-tool symbols src/main.cpp\n"
    "\n"
    "    List symbols from a header file:\n"
    "\n"
    "        ast-tool symbols include/parser.hpp\n"
    "\n"
    "    Export symbols as JSON for programmatic consumption:\n"
    "\n"
    "        ast-tool symbols --json src/lexer.cpp\n"
    "\n"
    "    Export symbols as readable JSON for inspection:\n"
    "\n"
    "        ast-tool symbols --json --pretty src/main.cpp\n";

static const char kHelpOutline[] =
    "NAME\n"
    "    outline - Print a hierarchical outline of named AST nodes.\n"
    "\n"
    "SYNOPSIS\n"
    "    ast-tool outline <file>\n"
    "\n"
    "DESCRIPTION\n"
    "    Parse <file> and print all named AST nodes in a depth-indented\n"
    "    tree.  Anonymous punctuation and keyword tokens are omitted.\n"
    "\n"
    "ARGUMENTS\n"
    "    <file>    Path to the source file to outline.\n"
    "\n"
    "OUTPUT\n"
    "    One line per named AST node, indented by tree depth:\n"
    "\n"
    "        <indent><type>[ \"<text>\"] @<line>:<col>\n"
    "\n"
    "    <indent>  Two spaces per depth level.\n"
    "    <type>    Node type string.\n"
    "    <text>    Source text of the node, present only on leaf nodes.\n"
    "    <line>    1-based source line.\n"
    "    <col>     1-based source column.\n"
    "\n"
    "EXAMPLES\n"
    "    Inspect the structure of a source file:\n"
    "\n"
    "        ast-tool outline src/main.cpp\n"
    "\n"
    "    Inspect the public interface of a header file:\n"
    "\n"
    "        ast-tool outline include/parser.hpp\n"
    "\n"
    "    Inspect a deeply nested implementation file:\n"
    "\n"
    "        ast-tool outline project/src/codegen.cpp\n";

static const char kHelpFind[] =
    "NAME\n"
    "    find - Find AST nodes by type, text, position, or ID.\n"
    "\n"
    "SYNOPSIS\n"
    "    ast-tool find [--type <type>] [--grammar <grammar>] [--text <text>]\n"
    "                  [--id <hex>] [--line <n>] [--column <n>] <file>\n"
    "\n"
    "DESCRIPTION\n"
    "    Parse <file> and print every AST node that matches all supplied\n"
    "    filters simultaneously.  When no filter is given, all nodes are\n"
    "    printed.\n"
    "\n"
    "    --line and --column must be supplied together to filter by position.\n"
    "\n"
    "ARGUMENTS\n"
    "    <file>    Path to the source file to search.\n"
    "\n"
    "OPTIONS\n"
    "    --type <type>       Match nodes whose type equals <type>\n"
    "                        (e.g. function_definition, identifier).\n"
    "    --grammar <grammar> Match nodes whose grammar type equals <grammar>.\n"
    "    --text <text>       Match nodes whose source text equals <text>.\n"
    "    --id <hex>          Match the node with the given hash. Accepts\n"
    "                        uppercase or lowercase hex, with or without\n"
    "                        a leading 0x prefix (e.g. 9E52E360, 0x9e52e360).\n"
    "    --line <n>          Match nodes that start on line <n> (1-based).\n"
    "    --column <n>        Match nodes that start at column <n> (1-based).\n"
    "\n"
    "OUTPUT\n"
    "    One line per matching AST node:\n"
    "\n"
    "        <ID> <type> @<line>:<col>[ \"<text>\"]\n"
    "\n"
    "    <ID>      32-bit node hash in uppercase hexadecimal.\n"
    "    <type>    Node type string.\n"
    "    <line>    1-based source line.\n"
    "    <col>     1-based source column.\n"
    "    <text>    Source text of the node, present when non-empty.\n"
    "\n"
    "EXAMPLES\n"
    "    Find all function definitions in a file:\n"
    "\n"
    "        ast-tool find --type function_definition src/parser.cpp\n"
    "\n"
    "    Find the node at a specific cursor position:\n"
    "\n"
    "        ast-tool find --line 42 --column 17 src/parser.cpp\n"
    "\n"
    "    Find all uses of a specific identifier:\n"
    "\n"
    "        ast-tool find --type identifier --text initialize src/main.cpp\n"
    "\n"
    "    Look up a specific node by ID:\n"
    "\n"
    "        ast-tool find --id <node-id> src/main.cpp\n";

static const char kHelpRange[] =
    "NAME\n"
    "    range - Find AST nodes that intersect a source range.\n"
    "\n"
    "SYNOPSIS\n"
    "    ast-tool range [--start-line <n>] [--start-column <n>]\n"
    "                   [--end-line <n>]   [--end-column <n>]   <file>\n"
    "\n"
    "DESCRIPTION\n"
    "    Parse <file> and print every AST node whose source span overlaps\n"
    "    the specified line and column range.  All options are optional;\n"
    "    omitted bounds default to 0.\n"
    "\n"
    "ARGUMENTS\n"
    "    <file>    Path to the source file to search.\n"
    "\n"
    "OPTIONS\n"
    "    --start-line <n>    First line of the source range (1-based).\n"
    "    --start-column <n>  First column of the source range (1-based).\n"
    "    --end-line <n>      Last line of the source range (1-based).\n"
    "    --end-column <n>    Last column of the source range (1-based).\n"
    "\n"
    "OUTPUT\n"
    "    One line per matching AST node:\n"
    "\n"
    "        <ID> <type> @<line>:<col>[ \"<text>\"]\n"
    "\n"
    "    <ID>      32-bit node hash in uppercase hexadecimal.\n"
    "    <type>    Node type string.\n"
    "    <line>    1-based source line.\n"
    "    <col>     1-based source column.\n"
    "    <text>    Source text of the node, present when non-empty.\n"
    "\n"
    "EXAMPLES\n"
    "    Find all nodes on a single line:\n"
    "\n"
    "        ast-tool range \\\n"
    "            --start-line 42 --start-column 1 \\\n"
    "            --end-line   42 --end-column   80 \\\n"
    "            src/parser.cpp\n"
    "\n"
    "    Find all nodes in a block of code:\n"
    "\n"
    "        ast-tool range \\\n"
    "            --start-line 100 --start-column 1 \\\n"
    "            --end-line   150 --end-column   1 \\\n"
    "            src/codegen.cpp\n"
    "\n"
    "    Find the node at an exact cursor position:\n"
    "\n"
    "        ast-tool range \\\n"
    "            --start-line 17 --start-column 5 \\\n"
    "            --end-line   17 --end-column   5 \\\n"
    "            include/parser.hpp\n";

static const char kHelpParent[] =
    "NAME\n"
    "    parent - Print the parent of an AST node.\n"
    "\n"
    "SYNOPSIS\n"
    "    ast-tool parent --id <hex> <file>\n"
    "\n"
    "DESCRIPTION\n"
    "    Parse <file> and print the parent AST node of the node identified\n"
    "    by --id.  Use the dump, find, or outline commands to obtain node IDs.\n"
    "\n"
    "ARGUMENTS\n"
    "    <file>    Path to the source file.\n"
    "\n"
    "OPTIONS\n"
    "    --id <hex>    ID of the target node. Required. Accepts uppercase or\n"
    "                  lowercase hex, with or without a leading 0x prefix\n"
    "                  (e.g. 9E52E360, 0x9e52e360).\n"
    "\n"
    "OUTPUT\n"
    "    One line describing the parent AST node:\n"
    "\n"
    "        <ID> <type> @<line>:<col>[ \"<text>\"]\n"
    "\n"
    "    <ID>      32-bit node hash in uppercase hexadecimal.\n"
    "    <type>    Node type string.\n"
    "    <line>    1-based source line.\n"
    "    <col>     1-based source column.\n"
    "    <text>    Source text of the node, present when non-empty.\n"
    "\n"
    "EXAMPLES\n"
    "    Find the node at a cursor position, then walk up to its parent:\n"
    "\n"
    "        ast-tool find --line 42 --column 17 src/parser.cpp\n"
    "        ast-tool parent --id <node-id> src/parser.cpp\n"
    "\n"
    "    Walk up two levels to reach an enclosing declaration:\n"
    "\n"
    "        ast-tool parent --id <node-id>   src/main.cpp\n"
    "        ast-tool parent --id <parent-id> src/main.cpp\n";

static const char kHelpChildren[] =
    "NAME\n"
    "    children - Print the children of an AST node.\n"
    "\n"
    "SYNOPSIS\n"
    "    ast-tool children --id <hex> <file>\n"
    "\n"
    "DESCRIPTION\n"
    "    Parse <file> and print all immediate child AST nodes of the node\n"
    "    identified by --id.  Use the dump, find, or outline commands to\n"
    "    obtain node IDs.\n"
    "\n"
    "ARGUMENTS\n"
    "    <file>    Path to the source file.\n"
    "\n"
    "OPTIONS\n"
    "    --id <hex>    ID of the target node. Required. Accepts uppercase or\n"
    "                  lowercase hex, with or without a leading 0x prefix\n"
    "                  (e.g. 9E52E360, 0x9e52e360).\n"
    "\n"
    "OUTPUT\n"
    "    One line per child AST node:\n"
    "\n"
    "        <ID> <type> @<line>:<col>[ \"<text>\"]\n"
    "\n"
    "    <ID>      32-bit node hash in uppercase hexadecimal.\n"
    "    <type>    Node type string.\n"
    "    <line>    1-based source line.\n"
    "    <col>     1-based source column.\n"
    "    <text>    Source text of the node, present when non-empty.\n"
    "\n"
    "EXAMPLES\n"
    "    Find a function definition, then list its child nodes:\n"
    "\n"
    "        ast-tool find --type function_definition src/parser.cpp\n"
    "        ast-tool children --id <node-id> src/parser.cpp\n"
    "\n"
    "    Inspect the children of the root node:\n"
    "\n"
    "        ast-tool dump src/main.cpp\n"
    "        ast-tool children --id <root-id> src/main.cpp\n";

static const char kHelpSearch[] =
    "NAME\n"
    "    search - Search for semantic symbols across a workspace.\n"
    "\n"
    "SYNOPSIS\n"
    "    ast-tool search [--name <name>] [--fqn <fqn>] [--kind <kind>]\n"
    "                    [--file <path>] [--name-regex <re>] [--fqn-regex <re>]\n"
    "                    [--file-regex <re>] [--json [--pretty]] <root>\n"
    "\n"
    "DESCRIPTION\n"
    "    Scan every source file under <root>, extract semantic symbols, and\n"
    "    print those that match all supplied filters.  When no filter is\n"
    "    given, all symbols in the workspace are printed.\n"
    "\n"
    "    Exact filters and regex filters for the same field are mutually\n"
    "    exclusive (e.g. --name and --name-regex cannot be combined).\n"
    "\n"
    "ARGUMENTS\n"
    "    <root>    Workspace root directory to scan recursively.\n"
    "\n"
    "OPTIONS\n"
    "  Exact filters (substring match):\n"
    "    --name <name>       Match symbols whose unqualified name contains <name>.\n"
    "    --fqn <fqn>         Match symbols whose fully-qualified name contains\n"
    "                        <fqn>.\n"
    "    --kind <kind>       Match symbols of the given kind (function, method,\n"
    "                        class, struct, enum, variable, ...).\n"
    "    --file <path>       Match symbols in files whose path contains <path>.\n"
    "\n"
    "  Regex filters (RE2 syntax):\n"
    "    --name-regex <re>   Match symbols whose unqualified name matches <re>.\n"
    "    --fqn-regex  <re>   Match symbols whose fully-qualified name matches\n"
    "                        <re>.\n"
    "    --file-regex <re>   Match symbols in files whose path matches <re>.\n"
    "\n"
    "  Output format:\n"
    "    --json              Output results as a JSON array.\n"
    "    --pretty            Pretty-print the JSON output. Implies --json.\n"
    "\n"
    "OUTPUT\n"
    "    Default (plain text), one symbol per line:\n"
    "\n"
    "        <kind> <fqn> <file>:<line>:<col>\n"
    "\n"
    "    JSON fields per symbol:\n"
    "\n"
    "        kind            Symbol kind (function, class, variable, ...).\n"
    "        name            Unqualified symbol name.\n"
    "        fqn             Fully-qualified symbol name.\n"
    "        file            Path of the file that declares the symbol.\n"
    "        line            1-based declaration line.\n"
    "        column          1-based declaration column.\n"
    "        owning_scope    Lexical scope kind that contains the symbol.\n"
    "\n"
    "EXAMPLES\n"
    "    List all symbols in a workspace:\n"
    "\n"
    "        ast-tool search src/\n"
    "\n"
    "    Find all functions in a workspace:\n"
    "\n"
    "        ast-tool search --kind function src/\n"
    "\n"
    "    Find all symbols whose name contains 'parse':\n"
    "\n"
    "        ast-tool search --name parse src/\n"
    "\n"
    "    Find all classes declared in header files:\n"
    "\n"
    "        ast-tool search --kind class --file-regex '\\.hpp$' src/\n"
    "\n"
    "    Find all symbols in a specific namespace:\n"
    "\n"
    "        ast-tool search --fqn-regex '^ast::' src/\n"
    "\n"
    "    Export results as JSON for programmatic consumption:\n"
    "\n"
    "        ast-tool search --kind function --json --pretty src/\n";

// ---------------------------------------------------------------------------
// Command metadata tables

static const CommandEntry kCommands[] = {
    // AST Inspection
    {"dump",     "Print all AST nodes of a source file.",           CommandCategory::ASTInspection,   kHelpDump},
    {"outline",  "Print a hierarchical outline of named AST nodes.", CommandCategory::ASTInspection,  kHelpOutline},
    {"find",     "Find AST nodes by type, text, position, or ID.",  CommandCategory::ASTInspection,   kHelpFind},
    {"range",    "Find AST nodes that intersect a source range.",   CommandCategory::ASTInspection,   kHelpRange},
    {"parent",   "Print the parent of an AST node.",                CommandCategory::ASTInspection,   kHelpParent},
    {"children", "Print the children of an AST node.",              CommandCategory::ASTInspection,   kHelpChildren},
    // Semantic Analysis
    {"symbols",  "Extract semantic symbols from a source file.",    CommandCategory::SemanticAnalysis, kHelpSymbols},
    {"search",   "Search for semantic symbols across a workspace.", CommandCategory::SemanticAnalysis, kHelpSearch},
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

void print_command_help(const char* topic)
{
    if(!topic) {
        print_top_level_help();
        return;
    }
    for(int i = 0; i < kCommandCount; ++i) {
        if(strcmp(topic, kCommands[i].name) == 0) {
            fputs(kCommands[i].detail, stdout);
            return;
        }
    }
    fprintf(stderr, "error: unknown help topic '%s'\n", topic);
    fprintf(stderr, "Run 'ast-tool help' to see available commands.\n");
}

} // namespace ast
