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

Resolution: if `<symbol>` contains `::`, it is matched against fully-qualified names; otherwise against unqualified names. If the query matches more than one symbol, the command fails and lists candidates — see [Ambiguity Recovery](#ambiguity-recovery) below.

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

Find every call site in the workspace where a specific function is directly called. Only direct calls resolvable by the semantic resolver are reported — indirect calls through function pointers or virtual dispatch are not included.

The target must be a function, method, constructor, or destructor.

```
ast-tool callers [--json [--pretty]] <symbol> <root>
```

`<root>` must be a **directory** path. Passing a file path causes the error "workspace is empty or could not be analyzed".

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

Find every function directly called within a specific function's body. Only direct calls resolvable by the semantic resolver are reported.

The target must be a function, method, constructor, or destructor.

```
ast-tool callees [--json [--pretty]] <symbol> <root>
```

`<root>` must be a **directory** path.

Output (default): `<callee_fqn> <file>:<line>:<col>`.

Examples:
```
ast-tool callees parse src/
ast-tool callees ast::parse src/
```

---

## Flags That Exist Only on `find`

The following flags are valid **only** on `find`. They do not exist on `callers`, `callees`, or `references`. Passing them to those commands fails with "symbol '--flag' not found":

| Flag | Valid on |
|------|----------|
| `--id <hex>` | `find`, `parent`, `children` only |
| `--line <n>` | `find` only |
| `--column <n>` | `find` only |
| `--kind <kind>` | `search` only |
| `--file <path>` | `search` only |
| `--name <name>` | `search` only |

---

## Error Quick Reference

| Error message | Cause | Action |
|---|---|---|
| `symbol 'X' not found in workspace` | Wrong namespace, misspelled, or missing qualifier | Run `search --name X <root>` to find the correct FQN |
| `symbol 'X' is ambiguous (N matches)` | Multiple symbols match the query | Read the candidate list; see [Ambiguity Recovery](#ambiguity-recovery) |
| `workspace at 'X' is empty or could not be analyzed` | `<root>` is a file, not a directory | Pass a directory path as `<root>` |
| Empty output, exit 0 | Symbol exists but has no callers/references | Correct — function may be unused or only called indirectly |

---

## Ambiguity Recovery

When `callers`, `callees`, or `references` fails with "is ambiguous (N matches)", the error lists each candidate with its kind and file:

```
error: symbol 'X' is ambiguous (2 matches); use a fully-qualified name (::) to disambiguate:
  Function auth::AuthToken::validate .\src\auth\auth_token.cpp:7
  Method auth::AuthToken::validate .\src\auth\auth_token.h:5
```

Identify which case applies:

### Case 1 — Different namespaces or unrelated classes

The candidates have different FQNs. Add more namespace qualifiers:
```
ast-tool callers auth::AuthToken::validate src/
```

### Case 2 — C++ declaration/definition pair (same FQN, .h and .cpp)

The candidates share the same FQN but appear in different files — one in a header (kind `method` or `function`) and one in an implementation file (kind `function`). This is the most common ambiguity in C++ codebases.

**The disambiguation hint in the error message does not apply here** — the FQN is already fully qualified. `callers`, `callees`, and `references` have no `--kind` or `--file` filter to select between declaration and definition.

**Resolution strategy:**

Step 1 — Check whether the header and implementation are in separate directories:
```
ast-tool search --fqn auth::AuthToken::validate src/
```
Look at the `file` column in the results. If `.h` and `.cpp` files are in separate subdirectories, narrow `<root>` to the implementation directory:
```
ast-tool callers auth::AuthToken::validate src/impl/
```

Step 2 — If they share the same directory, fall back to text search. Use Grep rather than retrying `callers`:
```
# Find call sites in implementation files
grep -rn "\bvalidate\s*(" src/ --include="*.cpp"
```

Do not retry `callers` with the same FQN more than twice against the same `<root>`. If it fails twice, proceed directly to the Grep fallback.

### Case 3 — Multiple unrelated symbols with the same unqualified name

Run `search --name X <root>` to see all matches, identify the correct one, and supply its exact FQN:
```
ast-tool search --name validate src/
# → pick the correct FQN from the output, e.g. auth::TokenValidator::validate
ast-tool callers auth::TokenValidator::validate src/
```

---

## Recommended Workflow for Call-Site Analysis

When asked to find callers of a function named `Foo::bar`:

```text
1. Search to confirm the FQN
   ast-tool search --name bar src/

2. If FQN is unambiguous → callers
   ast-tool callers Foo::bar src/

3. If callers fails with "not found" → fix the FQN from search output, retry once

4. If callers fails with "ambiguous" → apply Case 2 or Case 3 above

5. If two callers attempts both fail → Grep fallback
   grep -rn "\bbar\s*(" src/ --include="*.cpp"
```

Stop after two failed attempts on the same command. Each retry should use a meaningfully different approach, not a trivial variation.

---

## Output Formats

All commands support:
- Plain text (default)
- JSON (`--json`)
- Pretty-printed JSON (`--pretty`, implies `--json`)

Use plain text for quick inspection. Use `--json` only when you need to process results programmatically. Large `--json --pretty` output consumes significant context; prefer plain text when you only need file:line locations.

---

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

### Use text search (Grep) when

- Finding TODO or FIXME comments
- Locating string literals or log messages
- Searching documentation or configuration files
- `callers`/`references` failed twice with an unresolvable ambiguity

---

## Common Mistakes

**Using grep to find symbol declarations.**
`grep "class Foo"` matches occurrences in comments, strings, and forward declarations. Use `search --name "Foo" --kind class` for authoritative results.

**Using grep to find callers.**
Text search matches all occurrences of the function name — in strings, comments, and forward declarations — not just call sites. Use `callers` for accurate call-site enumeration when the symbol is unambiguous.

**Passing a file path as `<root>`.**
`callers`, `callees`, and `references` require a directory as `<root>`. The error "workspace is empty or could not be analyzed" means `<root>` is a file. Pass the containing directory instead.

**Assuming an ambiguous query will auto-resolve.**
If a command reports multiple candidates, do not retry with the same FQN. Read the candidate list and apply the [Ambiguity Recovery](#ambiguity-recovery) strategy.

**Using `--id`, `--line`, or `--column` with `callers`, `callees`, or `references`.**
These flags exist only on `find`. They cause the error "symbol '--flag' not found" when used with other commands.

**Treating `callers` as a transitive call graph.**
`callers` reports only direct call sites. To understand indirect callers, apply `callers` recursively to each discovered caller.

**Retrying the same failing command with minor variations.**
If `callers` fails twice with the same error for the same symbol, switch strategy. Repeating with `--pretty`, adding `2>&1`, or changing the path format will not resolve a structural ambiguity or a wrong FQN.

---

## Best Practices

- Run `search --name <name> <root>` before `callers`/`references` to confirm the exact FQN and avoid "not found" failures.
- Use `search --kind <kind>` to filter early and reduce result volume.
- Use `--fqn` or `--fqn-regex` to scope queries to a specific namespace or class.
- Cache `search --json` for large workspaces; query with `jq` rather than re-running.
- For symbol ambiguity between unrelated classes, use the fully-qualified name.
- For C++ declaration/definition ambiguity (same FQN in .h and .cpp), narrow `<root>` to the implementation directory, or fall back to Grep after two failures.
- Prefer plain text output over `--json --pretty` unless you need to process results programmatically.
