---
name: context-export workflows
description: Workflows for composing ast-tool outputs into structured context packages
---

# Context Export Workflows

## Workflow 1 — Single-File Context Package

**Goal:** Full structural context for one file: outline + symbol table.

```bash
FILE=src/parser.cpp

echo "=== OUTLINE ==="
ast-tool outline "$FILE"

echo ""
echo "=== SYMBOLS ==="
ast-tool symbols "$FILE"
```

Paste both sections into an LLM prompt for file-scoped review or refactoring.

---

## Workflow 2 — Module Context Package (JSON)

**Goal:** Structured JSON context for all files in a subdirectory.

```bash
DIR=src/parser

# Symbol table for every file
ast-tool search --file "$DIR" --json . > module_symbols.json

# Per-file outlines (requires iterating files)
for f in "$DIR"/*.cpp "$DIR"/*.h; do
  echo "--- $f ---"
  ast-tool outline "$f"
done > module_outlines.txt
```

---

## Workflow 3 — Public API Snapshot

**Goal:** Export the public API surface of a library's headers.

```bash
ast-tool search --file ".h" --json ./include > api_symbols.json

# Human-readable version
ast-tool search --file ".h" --pretty ./include
```

Feed `api_symbols.json` into the `api-review` skill or an LLM review prompt.

---

## Workflow 4 — Find All Call Sites of a Function

**Goal:** Locate every call to `my_function` across the workspace for impact analysis.

```bash
# 1. Find all files that declare or reference the symbol
ast-tool search --name "my_function" --json ./src \
  | jq -r '.[].file' | sort -u > files_with_symbol.txt

# 2. For each file, find the AST node and its context
while IFS= read -r f; do
  echo "=== $f ==="
  ast-tool find --text "my_function" "$f"
done < files_with_symbol.txt
```

**Avoid:** `grep -rl "my_function"` — this matches occurrences in comments, strings, and macro arguments. Use `ast-tool search` to identify relevant files, then `ast-tool find --text` for node-level detail.

---

## Workflow 5 — Workspace Navigation Index

**Goal:** Build a full symbol index for the project (e.g., for a code-navigation tool).

```bash
ast-tool search --pretty ./src > .ast-tool-index.json
```

Update the index after significant changes:

```bash
ast-tool search --json ./src > .ast-tool-index.json
```

---

## Workflow 6 — Changed-File Context for PR Review

**Goal:** Collect symbol context for all files changed in a PR.

```bash
# Get changed files from git
CHANGED=$(git diff --name-only HEAD~1 HEAD -- '*.cpp' '*.h' '*.py' '*.ts')

for f in $CHANGED; do
  echo "=== $f ==="
  ast-tool outline "$f"
  ast-tool symbols "$f"
  echo ""
done > pr_context.txt
```
