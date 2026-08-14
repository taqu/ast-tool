---
name: workspace-analysis
description: Search for symbol declarations across an entire project directory using ast-tool search. Supports filtering by name, FQN, kind, and file with exact substring or RE2 regex matching.
triggers:
  - "search symbols"
  - "find declaration"
  - "where is X defined"
  - "search across project"
  - "find all classes"
  - "find function in workspace"
  - "search codebase for"
languages: [c, cpp, csharp, python, javascript, typescript, tsx, go, rust, java, bash, ruby, scala, css, html]
---

# Workspace Analysis Skill

## Purpose

Query the symbol table across an entire source directory — not just a single file — to find where declarations are defined, enumerate all symbols of a given kind, or locate symbols matching a pattern.

## Command Covered

| Command  | Description                                         |
|----------|-----------------------------------------------------|
| `search` | Query symbols across an entire workspace directory  |

## Filter Types

### Exact (substring) Filters

| Flag     | Matches against         |
|----------|-------------------------|
| `--name` | Simple symbol name      |
| `--fqn`  | Fully-qualified name    |
| `--kind` | Symbol kind             |
| `--file` | Source file path        |

### Regex Filters (RE2 syntax)

| Flag            | Matches against      |
|-----------------|----------------------|
| `--name-regex`  | Simple symbol name   |
| `--fqn-regex`   | Fully-qualified name |
| `--file-regex`  | Source file path     |

Multiple filters are ANDed together.

## Output Formats

- Plain text (default)
- JSON (`--json`)
- Pretty-printed JSON (`--pretty`, implies `--json`)

## When to Use This Skill

- The user wants to find where a specific function or class is declared anywhere in the project.
- The user wants to list all symbols of a given kind (e.g., all enums in the codebase).
- The user wants to enumerate all symbols in a subset of files matching a path pattern.
- Cross-file navigation that goes beyond a single file's `symbols` output.

## Related Skills

- `semantic-analysis` — single-file symbol extraction
- `ast-inspection` — raw AST traversal of a single file
- `context-export` — assembling multi-file context for LLM consumption

## Tool Selection

### Use `ast-tool search` when the task involves

- Finding where a specific symbol is declared anywhere in the project
- Listing all symbols of a given kind (all enums, all classes, all functions)
- Enumerating symbols in a subset of files matching a path pattern
- Cross-file navigation that goes beyond a single file's symbol table

`search` provides authoritative declaration locations; it does not rely on text matching and does not produce false positives from comments or string literals.

### Use text search (grep, ripgrep) when the task involves

- Finding TODO or FIXME comments across the project
- Locating string literals or log message text
- Searching build scripts, configuration files, or documentation
- Finding arbitrary text in non-source assets

Text search is appropriate when semantic information about declarations is not required.

### Decision Guide

| Question | Tool |
|---|---|
| "Where is `parse_expression` defined?" | `search --name "parse_expression"` |
| "List all enums in the project" | `search --kind enum` |
| "Find all functions in `src/parser/`" | `search --kind function --file "parser"` |
| "Find all types in the `MyNS` namespace" | `search --fqn-regex "^MyNS::"` |
| "Find TODO comments" | text search |
| "Find log message text" | text search |
| "What is declared in this specific file?" | `symbols` (semantic-analysis skill) |

## Common Mistakes

**Using grep to find symbol declarations.**
`grep "class Foo"` matches occurrences in comments, strings, and forward declarations in addition to the canonical definition. Use `search --name "Foo" --kind class` for authoritative results.

**Walking source files manually to build a symbol list.**
Iterating files and pattern-matching source text is fragile and language-specific. `search` walks the directory tree, parses every file with the correct grammar, and returns structured results.

**Using text search to infer scope or namespace membership.**
Regex cannot reliably determine whether a symbol belongs to a namespace or class scope. Use `--fqn` or `--fqn-regex` to filter by fully-qualified name.

**Re-running search for every downstream question.**
`search --json` output can be saved to a file and queried repeatedly with `jq` without re-parsing the workspace.

## Best Practices

- Start with `search` before using `grep` when looking for symbol declarations.
- Use `--file` or `--file-regex` to restrict the search scope when the relevant files are known.
- Use `--kind` to filter early and reduce result volume.
- Cache `search --json` output to a file for large workspaces and query it with `jq`.
- Use `symbols` (semantic-analysis skill) when the file is already identified; reserve `search` for cross-file queries.
- Prefer exact filters (`--name`, `--kind`, `--file`) before switching to regex variants.
