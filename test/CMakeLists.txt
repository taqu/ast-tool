cmake_minimum_required(VERSION 3.50)

set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")

set(PROJECT_NAME ast-tool-test)
project(${PROJECT_NAME})

set(MAIN_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/test.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/test.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_helpers.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    # C extractor tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-c/test_extractor_c.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-c/test_extractor_c.cpp"
    # C++ extractor tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-cpp/test_extractor_cpp.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-cpp/test_extractor_cpp.cpp"
    # Python extractor tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-python/test_extractor_python.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-python/test_extractor_python.cpp"
    # Rust extractor tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-rust/test_extractor_rust.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-rust/test_extractor_rust.cpp"
    # Go extractor tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-go/test_extractor_go.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-go/test_extractor_go.cpp"
    # Java extractor tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-java/test_extractor_java.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-java/test_extractor_java.cpp"
    # JavaScript extractor tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-javascript/test_extractor_javascript.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-extractor-javascript/test_extractor_javascript.cpp"
    # Scope model tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-scope/test_scope.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-scope/test_scope.cpp"
    # Scope builder tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-scope/test_scope_builder.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-scope/test_scope_builder.cpp"
    # Symbol-scope association tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-symbol-scope/test_symbol_scope.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-symbol-scope/test_symbol_scope.cpp"
    # Scope visibility tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-scope-visibility/test_scope_visibility.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-scope-visibility/test_scope_visibility.cpp"
    # Lexical name lookup tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-lookup/test_lookup.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-lookup/test_lookup.cpp"
    # Workspace analysis tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-workspace/test_workspace.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-workspace/test_workspace.cpp"
    # Semantic search + regex tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-search/test_search.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-search/test_search.cpp"
    # Identifier resolver tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-resolver/test_resolver.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-resolver/test_resolver.cpp"
    # Find references tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-references/test_references.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-references/test_references.cpp"
    # Callers tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-callers/test_callers.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-callers/test_callers.cpp"
    # Callees tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-callees/test_callees.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-callees/test_callees.cpp"
    # Semantic diff tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-semantic-diff/test_semantic_diff.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-semantic-diff/test_semantic_diff.cpp"
    # Context export tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-context-export/test_context_export.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-context-export/test_context_export.cpp"
    # ASTNodeType enum tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-node-type/test_node_type.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-node-type/test_node_type.cpp"
    # AST cache tests
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-cache/test_cache.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/ast-cache/test_cache.cpp"
)

source_group("include" FILES ${HEADERS})
source_group("src" FILES ${SOURCES} ${MAIN_SOURCES})
source_group("include/mimalloc" FILES ${MIMALLOC_HEADERS})
source_group("src/mimalloc" FILES ${MIMALLOC_SOURCES})
source_group("src/xxhash" FILES ${XXHASH_SOURCES})

set(FILES ${HEADERS} ${SOURCES})

set(OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/bin")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${OUTPUT_DIRECTORY}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${OUTPUT_DIRECTORY}")

add_executable(${PROJECT_NAME} ${FILES} ${MIMALLOC_SOURCES} ${XXHASH_SOURCES} ${MAIN_SOURCES})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 23)

# Make test headers discoverable from any sub-directory
target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${CMAKE_CURRENT_SOURCE_DIR}/../src")

if(MSVC)
    set(CMAKE_CXX_FLAGS "${DEFAULT_CXX_FLAGS} -DAST_TOOL_TEST")
    target_link_libraries(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:TREESITTER ${TREESITTER_LANGS} ${ABSL_DEBUG_LIBS} RE2 GIT2 winhttp.lib rpcrt4.lib crypt32.lib>
        $<$<NOT:$<CONFIG:Debug>>:TREESITTER ${TREESITTER_LANGS} ${ABSL_RELEASE_LIBS} RE2 GIT2 winhttp.lib rpcrt4.lib crypt32.lib>)

elseif(UNIX)
    target_compile_options(${PROJECT_NAME} PRIVATE "-Wall;-O3;-g;-march=x86-64-v3;-DAST_TOOL_TEST")
    target_link_libraries(${PROJECT_NAME}
        PRIVATE
            TREESITTER ${TREESITTER_LANGS} ${RE2_LIBRARIES} PkgConfig::pkg_cfg_libgit2 absl::log_initialize
            Threads::Threads)
elseif(APPLE)
endif()

set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        OUTPUT_NAME_DEBUG "${PROJECT_NAME}" OUTPUT_NAME_RELEASE "${PROJECT_NAME}"
        VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_ROOT}")

if(MSVC)
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DLL_FILES}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying external DLLs..."
)
endif()

