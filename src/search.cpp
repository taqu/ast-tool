#include "search.h"
#include <cstring>
#include <print>
#include "ast-extractor.h"
#include "ast-scope.h"
#include "ast-search.h"
#include "ast-tool.h"
#include "ast-workspace.h"

namespace ast
{
namespace
{
    void print_result_json_pretty(const SearchResult& r, bool last)
    {
        std::print(" {{\n");
        std::print("  \"kind\": \"{}\",\n",        getSymbolKindName(r.kind));
        std::print("  \"name\": \"{}\",\n",        r.name);
        std::print("  \"fqn\": \"{}\",\n",         r.fqn);
        std::print("  \"file\": \"{}\",\n",        r.sourceFile);
        std::print("  \"line\": {},\n",            r.line + 1);
        std::print("  \"column\": {},\n",          r.column + 1);
        std::print("  \"owning_scope\": \"{}\"\n", getScopeKindName(r.owningScope));
        std::print(" }}");
        std::print("{}\n", last ? "" : ",");
    }

    void print_result_json(const SearchResult& r, bool last)
    {
        std::print("{{");
        std::print("\"kind\":\"{}\",",        getSymbolKindName(r.kind));
        std::print("\"name\":\"{}\",",        r.name);
        std::print("\"fqn\":\"{}\",",         r.fqn);
        std::print("\"file\":\"{}\",",        r.sourceFile);
        std::print("\"line\":{},",            r.line + 1);
        std::print("\"column\":{},",          r.column + 1);
        std::print("\"owning_scope\":\"{}\"", getScopeKindName(r.owningScope));
        std::print("}}");
        if(!last) {
            std::print(",");
        }
    }
} // namespace

bool parse_search(Arguments& arguments, int32_t argc, const char** argv)
{
#define STREQUALS(str, literal) (0 == ::strcmp(str, literal))
    arguments.sub_ = SubCommand::Search;
    ArgSearch& args = arguments.search_;
    args.root_        = nullptr;
    args.name_        = nullptr;
    args.fqn_         = nullptr;
    args.kind_        = nullptr;
    args.file_        = nullptr;
    args.name_regex_  = nullptr;
    args.fqn_regex_   = nullptr;
    args.file_regex_  = nullptr;
    args.json_        = false;
    args.pretty_      = false;

    for(int32_t i = 2; i < argc; ++i) {
        if(STREQUALS(argv[i], "--name")) {
            if((i + 1) < argc) args.name_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--fqn")) {
            if((i + 1) < argc) args.fqn_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--kind")) {
            if((i + 1) < argc) args.kind_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--file")) {
            if((i + 1) < argc) args.file_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--name-regex")) {
            if((i + 1) < argc) args.name_regex_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--fqn-regex")) {
            if((i + 1) < argc) args.fqn_regex_ = argv[++i];
            continue;
        }
        if(STREQUALS(argv[i], "--file-regex")) {
            if((i + 1) < argc) args.file_regex_ = argv[++i];
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
        std::print(stderr, "error: {}\n", q.error());
        return false;
    }

    Workspace ws = analyze_workspace(arguments.root_);
    SemanticSearchEngine engine(ws);
    std::vector<SearchResult> results = engine.search(*q);

    if(arguments.json_) {
        std::print("[");
        if(arguments.pretty_) {
            std::print("\n");
        }
        for(size_t i = 0; i < results.size(); ++i) {
            bool last = (i == results.size() - 1);
            if(arguments.pretty_) {
                print_result_json_pretty(results[i], last);
            } else {
                print_result_json(results[i], last);
            }
        }
        std::print("]\n");
    } else {
        for(const SearchResult& r : results) {
            std::print("{} {} {}:{}:{}\n",
                getSymbolKindName(r.kind),
                r.fqn,
                r.sourceFile,
                r.line + 1,
                r.column + 1);
        }
    }
    return true;
}
} // namespace ast
