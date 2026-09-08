# AI Integration

`ast-tool` is designed for use by AI coding agents as well as human developers.
This page explains how agents should integrate with the tool, which commands to use for common development tasks, and how to avoid common mistakes.

The companion document [`skills/semantic-analysis/SKILL.md`](../skills/semantic-analysis/SKILL.md) is a standalone operational guide intended to be passed directly to AI agents as context (for example, as a Claude Code Skill).

---

## Why ast-tool for AI Agents

Text search (`grep`, string matching) is fragile for structural code questions. `ast-tool` gives agents:

- **Precise structural answers** — "what functions are in this file?" rather than "which lines contain the word `function`?"
- **Stable node references** — node IDs survive reformatting and can be used across multiple commands in a session
- **Semantic resolution** — fully-qualified names, access modifiers, and scope membership without manual parsing
- **Multi-language support** — one tool, one interface, 15 languages

---

## Recommended Workflows

### Understanding an unfamiliar file

Start with structural overview, then extract symbols:

```sh
ast-tool outline src/parser.cpp
ast-tool symbols --json --pretty src/parser.cpp
```

`outline` shows nesting; `symbols --json` gives the full structured picture of declarations.

### Locating a declaration

When you know the name:

```sh
ast-tool search --name parseExpression src/
ast-tool search --fqn 'Parser::parseExpression' src/
```

When you know the line (e.g. from a compiler error):

```sh
ast-tool find --line 42 --column 1 src/parser.cpp
```

### Inspecting a construct

Get the node ID from `find` or `outline`, then navigate:

```sh
# Find the function
ast-tool find --type function_definition --text parseExpression src/parser.cpp

# List its children (parameters, body, return type)
ast-tool children --id 9E52E360 src/parser.cpp

# Find its enclosing class
ast-tool parent --id 9E52E360 src/parser.cpp
```

### Querying the workspace

```sh
# All classes in a namespace
ast-tool search --kind class --fqn-regex '^ast::' src/

# All public functions in header files
ast-tool search --kind function --file-regex '\.hpp$' include/

# Export results for multi-step processing
ast-tool search --kind function --json src/ > symbols.json
```

### Tracing relationships

```sh
# Everywhere a symbol is referenced
ast-tool references Parser::parse src/

# Every direct caller of a function
ast-tool callers Parser::parse src/

# Every function a function directly calls
ast-tool callees Parser::parse src/
```

`references`/`callers`/`callees` are public CLI commands, not only internal library types — use them directly instead of reconstructing call/reference relationships by reading and cross-referencing files manually. They report only *direct*, semantically-resolved relationships (not a transitive graph, not text matches), and an unqualified name that matches more than one declaration fails closed with a candidate list rather than guessing — retry with a fully-qualified name.

---

## Command Selection

| Agent task | Command |
|-----------|---------|
| What is the structure of this file? | `outline` |
| What symbols does this file declare? | `symbols` |
| Where is symbol X declared? | `search --name X` or `search --fqn X` |
| Where is symbol X referenced/used? | `references X <root>` |
| Who calls function X? | `callers X <root>` |
| What does function X call? | `callees X <root>` |
| What is at line N, column M? | `find --line N --column M` |
| What nodes are in this range? | `range --start-line N --end-line M` |
| What contains this node? | `parent --id <hex>` |
| What does this node contain? | `children --id <hex>` |
| What is the complete syntax of this file? | `dump` (last resort) |

---

## Output for Programmatic Use

Use `--json` for any output that will be processed in subsequent steps:

```sh
ast-tool symbols --json src/lexer.cpp
ast-tool search --kind class --json --pretty src/
```

JSON output contains all available metadata (kind, access, qualifiers, fqn, file, line, column) and is stable across minor tool versions.

Plain-text output is designed for human reading and may change formatting between versions.

---

## Prompting Recommendations

When providing `ast-tool` as a capability to an AI agent, include `skills/semantic-analysis/SKILL.md` as context. It describes:

- which command to use for each task
- how to chain commands using node IDs
- common mistakes and how to avoid them
- the layered architecture (so the agent understands when to use semantic vs. AST-level commands)

Example system prompt fragment:

```
You have access to ast-tool, an AST analysis CLI. skills/semantic-analysis/SKILL.md
in the repository describes how to use it effectively. Prefer semantic commands 
(symbols, search, references, callers, callees) over raw AST traversal (dump) 
or manual text search when you need information about named declarations or 
their relationships.
```

---

## Semantic vs. AST-Level Commands

Use **semantic commands** (`symbols`, `search`, `references`, `callers`, `callees`) when the question is about named entities or their relationships:
- "What functions does this class have?"
- "Where is `Parser::parse` declared?"
- "Are there any static methods in namespace `ast`?"
- "Who calls `Parser::parse`?" / "What does `Parser::parse` call?"
- "Where is this symbol used?"

Use **AST commands** (`find`, `outline`, `range`, `parent`, `children`) when the question is about syntax structure:
- "What is at cursor position (42, 17)?"
- "What are the child nodes of this function definition?"
- "What is the enclosing construct of this expression?"

Use **`dump`** only when you need every node including anonymous punctuation tokens, or when investigating parser output.

Do not treat this as guaranteed automatic routing: whether an agent actually invokes a semantic command for a given task depends on the agent's own decision-making, not on ast-tool itself. If a semantic query fails to resolve after following its `--help` disambiguation guidance, fall back to ordinary file inspection rather than retrying the same query indefinitely.

---

## Limitations Agents Should Know

- **Local variables and parameters are not uniformly in the symbol table across all languages.** `symbols` and `search` primarily return declarations visible at namespace or class scope; `callers`/`callees`/`references` can still resolve a call through a directly-typed local or parameter where the language extractor supports it.
- **Node IDs change when files are modified.** Capture IDs and use them within a single analysis session; do not persist them across edits.
- **An ambiguous unqualified name fails closed.** `references`/`callers`/`callees` never guess between multiple matching declarations — they report the candidate list and expect a fully-qualified retry.
- **Indirect calls are not resolved.** `callers` and `callees` report only direct, resolvable calls. Virtual dispatch and function-pointer calls are not reported, and relationships are not transitive (direct call graph only).
- **No inferred receiver typing.** Calls through `auto`/`decltype`-typed receivers, complex chained/cast/subscript expressions, or `this->` are not resolved; only directly-typed object/pointer fields, locals, and parameters are.
- **Paths must stay within the active Windows code page.** A workspace/file path mixing scripts outside your system locale is not currently supported.
- **Line numbers are always 1-based** in CLI output (0-based internally in the C++ library API).
