#pragma once

#include "symbol.hpp"
#include "../ast/visitor.hpp"
#include "../ast/node.hpp"
#include <vector>
#include <memory>
#include <string>

namespace eval::semantic {

struct SemanticError {
    std::string message;
    int line;
    int column;
};

class Analyzer : public ast::Visitor {
public:
    Analyzer();

    void analyze(const ast::Node& root);

    const std::vector<SemanticError>& errors() const { return errors_; }
    Scope* globalScope() { return globalScope_.get(); }
    const Scope* globalScope() const { return globalScope_.get(); }

    void visitNode(const ast::Node& node) override;
    void visitFunctionDecl(const ast::FunctionDecl& node) override;
    void visitVarDecl(const ast::VarDecl& node) override;
    void visitCallExpr(const ast::CallExpr& node) override;
    void visitIfStmt(const ast::IfStmt& node) override;
    void visitLiteral(const ast::Literal& node) override;

private:
    void pushScope(const std::string& name);
    void popScope();
    void addError(const std::string& message, int line, int column);

    std::unique_ptr<Scope> globalScope_;
    std::vector<std::unique_ptr<Scope>> ownedScopes_;
    Scope* currentScope_;
    std::vector<SemanticError> errors_;
};

} // namespace eval::semantic
