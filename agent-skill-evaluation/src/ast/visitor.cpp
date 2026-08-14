#include "ast/visitor.hpp"
#include <iostream>

namespace eval::ast {

// ---------------------------------------------------------------------------
// Visitor base
// ---------------------------------------------------------------------------

void Visitor::dispatchNode(const Node& node) {
    switch (node.kind()) {
        case NodeKind::FunctionDecl:
            visitFunctionDecl(static_cast<const FunctionDecl&>(node));
            break;
        case NodeKind::VarDecl:
            visitVarDecl(static_cast<const VarDecl&>(node));
            break;
        case NodeKind::CallExpr:
            visitCallExpr(static_cast<const CallExpr&>(node));
            break;
        case NodeKind::IfStmt:
            visitIfStmt(static_cast<const IfStmt&>(node));
            break;
        case NodeKind::Literal:
            visitLiteral(static_cast<const Literal&>(node));
            break;
        default:
            visitNode(node);
            break;
    }
}

void Visitor::visitChildren(const Node& node) {
    for (const auto& child : node.children()) {
        if (child) {
            dispatchNode(*child);
        }
    }
}

void Visitor::visitNode(const Node& node) {
    visitChildren(node);
}

void Visitor::visitFunctionDecl(const FunctionDecl& node) {
    visitChildren(node);
}

void Visitor::visitVarDecl(const VarDecl& node) {
    visitChildren(node);
}

void Visitor::visitCallExpr(const CallExpr& node) {
    visitChildren(node);
}

void Visitor::visitIfStmt(const IfStmt& node) {
    visitChildren(node);
}

void Visitor::visitLiteral(const Literal& node) {
    visitChildren(node);
}

// ---------------------------------------------------------------------------
// PrintVisitor
// ---------------------------------------------------------------------------

void PrintVisitor::printIndent() {
    for (int i = 0; i < indent_; ++i) {
        out_ << "  ";
    }
}

void PrintVisitor::indented(const std::string& text) {
    printIndent();
    out_ << text << "\n";
}

void PrintVisitor::visitNode(const Node& node) {
    indented(nodeKindName(node.kind()) + " [" + node.toString() + "]");
    ++indent_;
    visitChildren(node);
    --indent_;
}

void PrintVisitor::visitFunctionDecl(const FunctionDecl& node) {
    printIndent();
    out_ << node.toString()
         << " @ " << node.location().file
         << ":" << node.location().line << "\n";
    ++indent_;
    visitChildren(node);
    --indent_;
}

void PrintVisitor::visitVarDecl(const VarDecl& node) {
    printIndent();
    out_ << node.toString()
         << " @ " << node.location().file
         << ":" << node.location().line << "\n";
    ++indent_;
    visitChildren(node);
    --indent_;
}

void PrintVisitor::visitCallExpr(const CallExpr& node) {
    indented(node.toString());
    ++indent_;
    visitChildren(node);
    --indent_;
}

void PrintVisitor::visitIfStmt(const IfStmt& node) {
    indented(node.toString());
    ++indent_;
    visitChildren(node);
    --indent_;
}

void PrintVisitor::visitLiteral(const Literal& node) {
    indented(node.toString());
}

// ---------------------------------------------------------------------------
// CollectVisitor
// ---------------------------------------------------------------------------

void CollectVisitor::collectAndDescend(const Node& node) {
    if (node.kind() == target_) {
        collected_.push_back(&node);
    }
    visitChildren(node);
}

void CollectVisitor::visitNode(const Node& node) {
    collectAndDescend(node);
}

void CollectVisitor::visitFunctionDecl(const FunctionDecl& node) {
    collectAndDescend(node);
}

void CollectVisitor::visitVarDecl(const VarDecl& node) {
    collectAndDescend(node);
}

void CollectVisitor::visitCallExpr(const CallExpr& node) {
    collectAndDescend(node);
}

void CollectVisitor::visitIfStmt(const IfStmt& node) {
    collectAndDescend(node);
}

void CollectVisitor::visitLiteral(const Literal& node) {
    collectAndDescend(node);
}

} // namespace eval::ast
