#ifndef INC_AST_SYMBOLS_H_
#define INC_AST_SYMBOLS_H_
#include <cstdint>

namespace ast
	{
	struct Arguments;
	struct ArgSymbols;
	bool parse_symbols(Arguments& arguments, int32_t argc, const char8_t** argv);
	/**
	 * @brief 
	 * @param arguments 
	 * @return 
	 */
	bool symbols(const ArgSymbols& arguments);
}
#endif //INC_AST_SYMBOLS_H_
