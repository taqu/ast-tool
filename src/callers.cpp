#include "callers.h"
#include "cli-semantic.h"
#include "ast-callers.h"
#include "ast-tool.h"
#include <algorithm>
#include <print>

namespace ast
{
namespace
{
    void print_site_json_pretty(const CallSite& s, bool last)
    {
        std::print(" {{\n");
        if(s.caller) {
            std::print("  \"caller_kind\": \"{}\",\n", getSymbolKindName(s.caller->symbol.kind));
            std::print("  \"caller_fqn\": \"{}\",\n", (const char*)s.caller->symbol.fqn.c_str());
        } else {
            std::print("  \"caller_kind\": \"file_scope\",\n");
            std::print("  \"caller_fqn\": \"\",\n");
        }
        std::print("  \"file\": \"{}\",\n", (const char*)s.sourceFile.u8string().c_str());
        std::print("  \"line\": {},\n", s.line + 1);
        std::print("  \"column\": {}\n", s.column + 1);
        std::print(" }}");
        std::print("{}\n", last ? "" : ",");
    }

    void print_site_json(const CallSite& s, bool last)
    {
        std::print("{{");
        if(s.caller) {
            std::print("\"caller_kind\":\"{}\",", getSymbolKindName(s.caller->symbol.kind));
            std::print("\"caller_fqn\":\"{}\",", (const char*)s.caller->symbol.fqn.c_str());
        } else {
            std::print("\"caller_kind\":\"file_scope\",");
            std::print("\"caller_fqn\":\"\",");
        }
        std::print("\"file\":\"{}\",", (const char*)s.sourceFile.u8string().c_str());
        std::print("\"line\":{},", s.line + 1);
        std::print("\"column\":{}", s.column + 1);
        std::print("}}");
        if(!last) {
            std::print(",");
        }
    }
} // namespace

bool parse_callers(Arguments& arguments, int32_t argc, const char8_t** argv)
{
    arguments.sub_ = SubCommand::Callers;
    ArgCallers& args = arguments.callers_;
    return cli::parse_symbol_root_args(args.symbol_, args.root_, args.json_, args.pretty_, argc, argv);
}

bool callers(const ArgCallers& arguments)
{
    if(arguments.symbol_ == nullptr || arguments.root_ == nullptr) {
        std::print(stderr, "error: missing required arguments: <symbol> <root>\n");
        return false;
    }
    return cli::with_resolved_symbol(arguments.root_, arguments.symbol_,
        [&](Workspace& ws, const WorkspaceSymbol& target) {
            if(!cli::is_callable(target.symbol.kind)) {
                std::print(stderr, "error: '{}' is a {} — callers requires a function, method, constructor, or destructor\n",
                           reinterpret_cast<const char*>(arguments.symbol_),
                           getSymbolKindName(target.symbol.kind));
                return false;
            }

            Callers svc(ws);
            std::vector<CallSite> sites = svc.find(target);

            std::stable_sort(sites.begin(), sites.end(), [](const CallSite& a, const CallSite& b) {
                if(a.sourceFile != b.sourceFile) return a.sourceFile < b.sourceFile;
                const std::u8string& fa = a.caller ? a.caller->symbol.fqn : std::u8string{};
                const std::u8string& fb = b.caller ? b.caller->symbol.fqn : std::u8string{};
                if(fa != fb) return fa < fb;
                if(a.line != b.line) return a.line < b.line;
                return a.column < b.column;
            });

            if(arguments.json_ || arguments.pretty_) {
                cli::print_json_array(sites, arguments.pretty_, print_site_json, print_site_json_pretty);
            } else {
                for(const CallSite& s : sites) {
                    const char* callerFqn = s.caller
                        ? (const char*)s.caller->symbol.fqn.c_str()
                        : "<file_scope>";
                    std::print("{} {}:{}:{}\n",
                               callerFqn,
                               (const char*)s.sourceFile.u8string().c_str(),
                               s.line + 1,
                               s.column + 1);
                }
            }
            return true;
        });
}
} // namespace ast
