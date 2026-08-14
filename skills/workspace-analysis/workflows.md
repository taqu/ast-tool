---
name: workspace-analysis workflows
description: Step-by-step workflows for searching symbols across an entire project
---

# Workspace Analysis Workflows

## Workflow 1 — Find a Symbol by Name

**Goal:** Locate where a function or class named `X` is declared anywhere in the project.

```bash
ast-tool search --name "parse_expression" ./src
```

Returns every symbol whose name contains `"parse_expression"` (substring match).

---

## Workflow 2 — Find All Symbols of a Given Kind

**Goal:** List all enums, all classes, or all interfaces in the project.

```bash
# All enums
ast-tool search --kind enum ./src

# All classes
ast-tool search --kind class ./src

# All functions (can be large — pipe to less)
ast-tool search --kind function ./src | less
```

---

## Workflow 3 — Narrow by File Pattern

**Goal:** Find symbols only in files matching a path pattern.

```bash
# All functions in files under src/parser/
ast-tool search --kind function --file "parser" ./src

# Using regex for more precision
ast-tool search --kind function --file-regex "parser/.*\\.cpp$" ./src
```

---

## Workflow 4 — Search by Fully-Qualified Name

**Goal:** Find a symbol in a specific namespace or class scope.

```bash
# Exact FQN substring
ast-tool search --fqn "MyNamespace::Parser::parse" ./src

# Regex FQN
ast-tool search --fqn-regex "^Parser::.+Expression" ./src
```

---

## Workflow 5 — Combined Filters

**Goal:** Find all methods named `validate` defined in files under `model/`.

```bash
ast-tool search --name "validate" --kind method --file "model" ./src
```

All filters are ANDed — only symbols matching all criteria are returned.

---

## Workflow 6 — JSON Output for Downstream Processing

**Goal:** Collect search results for programmatic use (e.g., building a navigation index).

```bash
ast-tool search --kind function --json ./src > functions.json

# Count results
jq length functions.json

# Extract file + name pairs
jq -r '.[] | "\(.file):\(.line)  \(.name)"' functions.json
```

---

## Workflow 7 — Regex Name Search

**Goal:** Find all symbols whose names follow a naming convention (e.g., `get_*` getters).

```bash
ast-tool search --name-regex "^get_" ./src
ast-tool search --name-regex ".*Handler$" ./src
```

RE2 regex syntax is used. Anchors (`^`, `$`) work as expected.
