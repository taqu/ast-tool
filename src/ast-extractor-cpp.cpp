#include "ast-extractor-langs.h"
#include "ast-extractor-cfamily.h"
#include "ast-ir.h"

namespace ast
{
namespace extractor
{
    // C++ currently uses the shared C/C++ extraction algorithm unchanged.
    // Future C++-only features (templates, concepts, lambdas, etc.) should be
    // layered on top of extract_symbols_cfamily() here, without needing to
    // modify ast-extractor-c.cpp or ast-extractor-cfamily.cpp.
    std::vector<Symbol> extract_symbols_cpp(const ast::AST& tree)
    {
        return extract_symbols_cfamily(tree);
    }

} // namespace extractor
} // namespace ast
