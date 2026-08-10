#include "test_scope_builder.h"
#include "ast-scope-builder.h"
#include "ast-ir.h"
#include <iostream>

namespace ast
{
namespace
{
    bool check(bool condition, const char* description)
    {
        if(!condition)
            std::cerr << "    FAIL: " << description << "\n";
        return condition;
    }

    /** Returns true when @p tree contains at least one scope of @p kind. */
    bool hasKind(const ScopeTree& tree, ScopeKind kind)
    {
        for(uint32_t i = 0; i < tree.size(); ++i) {
            if(tree[i].kind_ == kind) return true;
        }
        return false;
    }

    /** Returns the first scope of @p kind, or nullptr if none. */
    const Scope* findKind(const ScopeTree& tree, ScopeKind kind)
    {
        for(uint32_t i = 0; i < tree.size(); ++i) {
            if(tree[i].kind_ == kind) return &tree[i];
        }
        return nullptr;
    }

    /** Returns the count of scopes with @p kind. */
    uint32_t countKind(const ScopeTree& tree, ScopeKind kind)
    {
        uint32_t n = 0;
        for(uint32_t i = 0; i < tree.size(); ++i) {
            if(tree[i].kind_ == kind) ++n;
        }
        return n;
    }

    // -----------------------------------------------------------------------

    bool test_empty_ast()
    {
        bool ok = true;
        AST ast;
        ScopeTree tree = build_scope_tree(ast);
        ok &= check(tree.size() == 0, "empty AST produces empty scope tree");
        return ok;
    }

    bool test_nested_namespaces()
    {
        bool ok = true;
        AST ast = parse("test/ast-scope/samples/nested_namespaces.cpp");
        ok &= check(!!ast, "nested_namespaces.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);

        // translation_unit → Global
        // namespace Outer   → Namespace
        // namespace Inner   → Namespace
        ok &= check(tree.size() == 3, "nested namespaces: 3 scopes");
        ok &= check(countKind(tree, ScopeKind::Global)    == 1, "one Global scope");
        ok &= check(countKind(tree, ScopeKind::Namespace) == 2, "two Namespace scopes");

        // Global is the root
        const Scope* global = findKind(tree, ScopeKind::Global);
        ok &= check(global != nullptr, "global scope exists");
        ok &= check(global->parent_ == ScopeTree::InvalidId, "global has no parent");
        ok &= check(global->children_.size() == 1, "global has one child (Outer)");

        // Outer namespace
        uintptr_t outerId = global->children_[0];
        ok &= check(tree[outerId].kind_ == ScopeKind::Namespace, "Outer is Namespace");
        ok &= check(tree[outerId].parent_ == global->id_, "Outer parent is global");
        ok &= check(tree[outerId].children_.size() == 1, "Outer has one child (Inner)");

        // Inner namespace
        uintptr_t innerId = tree[outerId].children_[0];
        ok &= check(tree[innerId].kind_ == ScopeKind::Namespace, "Inner is Namespace");
        ok &= check(tree[innerId].parent_ == outerId, "Inner parent is Outer");
        ok &= check(tree[innerId].children_.empty(), "Inner has no children");

        return ok;
    }

    bool test_nested_classes()
    {
        bool ok = true;
        AST ast = parse("test/ast-scope/samples/nested_classes.cpp");
        ok &= check(!!ast, "nested_classes.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);

        // translation_unit → Global
        // class Outer       → Class
        // class Inner       → Class
        ok &= check(tree.size() == 3, "nested classes: 3 scopes");
        ok &= check(countKind(tree, ScopeKind::Global) == 1, "one Global scope");
        ok &= check(countKind(tree, ScopeKind::Class)  == 2, "two Class scopes");

        const Scope* global = findKind(tree, ScopeKind::Global);
        ok &= check(global->children_.size() == 1, "global has one child (Outer)");

        uintptr_t outerId = global->children_[0];
        ok &= check(tree[outerId].children_.size() == 1, "Outer has one child (Inner)");

        uintptr_t innerId = tree[outerId].children_[0];
        ok &= check(tree[innerId].parent_ == outerId, "Inner parent is Outer");
        ok &= check(tree[innerId].children_.empty(), "Inner has no children");

        return ok;
    }

    bool test_sibling_functions()
    {
        bool ok = true;
        AST ast = parse("test/ast-scope/samples/functions.cpp");
        ok &= check(!!ast, "functions.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);

        // translation_unit → Global
        // function_definition add → Function
        // compound_statement      → Block
        // function_definition sub → Function
        // compound_statement      → Block
        ok &= check(tree.size() == 5, "two functions: 5 scopes");
        ok &= check(countKind(tree, ScopeKind::Function) == 2, "two Function scopes");
        ok &= check(countKind(tree, ScopeKind::Block)    == 2, "two Block scopes");

        const Scope* global = findKind(tree, ScopeKind::Global);
        ok &= check(global->children_.size() == 2, "global has two function children");

        // Both direct children of global are Function scopes
        for(uintptr_t fnId : global->children_) {
            ok &= check(tree[fnId].kind_ == ScopeKind::Function, "global child is Function");
            ok &= check(tree[fnId].children_.size() == 1, "Function has one Block child");
            uintptr_t blkId = tree[fnId].children_[0];
            ok &= check(tree[blkId].kind_ == ScopeKind::Block, "function child is Block");
            ok &= check(tree[blkId].parent_ == fnId, "Block parent is Function");
        }

        return ok;
    }

    bool test_nested_blocks()
    {
        bool ok = true;
        AST ast = parse("test/ast-scope/samples/blocks.cpp");
        ok &= check(!!ast, "blocks.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);

        // translation_unit  → Global
        // function_definition test → Function
        // compound_statement (body) → Block
        // compound_statement (outer bare) → Block
        // compound_statement (inner bare) → Block
        ok &= check(tree.size() == 5, "nested blocks: 5 scopes");
        ok &= check(countKind(tree, ScopeKind::Function) == 1, "one Function scope");
        ok &= check(countKind(tree, ScopeKind::Block)    == 3, "three Block scopes");

        // Verify nesting depth: global → function → block → block → block
        const Scope* global = findKind(tree, ScopeKind::Global);
        ok &= check(global->children_.size() == 1, "global has one child");
        uintptr_t fnId = global->children_[0];
        ok &= check(tree[fnId].children_.size() == 1, "function has one block child");
        uintptr_t b0 = tree[fnId].children_[0];
        ok &= check(tree[b0].kind_ == ScopeKind::Block, "function body is Block");
        ok &= check(tree[b0].children_.size() == 1, "function body has one child block");
        uintptr_t b1 = tree[b0].children_[0];
        ok &= check(tree[b1].kind_ == ScopeKind::Block, "outer bare block is Block");
        ok &= check(tree[b1].children_.size() == 1, "outer bare block has one child");
        uintptr_t b2 = tree[b1].children_[0];
        ok &= check(tree[b2].kind_ == ScopeKind::Block, "inner bare block is Block");
        ok &= check(tree[b2].children_.empty(), "inner bare block has no children");

        return ok;
    }

    bool test_sibling_scopes()
    {
        bool ok = true;
        AST ast = parse("test/ast-scope/samples/siblings.cpp");
        ok &= check(!!ast, "siblings.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);

        // translation_unit → Global
        // namespace A, B, C → 3 Namespace scopes
        ok &= check(tree.size() == 4, "three sibling namespaces: 4 scopes");
        ok &= check(countKind(tree, ScopeKind::Namespace) == 3, "three Namespace scopes");

        const Scope* global = findKind(tree, ScopeKind::Global);
        ok &= check(global->children_.size() == 3, "global has 3 namespace children");

        // All three are siblings with the same parent
        for(uintptr_t nsId : global->children_) {
            ok &= check(tree[nsId].kind_ == ScopeKind::Namespace, "sibling is Namespace");
            ok &= check(tree[nsId].parent_ == global->id_, "sibling parent is global");
        }

        // Siblings are distinct
        ok &= check(global->children_[0] != global->children_[1], "siblings have distinct IDs");
        ok &= check(global->children_[1] != global->children_[2], "siblings have distinct IDs");

        return ok;
    }

    bool test_mixed_scopes()
    {
        bool ok = true;
        AST ast = parse("test/ast-scope/samples/mixed.cpp");
        ok &= check(!!ast, "mixed.cpp parsed");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);

        // translation_unit → Global
        // namespace MyLib → Namespace
        // class Container → Class
        // function_definition process → Function
        // compound_statement (process body) → Block
        // compound_statement (bare block) → Block
        // class Helper → Class
        ok &= check(tree.size() == 7, "mixed: 7 scopes");
        ok &= check(countKind(tree, ScopeKind::Global)    == 1, "one Global");
        ok &= check(countKind(tree, ScopeKind::Namespace) == 1, "one Namespace");
        ok &= check(countKind(tree, ScopeKind::Class)     == 2, "two Class scopes");
        ok &= check(countKind(tree, ScopeKind::Function)  == 1, "one Function scope");
        ok &= check(countKind(tree, ScopeKind::Block)     == 2, "two Block scopes");

        // Namespace has 2 class children (Container, Helper)
        const Scope* global = findKind(tree, ScopeKind::Global);
        ok &= check(global->children_.size() == 1, "global has 1 namespace child");
        uintptr_t nsId = global->children_[0];
        ok &= check(tree[nsId].children_.size() == 2, "namespace has 2 class children");

        return ok;
    }

    bool test_node_scope_mapping()
    {
        bool ok = true;
        AST ast = parse("test/ast-scope/samples/nested_namespaces.cpp");
        ok &= check(!!ast, "nested_namespaces.cpp parsed for node mapping");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);

        // Every node must be mapped to a valid scope
        for(uint32_t i = 0; i < ast.size(); ++i) {
            uintptr_t scopeId = tree.getNodeScope(i);
            ok &= check(scopeId != ScopeTree::InvalidId, "every node is mapped to a scope");
            if(scopeId != ScopeTree::InvalidId) {
                ok &= check(scopeId < tree.size(), "mapped scope ID is in range");
            }
        }

        // The root node (translation_unit) is mapped to the Global scope
        uintptr_t rootScope = tree.getNodeScope(0);
        ok &= check(rootScope != ScopeTree::InvalidId, "root node is mapped");
        ok &= check(tree[rootScope].kind_ == ScopeKind::Global, "root node maps to Global scope");

        return ok;
    }

    bool test_scope_source_ranges()
    {
        bool ok = true;
        AST ast = parse("test/ast-scope/samples/functions.cpp");
        ok &= check(!!ast, "functions.cpp parsed for range check");
        if(!ast) return false;

        ScopeTree tree = build_scope_tree(ast);

        // Every scope must have startByte_ <= endByte_
        for(uint32_t i = 0; i < tree.size(); ++i) {
            ok &= check(tree[i].startByte_ <= tree[i].endByte_, "scope startByte <= endByte");
        }

        // Each scope's range must be contained within its parent's range
        for(uint32_t i = 0; i < tree.size(); ++i) {
            uintptr_t parentId = tree[i].parent_;
            if(parentId != ScopeTree::InvalidId) {
                ok &= check(tree[parentId].startByte_ <= tree[i].startByte_, "child start >= parent start");
                ok &= check(tree[i].endByte_ <= tree[parentId].endByte_, "child end <= parent end");
            }
        }

        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_scope_builder()
{
    static const TestCase cases[] = {
        {"empty AST",            test_empty_ast},
        {"nested namespaces",    test_nested_namespaces},
        {"nested classes",       test_nested_classes},
        {"sibling functions",    test_sibling_functions},
        {"nested blocks",        test_nested_blocks},
        {"sibling scopes",       test_sibling_scopes},
        {"mixed scopes",         test_mixed_scopes},
        {"node-scope mapping",   test_node_scope_mapping},
        {"scope source ranges",  test_scope_source_ranges},
    };

    std::cout << "=== scope builder tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
