# ast-tool

**ast-tool** is a command-line AST analysis toolkit for source code.
It parses source files using [tree-sitter](https://tree-sitter.github.io/), builds a language-agnostic intermediate representation, and exposes that representation through targeted commands for inspection, semantic analysis, and workspace-wide queries.

---

## Documentation

| Page | Description |
|------|-------------|
| [Getting Started](Getting-Started) | Build, install, and run your first commands |
| [CLI](CLI) | Command overview, help system, and output conventions |
| [Architecture](Architecture) | Layered design, core types, and design principles |
| [AI Integration](AI-Integration) | Using ast-tool from AI coding agents |
| [Extension Guide](Extension-Guide) | Adding language extractors and semantic services |
| [API Reference](API-Reference) | Public header organization and key types |
| [Contributing](Contributing) | Coding style, philosophy, and contribution workflow |

---

## Supported Languages

Bash · C · C++ · C# · CSS · Go · HTML · Java · JavaScript · Python · Ruby · Rust · Scala · TypeScript · TSX

---

## Quick Start

```sh
# Inspect the structure of a file
ast-tool outline src/main.cpp

# Extract all named symbols from a file
ast-tool symbols src/parser.cpp

# Find all function definitions
ast-tool find --type function_definition src/parser.cpp

# Search across a workspace
ast-tool search --kind class src/

# Find references, callers, and callees of a symbol
ast-tool references Parser::parse src/
ast-tool callers Parser::parse src/
ast-tool callees Parser::parse src/
```

Run `ast-tool --help` to see all commands, `ast-tool <command> --help` for detailed documentation, and `ast-tool --version` to check the installed version.
