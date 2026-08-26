# Task: Implement Persistent Binary AST Cache Using SQLite and LZ4

## Goal

Implement a persistent AST cache using SQLite and LZ4 compression.

The AST representation has been prepared for binary serialization by replacing raw node type string pointers with compact enum identifiers.

Use this representation to persist parsed AST data across process executions.

The desired architecture is:

    Source File
        ↓
    Check AST Cache
        ↓
    Cache hit
        → load binary AST
        → LZ4 decompress
        → deserialize
        → use AST

    Cache miss
        ↓
    Parse source with Tree-sitter
        ↓
    Build AST / AST IR
        ↓
    Serialize AST to binary
        ↓
    LZ4 compress
        ↓
    Store in SQLite
        ↓
    Use AST

The persistent cache should be used transparently by the workspace and lazy AST loading system.

---

# Scope

Implement:

- binary serialization of AST data
- binary deserialization of AST data
- LZ4 compression and decompression
- SQLite-backed persistent AST storage
- per-file cache lookup
- cache invalidation when source files change
- integration with lazy per-path AST loading

Do not implement:

- a daemon
- a server mode
- JSON-RPC
- a distributed cache
- workspace-wide preloading
- advanced dependency invalidation
- incremental AST updates
- a full semantic database

The cache should operate at the individual source file level.

---

# Target Architecture

The workspace should behave approximately as follows:

    Workspace
    │
    ├── File Registry
    │       known source paths
    │
    ├── Memory AST Cache
    │       path → loaded AST
    │
    └── Persistent AST Cache
            SQLite database
                    ↓
                compressed binary AST

When a command requires an AST:

    get_ast(path)

        ↓

    1. Check memory cache

        hit
            ↓
        return AST

        miss
            ↓

    2. Check SQLite cache

        valid entry found
            ↓
        read blob
            ↓
        LZ4 decompress
            ↓
        deserialize AST
            ↓
        insert into memory cache
            ↓
        return AST

        cache miss / invalid
            ↓

    3. Parse source

        Tree-sitter
            ↓
        AST extraction
            ↓
        AST IR
            ↓
        serialize
            ↓
        LZ4 compress
            ↓
        store in SQLite
            ↓
        insert into memory cache
            ↓
        return AST

The command layer should not need to know whether the AST came from:

- memory
- SQLite cache
- fresh parsing

The existing workspace API should remain the abstraction boundary.

---

# SQLite Database Location

Store the cache in a workspace-local directory.

Preferred layout:

    <workspace>/
    └── .ast-tool/
        └── ast-cache.db

The `.ast-tool` directory should be created automatically if necessary.

The cache database should not be required to be committed to source control.

If the project already has a workspace cache or metadata directory, follow the existing convention instead.

---

# Cache Key and Validation

The cache must determine whether a stored AST still corresponds to the current source file.

Each cache entry should contain at least:

    normalized_path
    source content hash
    source size
    source modification time
    language
    AST format version
    compressed AST blob

Recommended lookup flow:

    get_ast(path)

        ↓

    normalize path

        ↓

    obtain current file metadata

        ↓

    lookup SQLite entry

        ↓

    metadata matches?

        yes
            ↓
        cache hit

        no
            ↓
        reparse source
        replace cache entry

Do not rely only on file modification time if that could cause stale cache entries.

A content hash should be the authoritative validation mechanism.

For performance, metadata such as:

    mtime
    size

may be used as a fast path to avoid recomputing the content hash when the file clearly has not changed.

The exact validation strategy should prioritize correctness first.

---

# Database Schema

Use a simple schema focused on per-file AST caching.

A starting point could be:

```sql id="kmz8kv"
CREATE TABLE IF NOT EXISTS ast_cache (
    path TEXT PRIMARY KEY,

    source_hash BLOB NOT NULL,
    source_size INTEGER NOT NULL,
    source_mtime INTEGER NOT NULL,

    language INTEGER NOT NULL,

    format_version INTEGER NOT NULL,

    uncompressed_size INTEGER NOT NULL,
    compressed_size INTEGER NOT NULL,

    ast_blob BLOB NOT NULL
);
````

The exact schema may be adjusted to match existing project types.

Keep the schema simple.

Do not normalize every AST node into relational database rows.

The AST should be stored as one binary blob per source file.

SQLite is used for:

* persistence
* cache lookup
* metadata
* invalidation
* atomic replacement of cached file entries

It is not intended to become a node-by-node AST query engine.

---

# Binary Serialization Requirements

Implement explicit binary serialization.

Do not serialize raw C++ object memory directly unless every serialized field is explicitly guaranteed to be portable and stable.

Do not write the entire `ASTNode` structure with:

```cpp id="ghkmd2"
file.write(
    reinterpret_cast<const char*>(&node),
    sizeof(ASTNode));
```

unless the implementation first proves that every field is safe and the format is explicitly defined.

Instead, define a binary format.

For example:

```text id="6etf7d"
AST File Header

magic
format_version
language
node_count
...
```

followed by serialized nodes.

Conceptually:

```text id="nx1q09"
Header
    ↓
Node 0
Node 1
Node 2
...
```

Each field should be written explicitly in a defined order.

Use fixed-width integer types where possible:

```
uint8_t
uint16_t
uint32_t
uint64_t
```

Do not serialize:

* pointers
* `const char*`
* process-local addresses
* `uintptr_t` values that are not stable AST indices

Before serialization, ensure that AST node identifiers are in their stable remapped/index form.

If the AST currently uses:

```text id="tdhpr2"
before remap_ids()
    id_ = Tree-sitter node identifier

after remap_ids()
    id_ = AST-local index
```

then only serialize the AST after the stable index representation has been established.

Do not serialize Tree-sitter node IDs.

---

# AST Format Versioning

The cache format must include an explicit version.

For example:

```cpp id="s6klce"
constexpr uint32_t kAstCacheFormatVersion = 1;
```

Every cached AST entry must store the format version.

If the version does not match:

```
current code format
    !=
cached format
```

then treat the cache entry as invalid.

The implementation should:

```
cache version mismatch
    ↓
ignore old cache entry
    ↓
reparse source
    ↓
write new cache entry
```

Do not attempt automatic migration in this task.

Version mismatch should simply invalidate the cached entry.

---

# LZ4 Compression

Use LZ4 to compress the serialized AST blob before storing it in SQLite.

The flow should be:

```text id="zbs5tt"
AST
    ↓
binary serialization
    ↓
uncompressed byte buffer
    ↓
LZ4 compression
    ↓
compressed byte buffer
    ↓
SQLite BLOB
```

On load:

```text id="wttphx"
SQLite BLOB
    ↓
LZ4 decompression
    ↓
binary byte buffer
    ↓
AST deserialization
    ↓
AST
```

Store enough metadata to know the expected decompressed size.

At minimum:

```
uncompressed_size
compressed_size
```

Use the existing LZ4 integration available in the project.

Handle compression failures safely.

Do not store corrupted or partially serialized cache entries.

---

# Compression Policy

LZ4 compression may not always reduce the size of very small ASTs.

Implement a simple policy:

```
serialize AST

    ↓

attempt LZ4 compression

    ↓

compressed data is smaller?

    yes
        ↓
    store compressed

    no
        ↓
    store uncompressed
```

If both compressed and uncompressed entries are supported, store an explicit compression mode.

For example:

```text id="c2s1pk"
compression = None
compression = LZ4
```

The cache loader must use the stored mode rather than assuming all blobs are compressed.

---

# Serialization Scope

Serialize all information required to reconstruct a usable AST / AST IR without Tree-sitter.

After deserialization, the AST should be usable by existing higher-level logic without reparsing the source file.

At minimum, inspect the AST and related structures to determine all required data.

Potential data may include:

* AST nodes
* node IDs
* parent IDs
* hashes
* flags
* node type enum
* grammar type enum
* source text or text references
* child relationships, if stored separately
* AST-level metadata
* language information

Do not serialize data that can be cheaply reconstructed unless necessary.

The goal is:

```
deserialize
    ↓
obtain a fully usable AST IR
```

without requiring Tree-sitter.

---

# ASTText Handling

Inspect `ASTText` carefully before implementing serialization.

Do not assume it can be serialized by copying its in-memory representation.

Determine whether `ASTText` contains:

* pointers
* views
* references into source buffers
* offsets
* owned strings

The serialized representation must not depend on process-local pointers.

If `ASTText` currently references source memory, serialize either:

1. stable byte offsets into the source file, or
2. owned text data,

depending on the existing AST design.

Prefer a compact representation.

If source text offsets are sufficient to reconstruct `ASTText`, avoid duplicating large source strings inside every AST node.

Document the chosen approach.

---

# Path Handling

Use a consistent normalized path representation for SQLite keys.

Requirements:

* the same file must not produce multiple cache entries because of equivalent paths
* relative versus absolute path handling must be consistent
* paths should be interpreted relative to the workspace where appropriate

Prefer a stable workspace-relative path as the database key if compatible with the existing workspace model.

For example:

```text id="vzk2ky"
src/main.cpp
include/foo.h
```

rather than machine-specific absolute paths.

---

# SQLite Access Layer

Keep SQLite details isolated.

Introduce a dedicated component conceptually similar to:

```text id="1aqzwg"
ASTCacheDatabase
```

or follow existing project naming conventions.

Responsibilities:

```text id="k39i2y"
open database
initialize schema

lookup(path)

store(path, metadata, blob)

remove(path)

clear invalid entries
```

The rest of the AST/workspace code should not contain raw SQL statements.

The workspace should interact with a cache abstraction.

For example:

```text id="xv2m1s"
Workspace
    ↓
PersistentASTCache
    ↓
SQLite
```

rather than:

```text id="ptsh7p"
Workspace
    ↓
direct SQL queries everywhere
```

---

# Memory Cache Integration

The persistent cache should work together with the existing lazy in-memory AST cache.

The lookup order must be:

```text id="n49l6q"
1. Memory AST cache

2. Persistent SQLite AST cache

3. Parse source
```

Do not bypass the memory cache when a database cache exists.

The expected behavior is:

First access in a process:

```text id="3s7qay"
Memory miss
    ↓
SQLite hit
    ↓
deserialize
    ↓
Memory cache insert
```

Second access:

```text id="qczcf7"
Memory hit
    ↓
return immediately
```

First access after a source change:

```text id="t8j8ay"
Memory miss / invalid
    ↓
SQLite entry invalid
    ↓
parse
    ↓
serialize
    ↓
compress
    ↓
replace SQLite entry
    ↓
Memory cache insert
```

---

# Error Handling and Cache Corruption

The cache must never make normal AST analysis fail permanently.

If any of the following occur:

* SQLite read error
* missing entry
* corrupted blob
* invalid format
* decompression failure
* deserialization failure
* invalid node count
* invalid parent index
* version mismatch

then:

```text id="vxovhy"
treat cache entry as invalid
    ↓
discard / remove entry if appropriate
    ↓
parse source normally
    ↓
replace cache entry
```

A corrupted cache must be recoverable automatically.

Do not require manual cache deletion for normal recovery.

---

# Validation of Deserialized Data

Do not blindly trust cached binary data.

Perform lightweight validation during deserialization.

At minimum validate:

* header magic
* format version
* buffer boundaries
* node count limits
* enum values
* node ID ranges
* parent ID ranges
* required AST metadata

The deserializer must not read beyond the available buffer.

Malformed cache data should fail cleanly.

---

# Atomicity

Cache writes should be safe against interruption.

Avoid leaving a partially written cache entry.

Use SQLite transactions or equivalent atomic replacement behavior.

The expected behavior is:

```text id="amwj7d"
BEGIN

serialize
compress
replace cache entry

COMMIT
```

If writing fails, the old valid cache entry should remain usable when possible.

---

# Concurrency

Inspect whether multiple `ast-tool` processes may access the same workspace.

The implementation should not corrupt the SQLite database if multiple CLI invocations occur.

Use SQLite's normal transactional behavior.

Do not introduce complex custom locking unless required.

If necessary, configure SQLite appropriately for concurrent readers and safe writers.

Keep this implementation simple.

---

# Performance Instrumentation

Add lightweight cache statistics.

At minimum, track:

```text id="wwyyqk"
memory_cache_hits
memory_cache_misses

persistent_cache_hits
persistent_cache_misses

files_parsed

bytes_serialized
bytes_compressed

serialization_time
compression_time

database_load_time
database_store_time

decompression_time
deserialization_time
```

These values may be:

* internal counters
* debug logging
* optional profiling output

Do not make them noisy in normal CLI output.

The purpose is to measure whether the persistent cache actually improves performance.

---

# Tests

Add tests for the following.

## Test 1: Serialize and Deserialize Round Trip

```text id="u4ph5s"
Source
    ↓
Parse
    ↓
AST
    ↓
Serialize
    ↓
Deserialize
    ↓
AST
```

Verify that the reconstructed AST is semantically equivalent to the original.

Check at least:

* node count
* node IDs
* parent relationships
* hashes
* flags
* node types
* grammar types
* text representation
* AST-level metadata

---

## Test 2: LZ4 Compression Round Trip

Verify:

```text id="wgc4k9"
serialized bytes
    ↓
compress
    ↓
decompress
    ↓
original serialized bytes
```

The final byte buffer must match the original.

---

## Test 3: SQLite Cache Hit

````text id="0wfjda"
First process:

    source
        ↓
    parse
        ↓
    store cache

Second process:

    same source
        ↓
    SQLite hit
        ↓
    deserialize

Verify that the second access does not invoke Tree-sitter parsing.

---

## Test 4: Source Change Invalidates Cache

```text id="7h2uub"
parse source version A
    ↓
store cache

modify source

request AST
    ↓
old cache invalid
    ↓
reparse source version B
````

Verify that the stale cached AST is not returned.

---

## Test 5: Cache Version Change

Create or simulate an entry with an old format version.

Verify:

```text id="go0tng"
version mismatch
    ↓
ignore cache
    ↓
reparse
```

---

## Test 6: Corrupted Blob Recovery

Store intentionally corrupted cache data.

Verify:

```text id="laxh20"
load cache
    ↓
failure
    ↓
parse source normally
    ↓
replace invalid cache entry
```

The command should still succeed.

---

## Test 7: Memory Cache Takes Priority

Within one process:

```text id="gkegpz"
get_ast(foo.cpp)
    ↓
SQLite load

get_ast(foo.cpp)
    ↓
Memory cache hit
```

Verify that the second access performs:

* no SQLite read
* no decompression
* no deserialization
* no Tree-sitter parse

---

## Test 8: Existing Semantic Commands

Run existing tests for:

* find
* search
* references
* callers
* callees
* symbols
* outline

Verify that the deserialized AST works identically to a freshly parsed AST.

---

# Implementation Order

Use the following order.

## Step 1

Inspect the complete AST and ASTText ownership model.

Identify all fields required to reconstruct a usable AST.

Do not start serialization until pointer/reference fields are understood.

---

## Step 2

Define the binary format.

Add:

* magic value
* format version
* fixed-width fields
* bounds validation

---

## Step 3

Implement in-memory binary serialization and deserialization.

Add round-trip tests before involving SQLite or LZ4.

---

## Step 4

Add LZ4 compression and decompression.

Verify binary round trips.

---

## Step 5

Implement the SQLite cache layer.

Keep SQL isolated behind a dedicated cache component.

---

## Step 6

Integrate with:

```
Workspace::get_ast(path)
```

using:

```text id="c53itf"
memory
    ↓ miss
SQLite
    ↓ miss
parse
```

---

## Step 7

Add invalidation and corruption recovery.

---

## Step 8

Add instrumentation and measure:

* cold parse
* SQLite cache load
* memory cache hit

Use representative repositories.

---

# Acceptance Criteria

The implementation is complete when:

1. ASTs can be serialized into an explicit binary format.

2. The binary format contains no process-local pointers.

3. AST node type and grammar type identifiers are serialized as stable enum values.

4. Serialized ASTs can be LZ4 compressed.

5. ASTs can be stored and loaded as SQLite BLOBs.

6. The cache is keyed per source file.

7. Source changes invalidate stale entries.

8. Cache format changes invalidate incompatible entries.

9. The lookup order is:

   ```
   memory
       ↓
   SQLite persistent cache
       ↓
   source parsing
   ```

10. A cached AST can be deserialized and used by existing semantic operations without Tree-sitter parsing.

11. Corrupted cache data automatically falls back to normal parsing.

12. Existing semantic command tests continue to pass.

13. Performance counters make it possible to compare:

    cold parse
    persistent cache load
    memory cache hit

14. No database cache is required for workspace initialization.

15. The implementation remains per-file and lazy; do not preload or deserialize the entire workspace.

---

# Final Deliverables

After implementation, provide:

1. A summary of the binary AST format.

2. The format version and invalidation strategy.

3. The SQLite schema.

4. The LZ4 compression strategy.

5. How `ASTText` is represented in the serialized format.

6. The cache lookup flow.

7. Performance measurements for:

   ```
   cold parse
   SQLite cache load
   memory cache hit
   ```

8. Test results.

9. Any remaining fields that cannot currently be serialized safely and why.

Keep the implementation focused on a robust per-file persistent AST cache. The primary objective is to avoid reparsing unchanged source files across separate `ast-tool` process executions while preserving the existing lazy per-path AST loading architecture.

