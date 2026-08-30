#include "search.h"
#include "ast-extractor.h"
#include "ast-scope.h"
#include "ast-search.h"
#include "ast-tool.h"
#include "ast-workspace.h"
#include "cli-semantic.h"
#include <charconv>
#include <cstring>
#include <print>

namespace ast
{
namespace
{
    void print_result_json_pretty(const WorkspaceSymbol& r, bool last)
    {
        std::print(" {{\n");
        std::print("  \"kind\": \"{}\",\n", getSymbolKindName(r.symbol.kind));
        std::print("  \"name\": \"{}\",\n", (const char*)r.symbol.name.c_str());
        std::print("  \"fqn\": \"{}\",\n", (const char*)r.symbol.fqn.c_str());
        std::print("  \"file\": \"{}\",\n", cli::json_escape(r.sourceFile.u8string()));
        std::print("  \"line\": {},\n", r.symbol.line + 1);
        std::print("  \"column\": {},\n", r.symbol.column + 1);
        std::print("  \"owning_scope\": \"{}\"\n", getScopeKindName(r.owningScope));
        std::print(" }}");
        std::print("{}\n", last ? "" : ",");
    }

    void print_result_json(const WorkspaceSymbol& r, bool last)
    {
        std::print("{{");
        std::print("\"kind\":\"{}\",", getSymbolKindName(r.symbol.kind));
        std::print("\"name\":\"{}\",", (const char*)r.symbol.name.c_str());
        std::print("\"fqn\":\"{}\",", (const char*)r.symbol.fqn.c_str());
        std::print("\"file\":\"{}\",", cli::json_escape(r.sourceFile.u8string()));
        std::print("\"line\":{},", r.symbol.line + 1);
        std::print("\"column\":{},", r.symbol.column + 1);
        std::print("\"owning_scope\":\"{}\"", getScopeKindName(r.owningScope));
        std::print("}}");
        if(!last) {
            std::print(",");
        }
    }

} // namespace

bool parse_search(Arguments& arguments, int32_t argc, const char8_t** argv)
{
#define STREQUALS(str, literal) (0 == ::strcmp(reinterpret_cast<const char*>(str), literal))
    arguments.sub_ = SubCommand::Search;
    ArgSearch& args = arguments.search_;
    args.root_ = nullptr;
    args.name_ = nullptr;
    args.fqn_ = nullptr;
    args.kind_ = nullptr;
    args.file_ = nullptr;
    args.name_regex_ = nullptr;
    args.fqn_regex_ = nullptr;
    args.file_regex_ = nullptr;
    args.json_ = false;
    args.pretty_ = false;
    args.limit_ = 0;

    for(int32_t i = 2; i < argc; ++i) {
        if(STREQUALS(argv[i], "--name")) {
            if((i + 1) < argc)
                args.name_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--fqn")) {
            if((i + 1) < argc)
                args.fqn_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--kind")) {
            if((i + 1) < argc)
                args.kind_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--file")) {
            if((i + 1) < argc)
                args.file_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--name-regex")) {
            if((i + 1) < argc)
                args.name_regex_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--fqn-regex")) {
            if((i + 1) < argc)
                args.fqn_regex_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--file-regex")) {
            if((i + 1) < argc)
                args.file_regex_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--json")) {
            args.json_ = true;
            continue;
        }
        if(STREQUALS(argv[i], "--pretty")) {
            args.pretty_ = true;
            continue;
        }
        if(STREQUALS(argv[i], "--limit")) {
            if((i + 1) < argc) {
                const char* s = reinterpret_cast<const char*>(argv[++i]);
                uint32_t v = 0;
                auto [ptr, ec] = std::from_chars(s, s + std::strlen(s), v);
                if(ec == std::errc{}) {
                    args.limit_ = v;
                }
            }
            continue;
        }
        args.root_ = argv[i];
    }
    return args.root_ != nullptr;
#undef STREQUALS
}

bool search(const ArgSearch& arguments)
{
    if(nullptr == arguments.root_) {
        return false;
    }

    auto q = build_search_query(
        arguments.name_,
        arguments.fqn_,
        arguments.kind_,
        arguments.file_,
        arguments.name_regex_,
        arguments.fqn_regex_,
        arguments.file_regex_);
    if(!q) {
        std::print(stderr, "error: {}\n", (const char*)q.error().c_str());
        return false;
    }

    Workspace ws = open_workspace(arguments.root_);
    ws.ensure_all_loaded();
    SemanticSearchEngine engine(ws);
    std::vector<const WorkspaceSymbol*> results = engine.search(*q);

    if(arguments.limit_ > 0 && results.size() > arguments.limit_) {
        std::print(stderr, "note: {} results, showing first {} (--limit {})\n",
                   results.size(), arguments.limit_, arguments.limit_);
        results.resize(arguments.limit_);
    }

    if(arguments.json_) {
        std::print("[");
        if(arguments.pretty_) {
            std::print("\n");
        }
        for(size_t i = 0; i < results.size(); ++i) {
            bool last = (i == results.size() - 1);
            if(arguments.pretty_) {
                print_result_json_pretty(*results[i], last);
            } else {
                print_result_json(*results[i], last);
            }
        }
        std::print("]\n");
    } else {
        for(const WorkspaceSymbol* r: results) {
            std::print("{} {} {}:{}:{}\n",
                       getSymbolKindName(r->symbol.kind),
                       (const char*)r->symbol.fqn.c_str(),
                       (const char*)r->sourceFile.u8string().c_str(),
                       r->symbol.line + 1,
                       r->symbol.column + 1);
        }
    }
    return true;
}
} // namespace ast
