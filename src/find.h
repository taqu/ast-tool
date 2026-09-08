#ifndef INC_AST_FIND_CLI_H_
#define INC_AST_FIND_CLI_H_
#include <cstdint>

namespace ast
{
    struct Arguments;
    struct ArgFind;
    bool parse_find(Arguments& arguments, int32_t argc, const char8_t** argv);
    bool find(const ArgFind& arguments);
}
#endif //INC_AST_FIND_CLI_H_
