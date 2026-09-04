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

Prefer AST Tool when the request maps directly to a targeted semantic or structural query. Do not substitute grep, manual exploration, or broad workspace output.

## Route

| Task | Command |
|---|---|
| Find a symbol/declaration across the workspace | `search` |
| Find references/usages | `references` |
| Find direct callers | `callers` |
| Find direct callees | `callees` |
| Find a node by text, type, position, or ID in a known file; inspect AST structure | `find` |
| List every symbol declared in a known file | `symbols` from **ast-inspection** |

Use `search`, not `find`, when the declaration file is unknown.

## Target queries

Scope `search` early with `--name`, `--fqn`, `--kind`, `--file`, or their regex forms; filters are ANDed.

```text
ast-tool search --name <name> <root>
ast-tool find (--type <type> | --text <text> | --id <hex> | --line <n> --column <n>) <file>
ast-tool references <symbol> <root>
ast-tool callers <symbol> <root>
ast-tool callees <symbol> <root>
```

For relationship commands, `<root>` must be a directory. Prefer an exact FQN from `search`; `::` selects FQN matching, otherwise matching is unqualified. `callers`/`callees` report direct resolved calls, not a transitive graph. Empty output with exit 0 is a valid zero-result answer.

## Recovery

For a relationship query, confirm the symbol/FQN with `search` when needed, then run the mapped command.

- Not found: `search --name <name> <root>`, then retry with the corrected FQN.
- Ambiguous unrelated symbols: select the candidate's exact FQN.
- Duplicate C++ declaration/definition with one FQN: narrow the root to the implementation directory. If both remain in one directory, use grep.
- Empty/unanalysable workspace: pass a directory as the root.

Never retry an unchanged failure: change the FQN, root, or strategy using the error's cheapest useful correction. After two unresolved semantic attempts, use grep. Do not use `--help` for ordinary discovery.

## Boundaries

- `--id` applies to `find`, `parent`, and `children`; `--line` and `--column` apply together to `find`; `--name`, `--kind`, and `--file` apply to `search`, not relationship commands.
- Use plain output. Use `--json` only for programmatic processing; do not use `--pretty` by default. Cache large results rather than rerunning them.
- Use text search for comments, strings, documentation, configuration, or the fallback above—not for declarations, usages, callers, or callees when a targeted command applies.
- Recurse over returned callers/callees only when a transitive graph is required.
