#include "ast-search.h"
#include <cstring>
#if defined(_WIN32) || defined(_WIN64)
#    include <string.h>
#    define STRCASECMP(a, b) ::_stricmp(a, b)
#else
#    define STRCASECMP(a, b) ::strcasecmp(a, b)
#endif

namespace ast
{
namespace
{
    std::optional<SymbolKind> parse_symbol_kind(const char* s)
    {
        if(0 == STRCASECMP(s, "namespace"))    return SymbolKind::Namespace;
        if(0 == STRCASECMP(s, "class"))        return SymbolKind::Class;
        if(0 == STRCASECMP(s, "struct"))       return SymbolKind::Struct;
        if(0 == STRCASECMP(s, "union"))        return SymbolKind::Union;
        if(0 == STRCASECMP(s, "enum"))         return SymbolKind::Enum;
        if(0 == STRCASECMP(s, "function"))     return SymbolKind::Function;
        if(0 == STRCASECMP(s, "method"))       return SymbolKind::Method;
        if(0 == STRCASECMP(s, "constructor"))  return SymbolKind::Constructor;
        if(0 == STRCASECMP(s, "destructor"))   return SymbolKind::Destructor;
        if(0 == STRCASECMP(s, "variable"))     return SymbolKind::Variable;
        if(0 == STRCASECMP(s, "field"))        return SymbolKind::Field;
        if(0 == STRCASECMP(s, "enumvalue"))    return SymbolKind::EnumValue;
        if(0 == STRCASECMP(s, "macro"))        return SymbolKind::Macro;
        if(0 == STRCASECMP(s, "typedef"))      return SymbolKind::Typedef;
        if(0 == STRCASECMP(s, "usingalias"))   return SymbolKind::UsingAlias;
        return std::nullopt;
    }

    // Compiles a regex pattern and returns an error string if invalid.
    // Returns empty string on success.
    std::string try_compile_regex(const char* field_name, const char* pattern,
                                  std::optional<RegexPattern>& out)
    {
        RegexPattern pat(pattern);
        if(!pat.valid()) {
            return std::string("invalid ") + field_name + " pattern '" + pattern + "': " + pat.error();
        }
        out = std::move(pat);
        return {};
    }
} // namespace

std::expected<SearchQuery, std::string> build_search_query(
    const char* name,
    const char* fqn,
    const char* kind_str,
    const char* file,
    const char* name_regex,
    const char* fqn_regex,
    const char* file_regex)
{
    SearchQuery query;

    if(name) query.name = name;
    if(fqn)  query.fqn  = fqn;
    if(file) query.file = file;

    if(kind_str) {
        std::optional<SymbolKind> k = parse_symbol_kind(kind_str);
        if(!k) {
            return std::unexpected(std::string("unknown kind '") + kind_str + "'");
        }
        query.kind = k;
    }

    if(name_regex) {
        if(name) {
            return std::unexpected(std::string("name and name_regex cannot both be specified"));
        }
        if(std::string err = try_compile_regex("name_regex", name_regex, query.name_regex); !err.empty()) {
            return std::unexpected(std::move(err));
        }
    }

    if(fqn_regex) {
        if(fqn) {
            return std::unexpected(std::string("fqn and fqn_regex cannot both be specified"));
        }
        if(std::string err = try_compile_regex("fqn_regex", fqn_regex, query.fqn_regex); !err.empty()) {
            return std::unexpected(std::move(err));
        }
    }

    if(file_regex) {
        if(file) {
            return std::unexpected(std::string("file and file_regex cannot both be specified"));
        }
        if(std::string err = try_compile_regex("file_regex", file_regex, query.file_regex); !err.empty()) {
            return std::unexpected(std::move(err));
        }
    }

    return query;
}

SemanticSearchEngine::SemanticSearchEngine(const Workspace& workspace)
    : workspace_(workspace)
{
}

std::vector<const WorkspaceSymbol*> SemanticSearchEngine::search(const SearchQuery& query) const
{
    std::vector<const WorkspaceSymbol*> results;
    for(const WorkspaceSymbol& sym : workspace_.symbols) {
        // Exact filters first — cheap comparisons reduce the candidate set.
        if(query.kind && sym.symbol.kind != *query.kind)                                    continue;
        if(query.name && sym.symbol.name != *query.name)                                    continue;
        if(query.fqn  && sym.symbol.fqn  != *query.fqn)                                    continue;
        if(query.file && sym.sourceFile.find(*query.file) == std::string::npos)             continue;
        // Regex filters after exact — RE2 evaluation only on surviving candidates.
        if(query.name_regex && !query.name_regex->matches(sym.symbol.name))                 continue;
        if(query.fqn_regex  && !query.fqn_regex->matches(sym.symbol.fqn))                  continue;
        if(query.file_regex && !query.file_regex->matches(sym.sourceFile))                  continue;
        results.push_back(&sym);
    }
    return results;
}

} // namespace ast
