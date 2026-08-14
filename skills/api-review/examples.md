---
name: api-review examples
description: Annotated examples for API surface review and breaking-change detection
---

# API Review Examples

## Enumerate All Public Declarations

```bash
ast-tool search --file-regex "\\.h$" --pretty ./include
```

```json
[
  {"kind": "function", "name": "parse",      "fqn": "ast::parse",      "file": "include/ast.h",    "line": 12},
  {"kind": "class",    "name": "AstNode",    "fqn": "ast::AstNode",    "file": "include/ast.h",    "line": 30},
  {"kind": "enum",     "name": "NodeKind",   "fqn": "ast::NodeKind",   "file": "include/ast.h",    "line": 55},
  {"kind": "function", "name": "stringify",  "fqn": "ast::stringify",  "file": "include/utils.h",  "line": 8}
]
```

---

## Public Functions Only

```bash
ast-tool search --file-regex "\\.h$" --kind function --json ./include \
  | jq -r '.[] | "\(.fqn)  \(.file):\(.line)"'
```

Output:
```
ast::parse       include/ast.h:12
ast::stringify   include/utils.h:8
```

---

## Breaking-Change Diff

```bash
# Save new snapshot
ast-tool search --file-regex "\\.h$" --json ./include \
  | jq -r '.[].fqn' | sort > /tmp/api_new.txt

# Save old snapshot (from git)
git show v1.2.0:include/ | ... # (use git worktree or stash)

# Show removed symbols
comm -23 /tmp/api_old.txt /tmp/api_new.txt
```

Example output:
```
ast::AstNode::parent_id   ← REMOVED (breaking)
ast::internal_helper      ← REMOVED (was it public? check)
```

---

## Naming Convention Violations

```bash
# C++ snake_case check for public functions
ast-tool search --file-regex "\\.h$" --kind function --json ./include \
  | jq -r '.[].name' \
  | grep -Ev "^[a-z][a-z0-9_]*$"
```

Output (violations):
```
ParseExpression     ← PascalCase — should be parse_expression
buildAST            ← mixed case — should be build_ast
```

---

## Single-Header Detailed Review

```bash
ast-tool outline include/ast.h
echo "---"
ast-tool symbols include/ast.h
```

---

## API Completeness Check

```bash
# expected_api.txt contains one expected FQN per line
ast-tool search --file-regex "\\.h$" --json ./include \
  | jq -r '.[].fqn' | sort > /tmp/actual.txt

echo "=== Missing from actual API ==="
sort expected_api.txt | comm -23 - /tmp/actual.txt

echo "=== Unexpected symbols ==="
sort expected_api.txt | comm -13 - /tmp/actual.txt
```
