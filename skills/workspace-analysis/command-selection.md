---
name: workspace-analysis command selection
description: Choosing the right search filter and understanding filter semantics
---

# Command Selection Guide — Workspace Analysis

## Tool Selection: ast-tool vs Text Search

| Task | Use |
|---|---|
| Find where a symbol is declared | `search --name` |
| List all symbols of a given kind | `search --kind` |
| Find symbols in a specific namespace/class | `search --fqn` or `--fqn-regex` |
| Restrict search to a subdirectory or file pattern | `search --file` or `--file-regex` |
| What is declared in one specific file | `symbols` (semantic-analysis) |
| Find TODO comments across the project | text search (grep) |
| Find string literals or log messages | text search (grep) |
| Search build scripts or configuration files | text search (grep) |

Use `search` when the task requires authoritative declaration information across multiple files. Use text search for non-declaration content where semantic accuracy is not required.

---

## Exact vs Regex Filters

| Scenario                                         | Filter type   | Flag            |
|--------------------------------------------------|---------------|-----------------|
| Name contains a literal string                   | Exact         | `--name`        |
| Name matches a pattern (prefix, suffix, RE2)     | Regex         | `--name-regex`  |
| FQN contains a namespace/class qualifier         | Exact         | `--fqn`         |
| FQN matches a complex qualified pattern          | Regex         | `--fqn-regex`   |
| Symbol kind (function, class, enum, …)           | Exact         | `--kind`        |
| File path contains a subdirectory name           | Exact         | `--file`        |
| File path matches an extension or glob-like rule | Regex         | `--file-regex`  |

**Rule of thumb:** Start with exact filters (`--name`, `--kind`, `--file`). Switch to regex only when you need anchors, alternation, or complex patterns.

---

## search vs symbols

| Question                                      | Use       |
|-----------------------------------------------|-----------|
| "What is defined in *this* file?"             | `symbols` |
| "Where is X defined across the *project*?"   | `search`  |

---

## Combining Filters

All filters are **ANDed**. The more filters you add, the narrower the result.

```bash
# Returns functions whose name contains "parse" in files containing "parser"
ast-tool search --kind function --name "parse" --file "parser" ./src
```

There is no OR operator between filters. To OR, run multiple searches and merge results.

---

## Performance Considerations

`search` walks the entire directory tree, parsing every file. For large workspaces:

- Use `--file` or `--file-regex` to restrict to a subdirectory or file extension.
- Use `--kind` to skip irrelevant symbol types early.
- Redirect `--json` output to a file and query the file with `jq` instead of re-running.
