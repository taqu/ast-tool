#include "symbols.h"
#include "ast-extractor.h"
#include "ast-tool.h"
#include "cli-semantic.h"
#include <print>
#include <string.h>

namespace ast
{
bool parse_symbols(Arguments& arguments, int32_t argc, const char8_t** argv)
{
#define STREQUALS(str, literal) (0 == ::strncmp(str, literal, ::strlen(literal)))
    arguments.sub_ = SubCommand::Symbols;
    arguments.symbols_.input_ = nullptr;
    arguments.symbols_.json_ = false;
    arguments.symbols_.pretty_ = false;
    for(int32_t i = 2; i < argc; ++i) {
        if(STREQUALS(reinterpret_cast<const char*>(argv[i]), "--json")) {
            arguments.symbols_.json_ = true;
            continue;
        }
        if(STREQUALS(reinterpret_cast<const char*>(argv[i]), "--pretty")) {
            arguments.symbols_.pretty_ = true;
            continue;
        }
        arguments.symbols_.input_ = argv[i];
    }
    return true; // Defer a missing FILE to symbols() for an actionable diagnostic.
#undef STREQUALS
}

bool symbols(const ArgSymbols& arguments)
{
    if(nullptr == arguments.input_) {
        cli::print_error(
            cli::make_invalid_arguments(u8"missing FILE", u8"ast-tool symbols [--json [--pretty]] <file>"),
            arguments.json_, arguments.pretty_);
        return false;
    }
    AST ast = parse(arguments.input_);
    if(!ast) {
        cli::print_error(
            cli::make_invalid_arguments(u8"could not parse file", u8"ast-tool symbols [--json [--pretty]] <file>"),
            arguments.json_, arguments.pretty_);
        return false;
    }
    std::vector<ast::Symbol> symbols = ast::extract_symbols(ast);
    if(arguments.json_) {
        if(arguments.pretty_) {
            std::print("{{\n");
            for(size_t i = 0; i < symbols.size(); ++i) {
                std::print(" {{\n");
                std::print("  \"kind\": \"{}\",\n", getSymbolKindName(symbols[i].kind));
                std::print("  \"name\": \"{}\",\n", (const char*)symbols[i].name.c_str());
                std::print("  \"qualified_name\": \"{}\",\n", (const char*)symbols[i].fqn.c_str());
                std::print("  \"access\": \"{}\",\n", getAccessName(symbols[i].access));
                std::print("  \"static\": \"{}\",\n", symbols[i].isStatic);
                std::print("  \"constexpr\": \"{}\",\n", symbols[i].isConstexpr);
                std::print("  \"inline\": \"{}\",\n", symbols[i].isInline);
                std::print("  \"id\": \"{:X}\"\n", ast[symbols[i].nodeIndex].hash_);
                std::print(" }}");
                if(i < (symbols.size() - 1)) {
                    std::print(",\n");
                } else {
                    std::print("\n");
                }
            }
            std::print("}}\n");
        } else {
            std::print("{{");
            for(size_t i = 0; i < symbols.size(); ++i) {
                std::print("{{");
                std::print("\"kind\":\"{}\",", getSymbolKindName(symbols[i].kind));
                std::print("\"name\":\"{}\",", (const char*)symbols[i].name.c_str());
                std::print("\"qualified_name\":\"{}\",", (const char*)symbols[i].fqn.c_str());
                std::print("\"access\":\"{}\",", getAccessName(symbols[i].access));
                std::print("\"static\":\"{}\",", symbols[i].isStatic);
                std::print("\"constexpr\":\"{}\",", symbols[i].isConstexpr);
                std::print("\"inline\":\"{}\",", symbols[i].isInline);
                std::print("\"id\":\"{:X}\"", ast[symbols[i].nodeIndex].hash_);
                std::print("}}");
                if(i < (symbols.size() - 1)) {
                    std::print(",");
                }
            }
            std::print("}}\n");
        }
    } else {
        for(const ast::Symbol& symbol: symbols) {
            std::print("{} {:X}\n", (const char*)symbol.fqn.c_str(), ast[symbol.nodeIndex].hash_);
        }
    }
    return true;
}
} // namespace ast