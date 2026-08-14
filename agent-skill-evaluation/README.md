# agent-skill-evaluation

A mini source-code analysis library used as a benchmark for evaluating AI agent skills. The project provides a complete pipeline: Lexer → Parser → Semantic Analyzer → Workspace Index, all implemented in C++17.

## Purpose

This project serves as a realistic codebase for testing AI agent skills including:

- **ast-inspection** — navigating and querying AST nodes in a single file
- **semantic-analysis** — extracting symbol information (functions, variables, parameters)
- **workspace-analysis** — cross-file symbol search and dependency analysis
- **api-review** — reviewing the public API surface of the `eval` library
- **context-export** — assembling structured code context for LLM consumption

The code is intentionally structured to expose interesting analysis scenarios: inheritance hierarchies, visitor patterns, template functions, overloaded methods, and cross-translation-unit dependencies.

## Layout

```
agent-skill-evaluation/
├── CMakeLists.txt          # Root build file
├── include/
│   ├── ast/               # AST node types and visitor base
│   ├── parser/            # Lexer and recursive descent parser
│   ├── semantic/          # Symbol table, scope tree, semantic analyzer
│   └── workspace/         # Multi-file index with template query
├── src/                   # Implementations mirroring include/
├── tests/                 # Catch2 unit tests for each component
├── examples/              # Standalone programs demonstrating the API
└── docs/                  # Evaluation scenarios documentation
```

## Build Instructions

```sh
cmake -B build
cmake --build build
```

To run the tests:

```sh
cmake -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

To run the examples after building:

```sh
./build/basic_parse
./build/semantic_query
```

## Namespaces

| Namespace              | Contents                                        |
|------------------------|-------------------------------------------------|
| `eval::ast`            | Node types, visitor base, PrintVisitor, CollectVisitor |
| `eval::parser`         | TokenKind, Token, Lexer, Parser, ParseError     |
| `eval::semantic`       | SymbolKind, Symbol, Scope, Analyzer, SemanticError |
| `eval::workspace`      | FileEntry, Index (with template `findSymbolIf`) |

## Requirements

- C++17 compiler (GCC 9+, Clang 10+, MSVC 2019+)
- CMake 3.20+
- Internet access at configure time (FetchContent downloads Catch2 v3.7.1)

## Evaluation Goals

Each agent skill targets a different analysis capability:

1. **AST Inspection** — Traverse and display the AST of a single source file, locate nodes by kind, inspect parent/child relationships.
2. **Semantic Analysis** — Extract the symbol table from a file, enumerate functions, variables, and their types.
3. **Workspace Analysis** — Search across all files for a symbol by name or kind, discover which files include which.
4. **API Review** — Enumerate the public declarations of the `eval` library, check for consistency and potential breaking changes.
5. **Context Export** — Compose a structured multi-file context package suitable for LLM consumption or documentation generation.
