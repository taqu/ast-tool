#include "range.h"
#include <cassert>
#include <charconv>
#include <print>
#include <string>
#include <vector>
#include "ast-format.h"
#include "ast-tool.h"

namespace ast
{
namespace
{
    void format_match(const ASTNode& node)
    {
        std::print("{:X} {} @{}:{}", node.hash_, node.type_, node.start_.row_ + 1, node.start_.column_ + 1);
        std::u8string text = preview_text(node);
        if(!text.empty()) {
            std::print(" \"{}\"", (const char*)text.c_str());
        }
        std::print("\n");
    }

    bool point_lt(ASTPoint a, ASTPoint b)
    {
        return a.row_ < b.row_ || (a.row_ == b.row_ && a.column_ < b.column_);
    }

    bool intersects(const ASTNode& node, ASTPoint qStart, ASTPoint qEnd)
    {
        return point_lt(node.start_, qEnd) && point_lt(qStart, node.end_);
    }
} // namespace

bool parse_range(Arguments& arguments, int32_t argc, const char8_t** argv)
{
#define STREQUALS(str, literal) (0 == ::strcmp(reinterpret_cast<const char*>(str), literal))
    arguments.sub_ = SubCommand::Range;
    if(argc <= 2) {
        return false;
    }
    ArgRange& args = arguments.range_;
    args.input_ = nullptr;
    args.startLine_ = 0;
    args.startColumn_ = 0;
    args.endLine_ = 0;
    args.endColumn_ = 0;
    args.hasStart_ = false;
    args.hasEnd_ = false;

    for(int32_t i = 2; i < argc; ++i) {
        if(STREQUALS(argv[i], "--start-line")) {
            if((i + 1) < argc) {
                from_chars_10(argv[++i], args.startLine_);
            }
            continue;
        }
        if(STREQUALS(argv[i], "--start-column")) {
            if((i + 1) < argc) {
                from_chars_10(argv[++i], args.startColumn_);
            }
            continue;
        }
        if(STREQUALS(argv[i], "--end-line")) {
            if((i + 1) < argc) {
                from_chars_10(argv[++i], args.endLine_);
            }
            continue;
        }
        if(STREQUALS(argv[i], "--end-column")) {
            if((i + 1) < argc) {
                from_chars_10(argv[++i], args.endColumn_);
            }
            continue;
        }
        args.input_ = argv[i];
    }
    args.hasStart_ = (0 < args.startLine_ || 0 < args.startColumn_);
    args.hasEnd_ = (0 < args.endLine_ || 0 < args.endColumn_);
    return true;
#undef STREQUALS
}

bool range(const ArgRange& arguments)
{
    if(nullptr == arguments.input_) {
        return false;
    }
    AST ast = parse(arguments.input_);
    if(!ast) {
        return false;
    }

    // Convert 1-based input to 0-based ASTPoint coordinates
    ASTPoint qStart = {
        (0 < arguments.startLine_) ? arguments.startLine_ - 1 : 0,
        (0 < arguments.startColumn_) ? arguments.startColumn_ - 1 : 0,
    };
    ASTPoint qEnd = {
        (0 < arguments.endLine_) ? arguments.endLine_ - 1 : 0,
        (0 < arguments.endColumn_) ? arguments.endColumn_ - 1 : 0,
    };

    for(uint32_t i = 0; i < ast.size(); ++i) {
        const ASTNode& node = ast[i];
        if(intersects(node, qStart, qEnd)) {
            format_match(node);
        }
    }
    return true;
}
} // namespace ast
