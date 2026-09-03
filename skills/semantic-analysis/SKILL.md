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

Use AST Tool as the default targeted semantic/structural path instead of grep, broad reads, or workspace dumps.

## Routing

| Need | Command |
|---|---|
| Symbol/declaration across the workspace | `search` |
| References/usages | `references` |
| Direct callers | `callers` |
| Direct callees | `callees` |
| All symbols in one known file | `symbols` (ast-inspection skill) |
| AST nodes/structure in one known file | `find` |

Do not use `find` for cross-workspace symbol resolution.

## Syntax and Boundaries

```text
ast-tool search [--name <name>] [--fqn <fqn>] [--kind <kind>] [--file <path>]
                [--name-regex <re>] [--fqn-regex <re>] [--file-regex <re>]
                [--json [--pretty]] <root>
ast-tool find [--type <type>] [--text <text>] [--id <hex>]
              [--line <n>] [--column <n>] <file>
ast-tool references [--json [--pretty]] <symbol> <root>
ast-tool callers [--json [--pretty]] <symbol> <root>
ast-tool callees [--json [--pretty]] <symbol> <root>
```

- `search` filters are ANDed; scope early by name, kind, FQN, file, or regex.
- `find --line` and `--column` must be supplied together.
- `--id` applies to `find`/`parent`/`children`; `--line`/`--column` to `find`; `--kind`/`--file`/`--name` to `search`.
- `references`/`callers`/`callees` require a directory `<root>`; `callers`/`callees` require a function-like target and report direct resolved calls only.
- A symbol containing `::` matches FQNs; otherwise it matches unqualified names. A valid target with no results succeeds with empty output.

## Workflow and Recovery

For a semantic relationship query:

1. If identity is uncertain, run `search --name <name> <root>` and select the exact FQN.
2. Run `references`, `callers`, or `callees` for the requested relationship.
3. On `not found`, correct the FQN from targeted `search` output.
4. On ambiguity between unrelated symbols, select the listed candidate's exact FQN.
5. On a C++ declaration/definition pair with the same FQN, narrow `<root>` to an implementation-only directory. If impossible, use grep after at most two failed semantic attempts.

Never retry an unchanged failed command. Every retry must change the FQN, root, or strategy meaningfully; stop after two failures. Do not use `--help` for ordinary discovery. A file passed as semantic `<root>` causes `workspace ... empty`; pass its containing directory. Empty output with exit 0 is not a failure.

## Output and Fallback

Use plain text by default. Use `--json` only for programmatic processing, cache large results instead of rerunning, and do not use `--pretty` by default.

Use text search for comments, literals, documentation, configuration, or the unresolved-ambiguity fallback above. When a requested operation maps to the routing table, do not replace its AST Tool query with grep/manual exploration.
