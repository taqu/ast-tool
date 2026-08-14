#include "parser/lexer.hpp"
#include "parser/parser.hpp"
#include "semantic/analyzer.hpp"
#include "semantic/symbol.hpp"
#include "workspace/index.hpp"
#include "ast/node.hpp"
#include <iostream>
#include <string>

static void printSymbol(const eval::semantic::Symbol& sym) {
    std::cout << "  [" << sym.name << "] kind=";
    switch (sym.kind) {
        case eval::semantic::SymbolKind::Function:  std::cout << "Function";  break;
        case eval::semantic::SymbolKind::Variable:  std::cout << "Variable";  break;
        case eval::semantic::SymbolKind::Parameter: std::cout << "Parameter"; break;
        case eval::semantic::SymbolKind::Type:      std::cout << "Type";      break;
    }
    std::cout << " type=" << sym.type
              << " qualifiedName=" << sym.qualifiedName
              << " refs=" << sym.references.size()
              << "\n";
}

int main() {
    const std::string program = R"(
int counter;

int increment(int value) {
    int result;
    return result;
}

void run() {
    counter = increment(counter);
}
)";

    std::cout << "=== Source ===\n" << program << "\n";

    eval::parser::Lexer lexer(program, "example2.c");
    eval::parser::Parser parser(std::move(lexer));
    std::shared_ptr<eval::ast::TranslationUnit> tu;
    try {
        tu = parser.parseTranslationUnit();
    } catch (const eval::parser::ParseError& e) {
        std::cerr << "Parse error: " << e.what() << "\n";
        return 1;
    }

    eval::semantic::Analyzer analyzer;
    analyzer.analyze(*tu);

    std::cout << "=== Semantic Errors ===\n";
    if (analyzer.errors().empty()) {
        std::cout << "  (none)\n";
    } else {
        for (const auto& err : analyzer.errors()) {
            std::cout << "  " << err.line << ":" << err.column << ": " << err.message << "\n";
        }
    }

    std::cout << "\n=== Global Symbols ===\n";
    for (const auto& [name, sym] : analyzer.globalScope()->symbols()) {
        printSymbol(sym);
    }

    // Build a workspace index
    eval::workspace::Index index;
    eval::workspace::FileEntry entry;
    entry.path = "example2.c";
    entry.includes = {};
    for (const auto& [name, sym] : analyzer.globalScope()->symbols()) {
        entry.symbols.push_back(sym);
    }
    index.addFile(std::move(entry));

    // Use the template findSymbolIf to find all functions
    std::cout << "\n=== Functions in index ===\n";
    auto functions = index.findSymbolIf([](const eval::semantic::Symbol& s) {
        return s.kind == eval::semantic::SymbolKind::Function;
    });
    for (const auto* sym : functions) {
        std::cout << "  " << sym->qualifiedName << " -> " << sym->type << "\n";
    }

    // Dependencies
    std::cout << "\n=== Dependencies of example2.c ===\n";
    auto deps = index.dependenciesOf("example2.c");
    if (deps.empty()) {
        std::cout << "  (none)\n";
    }
    for (const auto& d : deps) {
        std::cout << "  " << d << "\n";
    }

    return 0;
}
