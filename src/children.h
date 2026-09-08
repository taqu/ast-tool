#ifndef INC_AST_CHILDREN_CLI_H_
#define INC_AST_CHILDREN_CLI_H_
#include <cstdint>

namespace ast
{
    struct Arguments;
    struct ArgChildren;
    bool parse_children(Arguments& arguments, int32_t argc, const char8_t** argv);
    bool children(const ArgChildren& arguments);
}
#endif // INC_AST_CHILDREN_CLI_H_
