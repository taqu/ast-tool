#ifndef INC_AST_EXTRACTOR_H_
#define INC_AST_EXTRACTOR_H_
/**
 * @file ast-extractor.h
 * @brief Language-agnostic API for extracting a flat list of declared Symbol%s
 *        (namespaces, types, functions, variables, ...) from a parsed AST.
 */
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ast
{
class AST;
/** Sentinel value for Symbol::nodeIndex meaning "no associated AST node". */
static constexpr size_t ExtractorInvalidId = size_t(-1);

/** The category of declaration a Symbol represents. */
enum class SymbolKind
{
    Namespace,
    Class,
    Struct,
    Union,
    Enum,
    Function,
    Method,
    Constructor,
    Destructor,
    Variable,
    Field,
    EnumValue,
    Macro,
    Typedef,
    UsingAlias,
};
/** Returns a human-readable name for @p kind (e.g. "Class", "Method"), or "Unknown" if unrecognized. */
const char* getSymbolKindName(SymbolKind kind);

/** Member/declaration access specifier. */
enum class Access
{
    Unknown,
    Public,
    Protected,
    Private,
};
/** Returns a human-readable, lowercase name for @p access (e.g. "public"), or "unknown" if unrecognized. */
const char* getAccessName(Access access);

/** A single extracted declaration and its metadata. */
struct Symbol
{
    std::u8string name; ///< The symbol's unqualified (local) name.
    std::u8string fqn;  ///< The symbol's fully qualified name, including enclosing namespace/type scopes.

    SymbolKind kind = SymbolKind::Function; ///< The declaration category this symbol represents.
    Access access = Access::Unknown;        ///< Access specifier in effect at the declaration, if applicable.

    bool isStatic = false;    ///< True if the declaration is marked static.
    bool isConstexpr = false; ///< True if the declaration is marked constexpr.
    bool isInline = false;    ///< True if the declaration is marked inline.

    size_t nodeIndex = ast::ExtractorInvalidId; ///< Index of the associated ASTNode within the source AST, or ExtractorInvalidId if none.
    uint32_t line = 0;                          ///< Source line (0-based) where the symbol is declared.
    uint32_t column = 0;                        ///< Source column (0-based) where the symbol is declared.
};

/**
 * @brief Extracts all recognized symbols from @p ast, dispatching to the extractor
 *        for the AST's detected language.
 * @return The extracted symbols, in declaration order. Empty if the language is
 *         unknown or unsupported.
 */
std::vector<Symbol> extract_symbols(const ast::AST& ast);
} // namespace ast
#endif // INC_AST_EXTRACTOR_H_
