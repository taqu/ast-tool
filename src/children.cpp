#include "children.h"
#include <cassert>
#include <charconv>
#include <cstdio>
#include <print>
#include <string>
#include "ast-format.h"
#include "ast-tool.h"

namespace ast
{
namespace
{
    bool from_chars_16(const char* begin, std::uint32_t& value)
    {
        assert(nullptr != begin);
        const char* end = begin + ::strlen(begin);
        if(2 <= (end - begin) && begin[0] == '0' && (begin[1] == 'x' || begin[1] == 'X')) {
            begin += 2;
        }
        auto [ptr, ec] = std::from_chars(begin, end, value, 16);
        return ec == std::errc() && ptr == end;
    }

    void format_match(const ASTNode& node)
    {
        std::print("{:X} {} @{}:{}", node.hash_, node.type_, node.start_.row_ + 1, node.start_.column_ + 1);
        std::string text = preview_text(node);
        if(!text.empty()) {
            std::print(" \"{}\"", text);
        }
        std::print("\n");
    }
} // namespace

bool parse_children(Arguments& arguments, int32_t argc, const char** argv)
{
#define STREQUALS(str, literal) (0 == ::strcmp(str, literal))
    arguments.sub_ = SubCommand::Children;
    if(argc <= 2) {
        return false;
    }
    ArgChildren& args = arguments.children_;
    args.input_ = nullptr;
    args.id_ = 0;
    args.hasId_ = false;

    for(int32_t i = 2; i < argc; ++i) {
        if(STREQUALS(argv[i], "--id")) {
            if((i + 1) < argc) {
                if(from_chars_16(argv[++i], args.id_)) {
                    args.hasId_ = true;
                }
            }
            continue;
        }
        args.input_ = argv[i];
    }
    return args.hasId_;
#undef STREQUALS
}

bool children(const ArgChildren& arguments)
{
    if(nullptr == arguments.input_ || !arguments.hasId_) {
        return false;
    }
    AST ast = parse(arguments.input_);
    if(!ast) {
        return false;
    }

    const ASTNode* target = nullptr;
    for(uint32_t i = 0; i < ast.size(); ++i) {
        if(ast[i].hash_ == arguments.id_) {
            target = &ast[i];
            break;
        }
    }

    if(nullptr == target) {
        std::print(stderr, "error: node {:X} not found\n", arguments.id_);
        return false;
    }

    for(uintptr_t childIndex : target->children_) {
        if(childIndex != InvalidId) {
            format_match(ast[childIndex]);
        }
    }
    return true;
}
} // namespace ast
