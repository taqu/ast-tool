#ifndef INC_AST_EXTRACTOR_LANGS_H_
#define INC_AST_EXTRACTOR_LANGS_H_
// Declarations of the per-language extractor entry points. This header is
// internal to the extractor subsystem: it is included by the dispatcher
// (ast-extractor.cpp) and by each language implementation file, but it is
// not part of the public API declared in "ast-extractor.h".
//
// To add support for a new language:
//   1. implement `extract_symbols_<lang>` in a new ast-extractor-<lang>.cpp
//   2. add its declaration below
//   3. add one case to the switch in ast-extractor.cpp
// No existing language implementation needs to change.
#include <vector>
#include "ast-extractor.h"

namespace ast
{
class AST;

namespace extractor
{
    std::vector<Symbol> extract_symbols_bash(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_c(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_cpp(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_csharp(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_css(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_go(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_html(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_java(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_javascript(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_python(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_ruby(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_rust(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_scala(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_typescript(const ast::AST& tree);
    std::vector<Symbol> extract_symbols_tsx(const ast::AST& tree);

} // namespace extractor
} // namespace ast
#endif // INC_AST_EXTRACTOR_LANGS_H_
