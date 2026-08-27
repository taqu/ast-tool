# Task: Parallelize `warm_cache` Using the Existing BlockingQueue Pattern

## Goal

Refactor and parallelize the workspace AST cache warming implementation.

The project already contains a reusable bounded producer-consumer queue:

```cpp
template<typename T>
class BlockingQueue
````

in `ast-workspace.h`.

The workspace analysis code already uses the following pattern:

```text
Directory Scanner
        ↓
BlockingQueue<Path>
        ↓
Multiple Worker Threads
        ↓
Analyze File
        ↓
Merge Result
```

Reuse this existing concurrency pattern for AST cache warming instead of introducing a new thread pool, task system, or queue implementation.

The new cache warming architecture should be:

```text
Directory Scanner
        ↓
BlockingQueue<filesystem::path>
        ↓
Parallel Cache Workers
        │
        ├── stat file
        ├── lookup cache metadata
        ├── validate metadata
        ├── compute content hash when necessary
        ├── parse stale/missing files
        ├── serialize AST
        └── compress AST
        ↓
BlockingQueue<CacheWriteRequest>
        ↓
Single SQLite Writer Thread
        ↓
Atomic / batched database updates
```

The objective is to parallelize the expensive file processing and AST parsing while keeping SQLite writes serialized and safe.

---

# Existing Sequential Logic

The current `warm_cache` logic is conceptually:

```cpp
for(const auto& path: files) {
    int64_t curSize = 0, curMtime = 0;
    bool statOk = cw_stat(path, curSize, curMtime);

    if(!statOk) {
        ++stats.files_failed;
        continue;
    }

    ASTCacheDatabase::Metadata meta;
    bool hasMeta = db.lookup_metadata(path.string(), meta);

    if(hasMeta &&
       meta.format_version == kAstCacheFormatVersion &&
       meta.source_size == curSize &&
       meta.source_mtime == curMtime) {
        ++stats.valid_entries;
        continue;
    }

    if(hasMeta &&
       meta.format_version == kAstCacheFormatVersion) {
        uint64_t curHash = cw_hash_file(path);

        if(curHash != 0 &&
           curHash == meta.source_hash) {

            db.update_mtime_size(
                path.string(),
                curSize,
                curMtime);

            ++stats.valid_entries;
            continue;
        }

        ++stats.stale_entries;
    }
    else {
        ++stats.missing_entries;
    }

    AST ast = parse(...);

    if(!ast) {
        ++stats.files_failed;
        continue;
    }

    cw_store(db, path, ast);

    ++stats.files_parsed;
    ++stats.files_updated;
}
```

Replace this sequential loop with a producer-consumer pipeline.

Do not change the cache validation semantics unless required for thread safety or integration with the writer queue.

---

# Required Architecture

## Stage 1: Directory Scanner

Reuse the existing workspace scanning pattern.

The scanner thread should:

1. Create the existing `IgnoreMatcher`.
2. Use the existing `scan_recursive()` implementation.
3. Push discovered source files into:

```cpp
BlockingQueue<std::filesystem::path>
```

Example structure:

```cpp
BlockingQueue<std::filesystem::path> fileQueue;

std::thread scanThread([&]() noexcept {
    try {
        IgnoreMatcher matcher(rootPath);

        scan_recursive(
            rootPath,
            matcher,
            [&](std::filesystem::path path) {
                fileQueue.push(std::move(path));
            });
    }
    catch(...) {
        // Record scan failure if appropriate.
    }

    fileQueue.markDone();
});
```

Do not introduce a separate workspace file discovery implementation.

Reuse the same discovery and ignore behavior used by normal workspace analysis.

---

# Do Not Accumulate All Paths Unnecessarily

The cache warmer should process files as they are discovered.

Prefer:

```text
scan
  ↓
push path
  ↓
worker starts processing immediately
```

rather than:

```text
scan entire workspace
  ↓
store all paths
  ↓
start processing
```

Do not keep an `allFiles` vector unless it is required for another existing feature.

The purpose of streaming paths into the queue is to overlap:

* directory scanning
* cache validation
* file hashing
* AST parsing

---

# Stage 2: Parallel Cache Workers

Create multiple worker threads using the existing workspace analysis style.

The worker count should be based on the existing CPU/core detection utilities.

For example:

```cpp
const uint32_t hwThreads = ast::get_physical_core_count();
const uint32_t nWorkers = std::max(1u, hwThreads);
```

However, cache warming is a background optimization and should not unnecessarily monopolize the machine.

If appropriate, cap the worker count using a conservative maximum such as:

```cpp
const uint32_t nWorkers =
    std::max(1u, std::min(4u, hwThreads));
```

Prefer an existing project convention for worker count if one exists.

Do not introduce a new general-purpose thread pool.

Reuse the existing `std::thread + BlockingQueue` approach.

Each worker should:

```text
pop path
    ↓
stat file
    ↓
lookup metadata
    ↓
fast metadata validation
    ↓
if needed:
    compute content hash
    ↓
if valid:
    enqueue metadata update if needed
else:
    parse AST
    ↓
    serialize
    ↓
    LZ4 compress
    ↓
    enqueue prepared cache entry
```

---

# Worker Function

Extract the per-file warming logic into a helper conceptually similar to:

```cpp
warm_one_file(...)
```

For example:

```cpp
void warm_one_file(
    const std::filesystem::path& path,
    ASTCacheDatabase& dbReader,
    BlockingQueue<CacheWriteRequest>& writeQueue,
    WarmStats& localStats);
```

The function should contain the logic currently inside the sequential loop.

Do not duplicate cache validation logic in multiple places.

---

# Cache Metadata Lookup

Workers need to perform:

```cpp
db.lookup_metadata(...)
```

Do not assume that one shared `ASTCacheDatabase` instance is safe for simultaneous access from multiple worker threads.

Inspect the existing `ASTCacheDatabase` implementation.

Prefer one SQLite read connection per worker.

The intended structure is:

```text
Worker 1
    ↓
SQLite connection A

Worker 2
    ↓
SQLite connection B

Worker 3
    ↓
SQLite connection C

...

Writer
    ↓
SQLite write connection W
```

A worker may create and own its read connection for its entire lifetime.

Conceptually:

```cpp
workers.emplace_back([&]() noexcept {
    ASTCacheDatabase dbReader;

    if(!dbReader.open_readonly(dbPath)) {
        // Record worker/database failure.
        return;
    }

    WarmStats localStats;

    std::filesystem::path path;

    while(fileQueue.pop(path)) {
        try {
            warm_one_file(
                path,
                dbReader,
                writeQueue,
                localStats);
        }
        catch(...) {
            ++localStats.files_failed;
        }
    }

    merge_worker_stats(localStats);
});
```

Adapt this to the actual `ASTCacheDatabase` API.

Do not invent a new database abstraction if the existing class can support separate read-only connections.

---

# Tree-sitter / Parser Thread Safety

Before enabling parallel parsing, inspect the implementation of:

```cpp
parse(...)
```

Verify that parsing is safe when called concurrently.

In particular, check whether parsing uses:

* a shared `TSParser`
* global mutable parser state
* static mutable buffers
* shared extractor state

If parser state is not thread-safe, each worker must own independent parser state.

The desired model is:

```text
Worker 1 → Parser Context A
Worker 2 → Parser Context B
Worker 3 → Parser Context C
```

Do not share one `TSParser` instance across concurrent worker threads.

If the existing parser abstraction already creates parser state per call, preserve that behavior.

---

# Stage 3: SQLite Write Queue

Workers must not directly perform cache database writes.

Introduce:

```cpp
BlockingQueue<CacheWriteRequest>
```

All database modifications must be sent through this queue.

There should be one dedicated writer thread.

The writer queue should receive at least two types of requests.

## Type 1: Metadata Update

When:

```text
size/mtime changed
but
content hash is unchanged
```

the worker should not directly call:

```cpp
db.update_mtime_size(...)
```

Instead, enqueue a request containing:

```text
path
source_size
source_mtime
```

Conceptually:

```cpp
struct MetadataUpdate
{
    std::string path;
    int64_t source_size;
    int64_t source_mtime;
};
```

---

## Type 2: Full Cache Entry Update

When a file must be reparsed:

```text
parse
    ↓
serialize AST
    ↓
LZ4 compress
    ↓
prepare complete cache entry
    ↓
enqueue CacheEntryUpdate
```

Conceptually:

```cpp
struct CacheEntryUpdate
{
    std::string path;

    ASTCacheDatabase::Metadata metadata;

    CompressionMode compression;

    std::vector<std::byte> blob;
};
```

Use the actual existing serialized cache representation if one already exists.

Do not duplicate serialization or compression logic.

Refactor existing helpers if necessary so that workers can prepare a complete cache entry without writing it directly to SQLite.

For example, split an existing helper conceptually like:

```text
cw_store()
```

into:

```text
cw_prepare_entry()
    ↓
produces complete serialized/compressed entry

cw_store_prepared_entry()
    ↓
writes prepared entry to SQLite
```

Reuse existing serialization and compression code.

---

# CacheWriteRequest

Use an appropriate tagged request type.

For example:

```cpp
using CacheWriteRequest =
    std::variant<
        MetadataUpdate,
        CacheEntryUpdate
    >;
```

If the project avoids `std::variant`, use the project's existing preferred style.

Keep the request representation simple.

---

# Single SQLite Writer

Create one writer thread that owns the SQLite write connection.

Conceptually:

```cpp
std::thread writerThread([&]() noexcept {
    CacheWriteRequest request;

    while(writeQueue.pop(request)) {
        try {
            apply_write_request(
                dbWriter,
                request);
        }
        catch(...) {
            record_write_failure(...);
        }
    }
});
```

Only this writer thread should perform:

* `INSERT`
* `UPDATE`
* `REPLACE`
* cache entry deletion
* other database modifications related to warming

The workers may perform read-only metadata lookups using their own database connections.

---

# Atomic Database Updates

Each full AST cache update must remain atomic.

The worker must prepare the complete cache entry before it reaches the writer:

```text
parse
    ↓
build AST
    ↓
serialize completely
    ↓
compress completely
    ↓
create CacheEntryUpdate
    ↓
enqueue
```

The writer then commits the already prepared entry.

Do not expose:

* partially serialized ASTs
* partially compressed blobs
* partially updated metadata

Use SQLite transactions.

For a single full entry update:

```text
BEGIN
    replace complete entry
COMMIT
```

If batching is implemented, all writes in a committed batch must still be complete and internally consistent.

---

# Batched SQLite Transactions

The single writer may batch requests to reduce transaction overhead.

For example:

```text
pop up to N requests
    ↓
BEGIN
    ↓
apply requests
    ↓
COMMIT
```

A reasonable initial batch size may be:

```cpp
constexpr size_t kWriteBatchSize = 64;
```

However, do not introduce complex batching logic if the existing database layer already has a suitable transaction mechanism.

Correctness and simplicity are more important than maximum throughput.

The writer must flush all remaining queued requests before exiting.

---

# Worker Statistics

The existing sequential statistics updates are not thread-safe.

Do not update shared statistics directly from every worker.

Do not add a mutex around every individual statistic increment.

Instead, each worker should maintain local statistics:

```cpp
struct WarmStats
{
    uint64_t valid_entries = 0;
    uint64_t missing_entries = 0;
    uint64_t stale_entries = 0;

    uint64_t files_parsed = 0;
    uint64_t files_updated = 0;
    uint64_t files_failed = 0;

    double parsing_ms = 0.0;

    // Add existing relevant fields as necessary.
};
```

Each worker:

```text
process files
    ↓
accumulate local statistics
    ↓
finish
    ↓
merge local statistics once
```

For example:

```cpp
std::mutex statsMu;

{
    std::lock_guard lock(statsMu);
    stats.merge(localStats);
}
```

Avoid taking `statsMu` for every file unless required for immediate reporting.

---

# Writer Statistics

Database write failures and write timing should be collected by the writer thread.

The writer may maintain:

```text
database_write_time
entries_written
metadata_updates
write_failures
```

Merge writer statistics after the writer exits.

Keep worker parsing statistics and writer database statistics logically separate.

---

# Queue Lifecycle

The shutdown order must be correct.

The intended lifecycle is:

```text
1. Start writer thread.

2. Start worker threads.

3. Start scanner thread.

4. Scanner finishes:
       fileQueue.markDone()

5. Workers continue draining fileQueue.

6. All workers exit.

7. Main thread joins all workers.

8. No worker can produce additional write requests.

9. Call:
       writeQueue.markDone()

10. Writer continues draining all remaining requests.

11. Writer exits.

12. Join writer thread.
```

Conceptually:

```cpp
scanThread.join();

for(auto& worker : workers) {
    worker.join();
}

writeQueue.markDone();

writerThread.join();
```

Do not call:

```cpp
writeQueue.markDone();
```

before all workers have finished producing requests.

---

# Queue Capacity and Backpressure

Reuse the bounded nature of the existing `BlockingQueue`.

The queue sizes should prevent unbounded memory growth.

For example:

```text
fileQueue:
    bounds the number of pending paths

writeQueue:
    bounds the number of prepared AST blobs waiting for SQLite
```

The write queue is especially important because serialized/compressed AST blobs may consume significant memory.

A bounded queue provides natural backpressure:

```text
workers produce entries too quickly
    ↓
writeQueue fills
    ↓
workers block
    ↓
memory remains bounded
```

Choose reasonable capacities based on the existing queue usage patterns.

Do not create an unbounded vector of prepared AST blobs.

---

# File Processing Logic

Preserve the existing validation flow.

For each path:

```text
stat
    ↓
metadata lookup
    ↓

metadata matches:
    format version
    size
    mtime
        ↓
    valid
        ↓
    skip


otherwise:

    metadata exists and format matches?
        ↓ yes
    compute content hash
        ↓

    hash matches
        ↓
    enqueue MetadataUpdate
        ↓
    valid


    hash differs
        ↓
    stale
        ↓
    parse


metadata missing / format mismatch
        ↓
    missing
        ↓
    parse
```

The worker should only parse when necessary.

Do not deserialize cached AST blobs during cache warming merely to determine freshness.

---

# Suggested Worker Structure

The final worker logic should be approximately:

```cpp
void worker()
{
    ASTCacheDatabase dbReader = open_worker_reader();

    WarmStats localStats;

    std::filesystem::path path;

    while(fileQueue.pop(path)) {
        try {
            warm_one_file(
                path,
                dbReader,
                writeQueue,
                localStats);
        }
        catch(...) {
            ++localStats.files_failed;
        }
    }

    merge_worker_stats(localStats);
}
```

The `warm_one_file()` implementation should:

```text
stat
    ↓
lookup metadata
    ↓
fast validation
    ↓
optional hash validation
    ↓

valid:
    optionally enqueue MetadataUpdate
    return

invalid:
    parse
    ↓
    serialize
    ↓
    compress
    ↓
    enqueue CacheEntryUpdate
```

---

# Do Not Hold Locks During Parsing

Do not use a global mutex around:

```text
parse
serialize
compress
```

These are intentionally parallel operations.

Synchronization should only be used for:

* final statistics merge
* queue internals
* SQLite writer ownership
* any parser state that is proven to require thread isolation

Do not accidentally serialize the worker pipeline with a global lock.

---

# Normal Commands During Background Warming

This change must remain compatible with the existing background warming design.

While warming is running:

```text
background:
    workers validate and update cache

foreground:
    normal find/search/etc.
```

The foreground command must not wait for the warmer except for normal SQLite locking behavior.

The database writer should commit complete entries atomically.

A foreground command should either observe:

```text
old valid cache entry
```

or:

```text
new valid cache entry
```

It must not observe a partially written AST entry.

---

# Duplicate Warmer Protection

Do not change the existing workspace-level single-warmer lock design.

The expected behavior remains:

```text
first cache warmer
    ↓
acquires WorkspaceWarmLock
    ↓
runs pipeline


second cache warmer
    ↓
cannot acquire lock
    ↓
exit successfully immediately
```

The parallel worker implementation applies only inside the process that successfully acquired the workspace warm lock.

---

# Error Handling

A failure processing one file must not terminate the entire warming operation.

For example:

```text
parse failure
serialization failure
compression failure
metadata lookup failure
```

should:

```text
record failure
    ↓
continue with next file
```

Likewise, a failed database write should not terminate the writer unless the database has become unusable.

Record the failure and continue when possible.

The cache warmer remains best-effort.

---

# Tests

Add or update tests for the parallel implementation.

## Test 1: Fully Cached Workspace

All entries are valid.

Verify:

* zero files parsed
* zero full cache entries written
* workers exit correctly
* writer exits correctly

---

## Test 2: Mixed Workspace

Use a workspace containing:

```text
valid cached files
missing files
stale files
mtime-only changed files with identical content
```

Verify:

```text
valid
    → skipped

mtime-only change
    → metadata update

missing
    → parsed and stored

stale
    → parsed and stored
```

---

## Test 3: Parallel Parsing

Use enough stale files to ensure multiple workers are active.

Verify that:

* multiple files may be processed concurrently
* resulting cache entries are valid
* no duplicate or corrupted entries are produced

If practical, add instrumentation or a test hook to confirm more than one worker performs parsing.

---

## Test 4: Single Writer

Verify that all database writes pass through the writer thread.

Workers must not directly perform SQLite write operations.

---

## Test 5: Queue Shutdown

Verify:

```text
scanner finishes
    ↓
fileQueue.markDone()
    ↓
workers drain all paths
    ↓
workers finish
    ↓
writeQueue.markDone()
    ↓
writer drains all requests
    ↓
writer exits
```

Ensure no requests are lost during shutdown.

---

## Test 6: Bounded Backpressure

Use a small write queue capacity and artificially slow the writer.

Verify that:

* workers block when the write queue is full
* memory does not grow without bound
* all entries are eventually written

---

## Test 7: Concurrent Foreground Access

Run cache warming while executing a normal AST cache read or semantic command.

Verify that:

* the foreground command succeeds
* it does not observe corrupted cache data
* SQLite remains usable
* complete old or new entries are observed

---

## Test 8: Worker Failure

Cause one file to fail parsing.

Verify:

* the worker records the failure
* other workers continue
* the warming operation completes

---

# Performance Measurements

Measure at least:

```text
1 worker
vs
N workers
```

for:

```text
cold workspace
partially stale workspace
fully warm workspace
```

Record:

* total elapsed time
* files parsed
* metadata validation time
* parsing time
* serialization time
* compression time
* database write time

The goal is not necessarily linear scaling.

The expected behavior is:

```text
fully warm workspace
    → dominated by scanning + metadata lookup

cold/stale workspace
    → benefits from parallel parsing
```

---

# Implementation Order

Implement in this order.

## Step 1

Inspect:

* `BlockingQueue`
* existing workspace scan/worker implementation
* `ASTCacheDatabase`
* parser thread safety
* current `warm_cache`
* existing serialization/compression helpers

Reuse existing infrastructure.

---

## Step 2

Extract the sequential per-file logic into:

```text
warm_one_file()
```

Keep behavior unchanged initially.

---

## Step 3

Introduce:

```text
BlockingQueue<filesystem::path>
```

and reuse the existing scanner/worker pattern.

---

## Step 4

Make SQLite metadata reads safe for parallel workers.

Prefer one read connection per worker.

---

## Step 5

Introduce:

```text
BlockingQueue<CacheWriteRequest>
```

and move all database writes into one writer thread.

---

## Step 6

Refactor cache storage if necessary so workers can:

```text
prepare cache entry
```

while the writer performs:

```text
store prepared cache entry
```

Do not duplicate serialization or compression logic.

---

## Step 7

Implement safe queue shutdown.

Ensure the writer is marked done only after every worker has exited.

---

## Step 8

Add worker-local statistics and final aggregation.

---

## Step 9

Run concurrency and performance tests.

Verify that the parallel implementation is faster for a cold or stale workspace and does not regress the fully warm fast path significantly.

---

# Acceptance Criteria

The implementation is complete when:

1. The existing `BlockingQueue` implementation is reused.
2. The existing workspace scanner pattern is reused.
3. Files begin processing while directory scanning is still in progress.
4. Multiple workers can validate and parse files concurrently.
5. Each worker has safe SQLite read access.
6. Parser state is safe for concurrent use.
7. Workers do not directly perform SQLite writes.
8. All SQLite writes are performed by one dedicated writer thread.
9. Prepared AST entries are fully serialized and compressed before enqueueing.
10. The writer stores only complete cache entries.
11. SQLite updates remain atomic.
12. Queue capacity provides bounded memory usage and backpressure.
13. Worker statistics are not updated through data races.
14. Queue shutdown does not lose pending write requests.
15. A fully cached unchanged workspace still exits quickly.
16. Cold and stale workspaces benefit from parallel processing.
17. Existing foreground commands remain safe while background warming is active.
18. Existing workspace warming lock behavior remains unchanged.
19. No new general-purpose thread pool or queue implementation is introduced.

---

# Final Deliverables

After implementation, provide:

1. A summary of the final producer-consumer architecture.

2. The number and purpose of each queue.

3. The worker count policy.

4. How SQLite read access is handled per worker.

5. How SQLite writes are serialized.

6. The transaction/batching strategy.

7. How queue shutdown is coordinated.

8. Parser thread-safety considerations and the solution used.

9. Performance comparison:

   * sequential warming
   * parallel warming
   * fully warm workspace

10. Test results.

11. Any remaining concurrency limitations or race conditions.

Keep the implementation focused on reusing the existing workspace concurrency architecture.

Do not introduce a new thread pool, task scheduler, or queue abstraction unless the existing `BlockingQueue` is proven insufficient.
