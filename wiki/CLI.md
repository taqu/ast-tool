# CLI

## Invocation

```
ast-tool [options] <command> [command options] [arguments]
```

Global options (`-h`, `--help`) may appear anywhere on the command line, including after the subcommand name.

---

## Command Categories

Commands are grouped into two categories.

### AST Inspection

These commands operate on the raw syntax tree. They are parser-level: they know about node types, positions, and structure, but not about language semantics such as fully-qualified names or access modifiers.

| Command | Purpose |
|---------|---------|
| `dump` | Print every AST node in a file in depth-first order |
| `outline` | Print named nodes only, indented by tree depth |
| `find` | Find nodes by type, text, position, or ID |
| `range` | Find nodes whose source span intersects a given range |
| `parent` | Print the parent of a specific node |
| `children` | Print the immediate children of a specific node |

### Semantic Analysis

These commands operate on extracted semantic symbols. They understand declarations, fully-qualified names, kinds, access modifiers, and storage qualifiers.

| Command | Purpose |
|---------|---------|
| `symbols` | Extract all named symbols from a single file |
| `search` | Query symbols across an entire workspace directory |

---

## Help System

### Top-level help

```sh
ast-tool help
ast-tool --help
ast-tool -h
```

### Per-command help

```sh
ast-tool help <command>
ast-tool <command> --help
ast-tool <command> -h
```

The `--help` flag is recognized before argument parsing, so it works even when required arguments are missing.

---

## Output Conventions

### Node output format

Commands that return AST nodes (`find`, `range`, `parent`, `children`) use a consistent one-line format:

```
<ID> <type> @<line>:<col>[ "<text>"]
```

- `<ID>` — 32-bit node hash in uppercase hexadecimal (e.g. `9E52E360`)
- `<type>` — tree-sitter node type string (e.g. `function_definition`, `identifier`)
- `<line>` — 1-based source line
- `<col>` — 1-based source column
- `<text>` — source text of the node; present only when non-empty

### dump output format

```
<type> <ID>
```

One line per node, in depth-first order.

### outline output format

```
<indent><type>[ "<text>"] @<line>:<col>
```

Indented by two spaces per depth level. Anonymous tokens are omitted.

### symbols output format (plain text)

```
<fqn> <ID>
```

### search output format (plain text)

```
<kind> <fqn> <file>:<line>:<col>
```

### JSON output

`symbols` and `search` support `--json` and `--pretty` for structured output. Use JSON when processing output programmatically.

---

## Node IDs

Every AST node has a stable 32-bit hash displayed as uppercase hex. Node IDs are:

- stable for a given file at a given content
- consistent across `dump`, `outline`, `find`, `range`, `parent`, and `children`
- the primary way to chain commands (find a node, then inspect its parent or children)

Node IDs change when the file is modified.

---

## Common Workflows

### Inspect a file's structure

```sh
ast-tool outline src/parser.cpp
```

### Find a declaration by name

```sh
ast-tool search --name parseExpression src/
# or in a single file:
ast-tool symbols src/parser.cpp
```

### Locate the node at a cursor position

```sh
ast-tool find --line 42 --column 17 src/parser.cpp
```

### Walk the tree upward from a node

```sh
ast-tool parent --id 9E52E360 src/parser.cpp
```

### Inspect what a node contains

```sh
ast-tool children --id 9E52E360 src/parser.cpp
```

### Export symbols as JSON

```sh
ast-tool symbols --json --pretty include/parser.hpp
```

### Filter workspace symbols by regex

```sh
ast-tool search --fqn-regex '^ast::' --kind function src/
```
