---
name: ast-inspection examples
description: Concrete annotated examples of every ast-inspection command
---

# AST Inspection Examples

## dump

```bash
# Plain text — all nodes depth-first
ast-tool dump src/main.cpp

# JSON output
ast-tool dump --json src/main.cpp

# Pretty JSON for reading
ast-tool dump --pretty src/main.cpp
```

Typical output line (plain):
```
[0x9E52E360] translation_unit (0:0)-(120:0) named
  [0x1A2B3C4D] function_definition (3:0)-(15:2) named
    [0xDEADBEEF] type_specifier (3:0)-(3:3) named
```

---

## outline

```bash
# Indented tree of named nodes only
ast-tool outline src/parser.py

# JSON representation
ast-tool outline --pretty src/parser.py
```

---

## find — by type

```bash
# Find all function definitions in a C++ file
ast-tool find --type function_definition src/main.cpp

# Find all identifiers named "argc"
ast-tool find --type identifier --text "argc" src/main.cpp
```

---

## find — by text

```bash
# Find any node whose source text contains "TODO"
ast-tool find --text "TODO" src/main.cpp
```

---

## find — by cursor position

```bash
# What is at line 42, column 10?
ast-tool find --line 42 --column 10 src/main.cpp

# JSON — useful for editor integration
ast-tool find --json --line 42 --column 10 src/main.cpp
```

---

## find — by node ID

```bash
# Retrieve the node with this exact ID
ast-tool find --id 9E52E360 src/main.cpp

# Hex prefix and case are both accepted
ast-tool find --id 0x9e52e360 src/main.cpp
```

---

## range

```bash
# All nodes intersecting lines 10–20
ast-tool range --start-line 10 --end-line 20 src/main.cpp

# Precise column range on a single line
ast-tool range --start-line 5 --start-column 4 --end-line 5 --end-column 20 src/main.cpp

# From beginning of file to line 5
ast-tool range --end-line 5 src/main.cpp
```

---

## parent

```bash
# Print the parent of node 9E52E360
ast-tool parent --id 9E52E360 src/main.cpp

# JSON output
ast-tool parent --json --id 9E52E360 src/main.cpp
```

---

## children

```bash
# List immediate children of a node
ast-tool children --id 9E52E360 src/main.cpp

# Pretty JSON
ast-tool children --pretty --id 9E52E360 src/main.cpp
```

---

## Chaining — navigate from cursor to parent

```bash
# 1. Find the node at the cursor
NODE_ID=$(ast-tool find --json --line 42 --column 10 src/main.cpp | jq -r '.[0].id')

# 2. Walk to its parent
ast-tool parent --id "$NODE_ID" src/main.cpp
```
