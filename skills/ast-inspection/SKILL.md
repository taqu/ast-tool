---
name: ast-inspection
description: Inspect the declared symbols and structural outline of a source file, and navigate parent/child relationships of AST-derived elements using ast-tool symbols, outline, parent, and children.
triggers:
  - "list symbols in file"
  - "what functions are in"
  - "outline file"
  - "show structure"
  - "parent of node"
  - "children of node"
  - "symbol table"
  - "declarations in file"
  - "structural overview"
languages: [c, cpp, csharp, python, javascript, typescript, tsx, go, rust, java, bash, ruby, scala, css, html]
---

# AST Inspection Skill

## Purpose

Inspect the symbols declared in a source file, understand a file's structural organization, and navigate parent/child relationships of AST-derived elements.

Use this skill when structural or syntactic inspection of a single file is needed. Use the **semantic-analysis** skill when cross-file symbol lookup, usages, callers, or callees are needed.

## Commands

| Command    | Description                                                  |
|------------|--------------------------------------------------------------|
| `symbols`  | List all named symbols declared in a source file             |
| `outline`  | Show the structural outline of a source file                 |
| `parent`   | Show the parent AST node of a given node                     |
| `children` | Show the child AST nodes of a given node                     |

## Command Selection Guide

```text
Need to see all symbols declared in a file
        ↓
      symbols

Need to understand the structural organization of a file
        ↓
      outline

Need to navigate up from a known node
        ↓
      parent

Need to inspect what a node contains
        ↓
     children
```

---

### `symbols` — List symbols in a file

Extract every named semantic symbol from a source file: functions, methods, classes, structs, enums, variables, namespaces, and other named declarations.

```
ast-tool symbols [--json [--pretty]] <file>
```

Output (default): `<fqn> <ID>` — one symbol per line.

Examples:
```
ast-tool symbols src/parser.cpp
ast-tool symbols include/parser.hpp
ast-tool symbols --json src/main.cpp
ast-tool symbols --json --pretty include/parser.hpp
```

Use the **semantic-analysis** skill (`search`) when the declaration file is not yet known.

---

### `outline` — Structural overview

Show a depth-indented tree of named AST nodes in a source file. Anonymous punctuation and keyword tokens are omitted.

```
ast-tool outline <file>
```

Output: `<indent><type>[ "<text>"] @<line>:<col>` — one named node per line.

Examples:
```
ast-tool outline src/parser.cpp
ast-tool outline include/parser.hpp
```

---

### `parent` — Navigate up

Show the parent AST node of a specific node. Requires a node ID obtained from `symbols`, `outline`, or `find` output.

```
ast-tool parent --id <hex> <file>
```

The `<hex>` ID appears in the output of `symbols`, `outline`, and `find`. It is a 32-bit hash in uppercase hexadecimal (e.g. `9E52E360`).

Example:
```
ast-tool parent --id 9E52E360 src/parser.cpp
```

---

### `children` — Navigate down

Show all immediate child AST nodes of a specific node. Requires a node ID obtained from `symbols`, `outline`, or `find` output.

```
ast-tool children --id <hex> <file>
```

Example:
```
ast-tool children --id 9E52E360 src/parser.cpp
```

---

## Output Formats

- `symbols`: plain text (`<fqn> <ID>`) or JSON (`--json`, `--pretty`)
- `outline`, `parent`, `children`: plain text only

## When to Use This Skill

- Enumerating all declarations in a source file before reading raw source.
- Understanding the organizational structure of a header or implementation file.
- Navigating AST relationships (parent/children) from a known node ID.

## Tool Selection

| Question | Tool |
|---|---|
| "What symbols are declared in this file?" | `symbols` |
| "What is the structure of this file?" | `outline` |
| "What is the parent of this node?" | `parent --id` |
| "What are the children of this node?" | `children --id` |
| "Where is X defined across the project?" | `search` (semantic-analysis skill) |
| "Who calls this function?" | `callers` (semantic-analysis skill) |
| "Find all usages of a symbol?" | `references` (semantic-analysis skill) |

## Common Mistakes

**Using grep to find function declarations.**
`grep "void parse"` matches occurrences in comments, strings, and call sites — not only declarations. Use `symbols --json` and filter by kind for authoritative declaration information.

**Using text search to understand file structure.**
Reading and parsing source lines to produce an outline is fragile. Use `outline` directly.

**Invoking `parent` or `children` without a node ID.**
Both commands require `--id <hex>`. Obtain the ID from `symbols`, `outline`, or `find` output first.

## Best Practices

- Use `symbols` as the first step when investigating what a file declares, before reading raw source.
- Use `outline` to understand structural organization before reading source directly.
- Use `--json` output when filtering symbols by kind or piping to downstream tools.
- Obtain node IDs from `symbols` or `outline` output before invoking `parent` or `children`.
- Use the semantic-analysis skill (`search`) when the target file is not yet identified.
