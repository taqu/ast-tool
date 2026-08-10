#ifndef INC_AST_EXTRACTOR_CFAMILY_H_
#define INC_AST_EXTRACTOR_CFAMILY_H_
// Shared C/C++ extraction logic.
//
// C and C++ currently parse with closely related Tree-sitter grammars and
// share the same extraction algorithm. This header exists only so that
// ast-extractor-c.cpp and ast-extractor-cpp.cpp can both call into that one
// algorithm; it is not a general-purpose "common" module (see
// ast-extractor-common.h for that) and must not be used by unrelated
// languages. C++-only features should be layered on top in
// ast-extractor-cpp.cpp without needing to modify this file or
// ast-extractor-c.cpp.
#include <vector>
#include "ast-extractor.h"

namespace ast
{
class AST;

namespace extractor
{
    std::vector<Symbol> extract_symbols_cfamily(const ast::AST& tree);

} // namespace extractor
} // namespace ast
#endif // INC_AST_EXTRACTOR_CFAMILY_H_
