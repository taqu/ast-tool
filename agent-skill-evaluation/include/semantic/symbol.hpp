#pragma once

#include <string>
#include <vector>
#include <unordered_map>

namespace eval::semantic {

enum class SymbolKind {
    Function,
    Variable,
    Parameter,
    Type
};

struct SymbolLocation {
    std::string file;
    int line;
    int column;
};

struct Symbol {
    std::string name;
    std::string qualifiedName;
    SymbolKind kind;
    std::string type;
    bool isConst;
    SymbolLocation definition;
    std::vector<SymbolLocation> references;
};

class Scope {
public:
    explicit Scope(std::string name, Scope* parent = nullptr)
        : name_(std::move(name)), parent_(parent) {}

    bool define(Symbol symbol);

    Symbol* lookup(const std::string& name);
    const Symbol* lookup(const std::string& name) const;

    const std::unordered_map<std::string, Symbol>& symbols() const { return symbols_; }
    const std::string& name() const { return name_; }
    Scope* parent() const { return parent_; }

    std::string qualifiedName() const;

private:
    std::string name_;
    Scope* parent_;
    std::unordered_map<std::string, Symbol> symbols_;
};

} // namespace eval::semantic
