---
name: ast-inspection references
description: Reference material for AST inspection — node IDs, grammar types, coordinate system
---

# AST Inspection References

## Node ID Format

- 32-bit hash encoded as uppercase hexadecimal (e.g., `9E52E360`)
- Stable as long as file content does not change
- Accepted by `find --id`, `parent --id`, `children --id`
- Prefix `0x` is optional; case-insensitive

## Coordinate System

All line and column numbers are **1-based**.

| Concept       | Value |
|---------------|-------|
| First line    | 1     |
| First column  | 1     |

When using `find --line`/`--column`, both flags must be provided together.

## Common Grammar Type Names (tree-sitter)

These vary by language parser. Examples for C/C++:

| Type                    | Meaning                        |
|-------------------------|--------------------------------|
| `translation_unit`      | Root of C/C++ file             |
| `function_definition`   | Function with body             |
| `function_declarator`   | Name + parameter list          |
| `declaration`           | Variable or forward declaration|
| `identifier`            | Any identifier token           |
| `type_specifier`        | A type name                    |
| `compound_statement`    | `{ ... }` block                |
| `call_expression`       | Function call                  |
| `binary_expression`     | `a + b`, `a == b`, etc.        |
| `if_statement`          | `if (...)` construct           |
| `return_statement`      | `return` statement             |
| `comment`               | Single or multi-line comment   |

For other languages, run `ast-tool dump --pretty <file>` and inspect the `"type"` fields.

## Output JSON Schema (single node)

```json
{
  "id": "9E52E360",
  "type": "function_definition",
  "grammar_type": "function_definition",
  "named": true,
  "start_line": 3,
  "start_column": 0,
  "end_line": 15,
  "end_column": 1,
  "text": "int main(int argc, char** argv) { ... }"
}
```

## Supported Languages

| Language   | Common extensions         |
|------------|---------------------------|
| C          | `.c`, `.h`                |
| C++        | `.cpp`, `.cc`, `.hpp`     |
| C#         | `.cs`                     |
| Python     | `.py`                     |
| JavaScript | `.js`, `.mjs`             |
| TypeScript | `.ts`                     |
| TSX        | `.tsx`                    |
| Go         | `.go`                     |
| Rust       | `.rs`                     |
| Java       | `.java`                   |
| Bash       | `.sh`, `.bash`            |
| Ruby       | `.rb`                     |
| Scala      | `.scala`                  |
| CSS        | `.css`                    |
| HTML       | `.html`, `.htm`           |

## Help Flag

`-h` / `--help` works at any point, including with missing arguments:

```bash
ast-tool find --help
ast-tool --help
```
