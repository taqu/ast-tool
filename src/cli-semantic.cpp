#include "cli-semantic.h"
#include <algorithm>
#include <cstring>
#include <print>
#include <unordered_set>

namespace ast
{
namespace cli
{

bool looks_like_option(const char8_t* arg)
{
    std::u8string_view sv{arg};
    return sv.size() > 2 && sv[0] == u8'-' && sv[1] == u8'-';
}

bool parse_symbol_root_args(
    const char8_t*& symbol,
    const char8_t*& root,
    bool& json,
    bool& pretty,
    const char8_t*& badOption,
    int32_t argc,
    const char8_t** argv)
{
#define STREQUALS(str, literal) (0 == ::strcmp(reinterpret_cast<const char*>(str), literal))
    symbol = nullptr;
    root   = nullptr;
    json   = false;
    pretty = false;
    badOption = nullptr;

    for(int32_t i = 2; i < argc; ++i) {
        if(STREQUALS(argv[i], "--json")) {
            json = true;
            continue;
        }
        if(STREQUALS(argv[i], "--pretty")) {
            pretty = true;
            continue;
        }
        if(looks_like_option(argv[i])) {
            if(badOption == nullptr) {
                badOption = argv[i];
            }
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
    if(byFqn) {
        // Preserve exact-FQN precedence.  Only when no exact identity exists
        // may a partially qualified relationship target fall back to a
        // qualified-name-boundary suffix match.
        for(const WorkspaceSymbol& sym : ws.symbols) {
            if(sym.symbol.fqn == q) {
                raw.push_back(&sym);
            }
        }
        if(raw.empty()) {
            std::u8string suffix = u8"::";
            suffix += q;
            for(const WorkspaceSymbol& sym : ws.symbols) {
                if(std::u8string_view{sym.symbol.fqn}.ends_with(suffix)) {
                    raw.push_back(&sym);
                }
            }
        }
    } else {
        for(const WorkspaceSymbol& sym : ws.symbols) {
            if(sym.symbol.name == q) {
                raw.push_back(&sym);
            }
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

bool workspace_has_content(const Workspace& ws, const char8_t* root, bool json, bool pretty)
{
    if(ws.parsedCount != 0 || ws.failedCount != 0) {
        return true;
    }
    RecoveryError err;
    err.category = ErrorCategory::InvalidArguments;
    err.message = std::u8string(u8"workspace at '") + root + u8"' is empty or could not be analyzed";
    err.next = u8"verify the workspace root path";
    print_error(err, json, pretty);
    return false;
}

bool with_resolved_symbol(
    const char8_t* root,
    const char8_t* symbol,
    bool json,
    bool pretty,
    std::function<bool(Workspace&, const WorkspaceSymbol&)> op)
{
    Workspace ws = open_workspace(root);
    ws.ensure_all_loaded();
    if(!workspace_has_content(ws, root, json, pretty)) {
        return false;
    }

    std::vector<const WorkspaceSymbol*> candidates = resolve_symbol_query(ws, symbol);
    if(candidates.empty()) {
        print_error(make_symbol_not_found(symbol, root), json, pretty);
        return false;
    }
    if(candidates.size() > 1) {
        print_error(make_ambiguous_symbol(symbol, candidates), json, pretty);
        return false;
    }

    return op(ws, *candidates[0]);
}

// ---------------------------------------------------------------------------
// Error Recovery UX (Phase 5)

const char* error_category_token(ErrorCategory category)
{
    switch(category) {
    case ErrorCategory::SymbolNotFound:       return "symbol_not_found";
    case ErrorCategory::AmbiguousSymbol:      return "ambiguous_symbol";
    case ErrorCategory::UnknownOption:        return "unknown_option";
    case ErrorCategory::InvalidArguments:     return "invalid_arguments";
    case ErrorCategory::InvalidQuery:         return "invalid_query";
    case ErrorCategory::UnsupportedQueryForm: return "unsupported_query_form";
    case ErrorCategory::InternalError:        return "internal_error";
    }
    return "internal_error";
}

const char* error_category_label(ErrorCategory category)
{
    switch(category) {
    case ErrorCategory::SymbolNotFound:       return "symbol not found";
    case ErrorCategory::AmbiguousSymbol:      return "ambiguous symbol";
    case ErrorCategory::UnknownOption:        return "unknown option";
    case ErrorCategory::InvalidArguments:     return "invalid arguments";
    case ErrorCategory::InvalidQuery:         return "invalid query";
    case ErrorCategory::UnsupportedQueryForm: return "unsupported query";
    case ErrorCategory::InternalError:        return "internal error";
    }
    return "internal error";
}

RecoveryError make_symbol_not_found(std::u8string_view query, std::u8string_view root)
{
    RecoveryError err;
    err.category = ErrorCategory::SymbolNotFound;
    err.query = std::u8string(query);

    // A query that already looks over-qualified is more likely to fail on an
    // exact FQN match; suggest re-searching just the last path component.
    std::u8string_view shortName = query;
    if(size_t pos = query.rfind(u8"::"); pos != std::u8string_view::npos) {
        shortName = query.substr(pos + 2);
    }
    err.next = u8"search " + std::u8string(shortName) + u8" " + std::u8string(root);
    return err;
}

RecoveryError make_ambiguous_symbol(std::u8string_view query, const std::vector<const WorkspaceSymbol*>& candidates)
{
    RecoveryError err;
    err.category = ErrorCategory::AmbiguousSymbol;
    err.query = std::u8string(query);
    err.totalCandidates = candidates.size();

    size_t shown = std::min(candidates.size(), kMaxErrorCandidates);
    err.candidates.reserve(shown);
    for(size_t i = 0; i < shown; ++i) {
        const WorkspaceSymbol* s = candidates[i];
        ErrorCandidate c;
        c.kind = reinterpret_cast<const char8_t*>(getSymbolKindName(s->symbol.kind));
        c.fqn  = s->symbol.fqn;
        c.file = s->sourceFile.u8string();
        c.line = s->symbol.line + 1;
        err.candidates.push_back(std::move(c));
    }

    bool alreadyQualified = query.find(u8"::") != std::u8string_view::npos;
    err.next = alreadyQualified
        ? u8"inspect candidates with search or symbols"
        : u8"retry with a fully-qualified name";
    return err;
}

RecoveryError make_unknown_option(std::u8string_view option, std::vector<std::u8string> validOptions)
{
    RecoveryError err;
    err.category = ErrorCategory::UnknownOption;
    err.query = std::u8string(option);
    err.validOptions = std::move(validOptions);
    return err;
}

RecoveryError make_invalid_arguments(std::u8string_view message, std::u8string_view usage)
{
    RecoveryError err;
    err.category = ErrorCategory::InvalidArguments;
    err.message = std::u8string(message);
    err.usage = std::u8string(usage);
    return err;
}

RecoveryError make_invalid_query(std::u8string_view message)
{
    RecoveryError err;
    err.category = ErrorCategory::InvalidQuery;
    err.message = std::u8string(message);
    return err;
}

RecoveryError make_unsupported_query_form(std::u8string_view message, std::u8string_view next)
{
    RecoveryError err;
    err.category = ErrorCategory::UnsupportedQueryForm;
    err.message = std::u8string(message);
    err.next = std::u8string(next);
    return err;
}

RecoveryError make_internal_error(std::u8string_view message)
{
    RecoveryError err;
    err.category = ErrorCategory::InternalError;
    err.message = std::u8string(message);
    return err;
}

std::string render_error_text(const RecoveryError& err)
{
    std::string out = "error: ";
    switch(err.category) {
    case ErrorCategory::SymbolNotFound:
        out += "symbol not found: ";
        out += reinterpret_cast<const char*>(err.query.c_str());
        out += "\n";
        break;
    case ErrorCategory::AmbiguousSymbol:
        out += "ambiguous symbol: ";
        out += reinterpret_cast<const char*>(err.query.c_str());
        out += "\n";
        if(!err.candidates.empty()) {
            out += "candidates:\n";
            for(const ErrorCandidate& c : err.candidates) {
                out += "  ";
                out += reinterpret_cast<const char*>(c.kind.c_str());
                out += " ";
                out += reinterpret_cast<const char*>(c.fqn.c_str());
                out += "  ";
                out += reinterpret_cast<const char*>(c.file.c_str());
                out += ":";
                out += std::to_string(c.line);
                out += "\n";
            }
            if(err.totalCandidates > err.candidates.size()) {
                out += "showing " + std::to_string(err.candidates.size()) + " of "
                     + std::to_string(err.totalCandidates) + " candidates\n";
            }
        }
        break;
    case ErrorCategory::UnknownOption:
        out += "unknown option: ";
        out += reinterpret_cast<const char*>(err.query.c_str());
        out += "\n";
        if(!err.validOptions.empty()) {
            out += "available: ";
            for(size_t i = 0; i < err.validOptions.size(); ++i) {
                if(i) out += ", ";
                out += reinterpret_cast<const char*>(err.validOptions[i].c_str());
            }
            out += "\n";
        }
        break;
    case ErrorCategory::InvalidArguments:
        out += reinterpret_cast<const char*>(err.message.c_str());
        out += "\n";
        if(!err.usage.empty()) {
            out += "usage: ";
            out += reinterpret_cast<const char*>(err.usage.c_str());
            out += "\n";
        }
        break;
    case ErrorCategory::InvalidQuery:
        out += "invalid query: ";
        out += reinterpret_cast<const char*>(err.message.c_str());
        out += "\n";
        break;
    case ErrorCategory::UnsupportedQueryForm:
        out += reinterpret_cast<const char*>(err.message.c_str());
        out += "\n";
        break;
    case ErrorCategory::InternalError:
        out += "internal error: ";
        out += reinterpret_cast<const char*>(err.message.c_str());
        out += "\n";
        break;
    }
    // Unknown-option already lists its own recovery ("available: ..."); a
    // generic "next:" would be redundant there.
    if(!err.next.empty() && err.category != ErrorCategory::UnknownOption) {
        out += "next: ";
        out += reinterpret_cast<const char*>(err.next.c_str());
        out += "\n";
    }
    return out;
}

std::string render_error_json(const RecoveryError& err, bool pretty)
{
    std::string nl = pretty ? "\n" : "";
    std::string ind = pretty ? "  " : "";
    std::string sp = pretty ? " " : "";

    std::vector<std::string> fields;
    fields.push_back("\"error\":" + sp + "\"" + error_category_token(err.category) + "\"");

    if(!err.query.empty()) {
        fields.push_back("\"query\":" + sp + "\"" + json_escape(err.query) + "\"");
    }
    if(!err.message.empty()) {
        fields.push_back("\"message\":" + sp + "\"" + json_escape(err.message) + "\"");
    }
    if(!err.candidates.empty()) {
        std::string arr = "[";
        for(size_t i = 0; i < err.candidates.size(); ++i) {
            const ErrorCandidate& c = err.candidates[i];
            arr += "{\"kind\":\"" + json_escape(c.kind) + "\","
                 + "\"fqn\":\"" + json_escape(c.fqn) + "\","
                 + "\"file\":\"" + json_escape(c.file) + "\","
                 + "\"line\":" + std::to_string(c.line) + "}";
            if(i + 1 != err.candidates.size()) arr += ",";
        }
        arr += "]";
        fields.push_back("\"candidates\":" + sp + arr);
        if(err.totalCandidates > err.candidates.size()) {
            fields.push_back("\"total_candidates\":" + sp + std::to_string(err.totalCandidates));
        }
    }
    if(!err.usage.empty()) {
        fields.push_back("\"usage\":" + sp + "\"" + json_escape(err.usage) + "\"");
    }
    if(!err.validOptions.empty()) {
        std::string arr = "[";
        for(size_t i = 0; i < err.validOptions.size(); ++i) {
            arr += "\"" + json_escape(err.validOptions[i]) + "\"";
            if(i + 1 != err.validOptions.size()) arr += ",";
        }
        arr += "]";
        fields.push_back("\"valid_options\":" + sp + arr);
    }
    if(!err.next.empty()) {
        fields.push_back("\"next\":" + sp + "\"" + json_escape(err.next) + "\"");
    }

    std::string out = "{" + nl;
    for(size_t i = 0; i < fields.size(); ++i) {
        out += ind + fields[i];
        if(i + 1 != fields.size()) out += ",";
        out += nl;
    }
    out += "}\n";
    return out;
}

void print_error(const RecoveryError& err, bool json, bool pretty)
{
    std::string rendered = json ? render_error_json(err, pretty) : render_error_text(err);
    std::print(stderr, "{}", rendered);
}

} // namespace cli
} // namespace ast
