# API Reference

This page describes the public header organization and the role of each header. It is an overview, not an exhaustive function reference. Detailed documentation lives in the headers themselves as Doxygen comments.

---

## Header Map

```
include/
├── ast-ir.h                 Core AST types and parse entry point
├── ast-tool.h               CLI argument types (depends on ast-ir.h)
├── ast-extractor.h          Symbol extraction API
├── ast-scope.h              Lexical scope model
├── ast-scope-builder.h      Scope tree construction
├── ast-scope-visibility.h   Visible-symbol computation
├── ast-lookup.h             Scope-chain name lookup
├── ast-resolver.h           Identifier → symbol resolution
├── ast-workspace.h          Workspace construction and TranslationUnit
├── ast-search.h             Semantic symbol search engine
├── ast-references.h         Workspace-wide reference discovery
├── ast-callers.h            Workspace-wide caller discovery
├── ast-callees.h            Workspace-wide callee discovery
├── ast-semantic-diff.h      Semantic comparison of two workspaces
├── ast-context-export.h     Context extraction for AI/IDE consumers
├── ast-regex.h              RE2 regex wrapper
└── helper.h                 Internal utility macros (not for external use)
```

Include only the headers you need. Lower-level headers (e.g. `ast-ir.h`) are re-included transitively through higher-level ones, but prefer explicit includes.

---

## ast-ir.h — Core AST

**Primary entry point:**

```cpp
ast::AST parse(const char* path);
```

Loads `path`, detects its language from the file extension, parses it with tree-sitter, and returns a fully constructed, id-remapped `AST`. Check `operator bool()` before using the result.

**Key types:**

| Type | Description |
|------|-------------|
| `ASTLanguage` | Enum of supported languages |
| `ASTFlag` | Bitmask: Named, Missing, Extra, Error |
| `ASTPoint` | Zero-based `{row_, column_}` |
| `ASTText` | Non-owning source text slice |
| `ASTNode` | A single flattened AST node |
| `AST` | Owns source text and the node vector |

**`ASTNode` key fields:**

| Field | Type | Description |
|-------|------|-------------|
| `id_` | `uintptr_t` | Array index (after `remap_ids()`) |
| `parent_` | `uintptr_t` | Parent index, or `InvalidId` |
| `children_` | `vector<uintptr_t>` | Child indices |
| `hash_` | `uint32_t` | Stable 32-bit node hash |
| `type_` | `const char*` | tree-sitter type name |
| `grammar_type_` | `const char*` | tree-sitter grammar type name |
| `text_` | `ASTText` | Source text slice |
| `start_` | `ASTPoint` | Start position (0-based) |
| `flags_` | `ASTFlag` | Node flags |

**Sentinel:** `ast::InvalidId = uintptr_t(-1)` — returned when no node exists.

---

## ast-extractor.h — Symbol Extraction

```cpp
std::vector<ast::Symbol> ast::extract_symbols(const ast::AST& ast);
```

Dispatches to the language-specific extractor and returns all extracted symbols.

**`Symbol` key fields:**

| Field | Type | Description |
|-------|------|-------------|
| `name` | `string` | Unqualified name |
| `fqn` | `string` | Fully-qualified name |
| `kind` | `SymbolKind` | Declaration category |
| `access` | `Access` | Access specifier |
| `isStatic` | `bool` | Storage modifier |
| `isConstexpr` | `bool` | Storage modifier |
| `isInline` | `bool` | Linkage modifier |
| `nodeIndex` | `size_t` | Index into owning AST |
| `line` | `uint32_t` | 0-based declaration line |
| `column` | `uint32_t` | 0-based declaration column |

**`SymbolKind` values:** Namespace, Class, Struct, Union, Enum, Function, Method, Constructor, Destructor, Variable, Field, EnumValue, Macro, Typedef, UsingAlias

---

## ast-scope.h — Lexical Scopes

```cpp
class ast::ScopeTree;
struct ast::Scope;
enum class ast::ScopeKind;
```

`ScopeTree` owns all `Scope` objects for one translation unit. Scopes are indexed by integer ID (`uintptr_t`). `ScopeTree::getNodeScope(nodeIndex)` and `getScopeOfSymbol(symbolIndex)` provide O(1) lookup after the tree is built.

**`ScopeKind` values:** Global, Namespace, Module, Class, Struct, Enum, Function, Method, Lambda, Block, Conditional, Loop, Switch

---

## ast-workspace.h — Workspace

```cpp
ast::Workspace ast::analyze_workspace(const char* root);
ast::Workspace ast::analyze_files(const std::vector<std::string>& files);
std::vector<std::string> ast::scan_workspace(const char* root);
```

**`Workspace` fields:**

| Field | Type | Description |
|-------|------|-------------|
| `translationUnits` | `vector<TranslationUnit>` | Primary storage, one per file |
| `symbols` | `vector<WorkspaceSymbol>` | Flat cross-file symbol index |
| `deps` | `vector<FileDependencies>` | Per-file include/import graph |
| `files` | `vector<string>` | Sorted list of discovered files |
| `parsedCount` | `uint32_t` | Successfully parsed files |
| `failedCount` | `uint32_t` | Files that failed to parse |

**`TranslationUnit` fields:** `ast`, `scopeTree`, `symbols`, `path`

**`WorkspaceSymbol` fields:** `symbol` (a `Symbol`), `sourceFile`, `owningScope`

---

## ast-search.h — Search

```cpp
std::expected<ast::SearchQuery, std::string>
ast::build_search_query(name, fqn, kind_str, file,
                        name_regex, fqn_regex, file_regex);

class ast::SemanticSearchEngine {
    explicit SemanticSearchEngine(const Workspace&);
    std::vector<const WorkspaceSymbol*> search(const SearchQuery&) const;
};
```

Pass `nullptr` for any unused filter. `build_search_query` compiles RE2 patterns once and validates the query. `SemanticSearchEngine::search` applies filters cheapest-first and returns non-owning pointers into the workspace.

---

## ast-references.h — Find References

```cpp
class ast::FindReferences {
    explicit FindReferences(const Workspace&);
    std::vector<ReferenceResult> find(const WorkspaceSymbol& target,
                                      bool includeDeclaration = false) const;
};
```

`ReferenceResult` fields: `referencedSymbol`, `sourceFile`, `line` (0-based), `column` (0-based), `owningScope`, `nodeIndex`.

---

## ast-callers.h / ast-callees.h — Call Graph

```cpp
class ast::Callers {
    explicit Callers(const Workspace&);
    std::vector<CallSite> find(const WorkspaceSymbol& target) const;
};

class ast::Callees {
    explicit Callees(const Workspace&);
    std::vector<CallSite> find(const WorkspaceSymbol& target) const;
};
```

`CallSite` fields: `caller`, `callee` (pointers into workspace), `sourceFile`, `line` (0-based), `column` (0-based), `nodeIndex`.

---

## ast-semantic-diff.h — Semantic Diff

```cpp
class ast::SemanticDiff {
    SemanticDiffResult compare(const Workspace& oldWs,
                               const Workspace& newWs) const;
};
```

`DiffEntry` fields: `kind` (`Added`, `Removed`, `Modified`), `before`, `after` (pointers into respective workspaces).

---

## ast-tool.h — CLI Layer

Includes `ast-ir.h`. Contains `SubCommand` enum, `ArgXxx` structs, the `Arguments` discriminated union, `parse()`, and `dispatch()`. Include this header only in CLI-related code.

---

## Coordinate Conventions

| Source | Convention |
|--------|-----------|
| `ASTNode::start_.row_` | 0-based |
| `ASTNode::start_.column_` | 0-based |
| `Symbol::line` | 0-based |
| `Symbol::column` | 0-based |
| `ReferenceResult::line` | 0-based |
| `CallSite::line` | 0-based |
| CLI output (`find`, `range`, etc.) | 1-based |

CLI commands add 1 when printing; library consumers work with 0-based values.
