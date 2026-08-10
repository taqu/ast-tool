#include "find.h"
#include <cassert>
#include <charconv>
#include <print>
#include <string>
#include <vector>
#include "ast-find.h"
#include "ast-format.h"
#include "ast-tool.h"

namespace ast
{
namespace
{
    /** Prints one matching node as "<id> <type> @row:col [\"preview\"]". */
    void format_match(const ASTNode& node)
    {
        std::print("{:X} {} @{}:{}", node.hash_, node.type_, node.start_.row_ + 1, node.start_.column_ + 1);
        std::string text = preview_text(node);
        if(!text.empty()) {
            std::print(" \"{}\"", text);
        }
        std::print("\n");
    }

    /**
     * @brief Convert string to uint32_t as decimal 
     * @param begin ... null terminated string
     * @param[out] value ... 
     * @return 
     */
    bool from_chars_10(const char* begin, std::uint32_t& value)
    {
        assert(nullptr != begin);
        const char* end = begin + ::strlen(begin);
        auto [ptr, ec] = std::from_chars(begin, end, value, 10);
        return ec == std::errc() && ptr == end;
    }

    /**
     * @brief Convert string to uint32_t as hexadecimal
     * @param begin ... null terminated string
     * @param value ...
     * @return 
     */
    bool from_chars_16(const char* begin, std::uint32_t& value)
    {
        assert(nullptr != begin);
        const char* end = begin + ::strlen(begin);
        if(2<=(end - begin) && begin[0] == '0' && (begin[1] == 'x' || begin[1] == 'X')) {
            begin += 2;
        }

        auto [ptr, ec] = std::from_chars(begin, end, value, 16);
        return ec == std::errc() && ptr == end;
    }
} // namespace

bool parse_find(Arguments& arguments, int32_t argc, const char** argv)
{
#define STREQUALS(str, literal) (0 == ::strcmp(str, literal))
    arguments.sub_ = SubCommand::Find;
    if(argc <= 2) {
        return false;
    }
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
        args.input_ = argv[i];
    }
    return true;
#undef STREQUALS
}

bool find(const ArgFind& arguments)
{
    if(nullptr == arguments.input_) {
        return false;
    }
    AST ast = parse(arguments.input_);
    if(!ast) {
        return false;
    }

    // --line/--column are accepted as 1-based (matching outline's @row:col display);
    // FindCriteria's position is zero-based, matching ASTPoint.
    FindCriteria criteria;
    criteria.type_ = arguments.type_;
    criteria.grammar_ = arguments.grammar_;
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
