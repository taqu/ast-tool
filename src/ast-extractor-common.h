#ifndef INC_AST_EXTRACTOR_COMMON_H_
#define INC_AST_EXTRACTOR_COMMON_H_
// Language-independent helpers shared by the per-language extractors.
//
// Everything in here is intentionally generic: it must not encode the node
// type names of any particular Tree-sitter grammar. Grammar-specific logic
// belongs in the language (or language-family) implementation files, never
// here. This header is internal to the extractor subsystem and is not part
// of the public API declared in "ast-extractor.h".
#include <cstdint>
#include <string>
#include <vector>
#include "ast-extractor.h"

namespace ast
{
class AST;
struct ASTNode;

namespace extractor
{
    // ── Generic AST traversal helpers ───────────────────────────────────────

    // Returns the first direct child of `node` whose type equals `type`,
    // or nullptr if none is found.
    const ast::ASTNode* findChild(const ast::AST& tree, const ast::ASTNode& node, const char8_t* type);

    // Returns true if any direct child of `node` has text exactly equal to `text`.
    bool childHasText(const ast::AST& tree, const ast::ASTNode& node, const char8_t* text);

    // ── Scope stack management ──────────────────────────────────────────────

    struct ScopeFrame
    {
        std::u8string name;           // empty for anonymous scopes
        SymbolKind  kind;
        uint32_t    endByte;
        Access      currentAccess;
        bool        isAccessAware;  // true for scopes that track member access (e.g. C++ class)
    };

    // Joins the named scopes on `stack` with `sep`, appending `symName`.
    std::u8string buildFQN(const std::vector<ScopeFrame>& stack, const std::u8string& symName, const char8_t* sep = u8"::");

    // Returns the current access level of the innermost access-aware scope.
    Access topAccess(const std::vector<ScopeFrame>& stack);

    // True if any enclosing scope is a named class/struct/union-like scope.
    bool inNamedClassScope(const std::vector<ScopeFrame>& stack);

    // True if any enclosing scope is a function/method-like scope.
    bool insideFunctionScope(const std::vector<ScopeFrame>& stack);

    // ── Symbol creation ──────────────────────────────────────────────────────

    Symbol makeSymbol(std::u8string name, std::u8string fqn, SymbolKind kind,
                       Access access, uint32_t idx, uint32_t row, uint32_t col);

} // namespace extractor
} // namespace ast
#endif // INC_AST_EXTRACTOR_COMMON_H_
