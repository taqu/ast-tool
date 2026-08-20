#ifndef INC_AST_CALLERS_CMD_H_
#define INC_AST_CALLERS_CMD_H_
#include <cstdint>

namespace ast
{
struct Arguments;
struct ArgCallers;
bool parse_callers(Arguments& arguments, int32_t argc, const char8_t** argv);
bool callers(const ArgCallers& arguments);
} // namespace ast
#endif // INC_AST_CALLERS_CMD_H_
