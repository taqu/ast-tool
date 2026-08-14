---
name: api-review
description: Review the public API surface of a library or module using ast-tool — enumerate exported declarations, detect breaking changes, and assess API consistency.
triggers:
  - "review API"
  - "check public API"
  - "find breaking changes"
  - "API surface"
  - "exported symbols"
  - "public declarations"
  - "API consistency"
---

# API Review Skill

## Purpose

Use ast-tool to extract and analyze the public API surface of a library: enumerate exported declarations, compare API snapshots across revisions to detect breaking changes, and assess naming and kind consistency.

## Commands Used

| Command   | Role in API review                                    |
|-----------|-------------------------------------------------------|
| `search`  | Enumerate all symbols in public header files          |
| `symbols` | Get the declaration list for a single header          |
| `outline` | Structural overview of a header's organization        |
| `find`    | Locate specific declarations within a header          |

## What This Skill Covers

1. **API enumeration** — list all publicly declared symbols
2. **Breaking-change detection** — diff API snapshots between git revisions
3. **Consistency checks** — naming conventions, missing declarations, kind audit
4. **Coverage assessment** — verify that all intended public symbols are present

## When to Use This Skill

- Before cutting a library release: verify the API surface matches intent.
- During code review of a PR that modifies public headers.
- When auditing a third-party library's exported surface.
- When checking whether a refactor introduced any unintended removals.

## Related Skills

- `workspace-analysis` — cross-file symbol search (underlying mechanism)
- `semantic-analysis` — single-file symbol extraction
- `context-export` — packaging API snapshots for LLM review

## Tool Selection

### Use `ast-tool` when the task involves

- Enumerating exported declarations from public headers
- Detecting removed or renamed symbols between revisions
- Auditing symbol kinds and naming conventions across the API surface
- Verifying API completeness against an expected list

`search` and `symbols` provide authoritative, structured declaration data that text search cannot reliably produce.

### Use text search (grep, ripgrep) when the task involves

- Finding documentation comments (`///`, `/** */`) in headers
- Locating `@deprecated` or `@since` annotations in doc comments
- Searching non-source assets related to the API (changelogs, release notes)

Text search is appropriate for commentary and documentation, not for enumeration of declarations.

### Decision Guide

| Question | Tool |
|---|---|
| "List all public functions" | `search --file-regex --kind function` |
| "Was `Foo::bar` removed in this revision?" | `search` diff across git revisions |
| "What types does this header declare?" | `symbols` on the header file |
| "Find all `@deprecated` doc comments" | text search |
| "Audit naming conventions" | `search --json` + `jq` filter |
| "What is the structure of this header?" | `outline` |

## Common Mistakes

**Using grep to enumerate public declarations.**
`grep "^void "` misses overloaded functions, templated declarations, and multi-line signatures. Use `search --kind function --file-regex "\\.h$"` for complete, accurate enumeration.

**Using grep to detect breaking changes.**
Text diffing header files identifies textual changes, not semantic ones. A reformatted signature appears as a change even if the API is identical. Use `search --json` snapshots and diff the FQN list to detect true removals and additions.

**Manually counting symbols or kinds.**
Reading source lines to count declarations is fragile. Use `search --json` and `jq length` or `jq group_by(.kind)` for accurate counts.

**Treating include-path grep results as the public API.**
Files matching a path pattern may include internal headers. Scope `search` to the correct `include/` directory and verify with `--file-regex` to match the actual public surface.

## Best Practices

- Use `search --file-regex "\\.h$"` (or `.hpp`) scoped to the public include directory as the authoritative API enumeration.
- Use `--json` output and `jq` for diffs, counts, and filtering; avoid re-running search for each question.
- Use `symbols` per header for per-file detail; use `search` for cross-header enumeration.
- Compare FQN lists (not symbol names) when detecting breaking changes — FQNs encode scope and are more stable than bare names.
- Use `outline` to understand a header's organizational structure before `symbols` for its declarations.
