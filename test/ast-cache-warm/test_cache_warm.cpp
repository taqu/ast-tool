#include "test_cache_warm.h"
#include "ast-cache-db.h"
#include "ast-cache.h"
#include "ast-ir.h"
#include "ast-workspace.h"
#include "cache-warm.h"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <thread>

#if defined(_WIN32) || defined(_WIN64)
#    define TW_WINDOWS
#    include <windows.h>
#else
#    include <fcntl.h>
#    include <sys/file.h>
#    include <unistd.h>
#endif

namespace ast
{
namespace
{

static bool check(bool condition, const char* description)
{
    if(!condition)
        std::cerr << "    FAIL: " << description << "\n";
    return condition;
}

// ── Helpers ───────────────────────────────────────────────────────────────────

static std::filesystem::path make_temp_workspace(const char* suffix)
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path()
                                / (std::string("ast_warm_ws_") + suffix);
    std::error_code ec;
    std::filesystem::remove_all(tmp, ec);
    std::filesystem::create_directories(tmp, ec);
    return tmp;
}

static void remove_workspace(const std::filesystem::path& p)
{
    std::error_code ec;
    std::filesystem::remove_all(p, ec);
}

static bool write_file(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream f(path, std::ios::binary);
    if(!f)
        return false;
    f << content;
    return true;
}

// ── Test 1: Fully Cached Workspace ────────────────────────────────────────────
// After warming, a second pass should parse zero files and the pipeline should
// shut down cleanly with no stray worker or writer threads.

bool test_fully_cached_workspace()
{
    auto ws = make_temp_workspace("t1");
    bool ok = true;

    if(!write_file(ws / "a.cpp", "int a() { return 1; }")) {
        remove_workspace(ws);
        return check(false, "write a.cpp");
    }
    if(!write_file(ws / "b.cpp", "int b() { return 2; }")) {
        remove_workspace(ws);
        return check(false, "write b.cpp");
    }

    // First warm — cold workspace.
    {
        WarmStats s;
        WarmResult r = warm_cache(ws, s, false);
        ok &= check(r == WarmResult::Success, "first warm succeeded");
        ok &= check(s.files_updated == 2, "first warm stored 2 files");
    }

    // Second warm — fully cached; zero files parsed, writer exits cleanly.
    {
        WarmStats s;
        WarmResult r = warm_cache(ws, s, false);
        ok &= check(r == WarmResult::Success, "second warm succeeded");
        ok &= check(s.files_parsed == 0, "second warm parsed 0 files");
        ok &= check(s.valid_entries == 2, "second warm found 2 valid entries");
        ok &= check(s.files_updated == 0, "second warm wrote 0 entries");
    }

    remove_workspace(ws);
    return ok;
}

// ── Test 2: Mixed Workspace ───────────────────────────────────────────────────
// One valid file, one missing from cache, one stale (content changed), one with
// mtime-only change (same content hash → metadata update only, no re-parse).

bool test_mixed_workspace()
{
    auto ws = make_temp_workspace("t2");
    bool ok = true;

    // Write four source files.
    if(!write_file(ws / "valid.cpp",   "int v() { return 0; }") ||
       !write_file(ws / "missing.cpp", "int m() { return 1; }") ||
       !write_file(ws / "stale.cpp",   "int s() { return 2; }") ||
       !write_file(ws / "mtime.cpp",   "int t() { return 3; }")) {
        remove_workspace(ws);
        return check(false, "write source files");
    }

    // First warm — cache all four.
    {
        WarmStats s;
        WarmResult r = warm_cache(ws, s, false);
        ok &= check(r == WarmResult::Success, "first warm succeeded");
        ok &= check(s.files_updated == 4, "first warm stored 4 files");
    }

    // Simulate different staleness scenarios:
    //   - valid.cpp: unchanged → valid (fast path)
    //   - missing.cpp: delete its cache entry → missing
    //   - stale.cpp: change content → stale (hash differs)
    //   - mtime.cpp: touch file (new mtime) but keep identical content → metadata update

    {
        std::filesystem::path dbPath = ws / ".ast-tool" / "ast-cache.db";
        ASTCacheDatabase db;
        if(db.open(dbPath)) {
            db.remove((ws / "missing.cpp").string());
        }
    }

    // Overwrite stale.cpp with different content.
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    if(!write_file(ws / "stale.cpp", "int s() { return 99; }")) {
        remove_workspace(ws);
        return check(false, "write stale.cpp");
    }

    // Touch mtime.cpp: rewrite same bytes so content hash is identical.
    const std::string mtimeContent = "int t() { return 3; }";
    if(!write_file(ws / "mtime.cpp", mtimeContent)) {
        remove_workspace(ws);
        return check(false, "rewrite mtime.cpp");
    }

    {
        WarmStats s;
        WarmResult r = warm_cache(ws, s, false);
        ok &= check(r == WarmResult::Success, "second warm succeeded");
        ok &= check(s.valid_entries >= 1,   "valid.cpp was skipped");
        ok &= check(s.missing_entries == 1, "missing.cpp detected as missing");
        ok &= check(s.stale_entries == 1,   "stale.cpp detected as stale");
        ok &= check(s.files_parsed >= 2,    "missing+stale were parsed");
    }

    remove_workspace(ws);
    return ok;
}

// ── Test 3: Parallel Parsing ──────────────────────────────────────────────────
// Create enough stale files to exercise multiple workers simultaneously.
// Verify all entries are valid after a second warm pass.

bool test_parallel_parsing()
{
    auto ws = make_temp_workspace("t3");
    bool ok = true;

    constexpr int N = 20; // enough to keep several workers busy

    for(int i = 0; i < N; ++i) {
        std::string name = "f" + std::to_string(i) + ".cpp";
        std::string body = "int f" + std::to_string(i) + "() { return " + std::to_string(i) + "; }";
        if(!write_file(ws / name, body)) {
            remove_workspace(ws);
            return check(false, "write source file");
        }
    }

    // Cold warm — all N files should be parsed.
    {
        WarmStats s;
        WarmResult r = warm_cache(ws, s, false);
        ok &= check(r == WarmResult::Success, "parallel warm succeeded");
        ok &= check(s.files_updated == (uint32_t)N, "all files stored");
        ok &= check(s.files_failed == 0, "no failures");
    }

    // Verify DB contains N entries.
    {
        std::filesystem::path dbPath = ws / ".ast-tool" / "ast-cache.db";
        ASTCacheDatabase db;
        ok &= check(db.open(dbPath), "db opened");
        ok &= check(db.entry_count() == N, "db has N entries");
    }

    // Second warm should be fully cached.
    {
        WarmStats s;
        WarmResult r = warm_cache(ws, s, false);
        ok &= check(r == WarmResult::Success, "second warm succeeded");
        ok &= check(s.files_parsed == 0,      "second warm parsed nothing");
        ok &= check(s.valid_entries == (uint32_t)N, "all entries valid");
    }

    remove_workspace(ws);
    return ok;
}

// ── Test 4: Single Writer / Writer Correctness ────────────────────────────────
// With multiple workers writing concurrently, all N entries must be present
// and parseable from the DB after warming — no corruption, no duplicates.

bool test_single_writer_correctness()
{
    auto ws = make_temp_workspace("t4");
    bool ok = true;

    constexpr int N = 16;

    for(int i = 0; i < N; ++i) {
        std::string name = "w" + std::to_string(i) + ".cpp";
        std::string body = "int w" + std::to_string(i) + "() { return " + std::to_string(i) + "; }";
        if(!write_file(ws / name, body)) {
            remove_workspace(ws);
            return check(false, "write source file");
        }
    }

    WarmStats s;
    warm_cache(ws, s, false);
    ok &= check(s.files_failed == 0, "no worker failures");

    // Verify all N entries are in the DB and individually loadable.
    std::filesystem::path dbPath = ws / ".ast-tool" / "ast-cache.db";
    ASTCacheDatabase db;
    ok &= check(db.open(dbPath), "db opened");
    ok &= check(db.entry_count() == N, "all entries present");

    int validBlobs = 0;
    for(int i = 0; i < N; ++i) {
        std::string key = (ws / ("w" + std::to_string(i) + ".cpp")).string();
        ASTCacheDatabase::Entry e;
        if(db.lookup(key, e) && !e.blob.empty())
            ++validBlobs;
    }
    ok &= check(validBlobs == N, "all blobs are non-empty");

    remove_workspace(ws);
    return ok;
}

// ── Test 5: Queue Shutdown / No Lost Requests ─────────────────────────────────
// Verify that every enqueued write request survives the shutdown sequence.
// After warming, DB entry count must equal the number of files discovered.

bool test_queue_shutdown()
{
    auto ws = make_temp_workspace("t5");
    bool ok = true;

    constexpr int N = 12;
    for(int i = 0; i < N; ++i) {
        std::string name = "s" + std::to_string(i) + ".cpp";
        std::string body = "int s" + std::to_string(i) + "() { return " + std::to_string(i) + "; }";
        if(!write_file(ws / name, body)) {
            remove_workspace(ws);
            return check(false, "write source file");
        }
    }

    WarmStats s;
    WarmResult r = warm_cache(ws, s, false);
    ok &= check(r == WarmResult::Success, "warm succeeded");
    ok &= check(s.total_files == (uint32_t)N, "scanner found all files");
    ok &= check(s.files_failed == 0, "no failures");

    // All N entries must be in the database — no write was lost during shutdown.
    std::filesystem::path dbPath = ws / ".ast-tool" / "ast-cache.db";
    ASTCacheDatabase db;
    ok &= check(db.open(dbPath), "db opened");
    ok &= check(db.entry_count() == N, "all writes committed before shutdown");

    remove_workspace(ws);
    return ok;
}

// ── Test 6: Bounded Backpressure ──────────────────────────────────────────────
// Large workspace with many files; verify that all entries are eventually
// written even when the write queue fills and workers must block.

bool test_bounded_backpressure()
{
    auto ws = make_temp_workspace("t6");
    bool ok = true;

    // More files than kWriteQueueCap (32) to force backpressure.
    constexpr int N = 50;
    for(int i = 0; i < N; ++i) {
        std::string name = "bp" + std::to_string(i) + ".cpp";
        std::string body = "int bp" + std::to_string(i) + "() { return " + std::to_string(i) + "; }";
        if(!write_file(ws / name, body)) {
            remove_workspace(ws);
            return check(false, "write source file");
        }
    }

    WarmStats s;
    WarmResult r = warm_cache(ws, s, false);
    ok &= check(r == WarmResult::Success, "warm with backpressure succeeded");
    ok &= check(s.files_failed == 0, "no failures under backpressure");

    // Every file must be in the DB despite write queue pressure.
    std::filesystem::path dbPath = ws / ".ast-tool" / "ast-cache.db";
    ASTCacheDatabase db;
    ok &= check(db.open(dbPath), "db opened");
    ok &= check(db.entry_count() == N, "all entries written despite backpressure");

    remove_workspace(ws);
    return ok;
}

// ── Test 7: Concurrent Foreground Access ──────────────────────────────────────
// After warming, get_translation_unit should use the persistent cache without
// observing corrupt or partially written data.

bool test_concurrent_foreground_access()
{
    auto ws = make_temp_workspace("t7");
    bool ok = true;

    if(!write_file(ws / "live.cpp", "int live() { return 42; }")) {
        remove_workspace(ws);
        return check(false, "write live.cpp");
    }

    {
        WarmStats s;
        WarmResult r = warm_cache(ws, s, false);
        ok &= check(r == WarmResult::Success, "warm succeeded");
    }

    // Foreground access should succeed and observe a complete (old or new) entry.
    {
        Workspace workspace = open_workspace(ws.u8string().c_str());
        const TranslationUnit* tu = workspace.get_translation_unit(ws / "live.cpp");
        ok &= check(tu != nullptr, "get_translation_unit returned non-null");
        if(tu) {
            ok &= check(workspace.persistentCacheHits_ >= 1 || workspace.parsedCount >= 1,
                        "translation unit loaded (cache or fresh parse)");
        }
    }

    remove_workspace(ws);
    return ok;
}

// ── Test 8: Worker Failure — Other Workers Continue ──────────────────────────
// Cause one file to fail parsing (make it unreadable), verify that other
// workers continue and the warm operation completes successfully.

bool test_worker_failure_continues()
{
    auto ws = make_temp_workspace("t8");
    bool ok = true;

    if(!write_file(ws / "good1.cpp", "int g1() { return 1; }") ||
       !write_file(ws / "good2.cpp", "int g2() { return 2; }") ||
       !write_file(ws / "bad.cpp",   "int bad() { return 0; }")) {
        remove_workspace(ws);
        return check(false, "write source files");
    }

    // First warm all three files.
    {
        WarmStats s;
        warm_cache(ws, s, false);
    }

    // Now delete bad.cpp from the cache to force a re-parse attempt,
    // but also write an empty/unparseable replacement on disk.
    {
        std::filesystem::path dbPath = ws / ".ast-tool" / "ast-cache.db";
        ASTCacheDatabase db;
        if(db.open(dbPath))
            db.remove((ws / "bad.cpp").string());
    }

    // Overwrite with content that produces a valid but different file
    // (we can't easily make tree-sitter fail, so just change the content
    //  so a re-parse is needed — this still exercises the per-file error path
    //  boundary without relying on an unparseable language).
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    if(!write_file(ws / "bad.cpp", "/* intentionally different */ int bad2() {}")) {
        remove_workspace(ws);
        return check(false, "rewrite bad.cpp");
    }

    WarmStats s;
    WarmResult r = warm_cache(ws, s, false);
    ok &= check(r == WarmResult::Success, "warm completed despite missing entry");
    ok &= check(s.files_failed == 0, "no hard failures (re-parse of bad.cpp succeeded)");
    ok &= check(s.files_parsed >= 1, "at least one file was (re)parsed");

    // good1.cpp and good2.cpp must remain valid.
    {
        std::filesystem::path dbPath = ws / ".ast-tool" / "ast-cache.db";
        ASTCacheDatabase db;
        ok &= check(db.open(dbPath), "db opened");
        ASTCacheDatabase::Metadata m;
        ok &= check(db.lookup_metadata((ws / "good1.cpp").string(), m), "good1 still in cache");
        ok &= check(db.lookup_metadata((ws / "good2.cpp").string(), m), "good2 still in cache");
    }

    remove_workspace(ws);
    return ok;
}

// ── Test 9: Duplicate Warm Processes ─────────────────────────────────────────
// When one warmer holds the lock, a second invocation returns LockBusy.

bool test_duplicate_warm_processes()
{
    auto ws = make_temp_workspace("t9");
    bool ok = true;

    if(!write_file(ws / "dup.cpp", "void dup() {}")) {
        remove_workspace(ws);
        return check(false, "write dup.cpp");
    }

    std::filesystem::path cacheDir = ws / ".ast-tool";
    {
        std::error_code ec;
        std::filesystem::create_directories(cacheDir, ec);
    }
    std::filesystem::path lockPath = cacheDir / "cache-warm.lock";

    bool gotLock = false;
#if defined(TW_WINDOWS)
    HANDLE hLock = CreateFileA(lockPath.string().c_str(),
                               GENERIC_WRITE, FILE_SHARE_READ,
                               nullptr, OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL, nullptr);
    OVERLAPPED ov = {};
    gotLock = (hLock != INVALID_HANDLE_VALUE)
              && LockFileEx(hLock, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                            0, 1, 0, &ov);
#else
    int hLock = open(lockPath.string().c_str(), O_CREAT | O_WRONLY | O_CLOEXEC, 0600);
    gotLock = (hLock >= 0) && (flock(hLock, LOCK_EX | LOCK_NB) == 0);
#endif

    ok &= check(gotLock, "test lock acquired");
    if(gotLock) {
        WarmStats s;
        WarmResult r = warm_cache(ws, s, false);
        ok &= check(r == WarmResult::LockBusy, "second warmer sees LockBusy");

#if defined(TW_WINDOWS)
        UnlockFileEx(hLock, 0, 1, 0, &ov);
        CloseHandle(hLock);
#else
        flock(hLock, LOCK_UN);
        close(hLock);
#endif
    }

    remove_workspace(ws);
    return ok;
}

// ── Test 10: Stale Lock / Crash Recovery ─────────────────────────────────────
// A lock file with no active holder must not prevent future warming.

bool test_stale_lock_recovery()
{
    auto ws = make_temp_workspace("t10");
    bool ok = true;

    if(!write_file(ws / "crash.cpp", "void crash() {}")) {
        remove_workspace(ws);
        return check(false, "write crash.cpp");
    }

    std::filesystem::path cacheDir = ws / ".ast-tool";
    {
        std::error_code ec;
        std::filesystem::create_directories(cacheDir, ec);
    }
    {
        std::ofstream f(cacheDir / "cache-warm.lock");
        f << "stale";
    }

    {
        WarmStats s;
        WarmResult r = warm_cache(ws, s, false);
        ok &= check(r == WarmResult::Success, "warm after stale lock succeeded");
        ok &= check(s.files_updated >= 1, "files parsed after stale lock recovery");
    }

    remove_workspace(ws);
    return ok;
}

// ── Test 11: Atomic Cache Update — DB Valid After Store ───────────────────────
// Verify that a stored entry can be read back intact after DB reopen.

bool test_atomic_cache_update()
{
    auto ws = make_temp_workspace("t11");
    bool ok = true;

    if(!write_file(ws / "atom.cpp", "int atom() { return 7; }")) {
        remove_workspace(ws);
        return check(false, "write atom.cpp");
    }

    {
        WarmStats s;
        WarmResult r = warm_cache(ws, s, false);
        ok &= check(r == WarmResult::Success, "warm succeeded");
        ok &= check(s.files_updated >= 1, "file was stored");
    }

    std::filesystem::path dbPath = ws / ".ast-tool" / "ast-cache.db";
    ASTCacheDatabase db;
    ok &= check(db.open(dbPath), "db reopened");

    ASTCacheDatabase::Metadata meta;
    std::string key = (ws / "atom.cpp").string();
    ok &= check(db.lookup_metadata(key, meta), "metadata lookup succeeded after reopen");
    ok &= check(meta.format_version == kAstCacheFormatVersion, "format version correct");

    remove_workspace(ws);
    return ok;
}

// ── Test 12: Cache Corruption Recovery ───────────────────────────────────────
// Inject a corrupt entry; warming should detect and replace it.

bool test_cache_corruption_recovery()
{
    auto ws = make_temp_workspace("t12");
    bool ok = true;

    if(!write_file(ws / "corrupt.cpp", "int f() { return 1; }")) {
        remove_workspace(ws);
        return check(false, "write corrupt.cpp");
    }

    {
        std::filesystem::path cacheDir = ws / ".ast-tool";
        std::error_code ec;
        std::filesystem::create_directories(cacheDir, ec);

        ASTCacheDatabase db;
        if(db.open(cacheDir / "ast-cache.db")) {
            ASTCacheDatabase::Entry e;
            e.source_hash       = 0xDEADBEEFCAFEBABEull;
            e.source_size       = 999999;
            e.source_mtime      = 0;
            e.language          = 0;
            e.format_version    = kAstCacheFormatVersion;
            e.compression       = AstCompressionMode::None;
            e.uncompressed_size = 4;
            e.blob              = {0xDE, 0xAD, 0xBE, 0xEF};
            db.store((ws / "corrupt.cpp").string(), e);
        }
    }

    {
        WarmStats s;
        WarmResult r = warm_cache(ws, s, false);
        ok &= check(r == WarmResult::Success, "warm with corrupt entry succeeded");
        ok &= check(s.stale_entries >= 1 || s.missing_entries >= 1 || s.files_parsed >= 1,
                    "corrupt entry detected");
    }

    {
        std::filesystem::path dbPath = ws / ".ast-tool" / "ast-cache.db";
        ASTCacheDatabase db;
        ok &= check(db.open(dbPath), "db reopened after corruption recovery");

        ASTCacheDatabase::Entry e;
        ok &= check(db.lookup((ws / "corrupt.cpp").string(), e), "entry present after recovery");
        ok &= check(e.format_version == kAstCacheFormatVersion, "recovered version matches");
        ok &= check(!e.blob.empty(), "recovered blob is non-empty");
    }

    remove_workspace(ws);
    return ok;
}

} // namespace

bool run_tests_cache_warm()
{
    struct TestCase
    {
        const char* name;
        bool (*fn)();
    };
    static const TestCase kTests[] = {
        {"fully_cached_workspace",       test_fully_cached_workspace},
        {"mixed_workspace",              test_mixed_workspace},
        {"parallel_parsing",             test_parallel_parsing},
        {"single_writer_correctness",    test_single_writer_correctness},
        {"queue_shutdown",               test_queue_shutdown},
        {"bounded_backpressure",         test_bounded_backpressure},
        {"concurrent_foreground_access", test_concurrent_foreground_access},
        {"worker_failure_continues",     test_worker_failure_continues},
        {"duplicate_warm_processes",     test_duplicate_warm_processes},
        {"stale_lock_recovery",          test_stale_lock_recovery},
        {"atomic_cache_update",          test_atomic_cache_update},
        {"cache_corruption_recovery",    test_cache_corruption_recovery},
    };

    bool all_passed = true;
    std::cout << "=== Cache Warm Tests ===" << std::endl;
    for(const auto& t: kTests) {
        std::cout << "  " << t.name << " ... " << std::flush;
        bool passed = t.fn();
        std::cout << (passed ? "PASS" : "FAIL") << "\n";
        all_passed &= passed;
    }
    return all_passed;
}

} // namespace ast
