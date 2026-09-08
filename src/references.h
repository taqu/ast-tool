#ifndef INC_AST_REFERENCES_CMD_H_
#define INC_AST_REFERENCES_CMD_H_
#include <cstdint>

namespace ast
{
struct Arguments;
struct ArgReferences;
bool parse_references(Arguments& arguments, int32_t argc, const char8_t** argv);
bool references(const ArgReferences& arguments);
} // namespace ast
#endif // INC_AST_REFERENCES_CMD_H_
