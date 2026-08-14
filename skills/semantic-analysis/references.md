---
name: semantic-analysis references
description: Reference tables for symbol kinds and JSON schema for the symbols command
---

# Semantic Analysis References

## Symbol Kinds by Language

| Kind         | C/C++ | C# | Python | JS/TS | Go | Rust | Java |
|--------------|:-----:|:--:|:------:|:-----:|:--:|:----:|:----:|
| `function`   | ✓     | ✓  | ✓      | ✓     | ✓  | ✓    | ✓    |
| `method`     | ✓     | ✓  | ✓      | ✓     | ✓  | ✓    | ✓    |
| `class`      | ✓     | ✓  | ✓      | ✓     |    |      | ✓    |
| `struct`     | ✓     | ✓  |        |       | ✓  | ✓    |      |
| `enum`       | ✓     | ✓  | ✓      | ✓     | ✓  | ✓    | ✓    |
| `namespace`  | ✓     | ✓  |        |       |    |      |      |
| `interface`  |       | ✓  |        | ✓     | ✓  | ✓    | ✓    |
| `variable`   | ✓     | ✓  | ✓      | ✓     | ✓  | ✓    | ✓    |
| `constant`   | ✓     | ✓  | ✓      | ✓     | ✓  | ✓    | ✓    |
| `type_alias` | ✓     | ✓  |        | ✓     | ✓  | ✓    |      |
| `module`     |       |    | ✓      | ✓     |    | ✓    |      |

*Actual kinds depend on the tree-sitter grammar for each language. Run `symbols --pretty` to see real output.*

## JSON Output Schema

```json
[
  {
    "kind":    "string  — symbol kind (function, class, struct, …)",
    "name":    "string  — simple unqualified name",
    "fqn":     "string  — fully-qualified name (may equal name)",
    "file":    "string  — source file path",
    "line":    "number  — 1-based line of declaration",
    "column":  "number  — 1-based column of declaration"
  }
]
```

## Flags

| Flag       | Effect                                       |
|------------|----------------------------------------------|
| `--json`   | Emit JSON array instead of plain text        |
| `--pretty` | Pretty-print JSON (implies `--json`)         |
| `-h`       | Show help and exit                           |

## Usage

```bash
ast-tool symbols [--json] [--pretty] <file>
```

Exactly one file argument is required.
