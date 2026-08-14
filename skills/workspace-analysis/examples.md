---
name: workspace-analysis examples
description: Annotated examples of the search command across a project directory
---

# Workspace Analysis Examples

## Find a Function by Name

```bash
ast-tool search --name "parse_expression" ./src
```

Output:
```
function  parse_expression  src/parser/expr.cpp:42:0
function  parse_expression  src/parser/expr.h:10:0
```

---

## Find All Enums in the Project

```bash
ast-tool search --kind enum ./src
```

---

## Find Symbols in a Subdirectory

```bash
# Only files whose path contains "model"
ast-tool search --kind class --file "model" ./src
```

---

## Regex Name Match — All Getters

```bash
ast-tool search --name-regex "^get_" ./src
```

---

## Regex FQN Match — All Methods in a Namespace

```bash
ast-tool search --fqn-regex "^MyLib::.+::parse" ./src
```

---

## Restrict to C++ Headers Only

```bash
ast-tool search --kind function --file-regex "\\.h$" ./src
```

---

## JSON Output — Build a Navigation Index

```bash
ast-tool search --json ./src > workspace_index.json

# Pretty-print
ast-tool search --pretty ./src > workspace_index.json
```

---

## Count Symbols by Kind (jq)

```bash
ast-tool search --json ./src | jq 'group_by(.kind) | map({kind: .[0].kind, count: length}) | sort_by(.count) | reverse'
```

Example output:
```json
[
  {"kind": "function", "count": 312},
  {"kind": "variable", "count": 198},
  {"kind": "struct",   "count": 47},
  {"kind": "enum",     "count": 23}
]
```

---

## Find All Declarations of a Symbol Across Files

```bash
ast-tool search --name "Config" --pretty ./src
```

```json
[
  {"kind": "struct",   "name": "Config", "fqn": "Config",        "file": "src/config.h",    "line": 5},
  {"kind": "class",    "name": "Config", "fqn": "net::Config",   "file": "src/net/config.h","line": 12},
  {"kind": "variable", "name": "Config", "fqn": "g_Config",      "file": "src/main.cpp",    "line": 7}
]
```
