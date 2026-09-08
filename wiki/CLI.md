# CLI

## Invocation

```
ast-tool <command> [options] [arguments]
```

Global options (`-h`/`--help`, `-v`/`--version`) may appear anywhere on the command line, including after the subcommand name — `--help` is recognized even when required arguments are missing.

`dump` also exists as a low-level command (print every AST node, including anonymous punctuation) but is intentionally omitted from the top-level help listing; run `ast-tool dump --help` directly.

---

## Command Categories

Commands are grouped into three categories.

### AST Inspection

These commands operate on the raw syntax tree. They are parser-level: they know about node types, positions, and structure, but not about language semantics such as fully-qualified names or access modifiers.

| Command | Purpose |
|---------|---------|
| `dump` (hidden) | Print every AST node in a file in depth-first order |
| `outline` | Print named nodes only, indented by tree depth |
| `find` | Find nodes by type, text, position, or ID |
| `range` | Find nodes whose source span intersects a given range |
| `parent` | Print the parent of a specific node |
| `children` | Print the immediate children of a specific node |

### Semantic Analysis

These commands operate on extracted semantic symbols and cross-file relationships. They understand declarations, fully-qualified names, kinds, access modifiers, storage qualifiers, and direct call/reference relationships.

| Command | Purpose |
|---------|---------|
| `symbols` | Extract all named symbols from a single file |
| `search` | Query symbols across an entire workspace directory |
| `references` | Find every location that references a symbol |
| `callers` | Find every direct caller of a function/method |
| `callees` | Find every function/method a function/method directly calls |

### Cache Management

| Command | Purpose |
|---------|---------|
| `cache warm` | Parse missing/stale files into the on-disk AST cache for a workspace |
| `cache status` | Show AST cache information for a workspace |
| `setup` | Configure Claude Code / Codex to warm the cache automatically at session start |

---

## Help System

### Top-level help

```sh
ast-tool help
ast-tool --help
ast-tool -h
```

### Version

```sh
ast-tool --version
ast-tool -v
```

### Per-command help

```sh
ast-tool help <command>
ast-tool <command> --help
ast-tool <command> -h
```

`cache warm`/`cache status` are two-word subcommands with their own dedicated help pages, reachable via either `ast-tool cache warm --help` or `ast-tool help cache warm` (and likewise for `cache status`).

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

### references output format (plain text)

```
<file>:<line>:<col>
```

### callers / callees output format (plain text)

```
<caller_fqn> <file>:<line>:<col>
```

(`callees` prints `<callee_fqn>` in the same position.) A valid target with no callers/callees/references produces an empty result at exit code `0` — not an error.

### JSON output

`symbols`, `search`, `references`, `callers`, and `callees` all support `--json` (compact) and `--json --pretty` (indented; implies `--json`). A successful result is always a JSON **array** on stdout — `[]` for a valid empty result. An error (not found, ambiguous, invalid arguments) prints a structured JSON object to **stderr** instead; stdout stays empty, so it is always safe to pipe stdout into a JSON parser. The CLI and JSON output are pre-1.0 and may change between minor releases.

```sh
ast-tool callers --json Parser::parse src/
# [{"caller_kind":"Function","caller_fqn":"...","file":"...","line":42,"column":8}, ...]
```

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

### Find every direct caller/callee of a function

```sh
ast-tool callers Parser::parse src/
ast-tool callees Parser::parse src/
```

### Find every reference to a symbol

```sh
ast-tool references Parser::parse src/
```

---

## Errors, Not-Found, and Ambiguity

| Situation | Behavior |
|---|---|
| Symbol not found | `error: symbol not found: <fqn>` to stderr, exit 1, with a `next: search <name> <root>` suggestion |
| Ambiguous unqualified name (multiple declarations match) | `error: ambiguous symbol: <name>` to stderr, exit 1, with the full candidate list (kind, FQN, file, line) — the command never guesses; retry with a fully-qualified name |
| Valid target, no relationships | Exit 0, empty plain-text output or `[]` in JSON — a legitimate answer, not a failure |
| Workspace root missing/empty/unanalyzable | `error: workspace at '<path>' is empty or could not be analyzed`, exit 1 |
| Unparseable/malformed source file | Structural commands degrade gracefully (tree-sitter error-recovery nodes or an empty result); no crash |

`callers`/`callees`/`references` resolve `<symbol>` as a fully-qualified name if it contains `::`, otherwise as an unqualified name. An unqualified name matching more than one declaration triggers the ambiguity behavior above rather than picking one arbitrarily.
