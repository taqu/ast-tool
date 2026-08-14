---
name: context-export command selection
description: Choosing which ast-tool commands to combine for different context-export scenarios
---

# Command Selection Guide — Context Export

## Tool Selection: ast-tool vs Text Search

| Content goal | Use |
|---|---|
| File structural shape | `outline` |
| Named declarations and locations | `symbols` |
| Cross-file symbol table | `search` |
| Call sites of a specific function | `search --name` + `find --text` |
| Full raw parse tree (rarely needed) | `dump` |
| TODO / FIXME comments | text search (grep) |
| Documentation or changelog text | text search (grep) |
| Non-source assets (build scripts, config) | text search (grep) |

Use ast-tool commands to produce all structural and semantic content. Use text search only for non-structural content that ast-tool does not model.

---

## What to Include in a Context Package

| Context goal                   | Commands to combine                          |
|--------------------------------|----------------------------------------------|
| Understand one file            | `outline` + `symbols`                        |
| Understand a module            | `search --file` + `outline` per file         |
| API surface of a library       | `search --file ".h"` or `symbols` per header |
| Impact of changing a function  | `search --name` + `find --text` per file     |
| Navigation index               | `search` (whole workspace, JSON)             |
| PR review context              | `outline` + `symbols` per changed file       |

## Granularity Decision

| Need                              | Command     | Output scope  |
|-----------------------------------|-------------|---------------|
| Whole-file shape (named nodes)    | `outline`   | structural    |
| Declared names + locations        | `symbols`   | semantic      |
| All occurrences of a name         | `find --text` / `search --name` | syntactic / semantic |
| Raw parse tree (rarely needed)    | `dump`      | syntactic     |

## Format Decision

| Consumer                     | Format         |
|------------------------------|----------------|
| LLM prompt (inline text)     | plain text     |
| LLM prompt (structured data) | `--json`       |
| Human reading                | `--pretty`     |
| File-based index             | `--json` → file|

## Composition Tips

- **Combine outline + symbols** for a two-level view: shape (outline) and declarations (symbols).
- **Use `--file` filter in search** to scope to a module without listing files manually.
- **Cache the workspace index** (`search --json`) and query it with `jq` rather than re-running for every question.
