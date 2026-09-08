# ast-tool

A command-line tool for structural and semantic source-code queries. It parses source files with [tree-sitter](https://tree-sitter.github.io/), builds a language-agnostic AST, and answers targeted questions about a codebase — symbol lookup, references, callers, callees — without reconstructing those relationships by hand from text search.

It is intended for interactive use, shell scripts, editor/IDE tooling, and Coding Agents that need a precise answer to a specific structural or semantic question.

## Features

- **Structural inspection** — outline a file, find AST nodes by type/text/position/ID, walk parent/child relationships.
- **Symbol extraction** — list every declared symbol in a file with its kind, fully-qualified name, access modifier, and storage qualifiers.
- **Workspace symbol search** — filter symbols across a directory tree by name, FQN, kind, or file path, with exact or regex matching.
- **References** — every location in the workspace that references a given symbol (semantic, not text matching).
- **Callers / callees** — every direct call site into, or out of, a given function or method.
- **Machine-readable output** — every semantic command supports `--json` (and `--pretty`) for scripting and agent use.

## Installation

Building from source is the only supported installation method for this release; there is no packaged binary or installer yet.

**Prerequisites (Windows):**
- CMake 3.50+
- Visual Studio 2022 (MSVC) with a C++23-capable toolchain
- Windows 10 SDK

```sh
cmake -B build -DCMAKE_INSTALL_PREFIX="D:\Programs\ast-tool"
cmake --build build --config Release
```

The executable and its required tree-sitter grammar DLLs are written to `bin/`. Copy the entire `bin/` directory to wherever you want to run it from — the tool is self-contained (statically-linked CRT) and works when relocated. `cmake --install` does not currently produce a working install; copying `bin/` is the supported path for this release.

**Verify the installation:**

```sh
ast-tool --version
ast-tool --help
```

Linux build prerequisites are recorded in `doc/build.md`, but Linux is not yet a release-qualified platform — see [Supported Platforms](#supported-platforms--languages) below.

## Quick Start

The examples below run against `evaluation/repositories/level1-store`, a small example C++ project bundled in this repository — substitute your own project's directory. (Analyzing a large workspace for the first time builds a persistent cache under `<root>/.ast-tool/`; later runs against the same root are fast. This repository's own `src/` includes a vendored copy of SQLite and is not a representative example — pointing an analysis command at it will be slow the first time.)

```sh
# Outline the structure of a file
ast-tool outline evaluation/repositories/level1-store/src/main.cpp

# List every symbol declared in a file
ast-tool symbols evaluation/repositories/level1-store/src/service/inventory_service.cpp

# Search for a symbol across a workspace
ast-tool search --name save evaluation/repositories/level1-store

# Find everywhere a symbol is referenced
ast-tool references store::InventoryService::save evaluation/repositories/level1-store

# Find every direct caller of a function
ast-tool callers InventoryService::save evaluation/repositories/level1-store

# Find every function a function directly calls
ast-tool callees store::OrderService::save evaluation/repositories/level1-store
```

Run `ast-tool --help` for the full command list, or `ast-tool <command> --help` for a command's detailed reference (arguments, output format, JSON fields, examples).

## Common Commands

| Command | Purpose |
|---|---|
| `outline` | Structural outline of a source file |
| `find` | Find AST nodes by type, text, position, or ID |
| `range` | Find AST nodes within a line/column range |
| `parent` / `children` | Navigate up/down from a specific AST node |
| `symbols` | List every symbol declared in a file |
| `search` | Search symbols across a workspace (exact or regex filters) |
| `references` | Find every reference to a symbol |
| `callers` | Find every direct caller of a function/method |
| `callees` | Find every function/method a function directly calls |
| `cache warm` / `cache status` | Manage the on-disk AST cache for a workspace |
| `setup` | Configure Claude Code / Codex to warm the cache automatically at session start |

For `search`, `references`, `callers`, and `callees`, the target `<root>` is a workspace directory scanned recursively. A `<symbol>` argument containing `::` is matched as a fully-qualified name; otherwise it is matched by unqualified name. If a query matches more than one declaration, the command fails with the candidate list rather than guessing — supply a fully-qualified name to disambiguate.

`callers`/`callees`/`references` report only **direct**, semantically-resolved relationships — not a transitive call graph, and not text matches.

## JSON / Automation Usage

`symbols`, `search`, `references`, `callers`, and `callees` all support `--json` (compact) and `--json --pretty` (indented). In JSON mode:

- Successful results print a JSON **array** to stdout (an empty result is `[]`, never an error).
- Errors (not found, ambiguous, invalid arguments) print a structured JSON object to **stderr**; stdout stays empty. stdout is always safe to pipe into a JSON parser.
- Exit code is `0` on success (including a valid empty result) and non-zero on failure.

```sh
ast-tool callers --json store::InventoryService::save evaluation/repositories/level1-store | jq '.[].caller_fqn'
```

The CLI and JSON output are **pre-1.0 and may change between minor releases** — pin a version if you depend on exact field names in automation.

## Coding Agent Usage

Prefer a targeted semantic query (`search`, `references`, `callers`, `callees`) over broad file reads when the question is about a symbol's declaration or relationships — it returns exactly the relevant locations instead of requiring the agent to reconstruct them from text search. Fall back to ordinary file inspection (`Read`, `Grep`) when the question isn't about a named symbol, or after a semantic query fails to resolve (see each command's `--help` for its disambiguation guidance).

This behavior is not guaranteed to be invoked automatically by every agent on every task — it depends on the agent's own routing. The `skills/semantic-analysis/SKILL.md` file in this repository is a ready-to-use Skill for Claude Code that documents the recommended command selection and recovery strategy in agent-consumable form.

## Supported Platforms / Languages

**Platforms:** Windows (64-bit) is the only platform this release has been built and tested on. Linux build prerequisites exist (`doc/build.md`) but Linux is not release-qualified. macOS is untested.

**Languages:** parsers and structural extraction (`outline`, `find`, `dump`, etc.) are available for Bash, C, C++, C#, CSS, Go, HTML, Java, JavaScript, Python, Ruby, Rust, Scala, TypeScript, and TSX. Symbol extraction and semantic relationship resolution (`symbols`, `search`, `references`, `callers`, `callees`) have dedicated regression coverage for C, C++, Python, Rust, Go, Java, and JavaScript; the remaining languages are supported by the same extraction machinery but have lighter test evidence and should be treated as provisional.

## Known Limitations

- No inferred receiver typing (`auto`, `decltype`, alias expansion) — only directly-typed object/pointer fields, locals, and parameters are resolved for `callers`/`callees`/`references` through a receiver expression.
- No inheritance-aware lookup, virtual dispatch expansion, function-pointer call resolution, or transitive (indirect) relationship traversal.
- No overload-sensitive member resolution — an ambiguous overload fails closed with a candidate list rather than guessing.
- A callee that itself has a separate declaration and out-of-line definition can, in a narrow case, be omitted from a `callees` result even when the calling function's own body was found correctly.
- Workspace, file, and cache paths must use characters representable in the active Windows code page; a path mixing scripts outside your system locale (e.g. a Western European accented character on a Japanese-locale system) is not currently supported.
- Local variables and parameters are not uniformly exposed as workspace symbols across all languages (though `callers`/`callees`/`references` can still resolve calls through a directly-typed local or parameter where the language extractor supports it).
- There is no `cmake --install` support yet; see Installation above.

## Documentation

- [Wiki: Getting Started](wiki/Getting-Started.md) — build, install, first commands
- [Wiki: CLI Reference](wiki/CLI.md) — full command reference, output conventions
- [Wiki: AI Integration](wiki/AI-Integration.md) — using ast-tool from Coding Agents
- [Wiki: Architecture](wiki/Architecture.md) — internal layer design, for contributors
- [Wiki: Extension Guide](wiki/Extension-Guide.md) — adding a language extractor or semantic service
- [Wiki: Contributing](wiki/Contributing.md) — coding style and contribution workflow

## License

MIT — see [LICENSE](LICENSE).
