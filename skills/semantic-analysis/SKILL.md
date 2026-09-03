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

Use AST Tool as the default semantic/context-reduction path; do not replace a targeted query with grep or a workspace dump.

## Command Selection

| Task | Command |
|---|---|
| Find a symbol or declaration across the workspace | `search` |
| Find usages of a symbol | `references` |
| Find direct callers of a function | `callers` |
| Find functions directly called by a function | `callees` |
| Find a node by text, type, position, or ID in a known file | `find` |

For all symbols declared in a known file, use `symbols` from the **ast-inspection** skill. Use `find` when AST structure is required. Use `search`, not `find`, for cross-workspace symbol lookup.

## Commands

### `search` — Workspace symbol discovery

Use when the declaration file is unknown or when enumerating symbols matching filters.

```text
ast-tool search [--name <name>] [--fqn <fqn>] [--kind <kind>] [--file <path>]
                [--name-regex <re>] [--fqn-regex <re>] [--file-regex <re>]
                [--json [--pretty]] <root>
```

Filters are ANDed. Default output is `<kind> <fqn> <file>:<line>:<col>`.

```text
ast-tool search --name parse src/
ast-tool search --kind class --file-regex '\.hpp$' include/
```

### `find` — Targeted AST lookup in a known file

```text
ast-tool find [--type <type>] [--text <text>] [--id <hex>]
              [--line <n>] [--column <n>] <file>
```

`--line` and `--column` must be supplied together.

```text
ast-tool find --type function_definition src/parser.cpp
ast-tool find --line 42 --column 17 src/parser.cpp
```

### `references` — Semantic usages

```text
ast-tool references [--json [--pretty]] <symbol> <root>
```

The declaration is excluded. A valid symbol with no references returns empty output successfully. Results are sorted by file, line, and column.

### `callers` — Direct callers

```text
ast-tool callers [--json [--pretty]] <symbol> <root>
```

The target must be a function, method, constructor, or destructor. Only direct, semantically resolved calls are reported; function pointers and virtual dispatch are excluded. Default output is `<caller_fqn> <file>:<line>:<col>`, or `<file_scope>` for file-scope calls.

### `callees` — Direct callees

```text
ast-tool callees [--json [--pretty]] <symbol> <root>
```

The target must be a function, method, constructor, or destructor. Only direct, semantically resolved calls are reported. Default output is `<callee_fqn> <file>:<line>:<col>`.

For `references`, `callers`, and `callees`, `<root>` must be a directory. A name containing `::` matches fully-qualified names; otherwise it matches unqualified names. A valid target with no results returns empty output successfully.

## Semantic Query Workflow and Recovery

For callers of `Foo::bar`:

1. Confirm the exact FQN: `ast-tool search --name bar src/`.
2. Run the targeted command: `ast-tool callers Foo::bar src/`.
3. On `not found`, correct the FQN from `search` output and retry once.
4. On ambiguity, use the candidate list as described below.
5. After two failed attempts, use the grep fallback.

Apply the same recovery rules to `references` and `callees`. Never retry an unchanged failed command; each retry must use a meaningfully different FQN, root, or strategy. Do not use `--help` for ordinary command discovery.

### Ambiguity recovery

- **Different namespaces/classes:** select the candidate and retry with its exact FQN.
- **Multiple unrelated symbols with one unqualified name:** run `search --name X <root>`, select the correct candidate, and use its exact FQN.
- **C++ declaration/definition with the same FQN:** narrowing by FQN cannot resolve this. If header and implementation are in separate directories, narrow `<root>` to the implementation directory. If they share a directory, use grep rather than retrying the semantic command.

`callers`, `callees`, and `references` have no `--kind` or `--file` filter. Do not retry the same FQN against the same root more than twice.

```text
ast-tool search --fqn auth::AuthToken::validate src/
ast-tool callers auth::AuthToken::validate src/impl/
grep -rn "\bvalidate\s*(" src/ --include="*.cpp"
```

## Errors and Flags

| Result | Action |
|---|---|
| `symbol 'X' not found in workspace` | Run `search --name X <root>` and use the correct FQN. |
| `symbol 'X' is ambiguous (N matches)` | Read candidates and apply ambiguity recovery. |
| `workspace ... is empty or could not be analyzed` | Pass a directory, not a file, as `<root>`. |
| Empty output, exit 0 | The symbol exists but has no direct results; do not treat it as an error. |

Flag scope:

| Flag | Valid command |
|---|---|
| `--id <hex>` | `find`, `parent`, `children` |
| `--line <n>`, `--column <n>` | `find` |
| `--kind`, `--file`, `--name` | `search` |

## Output and Tool Choice

Use plain text by default. Use `--json` only for programmatic processing, and cache large search results instead of rerunning them. Do not use `--pretty` by default because it increases context volume.

Use text search for comments, string literals, documentation, configuration, or the documented fallback after two unresolved semantic-query failures. Do not use it for declarations, usages, callers, or callees when the corresponding targeted AST Tool command applies.

Do not dump the entire workspace when a targeted query is available. Scope `search` early with name, kind, FQN, file, or regex filters. `callers` and `callees` are direct rather than transitive; recurse over discovered callers/callees only when a transitive graph is required.
