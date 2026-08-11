---
name: ast-tool
description: >
  Analyzes source code AST structure and extracts semantic symbols using tree-sitter.
  Use when inspecting code structure, locating declarations, navigating syntax trees,
  finding nodes at a cursor position, extracting named symbols (functions, classes,
  variables, enums, namespaces) from a single file, or querying an entire workspace
  for declarations by name, kind, or file pattern. Supports C, C++, C#, Python,
  JavaScript, TypeScript, TSX, Go, Rust, Java, Bash, Ruby, Scala, CSS, and HTML.
compatibility: Requires ast-tool binary on PATH. Tested on Linux, macOS, and Windows.
metadata:
  version: "1.0"
---

# ast-tool Skill Guide

This document is an operational guide for AI coding agents using `ast-tool`.
It explains *when* to reach for the tool, *why* each command exists, and *how* commands compose into workflows.
It is not a command reference — for full option details run `ast-tool help <command>`.

---

## Overview

`ast-tool` is a command-line AST analysis tool for source code.
It provides two layers of analysis:

- **AST inspection** — parse a file and navigate the syntax tree directly.
- **Semantic analysis** — extract and query named symbols across files.

Use `ast-tool` when you need precise, structural answers about source code that would be unreliable or expensive to obtain through text search or manual reading.

---

## Design Philosophy

Understanding the layering helps you pick the right command.

```
Source file
    ↓  tree-sitter (parsing only)
AST IR  (stable intermediate representation)
    ↓  extractors
Semantic objects  (symbols, scopes, qualifiers)
    ↓  workspace services
Search, references, callers, callees, diff, export
```

**Tree-sitter** performs parsing. It knows nothing about language semantics — it only produces a concrete syntax tree.

**AST IR** is a stable, parser-independent representation of that tree. All commands that navigate the AST (`dump`, `outline`, `find`, `range`, `parent`, `children`) operate on this layer.

**Semantic objects** are produced by language-specific extractors that interpret the AST. They carry fully-qualified names, kinds, access modifiers, and scope information. Commands like `symbols` and `search` operate at this layer.

**Workspace services** consume semantic objects to answer cross-file questions. Semantic analysis (`references`, `callers`, `callees`) is not yet exposed as CLI commands but will appear here.

The key principle: **higher layers never interact with tree-sitter directly**. As an agent, prefer the highest layer that answers your question. Use semantic commands before AST commands. Use targeted AST queries before raw dumps.

---

## Core Concepts

### Node ID

Every AST node has a stable 32-bit hash (displayed as uppercase hex, e.g. `9E52E360`).
Node IDs are consistent for the same file content and are the primary way to refer to a specific node across commands.

Obtain IDs from `dump`, `outline`, or `find`, then pass them to `parent` or `children`.

### Fully-Qualified Name (FQN)

Semantic symbols carry a fully-qualified name that encodes their full lexical path (e.g. `ast::Parser::parse`).
FQNs are the primary key for semantic lookups.

### Symbol Kind

Symbols are classified by kind: `function`, `method`, `class`, `struct`, `enum`, `variable`, `namespace`, and others.
Use `--kind` filters to narrow results.

### Line and Column Coordinates

All positional output uses 1-based line and column numbers.

---

## Command Selection Guide

| Goal | Command |
|------|---------|
| See every syntax node in a file | `dump` |
| Get a readable structural overview of a file | `outline` |
| Locate nodes by type, text, or position | `find` |
| Find nodes overlapping a line/column range | `range` |
| Walk up to the enclosing construct | `parent` |
| Inspect the parts of a construct | `children` |
| List all named symbols in a file | `symbols` |
| Find symbols by name, kind, or file across a workspace | `search` |

---

## Typical Workflows

### I want to understand the structure of a file

Start with `outline`. It prints only named nodes, indented by depth, which gives a compact structural overview without the noise of punctuation tokens.

```
ast-tool outline src/parser.cpp
```

If you need every node including keywords and punctuation, use `dump` instead.

### I want to locate a declaration

If you know the name, use `symbols` for a single file or `search` for a workspace:

```
ast-tool symbols src/parser.cpp
ast-tool search --name parse src/
```

If you know the line number (e.g. from a compiler error), use `find` with `--line` and `--column`:

```
ast-tool find --line 42 --column 1 src/parser.cpp
```

### I want to inspect a specific construct

1. Locate the node with `find` or `outline` to get its ID.
2. Use `children` to see what it contains.
3. Use `parent` to see what contains it.

```
ast-tool find --type function_definition --text parse src/parser.cpp
ast-tool children --id 9E52E360 src/parser.cpp
ast-tool parent   --id 9E52E360 src/parser.cpp
```

### I want to understand what symbols a file exports

Use `symbols` with `--json --pretty` to get the full structured view including access modifiers and qualifiers:

```
ast-tool symbols --json --pretty include/parser.hpp
```

This is the preferred first step before modifying a public interface.

### I want to find all classes or functions in a codebase

Use `search` with a `--kind` filter:

```
ast-tool search --kind class src/
ast-tool search --kind function --fqn-regex '^ast::' src/
```

### I want to find code related to a concept

Use `search` with `--name` (substring) or `--name-regex` (RE2):

```
ast-tool search --name parse src/
ast-tool search --name-regex '^(parse|lex)' src/
```

### I want to inspect the AST of a line range

Use `range` with start and end coordinates:

```
ast-tool range --start-line 10 --end-line 25 src/codegen.cpp
```

This is useful when reviewing a diff or focusing on a known code region.

---

## Best Practices

**Prefer semantic commands over AST commands** when you need named concepts.
`search --kind function` is more reliable than pattern-matching `function_definition` nodes manually.

**Use the most specific command available.**
If you need one symbol, use `find --type identifier --text <name>`, not `dump` followed by manual filtering.

**Use `--json` for programmatic consumption.**
When you need to process output in multiple steps, `--json` gives structured data that is easier to parse than plain text.

**Prefer `outline` over `dump` for structural exploration.**
`dump` includes thousands of anonymous tokens. `outline` surfaces only named nodes, which is almost always what you want when reading code.

**Chain commands using node IDs.**
`find` → `children` → `parent` is a natural traversal pattern. Capture the ID from one command and feed it to the next.

**Use `search` at workspace scope before `symbols` at file scope.**
If you don't know which file contains something, start with `search` across the directory. Narrow to a file only once you know where to look.

---

## Common Mistakes

**Using `dump` when `outline` is sufficient.**
`dump` emits every token including `{`, `;`, and keywords. Use it only when you specifically need anonymous tokens or exact node counts.

**Using text search (grep) instead of `find --type` or `search --kind`.**
Text search matches comments, strings, and identifiers indiscriminately. `find` and `search` operate on the parsed structure and are precise.

**Manually parsing plain-text output when `--json` is available.**
The plain-text format is for human reading. For multi-step processing use `--json`.

**Traversing the entire AST manually when a targeted query is available.**
If you want function definitions, use `find --type function_definition`, not `dump` followed by line-by-line filtering.

**Using AST node types for semantic questions.**
AST node types are grammar-level (e.g. `function_definition`). For questions about named entities — "what functions are in namespace X?" — use semantic commands (`symbols`, `search`) which carry FQNs and kinds.

**Assuming line numbers are 0-based.**
All `ast-tool` output uses 1-based line and column numbers.

---

## Examples

### Exploring unfamiliar code

You encounter `src/codegen.cpp` for the first time.

```
# Get a structural overview
ast-tool outline src/codegen.cpp

# List all named symbols with their qualifiers
ast-tool symbols --json --pretty src/codegen.cpp
```

The outline shows the nesting structure; `symbols` tells you the public API.

### Locating a function definition

You need to find where `ast::Parser::parseExpression` is defined.

```
ast-tool search --fqn 'Parser::parseExpression' src/
```

The result gives you the file, line, and column. Navigate there directly.

### Understanding what a function contains

You have a node ID from `find` and want to see what is inside a function body.

```
# Find the function node
ast-tool find --type function_definition --text parseExpression src/parser.cpp

# Inspect its children (parameters, body, return type)
ast-tool children --id <node-id> src/parser.cpp
```

### Identifying the enclosing class of a method

You are looking at a method and want to know which class it belongs to.

```
# Start from the method's node
ast-tool find --line 120 --column 5 src/parser.cpp

# Walk up until you reach a class_specifier or struct_specifier
ast-tool parent --id <method-id> src/parser.cpp
ast-tool parent --id <parent-id> src/parser.cpp
```

### Reviewing all classes in a header

Before modifying a header file, get a structural picture of what it declares.

```
ast-tool search --kind class --file include/parser.hpp src/
# or, scoped to a single file:
ast-tool symbols --json --pretty include/parser.hpp
```

### Inspecting a compiler-error location

A compiler reports an error at `src/lexer.cpp:87:12`. Identify the node.

```
ast-tool find --line 87 --column 12 src/lexer.cpp
```

Then inspect its context:

```
ast-tool parent --id <node-id> src/lexer.cpp
```

---

## Limitations

- `ast-tool` parses individual files; it does not perform whole-program type resolution or cross-translation-unit analysis beyond the workspace services.
- Local variables inside function bodies are **not** extracted into the symbol table. `symbols` and `search` return only declarations visible at namespace or class scope.
- Function overloads with identical fully-qualified names are deduplicated to the first occurrence in the symbol table.
- Node IDs are stable for a given file content but change when the file is modified.
- Semantic analysis is language-specific. Not all extractors support all symbol kinds equally.

---

## Quick Reference

```
ast-tool outline <file>                                # structural overview
ast-tool symbols <file>                                # symbols in a file
ast-tool search --kind <kind> <root>                   # symbols in a workspace
ast-tool find --type <type> <file>                     # nodes by grammar type
ast-tool find --line <n> --column <n> <file>           # node at cursor position
ast-tool range --start-line <n> --end-line <n> <file>  # nodes in a range
ast-tool children --id <hex> <file>                    # children of a node
ast-tool parent   --id <hex> <file>                    # parent of a node
ast-tool dump <file>                                   # all nodes (verbose)
```

Run `ast-tool help <command>` for full option documentation.
