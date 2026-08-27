#ifndef INC_AST_CACHE_CMD_H_
#define INC_AST_CACHE_CMD_H_
#include <cstdint>

namespace ast
{
struct Arguments;
struct ArgCache;
bool parse_cache(Arguments& arguments, int32_t argc, const char8_t** argv);
bool cache_warm_cmd(const ArgCache& args);
bool cache_status_cmd(const ArgCache& args);
} // namespace ast
#endif // INC_AST_CACHE_CMD_H_
