#include "ast/node.hpp"
#include <sstream>

namespace eval::ast {

std::string nodeKindName(NodeKind kind) {
    switch (kind) {
        case NodeKind::TranslationUnit: return "TranslationUnit";
        case NodeKind::FunctionDecl:   return "FunctionDecl";
        case NodeKind::VarDecl:        return "VarDecl";
        case NodeKind::IfStmt:         return "IfStmt";
        case NodeKind::WhileStmt:      return "WhileStmt";
        case NodeKind::ReturnStmt:     return "ReturnStmt";
        case NodeKind::CallExpr:       return "CallExpr";
        case NodeKind::BinaryExpr:     return "BinaryExpr";
        case NodeKind::Literal:        return "Literal";
        case NodeKind::Identifier:     return "Identifier";
        default:                       return "Unknown";
    }
}

std::string TranslationUnit::toString() const {
    return "TranslationUnit";
}

std::string FunctionDecl::toString() const {
    std::ostringstream oss;
    oss << "FunctionDecl " << returnType_ << " " << name_ << "(";
    for (size_t i = 0; i < params_.size(); ++i) {
        if (i > 0) oss << ", ";
        oss << params_[i].first << " " << params_[i].second;
    }
    oss << ")";
    return oss.str();
}

std::string VarDecl::toString() const {
    std::ostringstream oss;
    oss << "VarDecl ";
    if (isConst_) oss << "const ";
    oss << type_ << " " << name_;
    return oss.str();
}

std::string IfStmt::toString() const {
    return "IfStmt (" + conditionText_ + ")";
}

std::string WhileStmt::toString() const {
    return "WhileStmt (" + conditionText_ + ")";
}

std::string ReturnStmt::toString() const {
    return "ReturnStmt";
}

std::string CallExpr::toString() const {
    std::ostringstream oss;
    oss << "CallExpr " << callee_ << "/" << argCount_;
    return oss.str();
}

std::string BinaryExpr::toString() const {
    return "BinaryExpr " + op_;
}

std::string Literal::toString() const {
    return "Literal " + value_;
}

std::string Identifier::toString() const {
    return "Identifier " + name_;
}

} // namespace eval::ast
