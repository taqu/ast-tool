cmake_minimum_required(VERSION 3.50)

set(CMAKE_CONFIGURATION_TYPES "Debug" "Release")

set(PROJECT_NAME ast-tool)
project(${PROJECT_NAME})

set(PROJECT_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
set(HEADER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(SOURCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/src")
include_directories(AFTER ${HEADER_ROOT})

# tree-sitter
set(TREESITTER_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/tree-sitter")
set(TREESITTER_INCLUDE ${TREESITTER_ROOT}/include)
set(TREESITTER_LIB ${TREESITTER_ROOT}/lib)
set(TREESITTER_BIN ${TREESITTER_ROOT}/bin)
include_directories(AFTER ${TREESITTER_INCLUDE})
link_directories(AFTER ${TREESITTER_LIB})
add_library(TREESITTER MODULE IMPORTED)
set_target_properties(TREESITTER PROPERTIES IMPORTED_LOCATION_DEBUG "tree-sitterd.lib" IMPORTED_LOCATION_RELEASE "tree-sitter.lib")

set(TREESITTER_LANGS
    "tree-sitter-bash.lib"
    "tree-sitter-c.lib"
    "tree-sitter-cpp.lib"
    "tree-sitter-c-sharp.lib"
    "tree-sitter-css.lib"
    "tree-sitter-go.lib"
    "tree-sitter-html.lib"
    "tree-sitter-java.lib"
    "tree-sitter-javascript.lib"
    "tree-sitter-python.lib"
    "tree-sitter-ruby.lib"
    "tree-sitter-rust.lib"
    "tree-sitter-scala.lib"
    "tree-sitter-tsx.lib"
    "tree-sitter-typescript.lib"
)

set(MIMALLOC_INCLUDE "${HEADER_ROOT}/mimalloc")
set(MIMALLOC_SOURCE "${SOURCE_ROOT}/mimalloc")
set(MIMALLOC_HEADERS
    "${HEADER_ROOT}/mimalloc.h"
    "${HEADER_ROOT}/mimalloc-new-delete.h"
)

set(MIMALLOC_SOURCES
    "${MIMALLOC_SOURCE}/alloc.c"
    "${MIMALLOC_SOURCE}/alloc-aligned.c"
    "${MIMALLOC_SOURCE}/alloc-posix.c"
    "${MIMALLOC_SOURCE}/arena.c"
    "${MIMALLOC_SOURCE}/arena-meta.c"
    "${MIMALLOC_SOURCE}/bitmap.c"
    "${MIMALLOC_SOURCE}/heap.c"
    "${MIMALLOC_SOURCE}/init.c"
    "${MIMALLOC_SOURCE}/libc.c"
    "${MIMALLOC_SOURCE}/options.c"
    "${MIMALLOC_SOURCE}/os.c"
    "${MIMALLOC_SOURCE}/page.c"
    "${MIMALLOC_SOURCE}/page-map.c"
    "${MIMALLOC_SOURCE}/random.c"
    "${MIMALLOC_SOURCE}/stats.c"
    "${MIMALLOC_SOURCE}/theap.c"
    "${MIMALLOC_SOURCE}/threadlocal.c"
    "${MIMALLOC_SOURCE}/prim/prim.c")

set(RE2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/re2")
set(RE2_INCLUDE ${RE2_ROOT}/include)
set(RE2_LIB ${RE2_ROOT}/lib)
include_directories(AFTER ${RE2_INCLUDE})
link_directories(AFTER ${RE2_LIB})
add_library(RE2 MODULE IMPORTED)
set_target_properties(RE2 PROPERTIES IMPORTED_LOCATION_DEBUG "re2d.lib" IMPORTED_LOCATION_RELEASE "re2.lib")

set(ABSL_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/absl")
set(ABSL_INCLUDE ${ABSL_ROOT}/include)
set(ABSL_LIB ${ABSL_ROOT}/lib)
include_directories(AFTER ${ABSL_INCLUDE})
link_directories(AFTER ${ABSL_LIB})
add_library(ABSL MODULE IMPORTED)
include("${ABSL_ROOT}/absl_libs.cmake")

file(GLOB DLL_FILES "${TREESITTER_BIN}/*.dll")

########################################################################
# Sources
set(HEADERS
    "${HEADER_ROOT}/ast-ir.h"
    "${HEADER_ROOT}/ast-tool.h"
    "${HEADER_ROOT}/ast-extractor.h"
    "${HEADER_ROOT}/ast-scope.h"
    "${HEADER_ROOT}/ast-scope-builder.h"
    "${HEADER_ROOT}/ast-symbol-scope.h"
    "${HEADER_ROOT}/ast-scope-visibility.h"
    "${HEADER_ROOT}/ast-lookup.h"
    "${HEADER_ROOT}/ast-workspace.h"
    "${HEADER_ROOT}/ast-regex.h"
    "${HEADER_ROOT}/ast-search.h"
    "${HEADER_ROOT}/ast-resolver.h"
    "${HEADER_ROOT}/ast-references.h"
    "${HEADER_ROOT}/ast-callers.h"
    "${HEADER_ROOT}/ast-callees.h"
    "${HEADER_ROOT}/ast-semantic-diff.h"
    "${HEADER_ROOT}/ast-context-export.h"
)

set(SOURCES
    "${SOURCE_ROOT}/ast-tool.cpp"
    "${SOURCE_ROOT}/ast-extractor.cpp"
    "${SOURCE_ROOT}/ast-extractor-common.h"
    "${SOURCE_ROOT}/ast-extractor-common.cpp"
    "${SOURCE_ROOT}/ast-extractor-langs.h"
    "${SOURCE_ROOT}/ast-extractor-cfamily.h"
    "${SOURCE_ROOT}/ast-extractor-cfamily.cpp"
    "${SOURCE_ROOT}/ast-extractor-bash.cpp"
    "${SOURCE_ROOT}/ast-extractor-c.cpp"
    "${SOURCE_ROOT}/ast-extractor-cpp.cpp"
    "${SOURCE_ROOT}/ast-extractor-csharp.cpp"
    "${SOURCE_ROOT}/ast-extractor-css.cpp"
    "${SOURCE_ROOT}/ast-extractor-go.cpp"
    "${SOURCE_ROOT}/ast-extractor-html.cpp"
    "${SOURCE_ROOT}/ast-extractor-java.cpp"
    "${SOURCE_ROOT}/ast-extractor-javascript.cpp"
    "${SOURCE_ROOT}/ast-extractor-python.cpp"
    "${SOURCE_ROOT}/ast-extractor-ruby.cpp"
    "${SOURCE_ROOT}/ast-extractor-rust.cpp"
    "${SOURCE_ROOT}/ast-extractor-scala.cpp"
    "${SOURCE_ROOT}/ast-extractor-typescript.cpp"
    "${SOURCE_ROOT}/ast-extractor-tsx.cpp"
    "${SOURCE_ROOT}/dump.h"
    "${SOURCE_ROOT}/dump.cpp"
    "${SOURCE_ROOT}/symbols.h"
    "${SOURCE_ROOT}/symbols.cpp"
    "${SOURCE_ROOT}/outline.h"
    "${SOURCE_ROOT}/outline.cpp"
    "${SOURCE_ROOT}/ast-format.h"
    "${SOURCE_ROOT}/ast-format.cpp"
    "${SOURCE_ROOT}/ast-find.h"
    "${SOURCE_ROOT}/ast-find.cpp"
    "${SOURCE_ROOT}/find.h"
    "${SOURCE_ROOT}/find.cpp"
    "${SOURCE_ROOT}/range.h"
    "${SOURCE_ROOT}/range.cpp"
    "${SOURCE_ROOT}/parent.h"
    "${SOURCE_ROOT}/parent.cpp"
    "${SOURCE_ROOT}/children.h"
    "${SOURCE_ROOT}/children.cpp"
    "${SOURCE_ROOT}/ast-regex.cpp"
    "${SOURCE_ROOT}/ast-search.cpp"
    "${SOURCE_ROOT}/ast-resolver.cpp"
    "${SOURCE_ROOT}/ast-references.cpp"
    "${SOURCE_ROOT}/ast-callers.cpp"
    "${SOURCE_ROOT}/ast-callees.cpp"
    "${SOURCE_ROOT}/ast-call-utils.h"
    "${SOURCE_ROOT}/ast-semantic-diff.cpp"
    "${SOURCE_ROOT}/ast-context-export.cpp"
    "${SOURCE_ROOT}/search.h"
    "${SOURCE_ROOT}/search.cpp"
    "${SOURCE_ROOT}/help.h"
    "${SOURCE_ROOT}/help.cpp"
    "${SOURCE_ROOT}/ast-scope.cpp"
    "${SOURCE_ROOT}/ast-scope-builder.cpp"
    "${SOURCE_ROOT}/ast-symbol-scope.cpp"
    "${SOURCE_ROOT}/ast-scope-visibility.cpp"
    "${SOURCE_ROOT}/ast-lookup.cpp"
    "${SOURCE_ROOT}/ast-workspace.cpp"
)
set(XXHASH_SOURCES
    "${SOURCE_ROOT}/xxhash.h"
    "${SOURCE_ROOT}/xxhash.c"
)
set(MAIN_SOURCES
    "${SOURCE_ROOT}/main.cpp"
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

if(MSVC)
    set(DEFAULT_CXX_FLAGS "/DWIN32 /D_WINDOWS /D_MSBC /W4 /WX- /nologo /fp:precise /arch:AVX2 /Zc:wchar_t /TP /Gd /utf-8")
    if("1800" VERSION_LESS MSVC_VERSION)
        set(DEFAULT_CXX_FLAGS "${DEFAULT_CXX_FLAGS} /EHsc")
    endif()

    set(CMAKE_CXX_FLAGS "${DEFAULT_CXX_FLAGS}")
    set(CMAKE_CXX_FLAGS_DEBUG "/D_DEBUG /MDd /Zi /Ob0 /Od /RTC1 /Gy /GR- /GS /Gm-")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2 /GL /GR- /DNDEBUG")
    target_link_libraries(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:TREESITTER ${TREESITTER_LANGS} ${ABSL_DEBUG_LIBS} RE2>
        $<$<NOT:$<CONFIG:Debug>>:TREESITTER ${TREESITTER_LANGS} ${ABSL_RELEASE_LIBS} RE2>)
elseif(UNIX)
    set(DEFAULT_CXX_FLAGS "-Wall -O0 -g -march=x86-64-v3")
    set(CMAKE_CXX_FLAGS "${DEFAULT_CXX_FLAGS}")
    target_link_libraries(${PROJECT_NAME}
    PRIVATE
        $<$<CONFIG:Debug>:TREESITTER ${TREESITTER_LANGS} ${ABSL_DEBUG_LIBS} RE2>
        $<$<NOT:$<CONFIG:Debug>>:TREESITTER ${TREESITTER_LANGS} ${ABSL_RELEASE_LIBS} RE2>)
elseif(APPLE)
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>") 
set_target_properties(${PROJECT_NAME}
    PROPERTIES
        OUTPUT_NAME_DEBUG "${PROJECT_NAME}" OUTPUT_NAME_RELEASE "${PROJECT_NAME}"
        VS_DEBUGGER_WORKING_DIRECTORY "${PROJECT_ROOT}")
add_custom_command(
    TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${DLL_FILES}
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    COMMENT "Copying external DLLs..."
)

add_subdirectory(test)

