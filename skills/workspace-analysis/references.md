---
name: workspace-analysis references
description: Reference tables for search command flags, RE2 syntax, and JSON schema
---

# Workspace Analysis References

## search Command Flags

| Flag            | Type    | Description                                        |
|-----------------|---------|----------------------------------------------------|
| `--name`        | string  | Substring match against simple symbol name         |
| `--fqn`         | string  | Substring match against fully-qualified name       |
| `--kind`        | string  | Exact substring match against symbol kind          |
| `--file`        | string  | Substring match against file path                  |
| `--name-regex`  | RE2     | Regex match against simple name                    |
| `--fqn-regex`   | RE2     | Regex match against fully-qualified name           |
| `--file-regex`  | RE2     | Regex match against file path                      |
| `--json`        | flag    | Emit JSON array                                    |
| `--pretty`      | flag    | Pretty-print JSON (implies `--json`)               |
| `-h`            | flag    | Show help and exit                                 |

## Usage

```bash
ast-tool search [filters] [--json | --pretty] <directory>
```

`<directory>` is required. Walked recursively.

## RE2 Regex Quick Reference

| Pattern     | Meaning                              |
|-------------|--------------------------------------|
| `^foo`      | Name starts with `foo`              |
| `bar$`      | Name ends with `bar`                |
| `foo.*bar`  | Contains `foo` followed by `bar`    |
| `(a\|b)`   | Alternation — matches `a` or `b`    |
| `\\.cpp$`  | Path ends with `.cpp`               |
| `[A-Z].*`  | Starts with an uppercase letter     |

## JSON Output Schema

```json
[
  {
    "kind":   "string  — symbol kind",
    "name":   "string  — simple unqualified name",
    "fqn":    "string  — fully-qualified name",
    "file":   "string  — source file path",
    "line":   "number  — 1-based line",
    "column": "number  — 1-based column"
  }
]
```

## Symbol Kinds

See `semantic-analysis/references.md` for the full kind table by language.

## Directory Traversal

- `search` recurses into all subdirectories of the given `<directory>`.
- Binary files and unrecognized extensions are skipped automatically.
- Use `--file` or `--file-regex` to restrict the set of files processed.
