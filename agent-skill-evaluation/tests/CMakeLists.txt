include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
)
FetchContent_MakeAvailable(Catch2)

list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(Catch)

# test_lexer
add_executable(test_lexer test_lexer.cpp)
target_link_libraries(test_lexer PRIVATE eval Catch2::Catch2WithMain)
catch_discover_tests(test_lexer)

# test_parser
add_executable(test_parser test_parser.cpp)
target_link_libraries(test_parser PRIVATE eval Catch2::Catch2WithMain)
catch_discover_tests(test_parser)

# test_semantic
add_executable(test_semantic test_semantic.cpp)
target_link_libraries(test_semantic PRIVATE eval Catch2::Catch2WithMain)
catch_discover_tests(test_semantic)
