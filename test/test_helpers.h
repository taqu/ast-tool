#ifndef INC_TEST_HELPERS_H_
#define INC_TEST_HELPERS_H_
#include <iostream>
#include <string_view>
#include <vector>
#include "ast-extractor.h"

namespace ast { namespace test {

inline bool hasFQN(const std::vector<ast::Symbol>& syms,
                   std::string_view fqn, ast::SymbolKind kind)
{
    for(const auto& s : syms)
        if(s.fqn == fqn && s.kind == kind) return true;
    return false;
}

inline const ast::Symbol* findSymbol(const std::vector<ast::Symbol>& syms,
                                     std::string_view fqn, ast::SymbolKind kind)
{
    for(const auto& s : syms)
        if(s.fqn == fqn && s.kind == kind) return &s;
    return nullptr;
}

inline bool check(bool condition, const char* description)
{
    if(!condition)
        std::cerr << "    FAIL: " << description << "\n";
    return condition;
}

inline size_t countKind(const std::vector<ast::Symbol>& syms, ast::SymbolKind kind)
{
    size_t n = 0;
    for(const auto& s : syms)
        if(s.kind == kind) ++n;
    return n;
}

}} // namespace ast::test
#endif // INC_TEST_HELPERS_H_
