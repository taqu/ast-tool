#pragma once

#include "node.hpp"
#include <ostream>
#include <vector>

namespace eval::ast {

class Visitor {
public:
    virtual ~Visitor() = default;

    virtual void visitNode(const Node& node);
    virtual void visitFunctionDecl(const FunctionDecl& node);
    virtual void visitVarDecl(const VarDecl& node);
    virtual void visitCallExpr(const CallExpr& node);
    virtual void visitIfStmt(const IfStmt& node);
    virtual void visitLiteral(const Literal& node);

protected:
    void dispatchNode(const Node& node);
    void visitChildren(const Node& node);
};

class PrintVisitor : public Visitor {
public:
    explicit PrintVisitor(std::ostream& out) : out_(out), indent_(0) {}

    void visitNode(const Node& node) override;
    void visitFunctionDecl(const FunctionDecl& node) override;
    void visitVarDecl(const VarDecl& node) override;
    void visitCallExpr(const CallExpr& node) override;
    void visitIfStmt(const IfStmt& node) override;
    void visitLiteral(const Literal& node) override;

private:
    void printIndent();
    void indented(const std::string& text);

    std::ostream& out_;
    int indent_;
};

class CollectVisitor : public Visitor {
public:
    explicit CollectVisitor(NodeKind target) : target_(target) {}

    void visitNode(const Node& node) override;
    void visitFunctionDecl(const FunctionDecl& node) override;
    void visitVarDecl(const VarDecl& node) override;
    void visitCallExpr(const CallExpr& node) override;
    void visitIfStmt(const IfStmt& node) override;
    void visitLiteral(const Literal& node) override;

    const std::vector<const Node*>& collected() const { return collected_; }
    void clear() { collected_.clear(); }

private:
    void collectAndDescend(const Node& node);

    NodeKind target_;
    std::vector<const Node*> collected_;
};

} // namespace eval::ast
