#ifndef INC_AST_DUMP_H_
#define INC_AST_DUMP_H_
#include <cstdint>

namespace ast
{
	struct Arguments;
	struct ArgDump;
	bool parse_dump(Arguments& arguments, int32_t argc, const char** argv);
	bool dump(const ArgDump& arguments);
}
#endif //INC_AST_DUMP_H_
