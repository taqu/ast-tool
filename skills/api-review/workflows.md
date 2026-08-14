---
name: api-review workflows
description: Step-by-step workflows for reviewing a library's public API with ast-tool
---

# API Review Workflows

## Workflow 1 — Enumerate the Public API

**Goal:** List all declarations exported from public headers.

```bash
# All symbols from .h files under include/
ast-tool search --file-regex "\\.h$" --pretty ./include > api.json

# Human-readable
ast-tool search --file-regex "\\.h$" ./include
```

---

## Workflow 2 — Filter to Specific Symbol Kinds

**Goal:** Review only functions, or only classes, in the public API.

```bash
# Public functions
ast-tool search --file-regex "\\.h$" --kind function --json ./include \
  | jq -r '.[] | "\(.fqn)  \(.file):\(.line)"'

# Public classes and structs
ast-tool search --file-regex "\\.h$" --json ./include \
  | jq '[.[] | select(.kind == "class" or .kind == "struct")]'
```

---

## Workflow 3 — Breaking-Change Detection

**Goal:** Find symbols removed or renamed between two git revisions.

```bash
# Snapshot API at HEAD
ast-tool search --file-regex "\\.h$" --json ./include \
  | jq -r '.[].fqn' | sort > api_new.txt

# Snapshot API at the previous release tag
git stash
git checkout v1.0.0
ast-tool search --file-regex "\\.h$" --json ./include \
  | jq -r '.[].fqn' | sort > api_old.txt
git checkout -

# Removed symbols (potential breaking changes)
comm -23 api_old.txt api_new.txt

# Added symbols (new API)
comm -13 api_old.txt api_new.txt
```

---

## Workflow 4 — Naming Convention Audit

**Goal:** Verify that all public functions follow a naming convention (e.g., `snake_case`).

```bash
ast-tool search --file-regex "\\.h$" --kind function --json ./include \
  | jq -r '.[].name' \
  | grep -Ev "^[a-z][a-z0-9_]*$" \
  | sort -u
```

Non-matching names are printed. An empty result means all functions comply.

---

## Workflow 5 — Per-Header Review

**Goal:** Review the structure and declarations of each public header individually.

```bash
for header in include/*.h; do
  echo "========================================"
  echo "HEADER: $header"
  echo "--- Outline ---"
  ast-tool outline "$header"
  echo "--- Symbols ---"
  ast-tool symbols "$header"
  echo ""
done
```

---

## Workflow 6 — API Completeness Check

**Goal:** Verify that every intended public symbol is present.

```bash
# Expected API (manually maintained list)
cat expected_api.txt
# parse_expression
# build_ast
# AstNode

# Actual API names
ast-tool search --file-regex "\\.h$" --json ./include | jq -r '.[].name' | sort > actual_names.txt

# Missing from actual
sort expected_api.txt | comm -23 - actual_names.txt
```
