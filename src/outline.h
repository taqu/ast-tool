#ifndef INC_AST_OUTLINE_H_
#define INC_AST_OUTLINE_H_
#include <cstdint>

namespace ast
{
    struct Arguments;
    struct ArgOutline;
    bool parse_outline(Arguments& arguments, int32_t argc, const char** argv);
    bool outline(const ArgOutline& arguments);
}
#endif //INC_AST_OUTLINE_H_
