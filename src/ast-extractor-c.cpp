#include "ast-extractor-langs.h"
#include "ast-extractor-cfamily.h"
#include "ast-ir.h"

namespace ast
{
namespace extractor
{
    // C currently uses the shared C/C++ extraction algorithm unchanged.
    // Add C-specific handling here if C's needs ever diverge from C++'s;
    // doing so must not require changes to ast-extractor-cpp.cpp.
    std::vector<Symbol> extract_symbols_c(const ast::AST& tree)
    {
        return extract_symbols_cfamily(tree);
    }

} // namespace extractor
} // namespace ast
