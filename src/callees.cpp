#include "callees.h"
#include "cli-semantic.h"
#include "ast-callees.h"
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
        if(s.callee) {
            std::print("  \"callee_kind\": \"{}\",\n", getSymbolKindName(s.callee->symbol.kind));
            std::print("  \"callee_fqn\": \"{}\",\n", (const char*)s.callee->symbol.fqn.c_str());
        } else {
            std::print("  \"callee_kind\": \"unknown\",\n");
            std::print("  \"callee_fqn\": \"\",\n");
        }
        std::print("  \"file\": \"{}\",\n", cli::json_escape(s.sourceFile.u8string()));
        std::print("  \"line\": {},\n", s.line + 1);
        std::print("  \"column\": {}\n", s.column + 1);
        std::print(" }}");
        std::print("{}\n", last ? "" : ",");
    }

    void print_site_json(const CallSite& s, bool last)
    {
        std::print("{{");
        if(s.callee) {
            std::print("\"callee_kind\":\"{}\",", getSymbolKindName(s.callee->symbol.kind));
            std::print("\"callee_fqn\":\"{}\",", (const char*)s.callee->symbol.fqn.c_str());
        } else {
            std::print("\"callee_kind\":\"unknown\",");
            std::print("\"callee_fqn\":\"\",");
        }
        std::print("\"file\":\"{}\",", cli::json_escape(s.sourceFile.u8string()));
        std::print("\"line\":{},", s.line + 1);
        std::print("\"column\":{}", s.column + 1);
        std::print("}}");
        if(!last) {
            std::print(",");
        }
    }
} // namespace

bool parse_callees(Arguments& arguments, int32_t argc, const char8_t** argv)
{
    arguments.sub_ = SubCommand::Callees;
    ArgCallees& args = arguments.callees_;
    cli::parse_symbol_root_args(args.symbol_, args.root_, args.json_, args.pretty_, args.badOption_, argc, argv);
    return true; // Defer missing/bad-argument reporting to callees() for an actionable diagnostic.
}

bool callees(const ArgCallees& arguments)
{
    if(arguments.badOption_ != nullptr) {
        cli::print_error(
            cli::make_unknown_option(arguments.badOption_, {u8"--json", u8"--pretty"}),
            arguments.json_, arguments.pretty_);
        return false;
    }
    if(arguments.symbol_ == nullptr || arguments.root_ == nullptr) {
        cli::print_error(
            cli::make_invalid_arguments(u8"missing required arguments: <symbol> <root>", u8"ast-tool callees <symbol> <root>"),
            arguments.json_, arguments.pretty_);
        return false;
    }
    return cli::with_resolved_symbol(arguments.root_, arguments.symbol_, arguments.json_, arguments.pretty_,
        [&](Workspace& ws, const WorkspaceSymbol& target) {
            if(!cli::is_callable(target.symbol.kind)) {
                std::u8string message = std::u8string(u8"'") + target.symbol.fqn + u8"' is a "
                    + reinterpret_cast<const char8_t*>(getSymbolKindName(target.symbol.kind))
                    + u8" — callees requires a function, method, constructor, or destructor";
                cli::print_error(
                    cli::make_unsupported_query_form(message, u8"inspect with search or symbols"),
                    arguments.json_, arguments.pretty_);
                return false;
            }

            Callees svc;
            std::vector<CallSite> sites = svc.find(ws, target);

            std::stable_sort(sites.begin(), sites.end(), [](const CallSite& a, const CallSite& b) {
                if(a.sourceFile != b.sourceFile) return a.sourceFile < b.sourceFile;
                if(a.line != b.line) return a.line < b.line;
                if(a.column != b.column) return a.column < b.column;
                const std::u8string& fa = a.callee ? a.callee->symbol.fqn : std::u8string{};
                const std::u8string& fb = b.callee ? b.callee->symbol.fqn : std::u8string{};
                return fa < fb;
            });

            if(arguments.json_ || arguments.pretty_) {
                cli::print_json_array(sites, arguments.pretty_, print_site_json, print_site_json_pretty);
            } else {
                if(sites.empty()) {
                    // A resolved, callable symbol with zero call sites is a valid empty
                    // result, not a failure — distinguish it from symbol resolution errors.
                    std::print(stderr, "note: no callees found for: {}\n", (const char*)target.symbol.fqn.c_str());
                } else {
                    for(const CallSite& s : sites) {
                        const char* calleeFqn = s.callee
                            ? (const char*)s.callee->symbol.fqn.c_str()
                            : "<unresolved>";
                        std::print("{} {}:{}:{}\n",
                                   calleeFqn,
                                   (const char*)s.sourceFile.u8string().c_str(),
                                   s.line + 1,
                                   s.column + 1);
                    }
                }
            }
            return true;
        });
}
} // namespace ast
