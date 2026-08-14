---
name: semantic-analysis command selection
description: When to use symbols vs other ast-tool commands
---

# Command Selection Guide — Semantic Analysis

## symbols vs AST Inspection Commands

| Need                                          | Use           |
|-----------------------------------------------|---------------|
| Named declarations (functions, classes, etc.) | `symbols`     |
| Every raw parse-tree node                     | `dump`        |
| Structural overview of named nodes            | `outline`     |
| Node at a specific cursor position            | `find --line` |
| Nodes of a specific grammar type              | `find --type` |

`symbols` operates at the **semantic** level — it understands what the language considers a declaration. `dump`/`outline`/`find` operate at the **syntactic** level — they expose the raw parse tree without semantic interpretation.

## symbols vs search (Workspace)

| Scope              | Command   |
|--------------------|-----------|
| Single file        | `symbols` |
| Entire directory   | `search`  |

Use `symbols` when you already know which file you care about. Use `search` (workspace-analysis skill) when you need to find where a symbol is declared across many files.

## Tool Selection: ast-tool vs Text Search

| Task | Use |
|---|---|
| Enumerate all declarations in a file | `symbols` |
| Check if a function is defined in a file | `symbols --json` + filter |
| Find where a symbol is declared in the project | `search` (workspace-analysis) |
| Get raw parse-tree structure | `dump` / `outline` (ast-inspection) |
| Find TODO comments | text search (grep) |
| Find log message strings | text search (grep) |
| Search documentation files | text search (grep) |

Use `symbols` when the goal is semantic: what named constructs exist and where. Use text search when the goal is non-structural content that does not correspond to a language declaration.

---

## Output Format Selection

| Use case                           | Flag      |
|------------------------------------|-----------|
| Quick human scan                   | (default) |
| Downstream processing (`jq`, etc.) | `--json`  |
| Debugging / formatted reading      | `--pretty`|
