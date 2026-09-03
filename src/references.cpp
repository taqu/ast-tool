#include "references.h"
#include "cli-semantic.h"
#include "ast-references.h"
#include "ast-tool.h"
#include <algorithm>
#include <print>

namespace ast
{
namespace
{
    void print_ref_json_pretty(const ReferenceResult& r, bool last)
    {
        std::print(" {{\n");
        std::print("  \"file\": \"{}\",\n", cli::json_escape(r.sourceFile.u8string()));
        std::print("  \"line\": {},\n", r.line + 1);
        std::print("  \"column\": {},\n", r.column + 1);
        std::print("  \"owning_scope\": \"{}\"\n", getScopeKindName(r.owningScope));
        std::print(" }}");
        std::print("{}\n", last ? "" : ",");
    }

    void print_ref_json(const ReferenceResult& r, bool last)
    {
        std::print("{{");
        std::print("\"file\":\"{}\",", cli::json_escape(r.sourceFile.u8string()));
        std::print("\"line\":{},", r.line + 1);
        std::print("\"column\":{},", r.column + 1);
        std::print("\"owning_scope\":\"{}\"", getScopeKindName(r.owningScope));
        std::print("}}");
        if(!last) {
            std::print(",");
        }
    }
} // namespace

bool parse_references(Arguments& arguments, int32_t argc, const char8_t** argv)
{
    arguments.sub_ = SubCommand::References;
    ArgReferences& args = arguments.references_;
    cli::parse_symbol_root_args(args.symbol_, args.root_, args.json_, args.pretty_, args.badOption_, argc, argv);
    return true; // Defer missing/bad-argument reporting to references() for an actionable diagnostic.
}

bool references(const ArgReferences& arguments)
{
    if(arguments.badOption_ != nullptr) {
        cli::print_error(
            cli::make_unknown_option(arguments.badOption_, {u8"--json", u8"--pretty"}),
            arguments.json_, arguments.pretty_);
        return false;
    }
    if(arguments.symbol_ == nullptr || arguments.root_ == nullptr) {
        cli::print_error(
            cli::make_invalid_arguments(u8"missing required arguments: <symbol> <root>", u8"ast-tool references <symbol> <root>"),
            arguments.json_, arguments.pretty_);
        return false;
    }
    return cli::with_resolved_symbol(arguments.root_, arguments.symbol_, arguments.json_, arguments.pretty_,
        [&](Workspace& ws, const WorkspaceSymbol& target) {
            FindReferences finder(ws);
            std::vector<ReferenceResult> refs = finder.find(target);

            std::stable_sort(refs.begin(), refs.end(), [](const ReferenceResult& a, const ReferenceResult& b) {
                if(a.sourceFile != b.sourceFile) return a.sourceFile < b.sourceFile;
                if(a.line != b.line) return a.line < b.line;
                return a.column < b.column;
            });

            if(arguments.json_ || arguments.pretty_) {
                cli::print_json_array(refs, arguments.pretty_, print_ref_json, print_ref_json_pretty);
            } else {
                if(refs.empty()) {
                    // A resolved symbol with zero references is a valid empty result,
                    // not a failure — distinguish it from symbol resolution errors.
                    std::print(stderr, "note: no references found for: {}\n", (const char*)target.symbol.fqn.c_str());
                } else {
                    for(const ReferenceResult& r : refs) {
                        std::print("{}:{}:{}\n",
                                   (const char*)r.sourceFile.u8string().c_str(),
                                   r.line + 1,
                                   r.column + 1);
                    }
                }
            }
            return true;
        });
}
} // namespace ast
