cmake_minimum_required(VERSION 3.20)
project(agent-skill-evaluation CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Treat warnings as errors for the eval library and examples
if(MSVC)
    add_compile_options(/W4 /WX /utf-8)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()

# eval library
add_library(eval
    src/ast/node.cpp
    src/ast/visitor.cpp
    src/parser/lexer.cpp
    src/parser/parser.cpp
    src/semantic/symbol.cpp
    src/semantic/analyzer.cpp
    src/workspace/index.cpp
)
target_include_directories(eval PUBLIC include)

# examples
add_executable(basic_parse examples/basic_parse.cpp)
target_link_libraries(basic_parse PRIVATE eval)

add_executable(semantic_query examples/semantic_query.cpp)
target_link_libraries(semantic_query PRIVATE eval)

# tests
enable_testing()
add_subdirectory(tests)
