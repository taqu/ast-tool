#include "semantic/symbol.hpp"

namespace eval::semantic {

bool Scope::define(Symbol symbol) {
    auto it = symbols_.find(symbol.name);
    if (it != symbols_.end()) {
        return false; // already defined
    }
    std::string key = symbol.name;
    symbols_.emplace(std::move(key), std::move(symbol));
    return true;
}

Symbol* Scope::lookup(const std::string& name) {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        return &it->second;
    }
    if (parent_) {
        return parent_->lookup(name);
    }
    return nullptr;
}

const Symbol* Scope::lookup(const std::string& name) const {
    auto it = symbols_.find(name);
    if (it != symbols_.end()) {
        return &it->second;
    }
    if (parent_) {
        return parent_->lookup(name);
    }
    return nullptr;
}

std::string Scope::qualifiedName() const {
    if (!parent_ || parent_->name().empty()) {
        return name_;
    }
    std::string parentQN = parent_->qualifiedName();
    if (parentQN.empty()) {
        return name_;
    }
    return parentQN + "::" + name_;
}

} // namespace eval::semantic
