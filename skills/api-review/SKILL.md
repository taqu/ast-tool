---
name: api-review
description: Review or assess the impact of API changes using ast-tool semantic commands — locate the target API, inspect its usages, and trace its direct caller and callee relationships.
triggers:
  - "review API"
  - "check public API"
  - "find breaking changes"
  - "API surface"
  - "exported symbols"
  - "public declarations"
  - "API consistency"
  - "impact of changing"
  - "who uses this function"
  - "assess change impact"
  - "is this safe to remove"
---

# API Review Skill

## Purpose

Compose semantic inspection commands to review or assess the impact of API changes: locate the target symbol, enumerate its usages, and trace its direct call relationships.

This skill describes when and how to combine `search`, `find`, `references`, `callers`, and `callees` during an API review. For full command documentation see the **semantic-analysis** skill.

## Typical Workflow

```text
1. Locate the target API
        ↓
     search / find

2. Inspect usages
        ↓
    references

3. Inspect incoming dependencies
        ↓
      callers

4. Inspect outgoing dependencies
        ↓
      callees
```

Use `search` when the exact symbol is not yet known. Use `find` when the file is known and you need to locate the node by name or position.

---

## Workflow Steps

### Step 1 — Locate the target API

If the declaration file is unknown, search the workspace:
```
ast-tool search --name <symbol> <root>
ast-tool search --kind function --name <symbol> <root>
```

If the symbol belongs to a known namespace:
```
ast-tool search --fqn-regex '^MyNs::' <root>
```

To enumerate all public declarations in header files:
```
ast-tool search --file-regex '\.hpp$' <root>
ast-tool search --kind function --file-regex '\.h$' <root>
```

If the file is known, locate the node directly:
```
ast-tool find --text <symbol> <file>
```

---

### Step 2 — Inspect usages

Find every location where the target symbol is referenced:
```
ast-tool references <symbol> <root>
ast-tool references MyNs::symbol <root>
```

- A large number of references indicates wide usage and high change impact.
- An empty result means the symbol is unreferenced — check scope before removing.
- The declaration site is excluded from results.

---

### Step 3 — Inspect incoming dependencies (callers)

Find every function that directly calls the target function:
```
ast-tool callers <symbol> <root>
ast-tool callers MyNs::symbol <root>
```

Callers must be reviewed or updated when the function signature or behavior changes.

Note: only direct callers are reported. Transitive callers (callers of callers) require applying `callers` recursively to each discovered caller.

---

### Step 4 — Inspect outgoing dependencies (callees)

Find every function directly called within the target function's body:
```
ast-tool callees <symbol> <root>
ast-tool callees MyNs::symbol <root>
```

Callees indicate what the implementation depends on. Changes to callees may affect the target function's behavior or its callers.

---

## When to Use This Skill

- Before modifying a function's signature: enumerate its callers and usages.
- Before removing a declaration: verify it has no references or callers.
- When reviewing a PR that changes a public function: trace who calls it.
- When assessing the impact of a refactor: combine `references` and `callers`.
- When checking if a function is dead code: verify zero references and zero callers.

## Decision Guide

| Question | Command |
|---|---|
| "Is this function used anywhere?" | `references` |
| "Who calls this function?" | `callers` |
| "What does this function call?" | `callees` |
| "Where is this symbol declared?" | `search` or `find` |
| "What does this file declare?" | `symbols` (ast-inspection skill) |
| "What is the structure of this header?" | `outline` (ast-inspection skill) |

## Common Mistakes

**Using grep to find callers.**
Text search matches all occurrences of the function name — including in comments, strings, and forward declarations — not just call sites. Use `callers` for accurate call-site enumeration.

**Assuming an empty `references` result means the symbol is safe to remove.**
Verify that the workspace root passed to `references` covers the full relevant source tree. An empty result is only reliable if the search scope is complete.

**Treating `callers` as a transitive call graph.**
`callers` reports only direct call sites. To understand indirect callers, apply `callers` recursively to the discovered caller set.

## Best Practices

- Run `references` first to understand the breadth of usage before running `callers`.
- Use fully-qualified names (containing `::`) when the unqualified name is ambiguous.
- Use `--json` output to compare results programmatically or diff across revisions.
- Scope `search` to the intended directory to avoid noise from test or generated code.
- For impact assessment, combine `references` + `callers`: `references` shows where the symbol appears; `callers` shows which functions actively invoke it.
