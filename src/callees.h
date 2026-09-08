#ifndef INC_AST_CALLEES_CMD_H_
#define INC_AST_CALLEES_CMD_H_
#include <cstdint>

namespace ast
{
struct Arguments;
struct ArgCallees;
bool parse_callees(Arguments& arguments, int32_t argc, const char8_t** argv);
bool callees(const ArgCallees& arguments);
} // namespace ast
#endif // INC_AST_CALLEES_CMD_H_
