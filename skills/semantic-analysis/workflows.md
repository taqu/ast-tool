---
name: semantic-analysis workflows
description: Step-by-step workflows for extracting semantic symbols from a single file
---

# Semantic Analysis Workflows

## Workflow 1 — List All Symbols in a File

**Goal:** Get the complete symbol table for a source file.

```bash
ast-tool symbols src/parser.cpp
```

Plain-text output lists each symbol with its kind, name, and location. Use this as a first pass to understand what is defined in a file.

---

## Workflow 2 — JSON Symbol Table

**Goal:** Obtain machine-readable symbols for downstream processing.

```bash
ast-tool symbols --json src/parser.cpp

# Pretty-printed for readability
ast-tool symbols --pretty src/parser.cpp
```

Pipe to `jq` for filtering:

```bash
# List only function names
ast-tool symbols --json src/parser.cpp | jq '[.[] | select(.kind == "function") | .name]'

# Count symbols by kind
ast-tool symbols --json src/parser.cpp | jq 'group_by(.kind) | map({kind: .[0].kind, count: length})'
```

---

## Workflow 3 — Check Whether a Symbol Is Defined

**Goal:** Verify that a specific function or class is declared in a file.

```bash
ast-tool symbols --json src/parser.cpp | jq '.[] | select(.name == "parse_expression")'
```

A non-empty result confirms the symbol is present.

---

## Workflow 4 — Compare Symbol Lists Across Revisions

**Goal:** See what symbols were added or removed between two versions of a file.

```bash
ast-tool symbols --json src/parser.cpp > symbols_new.json
git show HEAD~1:src/parser.cpp | ast-tool symbols --json /dev/stdin > symbols_old.json
diff <(jq -r '.[].name' symbols_old.json | sort) <(jq -r '.[].name' symbols_new.json | sort)
```

---

## Workflow 5 — Generate a File's Public API Summary

**Goal:** Produce a human-readable list of public declarations for documentation.

```bash
ast-tool symbols src/api.h
```

Use the output to document which functions are exported or to populate a header comment.
