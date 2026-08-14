---
name: api-review references
description: Reference tables for API review — breaking-change taxonomy, command flags, and jq patterns
---

# API Review References

## Breaking-Change Taxonomy

| Change                         | Breaking | Detectable with ast-tool           |
|--------------------------------|:--------:|------------------------------------|
| Public symbol removed          | ✓        | `comm -23` on FQN lists            |
| Public symbol renamed          | ✓        | Appears in removed + added         |
| Kind changed (e.g. fn→class)   | ✓        | Same FQN, different `kind` field   |
| New required parameter         | ✓        | Requires signature diff (not AST)  |
| Symbol visibility narrowed     | ✓        | Removed from public headers        |
| New public symbol added        | ✗        | `comm -13` on FQN lists            |
| New optional parameter         | ✗        | Backward-compatible extension      |
| Implementation-only change     | ✗        | Not visible in headers             |

## API Review Commands Quick Reference

```bash
# Full API snapshot (JSON)
ast-tool search --file-regex "\\.h$" --pretty ./include

# Functions only
ast-tool search --file-regex "\\.h$" --kind function ./include

# Classes and structs
ast-tool search --file-regex "\\.h$" --json ./include \
  | jq '[.[] | select(.kind == "class" or .kind == "struct")]'

# FQN list (for diffing)
ast-tool search --file-regex "\\.h$" --json ./include \
  | jq -r '.[].fqn' | sort
```

## Useful jq Patterns for API Review

```bash
# Group by kind
jq 'group_by(.kind) | map({kind: .[0].kind, symbols: map(.fqn)})' api.json

# Group by file
jq 'group_by(.file) | map({file: .[0].file, count: length, symbols: map(.name)})' api.json

# Find duplicated names (overloads or conflicts)
jq -r '.[].name' api.json | sort | uniq -d

# All symbols in a specific namespace
jq '[.[] | select(.fqn | startswith("ast::"))]' api.json
```

## File Extension Patterns for Public APIs

| Language   | Public API pattern                     |
|------------|----------------------------------------|
| C/C++      | `--file-regex "\\.(h\|hpp)$"`          |
| C#         | `--file-regex "\\.cs$"` + manual filter|
| Python     | `--file "__init__.py"` per package     |
| TypeScript | `--file "index.ts"` or `.d.ts`         |
| Go         | All `.go` (no headers; use `--kind`)   |
| Java       | `--file-regex "\\.java$"` + `public`   |

## Command Flags Summary

See `workspace-analysis/references.md` for the complete `search` flag table.
