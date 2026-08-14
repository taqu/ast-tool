#include "outline.h"
#include <print>
#include <string>
#include <vector>
#include "ast-format.h"
#include "ast-tool.h"

namespace ast
{
namespace
{
    /**
     * @brief One line of outline output: the AST node to print, and its depth
     *        within the *outline* hierarchy (skipped nodes do not add a level).
     */
    struct OutlineEntry
    {
        const ASTNode* node_;
        uint32_t depth_;
    };

    /** True if @p node is significant enough to appear in the outline (i.e. not anonymous punctuation). */
    bool is_outline_node(const ASTNode& node)
    {
        return ASTFlag::None != (node.flags_ & ASTFlag::Named);
    }

    /**
     * @brief Recursively walks @p node's subtree in source order, appending an OutlineEntry
     *        for every selected node to @p out. Skipped nodes are still traversed so their
     *        selected descendants are not lost, but they do not themselves add a depth level.
     */
    void collect(const AST& ast, const ASTNode& node, uint32_t depth, std::vector<OutlineEntry>& out)
    {
        uint32_t childDepth = depth;
        if(is_outline_node(node)) {
            out.push_back(OutlineEntry{&node, depth});
            childDepth = depth + 1;
        }
        for(uintptr_t child: node.children_) {
            collect(ast, ast[child], childDepth, out);
        }
    }

    /** Traverses @p ast from its root and returns the flattened, filtered outline. */
    std::vector<OutlineEntry> collect_outline(const AST& ast)
    {
        std::vector<OutlineEntry> entries;
        if(0 < ast.size()) {
            collect(ast, ast[0], 0, entries);
        }
        return entries;
    }

    /** Prints a single OutlineEntry as "<indent><type> [\"text\"] @row:col". Only leaf nodes carry text. */
    void format_entry(const OutlineEntry& entry)
    {
        const ASTNode& node = *entry.node_;
        for(uint32_t i = 0; i < entry.depth_; ++i) {
            std::print("  ");
        }
        std::print("{}", node.type_);
        if(node.children_.empty()) {
            std::u8string text = preview_text(node);
            if(!text.empty()) {
                std::print(" \"{}\"", (const char*)text.c_str());
            }
        }
        std::print(" @{}:{}\n", node.start_.row_ + 1, node.start_.column_ + 1);
    }

    /** Formats every entry in @p entries as plain, indented text. */
    void format_outline_text(const std::vector<OutlineEntry>& entries)
    {
        for(const OutlineEntry& entry: entries) {
            format_entry(entry);
        }
    }
} // namespace

bool parse_outline(Arguments& arguments, int32_t argc, const char8_t** argv)
{
    arguments.sub_ = SubCommand::Outline;
    if(argc <= 2) {
        return false;
    }
    arguments.outline_.input_ = argv[2];
    return true;
}

bool outline(const ArgOutline& arguments)
{
    if(nullptr == arguments.input_) {
        return false;
    }
    AST ast = parse(arguments.input_);
    if(!ast) {
        return false;
    }
    std::vector<OutlineEntry> entries = collect_outline(ast);
    format_outline_text(entries);
    return true;
}
} // namespace ast
