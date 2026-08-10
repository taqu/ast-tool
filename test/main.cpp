#include "ast-tool.h"
#include "test.h"
#include "ast-extractor-c/test_extractor_c.h"
#include "ast-extractor-cpp/test_extractor_cpp.h"
#include "ast-extractor-python/test_extractor_python.h"
#include "ast-extractor-rust/test_extractor_rust.h"
#include "ast-extractor-go/test_extractor_go.h"
#include "ast-extractor-java/test_extractor_java.h"
#include "ast-extractor-javascript/test_extractor_javascript.h"
#include "ast-scope/test_scope.h"
#include "ast-scope/test_scope_builder.h"
#include "ast-symbol-scope/test_symbol_scope.h"
#include "ast-scope-visibility/test_scope_visibility.h"
#include "ast-lookup/test_lookup.h"
#include "ast-workspace/test_workspace.h"
#include "ast-search/test_search.h"
#include "ast-resolver/test_resolver.h"
#include "ast-references/test_references.h"
#include "ast-callers/test_callers.h"
#include "ast-callees/test_callees.h"
#include "ast-semantic-diff/test_semantic_diff.h"
#include "ast-context-export/test_context_export.h"
#include <cassert>
#include <iostream>

int main(void)
{
    using namespace ast;

    bool all_passed = true;

    // Targeted regression tests
    all_passed &= testConversionOperators();
    all_passed &= testMemberOperators();

    // Per-language extractor test suites
    all_passed &= run_tests_c();
    all_passed &= run_tests_cpp();
    all_passed &= run_tests_python();
    all_passed &= run_tests_rust();
    all_passed &= run_tests_go();
    all_passed &= run_tests_java();
    all_passed &= run_tests_javascript();
    all_passed &= run_tests_scope();
    all_passed &= run_tests_scope_builder();
    all_passed &= run_tests_symbol_scope();
    all_passed &= run_tests_scope_visibility();
    all_passed &= run_tests_lookup();
    all_passed &= run_tests_workspace();
    all_passed &= run_tests_search();
    all_passed &= run_tests_resolver();
    all_passed &= run_tests_references();
    all_passed &= run_tests_callers();
    all_passed &= run_tests_callees();
    all_passed &= run_tests_semantic_diff();
    all_passed &= run_tests_context_export();

    std::cout << "===================================================" << std::endl;
    std::cout << (all_passed ? "ALL TESTS PASSED" : "SOME TESTS FAILED") << std::endl;

    // Smoke-test the dump command
    {
        Arguments arguments;
        const char* argv[] = {"ast-tool", "dump", "data/test00.cpp"};
        if(!parse(arguments, 3, argv)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }
    }

    // Smoke-test the symbols command
    {
        Arguments arguments;
        const char* argv0[] = {"ast-tool", "symbols", "data/test00.cpp"};
        if(!parse(arguments, 3, argv0)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }

        const char* argv1[] = {"ast-tool", "symbols", "--json", "data/test00.cpp", "--pretty"};
        if(!parse(arguments, 5, argv1)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }
    }

    // Smoke-test the outline command
    {
        Arguments arguments;
        const char* argv[] = {"ast-tool", "outline", "data/test00.cpp"};
        if(!parse(arguments, 3, argv)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }
    }

    // Smoke-test the find command
    {
        Arguments arguments;
        const char* argv0[] = {"ast-tool", "find", "--type", "function_definition", "data/test00.cpp"};
        if(!parse(arguments, 5, argv0)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }

        const char* argv1[] = {"ast-tool", "find", "--type", "identifier", "--text", "status", "data/test00.cpp"};
        if(!parse(arguments, 7, argv1)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }

        const char* argv2[] = {"ast-tool", "find", "--line", "42", "--column", "5", "data/test00.cpp"};
        if(!parse(arguments, 7, argv2)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }

        const char* argv3[] = {"ast-tool", "find", "--id", "45961E8F", "data/test00.cpp"};
        if(!parse(arguments, 5, argv3)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }
    }
    // Smoke-test the range command
    {
        Arguments arguments;
        const char* argv0[] = {"ast-tool", "range", "--start-line", "68", "--start-column", "79", "--end-line", "68", "--end-column", "79", "data/test00.cpp"};
        if(!parse(arguments, 11, argv0)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }
    }
    // Smoke-test the parent command
    {
        Arguments arguments;
        const char* argv0[] = {"ast-tool", "parent", "--id", "9E52E360", "data/test00.cpp"};
        if(!parse(arguments, 5, argv0)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }
    }

    // Smoke-test the children command
    {
        Arguments arguments;
        const char* argv0[] = {"ast-tool", "children", "--id", "9E52E360", "data/test00.cpp"};
        if(!parse(arguments, 5, argv0)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }
    }

    // Smoke-test the search command with regex flags
    {
        Arguments arguments;
        const char* argv0[] = {"ast-tool", "search", "--name-regex", "^alpha", "test/ast-workspace/workspace"};
        if(!parse(arguments, 5, argv0)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }

        const char* argv1[] = {"ast-tool", "search", "--fqn-regex", "^AlphaNs::", "--json", "test/ast-workspace/workspace"};
        if(!parse(arguments, 6, argv1)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }

        const char* argv2[] = {"ast-tool", "search", "--file-regex", "\\.cpp$", "test/ast-workspace/workspace"};
        if(!parse(arguments, 5, argv2)){
            assert(false);
            return -1;
        }
        if(!dispatch(arguments)){
            assert(false);
            return -1;
        }
    }

    return all_passed ? 0 : 1;
}
