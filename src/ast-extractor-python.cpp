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
    // ── Node type string constants (Python grammar) ─────────────────────────
    constexpr const char* k_class_definition    = "class_definition";
    constexpr const char* k_function_definition = "function_definition";
    constexpr const char* k_identifier          = "identifier";
    constexpr const char* k_assignment          = "assignment";
    constexpr const char* k_block               = "block";

    // Extract the simple name from a class/function definition node: the
    // first identifier-grammar child. Stops at the body ("block") so a
    // malformed tree can never pick up a name from inside the body.
    std::string getDefName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(k_block)) break;
            if(child.grammarEquals(k_identifier)) return child.getText();
        }
        return {};
    }

    // Returns the simple name from a Python assignment left side.
    // Returns empty for attribute assignments (self.x = ...) and other complex forms.
    std::string getAssignmentName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            // The first child is always the left-hand side.
            if(child.typeEquals(k_identifier)) return child.getText();
            return {}; // attribute, tuple, subscript, etc. → not a simple assignment
        }
        return {};
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_python(const ast::AST& tree)
{
    std::vector<Symbol>             result;
    std::unordered_set<std::string> seen;   // key = fqn + ":" + kind ordinal
    std::vector<ScopeFrame>         scopeStack;

    auto emit = [&](Symbol sym) {
        std::string key = sym.fqn + ":" + std::to_string(static_cast<int>(sym.kind));
        if(!sym.fqn.empty() && seen.insert(key).second) {
            result.push_back(std::move(sym));
        }
    };

    for(uint32_t i = 0; i < tree.size(); ++i) {
        const ast::ASTNode& node = tree[i];

        while(!scopeStack.empty() && scopeStack.back().endByte <= node.startByte_) {
            scopeStack.pop_back();
        }

        // ── Class ────────────────────────────────────────────────────────
        if(node.typeEquals(k_class_definition)) {
            std::string name = getDefName(tree, node);
            if(!name.empty()) {
                std::string fqn = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Class, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Function / Method ─────────────────────────────────────────────
        if(node.typeEquals(k_function_definition)) {
            std::string name = getDefName(tree, node);
            if(!name.empty()) {
                SymbolKind kind = SymbolKind::Function;
                if(inNamedClassScope(scopeStack)) {
                    kind = (name == "__init__") ? SymbolKind::Constructor
                                                : SymbolKind::Method;
                }
                std::string fqn = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, kind, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
                // Push a function scope so nested locals and inner functions
                // get correct FQNs and local assignments are suppressed.
                scopeStack.push_back({name, kind, node.endByte_, Access::Unknown, false});
            }
            continue;
        }

        // ── Assignment → module variable or class variable ────────────────
        // Skip assignments inside function bodies (local variables).
        if(node.typeEquals(k_assignment) && !insideFunctionScope(scopeStack)) {
            std::string name = getAssignmentName(tree, node);
            if(!name.empty()) {
                SymbolKind kind = inNamedClassScope(scopeStack)
                                      ? SymbolKind::Field
                                      : SymbolKind::Variable;
                std::string fqn = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, kind, Access::Unknown,
                               i, node.start_.row_, node.start_.column_));
            }
            continue;
        }
    }

    return result;
}

} // namespace extractor
} // namespace ast
