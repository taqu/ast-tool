# Getting Started

## Prerequisites

- CMake 3.50 or later
- A C++23-capable compiler (MSVC 2022, GCC 13, or Clang 16)
- The bundled tree-sitter libraries and language grammars (included in the repository under `tree-sitter/`)
- RE2 and Abseil (included under `thirdparty/`)

## Building

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

The executable is written to `bin/ast-tool` (or `bin/ast-tool.exe` on Windows).

### Debug Build

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### Running Tests

```sh
cmake --build build --target ast-tool-test
./test/bin/ast-tool-test
```

All tests must pass before submitting changes. See [Contributing](Contributing) for details.

---

## Verifying the Installation

```sh
ast-tool --help
```

Expected output:

```
AST analysis toolkit

Usage:
    ast-tool [options] <command> [command options]

Options:
    -h, --help    Show this help message.

Commands:

  AST Inspection
    dump        Print all AST nodes of a source file.
    outline     Print a hierarchical outline of named AST nodes.
    find        Find AST nodes by type, text, position, or ID.
    range       Find AST nodes that intersect a source range.
    parent      Print the parent of an AST node.
    children    Print the children of an AST node.

  Semantic Analysis
    symbols     Extract semantic symbols from a source file.
    search      Search for semantic symbols across a workspace.

Run:

    ast-tool help <command>

for detailed documentation of a command.
```

---

## First Commands

### Structural overview

```sh
ast-tool outline src/main.cpp
```

Prints all named AST nodes indented by depth — the fastest way to understand the structure of an unfamiliar file.

### Symbol extraction

```sh
ast-tool symbols src/main.cpp
```

Lists every named declaration with its fully-qualified name and node ID.

### Finding a specific construct

```sh
ast-tool find --type function_definition src/parser.cpp
```

### Workspace search

```sh
ast-tool search --kind class --fqn-regex '^ast::' src/
```

---

## Help System

Every command has a built-in help page:

```sh
ast-tool help outline
ast-tool outline --help    # equivalent
```

The top-level help groups commands by category. Per-command help covers all options, output format, and examples.
