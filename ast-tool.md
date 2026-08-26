# Task: Replace Eager Workspace AST Loading with Lazy Per-Path AST Loading

## Goal

Change the workspace analysis model so that ASTs are no longer parsed and loaded for the entire workspace before executing commands.

Current behavior:

    Workspace
        ↓
    Discover all files
        ↓
    Parse / build ASTs for all files
        ↓
    Keep all ASTs in memory
        ↓
    Execute commands such as find, search, references, callers, etc.

Target behavior:

    Workspace
        ↓
    Discover and register file paths
        ↓
    Do NOT parse all files eagerly
        ↓
    A command requests ASTs for specific paths
        ↓
    Parse and load ASTs lazily per path
        ↓
    Keep loaded ASTs in an in-memory cache
        ↓
    Reuse cached ASTs for subsequent commands

For this task, do NOT implement a database cache, persistent cache, daemon, or disk-based AST cache. The scope is limited to lazy AST loading and an in-memory per-path cache.

---

## Design Requirements

### 1. Workspace initialization must not parse the entire workspace

The workspace should still:

- discover source files
- register file paths
- determine language information if necessary
- build lightweight metadata required for later access

However, workspace initialization must not:

- parse every source file
- construct ASTs for every file
- eagerly populate the semantic layer for the entire workspace

The goal is to make workspace initialization lightweight.

---

### 2. Introduce lazy AST access by path

Provide a single path-based mechanism for obtaining an AST.

Conceptually:

    get_ast(path)

The implementation should:

    request AST for path
        ↓
    check in-memory cache

        cache hit
            ↓
        return existing AST

        cache miss
            ↓
        parse source file
            ↓
        construct AST / AST IR
            ↓
        store in memory cache
            ↓
        return AST

The command implementations should not need to know whether the AST was already loaded.

They should use the workspace API rather than parsing files directly.

---

### 3. Use an in-memory cache keyed by file path

Maintain an in-memory cache conceptually similar to:

    path
        →
    loaded AST / AST IR

For example:

    ast_cache[path] = parsed AST

Requirements:

- repeated requests for the same unchanged path should reuse the same in-memory AST
- parsing should happen at most once per path during a normal command sequence unless invalidation is required
- path normalization should be handled consistently
- avoid duplicate cache entries for equivalent paths

Do not implement persistent storage in this task.

---

### 4. Update commands to load ASTs on demand

Update commands such as:

- find
- search
- references
- callers
- callees
- symbols
- outline

so that they no longer depend on a fully preloaded workspace AST collection.

The desired pattern is:

    command
        ↓
    determine relevant file path(s)
        ↓
    workspace.get_ast(path)
        ↓
    AST is loaded from memory cache or parsed
        ↓
    execute command logic

Do not reintroduce an implicit "load all ASTs" operation inside individual commands.

---

### 5. Preserve existing architecture boundaries

Keep the existing architectural direction:

    Tree-sitter
        ↓
    AST IR
        ↓
    Semantic Layer
        ↓
    Workspace Analysis
        ↓
    Semantic Services
        ↓
    CLI / Agent / IDE

Tree-sitter should remain an implementation detail of parsing.

Higher-level command and semantic code should not directly manage Tree-sitter parser instances if the existing architecture already abstracts that responsibility.

The workspace should evolve from:

    Workspace = collection of all eagerly loaded ASTs

toward:

    Workspace = file registry + lazy AST provider + in-memory AST cache

---

## Important Scope Constraint

Do not attempt to solve the full semantic indexing problem in this change.

In particular, do NOT add:

- SQLite
- database-backed cache
- persistent AST storage
- serialized AST cache
- daemon/server mode
- JSON-RPC
- long-lived external processes
- a full workspace-wide precomputed symbol index

Those may be added later.

The purpose of this change is specifically to remove the eager:

    parse entire workspace

behavior and replace it with:

    parse ASTs only when a path actually needs analysis

---

## Handling Commands That Need Multiple Files

Some commands may require analysis of multiple files.

For those commands, it is acceptable to iterate over relevant paths:

    for path in candidate_paths:
        ast = workspace.get_ast(path)
        analyze(ast)

However, avoid blindly loading every file unless the command genuinely requires a workspace-wide scan.

If a command currently requires a full workspace scan because there is no lightweight way to narrow candidate files, preserve correctness first.

Do not introduce incorrect filtering merely to reduce the number of parsed files.

The primary improvement in this task is that:

- workspace startup does not parse everything
- AST parsing is demand-driven
- already loaded ASTs are reused

Further candidate indexing and persistent semantic indexes can be addressed later.

---

## Invalidation / File Changes

Implement only the minimum invalidation behavior necessary for correctness.

At minimum, avoid returning a clearly stale AST if the underlying source file changes during the same process lifetime.

If the current architecture already has file modification tracking, integrate with it.

Do not build a complex dependency or incremental invalidation system in this task.

A simple and maintainable per-file invalidation strategy is sufficient.

---

## Suggested Internal Structure

The exact names should follow the existing codebase conventions, but the design should be approximately:

    Workspace
    ├── file registry
    │     └── known source paths
    │
    ├── AST cache
    │     └── path -> AST / AST IR
    │
    └── get_ast(path)
          ├── normalize path
          ├── check cache
          ├── validate cache entry if necessary
          ├── parse on cache miss
          ├── store result
          └── return AST

Do not expose the cache implementation unnecessarily to higher layers.

---

## Performance Instrumentation

Add lightweight timing or counters where useful so we can verify the effect of this change.

At minimum, make it possible to distinguish:

- workspace initialization time
- number of discovered files
- number of files parsed
- AST cache hits
- AST cache misses
- command execution time

The expected behavior should be:

First command:

    workspace initialization
        → fast

    command
        → parse only required paths

Second command involving the same paths:

    AST cache hit
        → no reparsing

This instrumentation should be lightweight and should not significantly complicate the public CLI output.

---

## Tests

Add or update tests covering the following behavior.

### Test 1: Workspace initialization is lazy

Creating or opening a workspace should discover files without parsing every file.

Verify that the parse count is zero, or equivalent, immediately after workspace initialization.

### Test 2: First AST access parses the file

Requesting an AST for a path should parse that file and store it in the in-memory cache.

### Test 3: Repeated AST access uses the cache

Requesting the same AST twice should not parse the file twice.

Expected behavior:

    get_ast(foo.cpp)
        → parse count +1

    get_ast(foo.cpp)
        → parse count unchanged

### Test 4: Different paths are loaded independently

Accessing:

    foo.cpp
    bar.cpp

should parse only those two files.

Other workspace files should remain unloaded.

### Test 5: Existing commands still work

Run existing tests for commands including, where applicable:

- find
- search
- references
- callers
- callees
- symbols
- outline

Ensure that their functional behavior remains correct after removing eager workspace AST loading.

---

## Acceptance Criteria

The implementation is complete when:

1. Opening a workspace does not parse the entire workspace.
2. ASTs are loaded lazily by file path.
3. Loaded ASTs are cached in memory.
4. Repeated access to the same unchanged file reuses the cached AST.
5. Commands can operate without assuming that all workspace ASTs already exist in memory.
6. Existing command behavior remains correct.
7. Tests demonstrate lazy loading and cache reuse.
8. No database or persistent cache is introduced.
9. Performance counters or equivalent instrumentation can confirm how many files were actually parsed.

---

## Final Deliverables

After implementation, provide:

1. A summary of the architectural changes.
2. The main files and components modified.
3. A description of the new AST loading flow.
4. Test results.
5. Any commands that still require scanning many or all workspace paths, with an explanation of why.

Focus on a minimal, clean architectural change. Do not over-engineer the cache or add persistent storage at this stage.