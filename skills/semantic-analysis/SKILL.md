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

Use `ast-tool` to discover symbols, locate declarations or AST nodes, find semantic usages, and trace direct call relationships without building the project.

## Route the Request

| Need | Command | Boundary |
|---|---|---|
| Find a symbol/declaration across the workspace | `search` | Use when its file is unknown or when matching symbols by name, FQN, kind, or path. |
| Find a node by text, type, ID, or position in a known file | `find` | File-scoped structural lookup, not workspace symbol discovery. |
| Find every semantic usage of a symbol | `references` | Excludes the declaration; differs from callers because results are not limited to call sites. |
| Find functions that directly call a target | `callers` | Direct, semantically resolved call sites only. |
| Find functions directly called by a target | `callees` | Direct calls in the target body only. |
| List symbols declared in a known file | `symbols` | File inventory, not cross-workspace discovery. |

Prefer these direct semantic routes over grep/manual exploration whenever the request maps to one. Use Grep for textual content—comments, TODO/FIXME, string literals, logs, documentation, or configuration—or after the documented semantic fallback condition is met.

## Essential Syntax and Scope

```text
ast-tool search [--name <name>] [--fqn <fqn>] [--kind <kind>] [--file <path>]
                [--name-regex <re>] [--fqn-regex <re>] [--file-regex <re>]
                [--json [--pretty]] <root>
ast-tool find [--type <type>] [--text <text>] [--id <hex>]
              [--line <n> --column <n>] <file>
ast-tool references [--json [--pretty]] <symbol> <root>
ast-tool callers    [--json [--pretty]] <symbol> <root>
ast-tool callees    [--json [--pretty]] <symbol> <root>
ast-tool symbols <file>
```

`search` filters are ANDed. Use `--kind` to reduce volume and `--fqn`/`--fqn-regex` to scope a namespace or class. Use `search --name <name> <root>` before `callers` or `references` to confirm an exact FQN.

Typical targeted forms:

```text
ast-tool search --name parse src/
ast-tool search --kind class --file-regex '\.hpp$' include/
ast-tool find --type function_definition src/parser.cpp
ast-tool find --line 42 --column 17 src/parser.cpp
ast-tool references ast::parse src/
ast-tool callers ast::parse src/
ast-tool callees ast::parse src/
```

For `references`, `callers`, and `callees`, `<root>` must be a directory. A file root produces “workspace is empty or could not be analyzed”; retry with the containing directory. A symbol containing `::` matches FQNs; otherwise it matches unqualified names.

`find`'s `--line` and `--column` must be supplied together. `--line` and `--column` are only for `find`; `--kind`, `--file`, and `--name` are only for `search`; `--id` is only for `find`, `parent`, and `children`. Never pass these filters to `callers`, `callees`, or `references`.

## Semantic Boundaries

- `references` returns genuine semantic uses, not textual matches. A valid target with no references exits successfully with empty output.
- `references` excludes the declaration site and sorts results by file, line, and column.
- `callers` targets a function, method, constructor, or destructor. It reports direct calls only, excluding unresolved indirect calls through function pointers or virtual dispatch. Plain output is `<caller_fqn> <file>:<line>:<col>` or `<file_scope>` for file-scope calls. A valid target may have no callers; recurse over discovered callers only when a transitive graph is requested.
- `callees` has the same callable-target and direct-resolution limits, but reports calls made inside the target rather than calls to it.
- Empty output with exit 0 is not a failure. The symbol can be unused, or calls may be indirect and therefore outside semantic resolution.
- Do not use grep for declarations or callers when a semantic command applies: text matches include comments, strings, forward declarations, and unrelated occurrences.

## Error-Directed Recovery

Never retry an unchanged failed command. Use the diagnostic and known results to make the cheapest useful correction. Each retry must change the FQN, root, or command in a way that addresses the failure; `--pretty`, redirection, path-format changes, and similar variations do not fix semantic errors.

Avoid routine `--help` discovery and repeated syntax exploration after a targeted failure. Use help only when the required syntax is not stated here and the error does not supply it. Prefer one error-directed recovery action over trial-and-error sequences.

| Result | Meaning | Next action |
|---|---|---|
| `symbol 'X' not found in workspace` | Misspelling, wrong namespace, or missing qualifier | Run `search --name X <root>`, choose the returned FQN, and retry once. |
| `symbol 'X' is ambiguous (N matches)` | More than one declaration matches | Read the candidates and apply the ambiguity cases below. |
| `workspace ... is empty or could not be analyzed` | A file was passed as `<root>` | Use its containing directory. |
| Empty output, exit 0 | Valid symbol with no resolved results | Accept it; do not recover or retry. |

Stop after two failed attempts on the same semantic command. Do not substitute grep after the first correctable error; fall back only when ambiguity remains unresolved after the recovery rules below or after two meaningfully different semantic attempts fail.

## Ambiguity Recovery

For ambiguous `callers`, `callees`, or `references`, distinguish these cases from the candidate list.

The diagnostic lists each candidate's kind, FQN, and file. Inspect that list before choosing a recovery; do not assume the query will auto-resolve.

### Different namespaces or unrelated classes

Run `search --name X <root>` if needed, select the intended declaration, and retry with its exact, more-qualified FQN.

### C++ declaration/definition pair with the same FQN

A header declaration and implementation definition can share an FQN. Adding qualifiers cannot resolve this, and `callers`, `callees`, and `references` have no `--kind` or `--file` filters.

1. Run `search --fqn <FQN> <root>` and inspect candidate paths.
2. If header and implementation are in separate directories, narrow `<root>` to the implementation directory and retry.
3. If they share a directory, or the narrowed semantic attempt is still ambiguous, use Grep for implementation call sites instead of repeating the command.

Do not run the same FQN against the same root more than twice. After two failures, proceed directly to Grep.

### Same unqualified name, unrelated symbols

Run `search --name X <root>`, select the correct candidate, and retry using its exact FQN.

## Call-Site Workflow

For callers of `Foo::bar`:

1. Confirm the FQN with `search --name bar <root>`.
2. Run `callers Foo::bar <root>`.
3. On “not found,” correct the FQN from search output and retry once.
4. On ambiguity, apply the appropriate case above.
5. After two meaningfully different failed callers attempts, use a scoped Grep for `bar(` in implementation files.

Use the equivalent sequence for `references` or `callees`; do not replace the requested relationship with `callers` merely because all three accept a symbol and root.

## Output and Cost Boundaries

Plain text is the default and is preferred for quick inspection or file:line locations. Use `--json` only for programmatic processing. Do not use `--pretty` by default; large pretty JSON wastes context. For repeated analysis of a large workspace, cache one necessary `search --json` result and query it locally rather than rerunning broad searches.

Scope name, kind, FQN, file, and root as narrowly as available evidence allows. Do not dump the whole workspace when a targeted query can answer the request. Read only the returned locations needed for the task.

Default output is intentionally compact: `search` emits `<kind> <fqn> <file>:<line>:<col>`, callers/callees emit the related FQN and location, and references emit source locations. Results are deterministically sorted. Request structured output only when downstream processing requires it.
