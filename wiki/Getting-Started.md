# Getting Started

## Prerequisites

- CMake 3.11 or later
- A C++23-capable compiler (MSVC 2022, GCC 13, or Clang 16)
- The bundled tree-sitter libraries and language grammars (included in the repository under `tree-sitter/`)
- RE2 and Abseil (included under `thirdparty/`)

## Building from Source

On Windows (Visual Studio is a multi-config generator, so the build config is chosen with `--config` at build time, not at configure time):

```sh
cmake -B build -DCMAKE_INSTALL_PREFIX="C:\Programs\ast-tool"
cmake --build build --config Release
cmake --install build --config Release
```

`cmake --install` copies `ast-tool.exe`, the required tree-sitter grammar DLLs, and documentation to the install prefix. The installed `bin/` directory is self-contained and relocatable.

On Linux:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
cmake --install build --config Release
```

### Debug Build

```sh
cmake --build build --config Debug
```

### Running Tests

```sh
cmake --build build --config Release --target ast-tool-test
./test/bin/ast-tool-test.exe
```

All tests must pass before submitting changes. See [Contributing](Contributing) for details.

---

## Verifying the Installation

```sh
ast-tool --version
ast-tool --help
```

Expected output:

```
ast-tool 0.1.0
ast-tool — Source code inspection and semantic analysis.

Inspect AST structure, extract semantic symbols, and trace cross-file
symbol relationships (references, callers, callees) across a workspace.

Usage:
    ast-tool <command> [options]

Options:
    -h, --help       Show this help message.
    -v, --version    Show version information.

Commands:

  AST Inspection
    outline     Show the structural outline of a source file.
    find        Find AST nodes by type, text, position, or ID.
    range       Show AST nodes within a source range.
    parent      Show the parent AST node.
    children    Show the child AST nodes.

  Semantic Analysis
    symbols     List symbols in a source file.
    search      Search symbols in the workspace.
    references  Find references to a symbol.
    callers     Find direct callers of a function.
    callees     Find direct callees of a function.

  Cache Management
    cache       Manage the workspace AST cache (warm / status).
    setup       Configure coding-agent session-start hooks.

Run:

    ast-tool help <command>
    ast-tool <command> --help

for detailed documentation of a command.
```

(`dump` also exists — a low-level command that prints every AST node including anonymous punctuation tokens — but is intentionally omitted from this list; run `ast-tool dump --help` directly.)

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

### Relationship queries

```sh
ast-tool references Parser::parse src/
ast-tool callers Parser::parse src/
ast-tool callees Parser::parse src/
```

These report only **direct**, semantically-resolved relationships (not a transitive call graph, not text matches). A `<symbol>` containing `::` is matched as a fully-qualified name; an ambiguous unqualified name fails with a candidate list instead of guessing.

---

## Help System

Every command has a built-in help page:

```sh
ast-tool help outline
ast-tool outline --help    # equivalent
```

The top-level help groups commands by category. Per-command help covers all options, output format, and examples.
