#ifndef INC_AST_PARENT_CLI_H_
#define INC_AST_PARENT_CLI_H_
#include <cstdint>

namespace ast
{
    struct Arguments;
    struct ArgParent;
    bool parse_parent(Arguments& arguments, int32_t argc, const char8_t** argv);
    bool parent(const ArgParent& arguments);
}
#endif // INC_AST_PARENT_CLI_H_
