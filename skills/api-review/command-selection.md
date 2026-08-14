---
name: api-review command selection
description: Choosing commands and filters for different API review scenarios
---

# Command Selection Guide — API Review

## Tool Selection: ast-tool vs Text Search

| Task | Use |
|---|---|
| List all publicly declared symbols | `search --file-regex` |
| Review a single header's declarations | `symbols` |
| Detect removed symbols between revisions | `search --json` + diff FQN lists |
| Audit naming conventions | `search --kind --json` + `jq` filter |
| Find `@deprecated` doc comments | text search (grep) |
| Search changelogs or release notes | text search (grep) |
| Find documentation annotations | text search (grep) |

Use `search` and `symbols` for all declaration-level API analysis. Text search is appropriate only for commentary and documentation that is not a language declaration.

---

## Task → Command Mapping

| Task                                         | Primary command              | Filter                      |
|----------------------------------------------|------------------------------|-----------------------------|
| List all public declarations                 | `search`                     | `--file-regex "\\.h$"`      |
| List public functions only                   | `search`                     | `--kind function`           |
| Review a single header's structure           | `outline` + `symbols`        | (no filter — single file)   |
| Find a specific declaration                  | `search --name`              | `--file-regex "\\.h$"`      |
| Detect removed symbols (breaking change)     | `search` diff across revisions | `--file-regex "\\.h$"`    |
| Audit naming conventions                     | `search --kind function --json` + `jq` + `grep` | — |
| Check completeness against expected list     | `search --json` + `comm`     | —                           |

## Scoping the Search

| What to include                      | Flag                              |
|--------------------------------------|-----------------------------------|
| All `.h` files                       | `--file-regex "\\.h$"`            |
| All `.hpp` files                     | `--file-regex "\\.hpp$"`          |
| Both `.h` and `.hpp`                 | `--file-regex "\\.(h\|hpp)$"`     |
| Only a specific include subdirectory | `--file "include/net"`            |
| Python `__init__.py` exports         | `--file "__init__.py"`            |

## Symbol Kind Filters for API Review

| Kind           | Review focus                                |
|----------------|---------------------------------------------|
| `function`     | Callable API surface                        |
| `class`        | Type hierarchy                              |
| `struct`       | Data layout / POD types                     |
| `enum`         | Named constants / discriminants             |
| `type_alias`   | Typedef / using declarations                |
| `interface`    | Abstract contracts (C#, Java, TypeScript)   |
| `namespace`    | Organizational grouping (C++)               |

## Breaking Change Severity

| Change type               | Breaking? | Detection                     |
|---------------------------|-----------|-------------------------------|
| Symbol removed            | Yes       | `comm -23 old new`            |
| Symbol renamed            | Yes       | Appears in both removed+added |
| Kind changed (fn → class) | Yes       | FQN in both; kind differs     |
| Symbol added              | No        | `comm -13 old new`            |
| Signature change          | Likely    | Requires deeper analysis      |
