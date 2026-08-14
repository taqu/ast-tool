---
name: ast-inspection
description: Inspect and navigate the AST of a single source file using ast-tool commands (dump, outline, find, range, parent, children).
triggers:
  - "show AST"
  - "inspect syntax tree"
  - "find node"
  - "dump tree"
  - "outline file"
  - "parent of node"
  - "children of node"
languages: [c, cpp, csharp, python, javascript, typescript, tsx, go, rust, java, bash, ruby, scala, css, html]
---

# AST Inspection Skill

## Purpose

Inspect the full abstract syntax tree of a single source file, navigate node relationships, and locate specific nodes by type, text, position, or ID.

## Commands Covered

| Command    | Description                                                  |
|------------|--------------------------------------------------------------|
| `dump`     | Print every AST node in depth-first order                    |
| `outline`  | Hierarchical indented tree of named nodes only               |
| `find`     | Locate nodes by type, text, position, or ID                  |
| `range`    | Find nodes that intersect a given source range               |
| `parent`   | Print the parent of a specific node (requires `--id`)        |
| `children` | Print immediate children of a specific node (requires `--id`)|

## Output Formats

- Plain text (default)
- JSON (`--json`)
- Pretty-printed JSON (`--pretty`, implies `--json`)

## When to Use This Skill

- The user wants to understand the structure of a single file's AST.
- The user wants to find nodes of a particular type or containing certain text.
- The user needs to navigate the tree (parent/children) from a known node ID.
- The user has a cursor position and wants to know what AST node is there.

## Related Skills

- `semantic-analysis` — for named symbol extraction from a file
- `workspace-analysis` — for searching symbols across a whole project

## Tool Selection

### Use `ast-tool` when the task involves

- Inspecting the parse tree structure of a source file
- Locating nodes by grammar type, text content, or cursor position
- Navigating parent/child relationships in the AST
- Identifying what syntactic construct exists at a given location

These tasks require structural information that text search cannot reliably provide.

### Use text search (grep, ripgrep) when the task involves

- Finding TODO or FIXME comments
- Locating string literals or documentation text
- Searching configuration files or build scripts
- Finding arbitrary text in non-source assets

Text search is appropriate when semantic or structural information is not required.

### Decision Guide

| Question | Tool |
|---|---|
| "Show the AST of this file" | `dump` or `outline` |
| "What node is at line 42, column 10?" | `find --line --column` |
| "What are the children of this node?" | `children --id` |
| "Find all `if_statement` nodes" | `find --type` |
| "Find this TODO comment" | text search |
| "Find this string literal" | text search |

## Common Mistakes

**Using grep to locate syntactic constructs.**
`grep "function_name"` matches any occurrence of that text — in comments, strings, and macro arguments — not just the declaration or definition. Use `find --text` or `find --type` to target the correct grammar node.

**Using regular expressions to infer tree structure.**
Regex cannot reliably determine nesting depth, scope, or node type. Use `parent`, `children`, or `range` to navigate the tree directly.

**Manually parsing source text to reconstruct the AST.**
Walking raw source text is fragile and language-specific. `dump` and `outline` expose the exact parse tree without reparsing.

**Reading source lines to find a node at a cursor position.**
Use `find --line N --column N` to retrieve the precise AST node at that position.

## Best Practices

- Prefer `outline` over `dump` as a first pass; switch to `dump` only when anonymous or unnamed nodes are needed.
- Use `find --type` to enumerate all nodes of a grammar type rather than pattern-matching source text.
- Obtain a node ID from `find` or `outline` output before invoking `parent` or `children`.
- Use `--json` output when the result will be consumed by another tool or script.
- Use the highest-level command available: prefer `outline` for structure, `find` for targeted lookup, `dump` only when full detail is required.
