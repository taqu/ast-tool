---
name: ast-inspection command selection
description: Decision guide for choosing between dump, outline, find, range, parent, and children
---

# Command Selection Guide — AST Inspection

## Decision Tree

```
Do you need the full raw tree?
├── Yes → dump
└── No
    Do you want a structural overview of named nodes?
    ├── Yes → outline
    └── No
        Do you have a cursor position (line + column)?
        ├── Yes → find --line N --column N
        └── No
            Do you have a node ID?
            ├── Yes
            │   ├── Navigate up?   → parent --id ID
            │   ├── Navigate down? → children --id ID
            │   └── Just retrieve? → find --id ID
            └── No
                Do you want nodes in a line/column range?
                ├── Yes → range [--start-line N] [--end-line N] [...]
                └── No
                    Search by type or text → find [--type T] [--text T]
```

---

## Tool Selection: ast-tool vs Text Search

| Task | Use |
|---|---|
| Find all nodes of a grammar type | `find --type` |
| Find a node containing specific text | `find --text` |
| Determine what node is at line/column | `find --line --column` |
| Understand the file's tree structure | `outline` |
| Navigate from a known node to its parent | `parent --id` |
| Find TODO comments or string literals | text search (grep) |
| Search configuration or documentation files | text search (grep) |

Use `ast-tool` whenever the task concerns the syntactic structure or parse-tree relationships of source code. Use text search for non-structural content.

---

## Flag Compatibility Matrix

| Flag            | dump | outline | find | range | parent | children |
|-----------------|:----:|:-------:|:----:|:-----:|:------:|:--------:|
| `--json`        | ✓    | ✓       | ✓    | ✓     | ✓      | ✓        |
| `--pretty`      | ✓    | ✓       | ✓    | ✓     | ✓      | ✓        |
| `--type`        |      |         | ✓    |       |        |          |
| `--grammar`     |      |         | ✓    |       |        |          |
| `--text`        |      |         | ✓    |       |        |          |
| `--id`          |      |         | ✓    |       | ✓(req) | ✓(req)  |
| `--line`        |      |         | ✓    |       |        |          |
| `--column`      |      |         | ✓    |       |        |          |
| `--start-line`  |      |         |      | ✓     |        |          |
| `--start-column`|      |         |      | ✓     |        |          |
| `--end-line`    |      |         |      | ✓     |        |          |
| `--end-column`  |      |         |      | ✓     |        |          |

**Notes:**
- `parent` and `children` require `--id`; all other `find` filters are incompatible with `--id`.
- `--line` and `--column` must appear together in `find`.
- `--pretty` implies `--json`.

---

## Choosing an Output Format

| Scenario                              | Recommended format |
|---------------------------------------|--------------------|
| Human reading / quick inspection      | plain text (default) |
| Programmatic parsing / piping to jq   | `--json`           |
| Debugging / formatted human+machine   | `--pretty`         |
