---
name: semantic-analysis
description: Extract named semantic symbols (functions, classes, variables, enums, namespaces, etc.) from a single source file using ast-tool symbols.
triggers:
  - "list symbols"
  - "extract symbols"
  - "what functions are in"
  - "what classes are in"
  - "symbol table"
  - "declarations in file"
languages: [c, cpp, csharp, python, javascript, typescript, tsx, go, rust, java, bash, ruby, scala, css, html]
---

# Semantic Analysis Skill

## Purpose

Extract the semantic symbol table from a single source file: functions, classes, variables, enums, namespaces, and other named declarations — without needing to build the project.

## Command Covered

| Command   | Description                                              |
|-----------|----------------------------------------------------------|
| `symbols` | Extract named semantic symbols from a single source file |

## Output Formats

- Plain text (default)
- JSON (`--json`)
- Pretty-printed JSON (`--pretty`, implies `--json`)

## When to Use This Skill

- The user wants to enumerate all declarations in a file.
- The user wants to check whether a function or class is defined in a given file.
- The user wants a symbol table for documentation or code-review purposes.
- Preliminary step before using `workspace-analysis` to understand what kinds of symbols exist in the codebase.

## Symbol Kinds

Symbol kinds depend on the language. Common kinds include:

`function`, `method`, `class`, `struct`, `enum`, `variable`, `constant`, `namespace`, `interface`, `type_alias`

## Related Skills

- `ast-inspection` — for raw AST node traversal within a file
- `workspace-analysis` — for searching symbols across an entire project directory

## Tool Selection

### Use `ast-tool symbols` when the task involves

- Enumerating all named declarations in a source file
- Checking whether a specific function or class is defined in a file
- Producing a symbol table for documentation or code review
- Obtaining semantic-level information (kind, name, location) without building the project

`symbols` understands what the language considers a declaration. It does not require a full build and handles all supported languages consistently.

### Use text search (grep, ripgrep) when the task involves

- Finding TODO or FIXME comments
- Locating string literals or log messages
- Searching documentation or configuration files
- Finding text that is not a named declaration

Text search is appropriate when the query is not about named language constructs.

### Decision Guide

| Question | Tool |
|---|---|
| "What functions are defined in this file?" | `symbols` |
| "Is `MyClass` declared in this file?" | `symbols --json` + filter |
| "What is the symbol table for this header?" | `symbols` |
| "Find this TODO comment" | text search |
| "Where is X defined across the project?" | `search` (workspace-analysis skill) |
| "Show me the raw AST" | `dump` / `outline` (ast-inspection skill) |

## Common Mistakes

**Using grep to find function declarations.**
`grep "void parse"` matches occurrences in comments, strings, and call sites, not only declarations. Use `symbols --json` and filter by kind to get authoritative declaration information.

**Counting occurrences of a name to infer whether it is defined.**
Occurrence count is not declaration count. A name can appear many times without being declared in a file. Use `symbols` to determine whether a declaration exists.

**Manually extracting symbol names with regex.**
Hand-written patterns break on edge cases (templates, macros, multi-line signatures). `symbols` uses the language grammar and produces accurate, structured output.

**Treating text-search results as a symbol table.**
Text search does not distinguish declaration from call site, comment, or string. Always use `symbols` when declaration-level information is required.

## Best Practices

- Use `symbols` as the first step when investigating what a file declares, before reading raw source.
- Use `--json` output when filtering by kind or piping to `jq` for downstream processing.
- Prefer `symbols` over `ast-inspection` commands when named declarations are the goal; raw AST access is only necessary when syntactic detail beyond declarations is needed.
- Use `workspace-analysis` (`search`) when the target file is unknown; use `symbols` when the file is already identified.
- Avoid reparsing files by caching `--json` output when the same file will be queried multiple times.
