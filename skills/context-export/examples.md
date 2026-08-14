---
name: context-export examples
description: Concrete shell scripts and patterns for assembling ast-tool context packages
---

# Context Export Examples

## Single-File LLM Context

```bash
FILE=src/parser.cpp

{
  echo "## File: $FILE"
  echo ""
  echo "### Outline"
  ast-tool outline "$FILE"
  echo ""
  echo "### Symbols"
  ast-tool symbols "$FILE"
} | pbcopy   # macOS; use xclip on Linux or clip on Windows
```

---

## Module JSON Package

```bash
# All symbols in src/net/, output as a JSON file
ast-tool search --file "src/net" --pretty ./src > net_module_symbols.json
```

---

## PR Review Script

```bash
#!/usr/bin/env bash
# Generate context for all C/C++ files changed in the last commit
git diff --name-only HEAD~1 HEAD -- '*.c' '*.cpp' '*.h' | while read -r f; do
  [ -f "$f" ] || continue
  echo "========================================"
  echo "FILE: $f"
  echo "========================================"
  echo "--- OUTLINE ---"
  ast-tool outline "$f"
  echo ""
  echo "--- SYMBOLS ---"
  ast-tool symbols "$f"
  echo ""
done
```

---

## Navigation Index (build + query)

```bash
# Build
ast-tool search --json ./src > .nav-index.json

# Query: find all classes
jq '[.[] | select(.kind == "class")]' .nav-index.json

# Query: symbol at a specific file:line
jq --arg f "src/parser.cpp" --argjson l 42 \
  '[.[] | select(.file == $f and .line == $l)]' .nav-index.json
```

---

## API Snapshot for Documentation

```bash
# Export all symbols from public headers as pretty JSON
ast-tool search --file-regex "\\.h$" --pretty ./include > api_snapshot.json

# Extract function signatures
jq -r '.[] | select(.kind == "function") | "\(.fqn)  (\(.file):\(.line))"' api_snapshot.json
```

---

## Call-Site Impact Analysis

```bash
FUNC="parse_expression"

# Find all files that mention the function name
mapfile -t FILES < <(grep -rl "$FUNC" ./src)

echo "Call sites of $FUNC:"
for f in "${FILES[@]}"; do
  ast-tool find --text "$FUNC" --json "$f" | jq -r ".[] | \"$f:\(.start_line)  \(.text)\""
done
```
