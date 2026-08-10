#include "ast-extractor-langs.h"
#include "ast-extractor-common.h"
#include "ast-ir.h"
#include <unordered_set>

namespace ast
{
namespace extractor
{
namespace
{
    // ── Node type string constants (Bash grammar) ───────────────────────────
    constexpr const char* k_function_definition = "function_definition";
    constexpr const char* k_variable_assignment = "variable_assignment";
    constexpr const char* k_word                = "word";
    constexpr const char* k_variable_name       = "variable_name";

    // Extract the function name from a function_definition node.
    // The grammar produces: [optional "function" keyword] word body
    std::string getFuncName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(k_word)) return child.getText();
        }
        return {};
    }

    // Extract the variable name from a variable_assignment node.
    std::string getVarName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(k_variable_name)) return child.getText();
        }
        return {};
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_bash(const ast::AST& tree)
{
    std::vector<Symbol>             result;
    std::unordered_set<std::string> seen;
    std::vector<ScopeFrame>         scopeStack;

    auto emit = [&](Symbol sym) {
        std::string key = sym.fqn + ":" + std::to_string(static_cast<int>(sym.kind));
        if(!sym.fqn.empty() && seen.insert(key).second)
            result.push_back(std::move(sym));
    };

    for(uint32_t i = 0; i < tree.size(); ++i) {
        const ast::ASTNode& node = tree[i];

        while(!scopeStack.empty() && scopeStack.back().endByte <= node.startByte_)
            scopeStack.pop_back();

        // ── Function definition ──────────────────────────────────────────────
        // Handles both `name() { }` and `function name { }` forms.
        if(node.typeEquals(k_function_definition)) {
            std::string name = getFuncName(tree, node);
            if(!name.empty()) {
                std::string fqn = buildFQN(scopeStack, name, "::");
                emit(makeSymbol(name, fqn, SymbolKind::Function, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Function, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Variable assignment at file/global scope ─────────────────────────
        // Covers both plain `VAR=value` and variables inside `export`/`readonly`/
        // `declare` commands, since their variable_assignment children appear
        // in the DFS traversal at this point as well. Local variables inside
        // functions are suppressed by the insideFunctionScope guard.
        if(node.typeEquals(k_variable_assignment) && !insideFunctionScope(scopeStack)) {
            std::string name = getVarName(tree, node);
            if(!name.empty()) {
                std::string fqn = buildFQN(scopeStack, name, "::");
                emit(makeSymbol(name, fqn, SymbolKind::Variable, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }
    }

    return result;
}

} // namespace extractor
} // namespace ast
