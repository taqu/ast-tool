---
name: ast-inspection workflows
description: Step-by-step workflows for inspecting AST nodes with ast-tool
---

# AST Inspection Workflows

## Workflow 1 — Full Tree Dump

**Goal:** See every node in the file.

```
ast-tool dump <file>
ast-tool dump --json <file>
ast-tool dump --pretty <file>
```

Use `dump` when you need a complete picture of the parse tree before narrowing down to specific nodes.

---

## Workflow 2 — Named-Node Outline

**Goal:** Get a readable structural overview without noise nodes.

```
ast-tool outline <file>
ast-tool outline --pretty <file>
```

`outline` filters to named nodes only, producing an indented hierarchy that resembles the logical structure of the source code. Prefer this over `dump` when you want a high-level map.

---

## Workflow 3 — Find Nodes by Type or Text

**Goal:** Locate all nodes of a given grammar type or containing specific source text.

```
# By grammar type
ast-tool find --type function_definition <file>

# By source text (substring)
ast-tool find --text "my_function" <file>

# Combined
ast-tool find --type identifier --text "argc" <file>
```

---

## Workflow 4 — Find Node at Cursor Position

**Goal:** Identify the AST node at a specific line/column (e.g., from an editor cursor).

```
ast-tool find --line 42 --column 10 <file>
```

`--line` and `--column` must be provided together. Both are 1-based.

---

## Workflow 5 — Find Node by ID

**Goal:** Retrieve a specific node whose ID is already known.

```
ast-tool find --id 9E52E360 <file>
ast-tool find --id 0x9e52e360 <file>   # prefix optional, case-insensitive
```

Node IDs are 32-bit hashes stable per file content. They can be discovered from the output of `dump`, `outline`, `find`, or `range`.

---

## Workflow 6 — Inspect a Range

**Goal:** Find all nodes that intersect a byte or line range in the source.

```
ast-tool range --start-line 10 --end-line 20 <file>
ast-tool range --start-line 10 --start-column 5 --end-line 10 --end-column 30 <file>
```

All range flags are optional; omitted bounds default to the file start/end.

---

## Workflow 7 — Navigate Parent / Children

**Goal:** Walk up or down the tree from a known node ID.

```
# Find the parent node
ast-tool parent --id 9E52E360 <file>

# List immediate children
ast-tool children --id 9E52E360 <file>
```

Combine with `find` to first locate a node, extract its ID, then traverse relatives.
