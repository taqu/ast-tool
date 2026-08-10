#ifndef INC_AST_RANGE_CLI_H_
#define INC_AST_RANGE_CLI_H_
#include <cstdint>

namespace ast
{
    struct Arguments;
    struct ArgRange;
    bool parse_range(Arguments& arguments, int32_t argc, const char** argv);
    bool range(const ArgRange& arguments);
}
#endif // INC_AST_RANGE_CLI_H_
