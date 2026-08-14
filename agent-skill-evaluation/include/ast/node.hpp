#pragma once

#include <memory>
#include <string>
#include <vector>
#include <utility>

namespace eval::ast {

constexpr int kMaxNestingDepth = 64;

enum class NodeKind {
    TranslationUnit,
    FunctionDecl,
    VarDecl,
    IfStmt,
    WhileStmt,
    ReturnStmt,
    CallExpr,
    BinaryExpr,
    Literal,
    Identifier
};

std::string nodeKindName(NodeKind kind);

struct SourceLocation {
    std::string file;
    int line;
    int column;
};

class Node {
public:
    Node(NodeKind kind, SourceLocation loc)
        : kind_(kind), location_(std::move(loc)) {}

    virtual ~Node() = default;

    NodeKind kind() const { return kind_; }
    const SourceLocation& location() const { return location_; }
    const std::vector<std::shared_ptr<Node>>& children() const { return children_; }

    void addChild(std::shared_ptr<Node> child) {
        children_.push_back(std::move(child));
    }

    virtual std::string toString() const = 0;

private:
    NodeKind kind_;
    SourceLocation location_;
    std::vector<std::shared_ptr<Node>> children_;
};

class FunctionDecl : public Node {
public:
    using Param = std::pair<std::string, std::string>; // type, name

    FunctionDecl(SourceLocation loc,
                 std::string name,
                 std::string returnType,
                 std::vector<Param> params)
        : Node(NodeKind::FunctionDecl, std::move(loc))
        , name_(std::move(name))
        , returnType_(std::move(returnType))
        , params_(std::move(params)) {}

    const std::string& name() const { return name_; }
    const std::string& returnType() const { return returnType_; }
    const std::vector<Param>& params() const { return params_; }

    std::string toString() const override;

private:
    std::string name_;
    std::string returnType_;
    std::vector<Param> params_;
};

class VarDecl : public Node {
public:
    VarDecl(SourceLocation loc,
            std::string name,
            std::string type,
            bool isConst)
        : Node(NodeKind::VarDecl, std::move(loc))
        , name_(std::move(name))
        , type_(std::move(type))
        , isConst_(isConst) {}

    const std::string& name() const { return name_; }
    const std::string& type() const { return type_; }
    bool isConst() const { return isConst_; }

    std::string toString() const override;

private:
    std::string name_;
    std::string type_;
    bool isConst_;
};

class IfStmt : public Node {
public:
    IfStmt(SourceLocation loc, std::string conditionText)
        : Node(NodeKind::IfStmt, std::move(loc))
        , conditionText_(std::move(conditionText)) {}

    const std::string& conditionText() const { return conditionText_; }

    std::string toString() const override;

private:
    std::string conditionText_;
};

class WhileStmt : public Node {
public:
    WhileStmt(SourceLocation loc, std::string conditionText)
        : Node(NodeKind::WhileStmt, std::move(loc))
        , conditionText_(std::move(conditionText)) {}

    const std::string& conditionText() const { return conditionText_; }

    std::string toString() const override;

private:
    std::string conditionText_;
};

class ReturnStmt : public Node {
public:
    explicit ReturnStmt(SourceLocation loc)
        : Node(NodeKind::ReturnStmt, std::move(loc)) {}

    std::string toString() const override;
};

class CallExpr : public Node {
public:
    CallExpr(SourceLocation loc, std::string callee, int argCount)
        : Node(NodeKind::CallExpr, std::move(loc))
        , callee_(std::move(callee))
        , argCount_(argCount) {}

    const std::string& callee() const { return callee_; }
    int argCount() const { return argCount_; }

    std::string toString() const override;

private:
    std::string callee_;
    int argCount_;
};

class BinaryExpr : public Node {
public:
    BinaryExpr(SourceLocation loc, std::string op)
        : Node(NodeKind::BinaryExpr, std::move(loc))
        , op_(std::move(op)) {}

    const std::string& op() const { return op_; }

    std::string toString() const override;

private:
    std::string op_;
};

class Literal : public Node {
public:
    Literal(SourceLocation loc, std::string value)
        : Node(NodeKind::Literal, std::move(loc))
        , value_(std::move(value)) {}

    const std::string& value() const { return value_; }

    std::string toString() const override;

private:
    std::string value_;
};

class Identifier : public Node {
public:
    Identifier(SourceLocation loc, std::string name)
        : Node(NodeKind::Identifier, std::move(loc))
        , name_(std::move(name)) {}

    const std::string& name() const { return name_; }

    std::string toString() const override;

private:
    std::string name_;
};

class TranslationUnit : public Node {
public:
    explicit TranslationUnit(SourceLocation loc)
        : Node(NodeKind::TranslationUnit, std::move(loc)) {}

    std::string toString() const override;
};

} // namespace eval::ast
