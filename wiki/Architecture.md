# Architecture

## Layer Diagram

```
Source file
    │
    ▼  tree-sitter (parsing only)
AST IR          ← ast-ir.h
    │
    ▼  language-specific extractors
Semantic Layer  ← ast-extractor.h, ast-scope.h, ast-scope-builder.h
    │
    ▼  workspace construction
Workspace       ← ast-workspace.h
    │
    ▼  semantic services
Services        ← ast-search.h, ast-references.h, ast-callers.h,
                  ast-callees.h, ast-semantic-diff.h, ast-context-export.h
    │
    ▼
CLI / IDE / AI  ← ast-tool.h
```

Each layer depends only on the layer below it. Tree-sitter is an implementation detail of the AST IR layer; no code above that layer includes tree-sitter headers.

---

## AST IR (`ast-ir.h`)

The AST Intermediate Representation is a language-agnostic, flattened snapshot of the tree-sitter parse tree. It is the stable foundation that all higher layers build on.

### Key types

**`ASTLanguage`** — Enumeration of supported languages, detected from file extension.

**`ASTFlag`** — Bitmask describing a node: `Named`, `Missing`, `Extra`, `Error`.

**`ASTPoint`** — Zero-based `{row_, column_}` position.

**`ASTText`** — Non-owning slice of the source text buffer.

**`ASTNode`** — A single flattened node:
- `id_` — internal index (after `remap_ids()`)
- `parent_` — index of the parent node
- `children_` — indices of child nodes
- `hash_` — 32-bit XXH32 hash derived from file path, node type, and byte range; used as the stable node ID exposed to users
- `type_` / `grammar_type_` — tree-sitter type strings
- `text_` — source text slice
- `start_` / `end_` — byte offsets and `ASTPoint` positions
- `flags_` — `ASTFlag` bitmask

**`AST`** — Owns the source text and the flat node vector. Constructed by `parse(path)`, which loads the file, determines its language, parses it with tree-sitter, and calls `remap_ids()` to convert tree-sitter pointer IDs to stable array indices.

### Parser independence

Tree-sitter is called only inside the `parse()` function and the low-level `add()` helper. Once an `AST` object exists, all consumers interact with `ASTNode` values and never touch tree-sitter APIs. This isolates parser upgrades to a single implementation file.

---

## Semantic Layer

### Extractor (`ast-extractor.h`)

`extract_symbols(ast)` dispatches to a language-specific implementation based on `ast.language()`. Each implementation traverses the AST IR and produces a flat `std::vector<Symbol>`.

**`Symbol`** carries:
- `name` — unqualified name
- `fqn` — fully-qualified name (e.g. `ast::Parser::parse`)
- `kind` — `SymbolKind` enum (Namespace, Class, Struct, Union, Enum, Function, Method, Constructor, Destructor, Variable, Field, EnumValue, Macro, Typedef, UsingAlias)
- `access` — `Access` enum (Public, Protected, Private, Unknown)
- `isStatic`, `isConstexpr`, `isInline` — storage/linkage qualifiers
- `nodeIndex` — index into the owning `AST`
- `line`, `column` — 0-based declaration position

### Scope tree (`ast-scope.h`, `ast-scope-builder.h`)

`ScopeTree` models the lexical scope hierarchy for a single translation unit as a flat vector of `Scope` objects. Each `Scope` records its kind, parent, children, byte range, and the symbols declared within it.

`ScopeKind` covers: Global, Namespace, Module, Class, Struct, Enum, Function, Method, Lambda, Block, Conditional, Loop, Switch.

`build_scope_tree(ast)` constructs the scope tree from the AST IR; `associate_symbols(scopeTree, symbols)` populates the reverse maps between scopes and symbols.

### Symbol resolution (`ast-resolver.h`)

`IdentifierResolver` resolves an identifier node to the `WorkspaceSymbol` it declares, using the scope tree and the workspace symbol table. Resolution is semantic, not textual: it walks the scope chain outward from the identifier's enclosing scope.

---

## Workspace (`ast-workspace.h`)

A `Workspace` aggregates the results of parsing and analyzing an entire directory tree.

### Pipeline

```
scan_workspace(root)
    → list of source file paths
analyze_files(files)
    → for each file:
        parse → build_scope_tree → extract_symbols → associate_symbols
        → collect include/import paths
        → store TranslationUnit
    → merge into Workspace
```

**`TranslationUnit`** owns the `AST`, `ScopeTree`, and `std::vector<Symbol>` for one file.

**`Workspace`** provides:
- `translationUnits` — primary ownership, one per file
- `symbols` — flat `WorkspaceSymbol` index across all files (for O(N) cross-file queries)
- `deps` — per-file direct include/import graph
- `files` — sorted list of all discovered source files

Each file is parsed exactly once. Semantic services receive the already-built `TranslationUnit` objects and never re-parse.

---

## Semantic Services

Services operate exclusively on the `Workspace` that was built by the workspace pipeline. They never call `parse()` or access tree-sitter.

### SemanticSearchEngine (`ast-search.h`)

Filters `WorkspaceSymbol` objects by a `SearchQuery`. Filters are applied cheapest-first: exact comparisons (kind, name, fqn, file) before regex (name_regex, fqn_regex, file_regex). A `SearchQuery` is constructed and validated by `build_search_query()`, which compiles RE2 patterns exactly once.

### FindReferences (`ast-references.h`)

Locates every use-site of a given symbol across the workspace by traversing identifier nodes in each `TranslationUnit` and resolving them via `IdentifierResolver`. Matching is semantic, not textual. The declaration site is excluded by default.

### Callers / Callees (`ast-callers.h`, `ast-callees.h`)

`Callers` finds every function that directly calls a target function; `Callees` finds every function directly called by a target function. Both operate by traversing call-expression nodes and resolving identifiers. Indirect calls (function pointers, virtual dispatch) are not reported.

### SemanticDiff (`ast-semantic-diff.h`)

Compares two `Workspace` snapshots and reports Added, Removed, and Modified symbols. Symbols are matched by (fqn, kind). Source locations, comments, and whitespace are ignored.

---

## CLI Layer (`ast-tool.h`)

The CLI layer is intentionally thin. `parse(arguments, argc, argv)` fills an `Arguments` discriminated union; `dispatch(arguments)` calls the appropriate subcommand implementation.

New code that only needs the AST IR or semantic services should include the relevant header directly and omit `ast-tool.h`.

---

## Design Principles

**Strict layer isolation.** Every header includes only what it needs. No layer above AST IR touches tree-sitter. CLI types live exclusively in `ast-tool.h`.

**Parse once.** The workspace pipeline ensures each file is parsed exactly once. Semantic services consume the pre-built `TranslationUnit` objects.

**Semantic over syntactic.** Reference finding, call-graph analysis, and identifier resolution are semantic operations that use the scope tree and symbol table. Text matching is never used for semantic questions.

**Extension without modification.** Adding a new language extractor requires only a new `.cpp` file, a declaration in `ast-extractor-langs.h`, and one `case` in the dispatch switch. No existing extractor changes.
