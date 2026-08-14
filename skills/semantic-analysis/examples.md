---
name: semantic-analysis examples
description: Annotated examples of the symbols command
---

# Semantic Analysis Examples

## Basic Symbol Listing

```bash
# All symbols, plain text
ast-tool symbols src/main.cpp

# All symbols, pretty JSON
ast-tool symbols --pretty src/main.cpp
```

Example plain-text output:
```
function  main                src/main.cpp:3:0
function  parse_args          src/main.cpp:18:0
struct    Config              src/main.cpp:42:0
function  Config::validate    src/main.cpp:55:4
enum      ErrorCode           src/main.cpp:70:0
```

---

## JSON Output Structure

```bash
ast-tool symbols --pretty src/main.cpp
```

```json
[
  {
    "kind": "function",
    "name": "main",
    "fqn": "main",
    "file": "src/main.cpp",
    "line": 3,
    "column": 0
  },
  {
    "kind": "struct",
    "name": "Config",
    "fqn": "Config",
    "file": "src/main.cpp",
    "line": 42,
    "column": 0
  }
]
```

---

## Filter by Kind with jq

```bash
# Functions only
ast-tool symbols --json src/main.cpp | jq '[.[] | select(.kind == "function")]'

# Classes and structs
ast-tool symbols --json src/main.cpp | jq '[.[] | select(.kind == "class" or .kind == "struct")]'
```

---

## Extract Names Only

```bash
ast-tool symbols --json src/main.cpp | jq -r '.[].name'
```

---

## Find a Symbol's Line Number

```bash
ast-tool symbols --json src/main.cpp | jq '.[] | select(.name == "parse_args") | .line'
```

---

## Python File Example

```bash
ast-tool symbols --pretty app/routes.py
```

```json
[
  {"kind": "function", "name": "index",        "line": 10},
  {"kind": "function", "name": "get_user",     "line": 18},
  {"kind": "class",    "name": "UserSchema",   "line": 30},
  {"kind": "function", "name": "validate",     "line": 35}
]
```

---

## TypeScript File Example

```bash
ast-tool symbols --pretty src/api/client.ts
```

```json
[
  {"kind": "interface",   "name": "ApiClient",   "line": 5},
  {"kind": "class",       "name": "HttpClient",  "line": 15},
  {"kind": "method",      "name": "get",         "line": 20},
  {"kind": "method",      "name": "post",        "line": 28},
  {"kind": "type_alias",  "name": "RequestOpts", "line": 40}
]
```
