#include "cli-semantic.h"
#include <cstring>
#include <print>
#include <unordered_set>

namespace ast
{
namespace cli
{

bool parse_symbol_root_args(
    const char8_t*& symbol,
    const char8_t*& root,
    bool& json,
    bool& pretty,
    int32_t argc,
    const char8_t** argv)
{
#define STREQUALS(str, literal) (0 == ::strcmp(reinterpret_cast<const char*>(str), literal))
    symbol = nullptr;
    root   = nullptr;
    json   = false;
    pretty = false;

    for(int32_t i = 2; i < argc; ++i) {
        if(STREQUALS(argv[i], "--json")) {
            json = true;
            continue;
        }
        if(STREQUALS(argv[i], "--pretty")) {
            pretty = true;
            continue;
        }
        if(symbol == nullptr) {
            symbol = argv[i];
        } else {
            root = argv[i];
        }
    }
    return symbol != nullptr && root != nullptr;
#undef STREQUALS
}

std::vector<const WorkspaceSymbol*> resolve_symbol_query(
    const Workspace& ws,
    const char8_t* query)
{
    std::u8string_view q{query};
    std::vector<const WorkspaceSymbol*> raw;
    bool byFqn = q.find(u8"::") != std::u8string_view::npos;
    for(const WorkspaceSymbol& sym : ws.symbols) {
        if(byFqn ? sym.symbol.fqn == q : sym.symbol.name == q) {
            raw.push_back(&sym);
        }
    }
    if(raw.size() <= 1) return raw;

    // Collapse declaration/definition duplicates for callable symbols.
    // The C++ extractor assigns kind=Function to out-of-line definitions
    // (isQualified=true), while in-class declarations get the correct
    // Method/Constructor/Destructor kind.  When a FQN group contains both,
    // keep only the properly-kinded entries to avoid false ambiguity errors.
    std::unordered_set<std::u8string> fqnsWithProperCallable;
    for(const WorkspaceSymbol* p : raw) {
        if(p->symbol.kind == SymbolKind::Method
           || p->symbol.kind == SymbolKind::Constructor
           || p->symbol.kind == SymbolKind::Destructor) {
            fqnsWithProperCallable.insert(std::u8string(p->symbol.fqn));
        }
    }
    if(fqnsWithProperCallable.empty()) return raw;

    std::vector<const WorkspaceSymbol*> result;
    for(const WorkspaceSymbol* p : raw) {
        if(p->symbol.kind == SymbolKind::Function
           && fqnsWithProperCallable.count(std::u8string(p->symbol.fqn))) {
            continue; // misclassified out-of-line definition — skip it
        }
        result.push_back(p);
    }
    return result;
}

bool is_callable(SymbolKind kind)
{
    return kind == SymbolKind::Function
        || kind == SymbolKind::Method
        || kind == SymbolKind::Constructor
        || kind == SymbolKind::Destructor;
}

bool with_resolved_symbol(
    const char8_t* root,
    const char8_t* symbol,
    std::function<bool(Workspace&, const WorkspaceSymbol&)> op)
{
    Workspace ws = open_workspace(root);
    ws.ensure_all_loaded();
    if(ws.parsedCount == 0 && ws.failedCount == 0) {
        std::print(stderr, "error: workspace at '{}' is empty or could not be analyzed\n",
                   reinterpret_cast<const char*>(root));
        return false;
    }

    std::vector<const WorkspaceSymbol*> candidates = resolve_symbol_query(ws, symbol);
    if(candidates.empty()) {
        std::print(stderr, "error: symbol '{}' not found in workspace\n",
                   reinterpret_cast<const char*>(symbol));
        return false;
    }
    if(candidates.size() > 1) {
        std::print(stderr, "error: symbol '{}' is ambiguous ({} matches); use a fully-qualified name (::) to disambiguate:\n",
                   reinterpret_cast<const char*>(symbol),
                   candidates.size());
        for(const WorkspaceSymbol* s : candidates) {
            std::print(stderr, "  {} {} {}:{}\n",
                       getSymbolKindName(s->symbol.kind),
                       (const char*)s->symbol.fqn.c_str(),
                       (const char*)s->sourceFile.u8string().c_str(),
                       s->symbol.line + 1);
        }
        return false;
    }

    return op(ws, *candidates[0]);
}

} // namespace cli
} // namespace ast
