# Contributing

## Design Philosophy

Understand and follow these principles before submitting changes.

**Strict layer isolation.** Each layer may only depend on layers below it. No file above `ast-ir.h` may include tree-sitter headers. No file above `ast-workspace.h` may call `parse()` or `extract_symbols()` directly. The CLI header `ast-tool.h` must not be included by library code.

**Parse once.** The workspace pipeline parses each file exactly once. Semantic services receive pre-built `TranslationUnit` objects. A service that re-parses files violates this principle.

**Semantic over syntactic.** When answering a semantic question (what does this identifier refer to?), use the scope tree and symbol table. Never fall back to text matching for semantic resolution.

**Extension without modification.** Adding a new language extractor, semantic service, or CLI command must not require changing existing implementations. Use the extension points described in the [Extension Guide](Extension-Guide).

---

## Coding Style

**Language standard:** C++23.

**Naming conventions:**

| Entity | Convention | Example |
|--------|-----------|---------|
| Types and classes | PascalCase | `SymbolKind`, `SemanticSearchEngine` |
| Member variables | `snake_case_` (trailing underscore) | `workspace_`, `source_file_` |
| Free functions | `snake_case` | `extract_symbols`, `build_search_query` |
| Enum values | PascalCase | `ScopeKind::Function` |
| Constants | `kPascalCase` | `kCommands`, `InvalidId` |
| Macros | `UPPER_SNAKE_CASE` | (avoid; prefer `constexpr`) |

**Comments:**

- Do not add comments that explain what the code does. Well-named identifiers already do that.
- Add a comment only when the *why* is non-obvious: a hidden constraint, a workaround for a specific library behavior, or an invariant that would surprise a future reader.
- One short line is the maximum for non-Doxygen comments.
- Public API types and functions have Doxygen comments in the header. Implementation files do not repeat them.

**No defensive programming beyond system boundaries.** Do not add `null` checks, range checks, or error handling for scenarios that the internal API guarantees cannot occur. Validate only at the boundary between the tool and the external world (user input, file I/O, external library return values).

**No speculative abstraction.** Do not add helper functions, base classes, or templates for hypothetical future use. Three concrete implementations are better than a premature abstraction. Refactor when the third concrete case arrives.

---

## Contribution Workflow

1. **Read the architecture.** Review the [Architecture](Architecture) page and the relevant headers before writing code. Understand which layer your change belongs in.

2. **Write tests first or alongside the implementation.** Tests live in `test/`. Each semantic service has its own subdirectory with a workspace of representative source files.

3. **Run the full test suite before submitting:**

   ```sh
   cmake --build build
   ./test/bin/ast-tool-test
   ```

   All tests must pass. Do not submit changes that break existing tests.

4. **Keep commits focused.** One logical change per commit. Do not mix refactoring with feature work.

5. **Update documentation.** If your change affects user-visible behavior (new command, new option, changed output format), update the relevant CLI help text in `src/help.cpp` and this Wiki.

---

## Testing Requirements

Every new feature must have tests. Every bug fix must have a regression test.

**Test structure:**

- Tests are compiled into `test/ast-tool-test`.
- Each subsystem has a directory under `test/` (e.g. `test/ast-references/`, `test/ast-callers/`).
- Workspace directories contain representative source files used as test fixtures.
- Test entry points are declared in a header and called from `test/main.cpp`.

**What to test:**

- The positive case: the feature works correctly for representative inputs.
- The boundary case: empty input, a single element, the maximum meaningful depth.
- The negative case: inputs that should produce no results or report an error.

**Do not mock.** Tests use real source files parsed by the real extractor. Mocked objects diverge from real behavior and have caused regressions. See the test directory for examples.

---

## Documentation Expectations

**CLI help text** (`src/help.cpp`): Update `kHelpXxx` strings when a command's synopsis, options, output format, or examples change. Keep descriptions accurate; inaccurate help text is worse than no help text.

**`SKILL.md`**: Update the skill guide when the intended usage patterns change, new commands appear, or previously-undocumented limitations become relevant to agents.

**This Wiki**: Update affected pages when architecture changes, new extension points are added, or the API surface changes. The Wiki is the authoritative documentation for developers.

**No generated documentation.** Do not commit generated Doxygen output. The headers are the source of truth.

---

## Adding a New Command

1. Add `SubCommand::Xxx` to the enum in `include/ast-tool.h`.
2. Add `ArgXxx` struct to `include/ast-tool.h`.
3. Add `xxx_` member to the `Arguments` union.
4. Add `parse_xxx()` in `src/ast-tool.cpp`.
5. Add a `case SubCommand::Xxx` in `dispatch()`.
6. Implement the command in `src/xxx.h` and `src/xxx.cpp`.
7. Add the help string `kHelpXxx` and a `CommandEntry` row in `src/help.cpp`.
8. Add the files to `CMakeLists.txt`.
9. Write tests.
10. Update the Wiki [CLI](CLI) page.
