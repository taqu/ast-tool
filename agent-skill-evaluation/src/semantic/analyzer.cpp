#include "semantic/analyzer.hpp"
#include "ast/node.hpp"
#include <sstream>

namespace eval::semantic {

Analyzer::Analyzer() {
    globalScope_ = std::make_unique<Scope>("");
    currentScope_ = globalScope_.get();
}

void Analyzer::analyze(const ast::Node& root) {
    dispatchNode(root);
}

void Analyzer::pushScope(const std::string& name) {
    auto scope = std::make_unique<Scope>(name, currentScope_);
    currentScope_ = scope.get();
    ownedScopes_.push_back(std::move(scope));
}

void Analyzer::popScope() {
    if (currentScope_ && currentScope_->parent()) {
        currentScope_ = currentScope_->parent();
    }
}

void Analyzer::addError(const std::string& message, int line, int column) {
    errors_.push_back(SemanticError{message, line, column});
}

void Analyzer::visitNode(const ast::Node& node) {
    visitChildren(node);
}

void Analyzer::visitFunctionDecl(const ast::FunctionDecl& node) {
    Symbol sym;
    sym.name = node.name();
    sym.qualifiedName = currentScope_->qualifiedName().empty()
        ? node.name()
        : currentScope_->qualifiedName() + "::" + node.name();
    sym.kind = SymbolKind::Function;
    sym.type = node.returnType();
    sym.isConst = false;
    sym.definition = SymbolLocation{
        node.location().file,
        node.location().line,
        node.location().column
    };

    currentScope_->define(sym);

    pushScope(node.name());

    // Define parameters in function scope
    for (const auto& param : node.params()) {
        Symbol paramSym;
        paramSym.name = param.second;
        paramSym.qualifiedName = currentScope_->qualifiedName() + "::" + param.second;
        paramSym.kind = SymbolKind::Parameter;
        paramSym.type = param.first;
        paramSym.isConst = false;
        paramSym.definition = SymbolLocation{
            node.location().file,
            node.location().line,
            node.location().column
        };
        if (!param.second.empty()) {
            currentScope_->define(paramSym);
        }
    }

    visitChildren(node);

    popScope();
}

void Analyzer::visitVarDecl(const ast::VarDecl& node) {
    Symbol sym;
    sym.name = node.name();
    sym.qualifiedName = currentScope_->qualifiedName().empty()
        ? node.name()
        : currentScope_->qualifiedName() + "::" + node.name();
    sym.kind = SymbolKind::Variable;
    sym.type = node.type();
    sym.isConst = node.isConst();
    sym.definition = SymbolLocation{
        node.location().file,
        node.location().line,
        node.location().column
    };

    if (!currentScope_->define(sym)) {
        std::ostringstream oss;
        oss << "Redefinition of '" << node.name() << "'";
        addError(oss.str(), node.location().line, node.location().column);
    }

    visitChildren(node);
}

void Analyzer::visitCallExpr(const ast::CallExpr& node) {
    Symbol* sym = currentScope_->lookup(node.callee());
    if (!sym) {
        std::ostringstream oss;
        oss << "Undefined symbol '" << node.callee() << "'";
        addError(oss.str(), node.location().line, node.location().column);
    } else {
        SymbolLocation ref{
            node.location().file,
            node.location().line,
            node.location().column
        };
        sym->references.push_back(ref);
    }

    visitChildren(node);
}

void Analyzer::visitIfStmt(const ast::IfStmt& node) {
    pushScope("if");
    visitChildren(node);
    popScope();
}

void Analyzer::visitLiteral(const ast::Literal& node) {
    (void)node;
}

} // namespace eval::semantic
