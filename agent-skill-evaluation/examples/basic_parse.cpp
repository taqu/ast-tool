#include "parser/lexer.hpp"
#include "parser/parser.hpp"
#include "ast/visitor.hpp"
#include "ast/node.hpp"
#include <iostream>
#include <string>

int main() {
    const std::string program = R"(
int add(int a, int b) {
    return a;
}

void greet(int count) {
    if (count) {
        int result;
        add(count);
    }
}
)";

    std::cout << "=== Source ===\n" << program << "\n";
    std::cout << "=== AST ===\n";

    try {
        eval::parser::Lexer lexer(program, "example.c");
        eval::parser::Parser parser(std::move(lexer));
        auto tu = parser.parseTranslationUnit();

        eval::ast::PrintVisitor printer(std::cout);
        printer.visitNode(*tu);

        // Demonstrate CollectVisitor
        eval::ast::CollectVisitor collector(eval::ast::NodeKind::FunctionDecl);
        collector.visitNode(*tu);

        std::cout << "\n=== Functions found: " << collector.collected().size() << " ===\n";
        for (const auto* node : collector.collected()) {
            std::cout << "  " << node->toString() << "\n";
        }
    } catch (const eval::parser::ParseError& e) {
        std::cerr << "Parse error at " << e.line() << ":" << e.column()
                  << ": " << e.what() << "\n";
        return 1;
    }

    return 0;
}
