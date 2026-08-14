#include "dump.h"
#include <print>
#include "ast-tool.h"

namespace ast
{
bool parse_dump(Arguments& arguments, int32_t argc, const char8_t** argv)
{
    arguments.sub_ = SubCommand::Dump;
    if(argc <= 2) {
        return false;
    }
    arguments.dump_.input_ = argv[2];
    return true;
}

bool dump(const ArgDump& arguments)
{
    if(nullptr == arguments.input_){
        return false;
    }
    AST ast = parse(arguments.input_);
    if(!ast){
        return false;
    }
    for(uint32_t i=0; i<ast.size(); ++i){
        const ASTNode& node = ast[i];
        std::print("{} {:X}\n", node.type_, node.hash_);
    }
    return true;
}
} // namespace ast

