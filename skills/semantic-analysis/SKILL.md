---
name: semantic-analysis
description: Discover and analyze semantic symbols and relationships across a workspace using ast-tool — search for symbols, locate declarations by text or position, find references, and trace direct call relationships (callers and callees).
triggers:
  - "search symbols"
  - "find symbol"
  - "where is X defined"
  - "find declaration"
  - "find references"
  - "who calls"
  - "what does this call"
  - "callers of"
  - "callees of"
  - "find usages"
  - "search across project"
  - "list symbols"
languages: [c, cpp, csharp, python, javascript, typescript, tsx, go, rust, java, bash, ruby, scala, css, html]
---

# Semantic Analysis Skill

## Purpose

Discover matching symbols, locate specific declarations, find all usages of a symbol, and trace direct call relationships across a workspace — without needing to build the project.

## Commands

| Command      | Role                                                       |
|--------------|------------------------------------------------------------|
| `search`     | Discover symbols matching a name, kind, or path pattern    |
| `find`       | Locate AST nodes by type, text, or position in a file      |
| `references` | Find all locations where a symbol is referenced            |
| `callers`    | Find functions that directly call a target function        |
| `callees`    | Find functions directly called by a target function        |

## Command Selection Guide

```text
Need to discover matching symbols across the workspace
        ↓
      search

Need to locate a node by text, type, or position in a known file
        ↓
       find

Need to find all usages of a symbol
        ↓
    references

Need to know who directly calls a function
        ↓
      callers

Need to know what a function directly calls
        ↓
      callees
```

---

### `search` — Symbol discovery across the workspace

Use when the exact declaration file is not known, or when enumerating all symbols matching a pattern.

```
ast-tool search [--name <name>] [--fqn <fqn>] [--kind <kind>] [--file <path>]
               [--name-regex <re>] [--fqn-regex <re>] [--file-regex <re>]
               [--json [--pretty]] <root>
```

Multiple filters are ANDed together. Output (default): `<kind> <fqn> <file>:<line>:<col>`.

Examples:
```
ast-tool search --kind function src/
ast-tool search --name parse src/
ast-tool search --fqn-regex '^ast::' src/
ast-tool search --kind class --file-regex '\.hpp$' include/
ast-tool search --json src/
```

Cache `--json` output for large workspaces and query with `jq` rather than re-running.

---

### `find` — Locate nodes in a known file

Use when the file is already identified and you need to locate a node by type, text content, source position, or node ID.

```
ast-tool find [--type <type>] [--text <text>] [--id <hex>]
              [--line <n>] [--column <n>] <file>
```

`--line` and `--column` must be supplied together to filter by position.

Examples:
```
ast-tool find --type function_definition src/parser.cpp
ast-tool find --text parse src/parser.cpp
ast-tool find --line 42 --column 17 src/parser.cpp
ast-tool find --type identifier --text process src/main.cpp
```

Use `search` for cross-workspace symbol lookup; use `find` for targeted node lookup within a specific file.

---

### `references` — Find usages of a symbol

Find every location in the workspace where a specific symbol is referenced. Resolution is semantic: only genuine uses of the target declaration are reported.

```
ast-tool references [--json [--pretty]] <symbol> <root>
```

Resolution: if `<symbol>` contains `::`, it is matched against fully-qualified names; otherwise against unqualified names. If the query matches more than one symbol, the command fails and lists candidates — supply a fully-qualified name to disambiguate.

- The declaration site is excluded from results by default.
- A valid symbol with no references produces an empty result (not an error).
- Output sorted by file, line, column.

Examples:
```
ast-tool references parse src/
ast-tool references ast::parse src/
ast-tool references --json parse src/
```

---

### `callers` — Find direct callers

Find every call site in the workspace where a specific function is directly called. Only direct calls resolvable by the semantic resolver are reported — indirect calls through function pointers or virtual dispatch are not included. Transitive caller analysis is not performed.

The target must be a function, method, constructor, or destructor.

```
ast-tool callers [--json [--pretty]] <symbol> <root>
```

Output (default): `<caller_fqn> <file>:<line>:<col>`, or `<file_scope>` when the call is at file scope.

- A valid function with no callers produces an empty result (not an error).
- Output sorted by file, caller FQN, line, column.

Examples:
```
ast-tool callers parse src/
ast-tool callers ast::parse src/
```

---

### `callees` — Find direct callees

Find every function directly called within a specific function's body. Only direct calls resolvable by the semantic resolver are reported. Transitive callee analysis is not performed.

The target must be a function, method, constructor, or destructor.

```
ast-tool callees [--json [--pretty]] <symbol> <root>
```

Output (default): `<callee_fqn> <file>:<line>:<col>`, or `<unresolved>` when the callee cannot be resolved.

- A valid function with no callees produces an empty result (not an error).
- Output sorted by file, line, column, callee FQN.

Examples:
```
ast-tool callees parse src/
ast-tool callees ast::parse src/
```

---

## Output Formats

All commands support:
- Plain text (default)
- JSON (`--json`)
- Pretty-printed JSON (`--pretty`, implies `--json`)

## When to Use This Skill

- Finding where a symbol is declared anywhere in the project (`search`).
- Enumerating all symbols of a given kind (`search --kind`).
- Locating a node by text, type, or cursor position in a known file (`find`).
- Finding all usages of a symbol for impact analysis or refactoring (`references`).
- Understanding which functions call a target function (`callers`).
- Understanding what a function depends on (`callees`).

## Tool Selection

### Use `ast-tool` semantic commands when

- Finding where a specific symbol is declared
- Enumerating all symbols of a given kind across the project
- Finding usages, callers, or callees of a symbol
- Locating a node at a specific source position

### Use text search (grep, ripgrep) when

- Finding TODO or FIXME comments
- Locating string literals or log messages
- Searching documentation or configuration files

## Common Mistakes

**Using grep to find symbol declarations.**
`grep "class Foo"` matches occurrences in comments, strings, and forward declarations. Use `search --name "Foo" --kind class` for authoritative results.

**Using grep to find callers.**
Text search matches all occurrences of the function name — in strings, comments, and forward declarations — not just call sites. Use `callers` for accurate call-site enumeration.

**Assuming an ambiguous query will auto-resolve.**
If a command reports multiple candidates, supply a fully-qualified name (containing `::`) to disambiguate.

**Treating `callers` as a transitive call graph.**
`callers` reports only direct call sites. To understand indirect callers, apply `callers` recursively to each discovered caller.

## Best Practices

- Use `search --kind <kind>` to filter early and reduce result volume.
- Use `--fqn` or `--fqn-regex` to scope queries to a specific namespace or class.
- Cache `search --json` for large workspaces; query with `jq` rather than re-running.
- Use `--json` output when filtering results programmatically.
- For symbol ambiguity, use the fully-qualified name rather than the unqualified name.
- Prefer exact filters (`--name`, `--kind`, `--file`) before switching to regex variants.
