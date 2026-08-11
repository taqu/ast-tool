# Extension Guide

## Adding a Language Extractor

Language extractors are the primary extension point. Each extractor translates an `AST` object (language-agnostic) into a flat list of `Symbol` objects (semantic).

### Files involved

| File | Role |
|------|------|
| `src/ast-extractor-<lang>.cpp` | New extractor implementation |
| `src/ast-extractor-langs.h` | Declaration of the extractor entry point |
| `src/ast-extractor.cpp` | Dispatch switch |
| `CMakeLists.txt` | Build system entry |

### Step 1 — Implement the extractor

Create `src/ast-extractor-<lang>.cpp`:

```cpp
#include "ast-extractor-langs.h"
#include "ast-ir.h"

namespace ast { namespace extractor {

std::vector<Symbol> extract_symbols_<lang>(const ast::AST& tree)
{
    std::vector<Symbol> symbols;
    // Traverse tree, build Symbol objects, push_back into symbols.
    return symbols;
}

}} // namespace ast::extractor
```

The function receives the already-parsed `AST`. It must not call `parse()` or access tree-sitter directly. Traverse `tree` using `tree[i]` and `tree.size()`; check `node.typeEquals("function_definition")` or `node.grammarEquals(...)` to identify nodes of interest.

For building fully-qualified names and tracking scope, see the C-family implementation in `src/ast-extractor-cfamily.cpp` as a reference.

### Step 2 — Declare the entry point

Add to `src/ast-extractor-langs.h`:

```cpp
std::vector<Symbol> extract_symbols_<lang>(const ast::AST& tree);
```

### Step 3 — Register in the dispatch switch

In `src/ast-extractor.cpp`, add a `case` to the `switch` on `ast.language()`:

```cpp
case ASTLanguage::<Lang>:
    return extractor::extract_symbols_<lang>(ast);
```

### Step 4 — Register the language

In `include/ast-ir.h`, add the language to the `ASTLanguage` enum and in `src/ast-ir.cpp` (or equivalent) register the file extension in `get_language_type()`.

### Step 5 — Add to the build

In `CMakeLists.txt`, add `"${SOURCE_ROOT}/ast-extractor-<lang>.cpp"` to the `SOURCES` list. Also add the corresponding tree-sitter grammar library to `TREESITTER_LANGS`.

### Step 6 — Write tests

Add a test directory under `test/ast-symbols/` (or a language-specific workspace directory) with representative source files. Register the test in `test/main.cpp`.

---

## Symbol Construction Guidelines

When implementing an extractor:

- Set `symbol.fqn` to the fully-qualified name using `::` as a separator (e.g. `Outer::Inner::method`).
- Set `symbol.name` to the unqualified (local) name only.
- Set `symbol.kind` to the most specific applicable `SymbolKind`.
- Set `symbol.line` and `symbol.column` to 0-based values (the `ASTNode::start_` fields).
- Set `symbol.nodeIndex` to the index of the declaration node within the `AST`.
- Do not extract local variables inside function bodies — only namespace- and class-scope declarations belong in the symbol table.
- When functions are overloaded, the FQN will be shared. Deduplication by FQN is performed by the extractor dispatcher; avoid adding duplicate entries yourself.

---

## Adding a Semantic Service

Semantic services operate on an already-built `Workspace` and never parse files. They belong above the workspace layer.

### Minimal structure

```cpp
// include/ast-<service>.h
#pragma once
#include "ast-workspace.h"

namespace ast {

struct <Service>Result { /* ... */ };

class <Service> {
public:
    explicit <Service>(const Workspace& workspace);
    <Service>Result find(const WorkspaceSymbol& target) const;
private:
    const Workspace& workspace_;
};

} // namespace ast
```

```cpp
// src/ast-<service>.cpp
#include "ast-<service>.h"
// Implement find() by iterating workspace_.translationUnits
// and/or workspace_.symbols.
```

Services that need identifier resolution should use `IdentifierResolver` from `ast-resolver.h`. Services that need the scope tree have it available through `TranslationUnit::scopeTree`.

### Registration

Add the header to `HEADERS` and the implementation to `SOURCES` in `CMakeLists.txt`. If the service should be accessible from the CLI, add a corresponding `ArgXxx` struct to `ast-tool.h`, a `SubCommand` entry, a parser in `src/ast-tool.cpp`, and a handler file.

---

## Architectural Constraints

Follow these rules to keep the layer model intact:

1. **Extractors** must not call `parse()` or include tree-sitter headers. They receive an `AST` and return `Symbol` objects.
2. **Semantic services** must not call `parse()`, `build_scope_tree()`, or `extract_symbols()`. They receive a `Workspace`.
3. **The CLI layer** (`ast-tool.h`) must not be included by anything other than `main.cpp`, `src/ast-tool.cpp`, and the subcommand handler files.
4. **`ast-ir.h`** must not include `ast-tool.h`.
5. New public headers belong in `include/`; implementation-internal headers belong in `src/`.
