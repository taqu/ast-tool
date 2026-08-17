#include "ast-search.h"
#include <cstring>
#if defined(_WIN32) || defined(_WIN64)
#    include <string.h>
#    define STRCASECMP(a, b) ::_stricmp(reinterpret_cast<const char*>(a), b)
#else
#    define STRCASECMP(a, b) ::strcasecmp(reinterpret_cast<const char*>(a), b)
#endif

namespace ast
{
namespace
{
    std::optional<SymbolKind> parse_symbol_kind(const char8_t* s)
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
    std::u8string try_compile_regex(const char8_t* field_name, const char8_t* pattern,
                                  std::optional<RegexPattern>& out)
    {
        RegexPattern pat(reinterpret_cast<const char*>(pattern));
        if(!pat.valid()) {
            return std::u8string(u8"invalid ") + field_name + u8" pattern '" + pattern + u8"': " + reinterpret_cast<const char8_t*>(pat.error().c_str());
        }
        out = std::move(pat);
        return {};
    }
} // namespace

std::expected<SearchQuery, std::u8string> build_search_query(
    const char8_t* name,
    const char8_t* fqn,
    const char8_t* kind_str,
    const char8_t* file,
    const char8_t* name_regex,
    const char8_t* fqn_regex,
    const char8_t* file_regex)
{
    SearchQuery query;

    if(name) query.name = name;
    if(fqn)  query.fqn  = fqn;
    if(file) query.file = file;

    if(kind_str) {
        std::optional<SymbolKind> k = parse_symbol_kind(kind_str);
        if(!k) {
            return std::unexpected(std::u8string(u8"unknown kind '") + kind_str + u8"'");
        }
        query.kind = k;
    }

    if(name_regex) {
        if(name) {
            return std::unexpected(std::u8string(u8"name and name_regex cannot both be specified"));
        }
        if(std::u8string err = try_compile_regex(u8"name_regex", name_regex, query.name_regex); !err.empty()) {
            return std::unexpected(std::move(err));
        }
    }

    if(fqn_regex) {
        if(fqn) {
            return std::unexpected(std::u8string(u8"fqn and fqn_regex cannot both be specified"));
        }
        if(std::u8string err = try_compile_regex(u8"fqn_regex", fqn_regex, query.fqn_regex); !err.empty()) {
            return std::unexpected(std::move(err));
        }
    }

    if(file_regex) {
        if(file) {
            return std::unexpected(std::u8string(u8"file and file_regex cannot both be specified"));
        }
        if(std::u8string err = try_compile_regex(u8"file_regex", file_regex, query.file_regex); !err.empty()) {
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
        //if(query.file && sym.sourceFile.find(*query.file) == std::string::npos)             continue;
        if(query.file){
            const std::filesystem::path& sourceFile = sym.sourceFile;
            std::u8string file = sourceFile.u8string();
            if(file.find(*query.file) == std::u8string::npos){
                continue;
            }
        }
        // Regex filters after exact — RE2 evaluation only on surviving candidates.
        if(query.name_regex && !query.name_regex->matches(sym.symbol.name))                 continue;
        if(query.fqn_regex  && !query.fqn_regex->matches(sym.symbol.fqn))                  continue;
        if(query.file_regex && !query.file_regex->matches(sym.sourceFile))                  continue;
        results.push_back(&sym);
    }
    return results;
}

SemanticSearchEngineOneShot::SemanticSearchEngineOneShot(const SearchQuery& query)
    : query_(query)
{
}

bool SemanticSearchEngineOneShot::match(const WorkspaceSymbol& symbol) const
{
    // Exact filters first — cheap comparisons reduce the candidate set.
    if(query_.kind && symbol.symbol.kind != *query_.kind)
        return false;
    if(query_.name && symbol.symbol.name != *query_.name)
        return false;
    if(query_.fqn && symbol.symbol.fqn != *query_.fqn)
        return false;
    // if(query.file && symbol.sourceFile.find(*query.file) == std::string::npos)             return false;
    if(query_.file) {
        const std::filesystem::path& sourceFile = symbol.sourceFile;
        std::u8string file = sourceFile.u8string();
        if(file.find(*query_.file) == std::u8string::npos) {
            return false;
        }
    }
    // Regex filters after exact — RE2 evaluation only on surviving candidates.
    if(query_.name_regex && !query_.name_regex->matches(symbol.symbol.name))
        return false;
    if(query_.fqn_regex && !query_.fqn_regex->matches(symbol.symbol.fqn))
        return false;
    if(query_.file_regex && !query_.file_regex->matches(symbol.sourceFile))
        return false;
    return true;
}

} // namespace ast
