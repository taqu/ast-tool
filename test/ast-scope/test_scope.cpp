#include "test_scope.h"
#include "ast-scope.h"
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

    bool test_empty_tree()
    {
        bool ok = true;
        ScopeTree tree;
        ok &= check(tree.size() == 0, "empty tree has size 0");
        ok &= check(tree.findByNodeIndex(0) == ScopeTree::InvalidId, "findByNodeIndex on empty tree returns InvalidId");
        ok &= check(tree.findBySymbol(0) == ScopeTree::InvalidId, "findBySymbol on empty tree returns InvalidId");
        return ok;
    }

    bool test_add_root_scope()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t id = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 1000);
        ok &= check(id == 0, "first scope gets id 0");
        ok &= check(tree.size() == 1, "tree has one scope after add");
        const Scope& s = tree[id];
        ok &= check(s.id_ == 0, "scope id_ matches");
        ok &= check(s.kind_ == ScopeKind::Global, "scope kind is Global");
        ok &= check(s.nodeIndex_ == ScopeTree::InvalidNodeIndex, "nodeIndex is InvalidNodeIndex");
        ok &= check(s.startByte_ == 0, "startByte is 0");
        ok &= check(s.endByte_ == 1000, "endByte is 1000");
        ok &= check(s.parent_ == ScopeTree::InvalidId, "root has no parent");
        ok &= check(s.children_.empty(), "root starts with no children");
        ok &= check(s.symbols_.empty(), "root starts with no symbols");
        return ok;
    }

    bool test_parent_child_links()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 5000);
        uintptr_t ns     = tree.add(ScopeKind::Namespace, 2, 100, 900, global);
        uintptr_t cls    = tree.add(ScopeKind::Class, 5, 200, 800, ns);

        ok &= check(tree[ns].parent_ == global, "namespace parent is global");
        ok &= check(tree[cls].parent_ == ns, "class parent is namespace");

        ok &= check(tree[global].children_.size() == 1, "global has one child");
        ok &= check(tree[global].children_[0] == ns, "global child is namespace");

        ok &= check(tree[ns].children_.size() == 1, "namespace has one child");
        ok &= check(tree[ns].children_[0] == cls, "namespace child is class");

        ok &= check(tree[cls].children_.empty(), "class has no children yet");
        return ok;
    }

    bool test_multiple_children()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t fn1    = tree.add(ScopeKind::Function, 1, 10, 200, global);
        uintptr_t fn2    = tree.add(ScopeKind::Function, 3, 210, 400, global);
        uintptr_t fn3    = tree.add(ScopeKind::Method,   5, 410, 600, global);

        ok &= check(tree[global].children_.size() == 3, "global has three children");
        ok &= check(tree[global].children_[0] == fn1, "first child is fn1");
        ok &= check(tree[global].children_[1] == fn2, "second child is fn2");
        ok &= check(tree[global].children_[2] == fn3, "third child is fn3");
        return ok;
    }

    bool test_symbol_association()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 5000);
        uintptr_t ns     = tree.add(ScopeKind::Namespace, 2, 100, 900, global);

        tree.addSymbol(ns, 0);
        tree.addSymbol(ns, 1);
        tree.addSymbol(ns, 2);

        ok &= check(tree[ns].symbols_.size() == 3, "namespace has 3 symbols");
        ok &= check(tree[ns].symbols_[0] == 0, "first symbol index is 0");
        ok &= check(tree[ns].symbols_[1] == 1, "second symbol index is 1");
        ok &= check(tree[ns].symbols_[2] == 2, "third symbol index is 2");
        ok &= check(tree[global].symbols_.empty(), "global has no symbols");
        return ok;
    }

    bool test_find_by_node_index()
    {
        bool ok = true;
        ScopeTree tree;
        tree.add(ScopeKind::Global, ScopeTree::InvalidNodeIndex, 0, 5000);
        uintptr_t ns  = tree.add(ScopeKind::Namespace, 7,  100, 900,  0);
        uintptr_t cls = tree.add(ScopeKind::Class,     42, 200, 800, ns);

        ok &= check(tree.findByNodeIndex(7)  == ns,               "findByNodeIndex returns namespace for node 7");
        ok &= check(tree.findByNodeIndex(42) == cls,              "findByNodeIndex returns class for node 42");
        ok &= check(tree.findByNodeIndex(99) == ScopeTree::InvalidId, "findByNodeIndex returns InvalidId for unknown node");
        ok &= check(tree.findByNodeIndex(ScopeTree::InvalidNodeIndex) == 0, "findByNodeIndex finds global by InvalidNodeIndex");
        return ok;
    }

    bool test_find_by_symbol()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global,    ScopeTree::InvalidNodeIndex, 0, 5000);
        uintptr_t ns     = tree.add(ScopeKind::Namespace, 2,  100, 900, global);
        uintptr_t fn     = tree.add(ScopeKind::Function,  5,  200, 400, ns);

        tree.addSymbol(ns, 10);
        tree.addSymbol(fn, 20);
        tree.addSymbol(fn, 21);

        ok &= check(tree.findBySymbol(10) == ns,               "findBySymbol returns namespace for symbol 10");
        ok &= check(tree.findBySymbol(20) == fn,               "findBySymbol returns function for symbol 20");
        ok &= check(tree.findBySymbol(21) == fn,               "findBySymbol returns function for symbol 21");
        ok &= check(tree.findBySymbol(99) == ScopeTree::InvalidId, "findBySymbol returns InvalidId for unknown symbol");
        return ok;
    }

    bool test_scope_kind_names()
    {
        bool ok = true;
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Global))      == "Global",      "Global name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Namespace))   == "Namespace",   "Namespace name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Module))      == "Module",      "Module name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Class))       == "Class",       "Class name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Struct))      == "Struct",      "Struct name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Enum))        == "Enum",        "Enum name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Function))    == "Function",    "Function name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Method))      == "Method",      "Method name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Lambda))      == "Lambda",      "Lambda name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Block))       == "Block",       "Block name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Conditional)) == "Conditional", "Conditional name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Loop))        == "Loop",        "Loop name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Switch))      == "Switch",      "Switch name");
        ok &= check(std::string_view(getScopeKindName(ScopeKind::Unknown))     == "Unknown",     "Unknown name");
        return ok;
    }

    bool test_deeply_nested()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t global = tree.add(ScopeKind::Global,    ScopeTree::InvalidNodeIndex, 0, 9000);
        uintptr_t ns     = tree.add(ScopeKind::Namespace, 1,   50, 8000, global);
        uintptr_t cls    = tree.add(ScopeKind::Class,     2,  100, 7000, ns);
        uintptr_t method = tree.add(ScopeKind::Method,    3,  200, 6000, cls);
        uintptr_t block  = tree.add(ScopeKind::Block,     4,  300, 5000, method);
        uintptr_t inner  = tree.add(ScopeKind::Conditional, 5, 400, 4000, block);

        ok &= check(tree.size() == 6, "deeply nested tree has 6 scopes");
        ok &= check(tree[inner].parent_ == block,  "conditional parent is block");
        ok &= check(tree[block].parent_ == method, "block parent is method");
        ok &= check(tree[method].parent_ == cls,   "method parent is class");
        ok &= check(tree[cls].parent_ == ns,       "class parent is namespace");
        ok &= check(tree[ns].parent_ == global,    "namespace parent is global");
        ok &= check(tree[global].parent_ == ScopeTree::InvalidId, "global has no parent");

        // Each level has exactly one child
        ok &= check(tree[global].children_.size() == 1, "global has 1 child");
        ok &= check(tree[ns].children_.size()     == 1, "namespace has 1 child");
        ok &= check(tree[cls].children_.size()    == 1, "class has 1 child");
        ok &= check(tree[method].children_.size() == 1, "method has 1 child");
        ok &= check(tree[block].children_.size()  == 1, "block has 1 child");
        ok &= check(tree[inner].children_.empty(),      "innermost has no children");
        return ok;
    }

    bool test_source_ranges()
    {
        bool ok = true;
        ScopeTree tree;
        uintptr_t id = tree.add(ScopeKind::Function, 3, 1024, 2048, ScopeTree::InvalidId);
        ok &= check(tree[id].startByte_ == 1024, "startByte stored correctly");
        ok &= check(tree[id].endByte_   == 2048, "endByte stored correctly");
        return ok;
    }

    struct TestCase { const char* name; bool(*fn)(); };

} // namespace

bool run_tests_scope()
{
    static const TestCase cases[] = {
        {"empty tree",            test_empty_tree},
        {"add root scope",        test_add_root_scope},
        {"parent-child links",    test_parent_child_links},
        {"multiple children",     test_multiple_children},
        {"symbol association",    test_symbol_association},
        {"findByNodeIndex",       test_find_by_node_index},
        {"findBySymbol",          test_find_by_symbol},
        {"scope kind names",      test_scope_kind_names},
        {"deeply nested",         test_deeply_nested},
        {"source ranges",         test_source_ranges},
    };

    std::cout << "=== scope model tests ===\n";
    bool all = true;
    for(const auto& tc : cases) {
        bool ok = tc.fn();
        std::cout << "  [" << (ok ? "PASS" : "FAIL") << "] " << tc.name << "\n";
        all &= ok;
    }
    return all;
}

} // namespace ast
