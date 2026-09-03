#include "find.h"
#include <cassert>
#include <cstring>
#include <print>
#include <string>
#include <vector>
#include "ast-find.h"
#include "ast-format.h"
#include "ast-node-type.h"
#include "ast-tool.h"
#include "cli-semantic.h"

namespace ast
{
namespace
{
    /** Prints one matching node as "<id> <type> @row:col [\"preview\"]". */
    void format_match(const ASTNode& node)
    {
        std::print("{:X} {} @{}:{}", node.hash_, ast_node_type_to_string(node.type_), node.start_.row_ + 1, node.start_.column_ + 1);
        std::u8string text = preview_text(node);
        if(!text.empty()) {
            std::print(" \"{}\"", (const char*)text.c_str());
        }
        std::print("\n");
    }

} // namespace

bool parse_find(Arguments& arguments, int32_t argc, const char8_t** argv)
{
#define STREQUALS(str, literal) (0 == ::strcmp(reinterpret_cast<const char*>(str), literal))
    arguments.sub_ = SubCommand::Find;
    ArgFind& args = arguments.find_;
    args.input_ = nullptr;
    args.type_ = nullptr;
    args.grammar_ = nullptr;
    args.text_ = nullptr;
    args.id_ = 0;
    args.hasId_ = false;
    args.line_ = 0;
    args.column_ = 0;
    args.hasLine_ = false;
    args.hasColumn_ = false;
    args.badOption_ = nullptr;

    for(int32_t i = 2; i < argc; ++i) {
        if(STREQUALS(argv[i], "--type")) {
            if((i + 1) < argc) {
                args.type_ = argv[++i];
            }
            continue;
        }
        if(STREQUALS(argv[i], "--grammar")) {
            if((i + 1) < argc) {
                args.grammar_ = argv[++i];
            }
            continue;
        }
        if(STREQUALS(argv[i], "--id")) {
            if((i + 1) < argc) {
                if(from_chars_16(argv[++i], args.id_)){
                    args.hasId_ = true;
                }
            }
            continue;
        }
        if(STREQUALS(argv[i], "--line")) {
            if((i + 1) < argc) {
                if(from_chars_10(argv[++i], args.line_)){
                    args.hasLine_ = true;
                }
            }
            continue;
        }
        if(STREQUALS(argv[i], "--column")) {
            if((i + 1) < argc) {
                if(from_chars_10(argv[++i], args.column_)){
                    args.hasColumn_ = true;
                }
            }
            continue;
        }
        if(STREQUALS(argv[i], "--text")) {
            if((i + 1) < argc) {
                args.text_ = argv[++i];
            }
            continue;
        }
        if(cli::looks_like_option(argv[i])) {
            if(args.badOption_ == nullptr) {
                args.badOption_ = argv[i];
            }
            continue;
        }
        args.input_ = argv[i];
    }
    return true;
#undef STREQUALS
}

bool find(const ArgFind& arguments)
{
    static const std::vector<std::u8string> kFindOptions = {
        u8"--type", u8"--grammar", u8"--text", u8"--id", u8"--line", u8"--column",
    };

    if(arguments.badOption_ != nullptr) {
        cli::print_error(cli::make_unknown_option(arguments.badOption_, kFindOptions), false, false);
        return false;
    }
    if(nullptr == arguments.input_) {
        cli::print_error(
            cli::make_invalid_arguments(u8"missing FILE", u8"ast-tool find [options] <file>"),
            false, false);
        return false;
    }
    AST ast = parse(arguments.input_);
    if(!ast) {
        cli::print_error(cli::make_invalid_arguments(u8"could not parse file", u8"ast-tool find [options] <file>"), false, false);
        return false;
    }

    // --line/--column are accepted as 1-based (matching outline's @row:col display);
    // FindCriteria's position is zero-based, matching ASTPoint.
    FindCriteria criteria;
    criteria.type_ = (nullptr != arguments.type_)
        ? ast_node_type_from_string(reinterpret_cast<const char*>(arguments.type_))
        : ASTNodeType::Unknown;
    criteria.grammar_ = (nullptr != arguments.grammar_)
        ? ast_node_type_from_string(reinterpret_cast<const char*>(arguments.grammar_))
        : ASTNodeType::Unknown;
    criteria.text_ = arguments.text_;
    criteria.id_ = arguments.id_;
    criteria.hasId_ = arguments.hasId_;
    criteria.hasPosition_ = arguments.hasLine_ && arguments.hasColumn_;
    criteria.line_ = (0 < arguments.line_) ? arguments.line_ - 1 : 0;
    criteria.column_ = (0 < arguments.column_) ? arguments.column_ - 1 : 0;

    for(const ASTNode* node: find_nodes(ast, criteria)) {
        format_match(*node);
    }
    return true;
}
} // namespace ast
