#ifndef INC_AST_SEARCH_CMD_H_
#define INC_AST_SEARCH_CMD_H_
#include <cstdint>

namespace ast
{
struct Arguments;
struct ArgSearch;
bool parse_search(Arguments& arguments, int32_t argc, const char** argv);
bool search(const ArgSearch& arguments);
} // namespace ast
#endif // INC_AST_SEARCH_CMD_H_
