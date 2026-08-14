---
name: context-export
description: Compose structured code context from ast-tool outputs for LLM consumption, documentation generation, or code-review prompts. Assembles symbol tables, AST outlines, and node details into coherent multi-file packages.
triggers:
  - "export context"
  - "prepare context for LLM"
  - "build code context"
  - "summarize codebase"
  - "generate documentation context"
  - "package symbols for review"
---

# Context Export Skill

## Purpose

Combine the outputs of multiple ast-tool commands into structured, LLM-ready context packages. This is a composition skill — it orchestrates `symbols`, `search`, `outline`, and `find` to produce coherent snapshots of code for downstream consumption.

## Use Cases

| Goal                                     | Approach                                          |
|------------------------------------------|---------------------------------------------------|
| Review a single file's structure         | `outline` + `symbols`                             |
| Understand an entire module              | `search --file` + `outline` per file              |
| Find all callers of a function           | `search --name` + `find --text` per file          |
| Generate API documentation context      | `symbols --json` on public headers                |
| Build a navigation index                 | `search --json` over whole workspace              |

## When to Use This Skill

- Before asking an LLM to review or refactor a multi-file module.
- When producing structured documentation snapshots.
- When assembling a code-review packet that includes both structure and declarations.
- When the user says "give me the context" or "prepare the code for review."

## Related Skills

- `semantic-analysis` — single-file symbols
- `workspace-analysis` — cross-file symbol search
- `ast-inspection` — raw AST access
- `api-review` — formal API surface review

## Tool Selection

### Use `ast-tool` commands when assembling context that involves

- File structure and organization (`outline`)
- Named declarations and their locations (`symbols`)
- Cross-file symbol tables and declaration lookup (`search`)
- Specific node details or call-site nodes (`find --text`)

Compose these commands to produce accurate, structured context packages. Do not reconstruct this information from raw source text.

### Use text search (grep, ripgrep) when the context involves

- TODO or FIXME comments
- Documentation text or inline comments
- String literals or log messages
- Configuration files or build scripts not parsed by ast-tool

Text search is appropriate for non-structural, non-declaration content.

### Decision Guide

| Goal | Primary commands |
|---|---|
| Understand one file's structure | `outline` + `symbols` |
| Understand a module | `search --file` + `outline` per file |
| Find all call sites of a function | `search --name` + `find --text` per file |
| Export public API surface | `search --file-regex` on header files |
| Build a navigation index | `search --json` (whole workspace) |
| Collect context for PR review | `outline` + `symbols` per changed file |
| Find TODO comments across files | text search |

## Common Mistakes

**Using grep to locate call sites when building a context package.**
`grep -rl "my_function"` finds files containing the text, but includes matches in comments, strings, and macro arguments. Use `ast-tool search --name "my_function"` to identify files with a symbol reference, then `ast-tool find --text "my_function"` for node-level detail.

**Reconstructing structure from raw source text.**
Reading and parsing source lines to produce an outline duplicates work that `outline` and `symbols` perform correctly. Use ast-tool commands directly.

**Including raw `dump` output in LLM context packages.**
Full dump output is verbose and rarely necessary for LLM review. Prefer `outline` (named nodes only) and `symbols` (declarations only) for compact, useful context.

**Re-running searches for every question.**
Cache `search --json` output to a file for large workspaces and query it with `jq` instead of invoking ast-tool repeatedly.

## Best Practices

- Combine `outline` + `symbols` as the standard two-level view: structural shape and declaration list.
- Use `search --file` to scope context to a module without enumerating files manually.
- Use `search --name` + `find --text` (not grep) when locating call sites for impact analysis.
- Cache workspace index output (`search --json`) and query with `jq` rather than re-running.
- Choose `--json` format for LLM-structured prompts and plain text for inline human-readable context.
