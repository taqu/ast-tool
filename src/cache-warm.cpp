#include "cache-warm.h"
#include "ast-cache-db.h"
#include "ast-cache.h"
#include "ast-ir.h"
#include "ast-workspace.h"
#include "ast-tool.h"
#include "xxhash.h"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_set>
#include <variant>
#include <vector>

#if defined(_WIN32) || defined(_WIN64)
#    define WARM_WINDOWS
#    include <windows.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    define CW_STAT_STRUCT struct _stat64
#    define CW_STAT_FUNC(p, buf) _stat64(p, buf)
#else
#    include <fcntl.h>
#    include <sys/file.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <unistd.h>
#    define CW_STAT_STRUCT struct stat
#    define CW_STAT_FUNC(p, buf) stat(p, buf)
#endif

namespace ast
{

// ── OS-level exclusive file lock ──────────────────────────────────────────────

class WarmLock
{
public:
    WarmLock() = default;
    ~WarmLock() { release(); }
    WarmLock(const WarmLock&) = delete;
    WarmLock& operator=(const WarmLock&) = delete;

    bool try_acquire(const std::filesystem::path& lockPath)
    {
        if(held_)
            return true;
        std::string p = lockPath.string();
#if defined(WARM_WINDOWS)
        handle_ = CreateFileA(
            p.c_str(),
            GENERIC_WRITE,
            FILE_SHARE_READ,
            nullptr,
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            nullptr);
        if(handle_ == INVALID_HANDLE_VALUE) {
            handle_ = nullptr;
            return false;
        }
        OVERLAPPED ov = {};
        if(!LockFileEx(handle_, LOCKFILE_EXCLUSIVE_LOCK | LOCKFILE_FAIL_IMMEDIATELY,
                       0, 1, 0, &ov)) {
            CloseHandle(handle_);
            handle_ = nullptr;
            return false;
        }
#else
        fd_ = open(p.c_str(), O_CREAT | O_WRONLY | O_CLOEXEC, 0600);
        if(fd_ < 0)
            return false;
        if(flock(fd_, LOCK_EX | LOCK_NB) != 0) {
            close(fd_);
            fd_ = -1;
            return false;
        }
#endif
        held_ = true;
        return true;
    }

    void release()
    {
        if(!held_)
            return;
#if defined(WARM_WINDOWS)
        if(handle_) {
            OVERLAPPED ov = {};
            UnlockFileEx(handle_, 0, 1, 0, &ov);
            CloseHandle(handle_);
            handle_ = nullptr;
        }
#else
        if(fd_ >= 0) {
            flock(fd_, LOCK_UN);
            close(fd_);
            fd_ = -1;
        }
#endif
        held_ = false;
    }

    bool held() const noexcept { return held_; }

private:
#if defined(WARM_WINDOWS)
    void* handle_ = nullptr;
#else
    int fd_ = -1;
#endif
    bool held_ = false;
};

// ── File-stat helper ──────────────────────────────────────────────────────────

static bool cw_stat(const std::filesystem::path& path, int64_t& outSize, int64_t& outMtime)
{
    CW_STAT_STRUCT st;
    std::string p = path.string();
    if(CW_STAT_FUNC(p.c_str(), &st) != 0)
        return false;
    outSize  = (int64_t)st.st_size;
    outMtime = (int64_t)st.st_mtime;
    return true;
}

// ── Content hash helper ───────────────────────────────────────────────────────

static uint64_t cw_hash_file(const std::filesystem::path& path)
{
    std::string p = path.string();
    FILE* f = nullptr;
#if defined(WARM_WINDOWS)
    errno_t err = fopen_s(&f, p.c_str(), "rb");
    if(err != 0)
        return 0;
#else
    f = fopen(p.c_str(), "rb");
    if(!f)
        return 0;
#endif
    XXH64_state_t* state = XXH64_createState();
    XXH64_reset(state, 0);
    char buf[65536];
    size_t n;
    while((n = fread(buf, 1, sizeof(buf), f)) > 0)
        XXH64_update(state, buf, n);
    fclose(f);
    uint64_t h = XXH64_digest(state);
    XXH64_freeState(state);
    return h;
}

// ── Prepare a complete cache entry without writing to DB ──────────────────────

static bool cw_prepare_entry(const std::filesystem::path& path,
                              const AST& ast,
                              ASTCacheDatabase::Entry& out)
{
    std::vector<uint8_t> raw = ast_serialize(ast);
    if(raw.empty())
        return false;

    std::vector<uint8_t> compressed = ast_lz4_compress(raw.data(), raw.size());

    if(ast.text() && ast.text_size() > 0)
        out.source_hash = XXH64(ast.text(), (size_t)ast.text_size(), 0);

    int64_t fsize = 0, fmtime = 0;
    cw_stat(path, fsize, fmtime);
    out.source_size      = fsize;
    out.source_mtime     = fmtime;
    out.language         = static_cast<uint32_t>(ast.language());
    out.format_version   = kAstCacheFormatVersion;
    out.uncompressed_size = (int64_t)raw.size();

    if(!compressed.empty() && compressed.size() < raw.size()) {
        out.compression = AstCompressionMode::LZ4;
        out.blob        = std::move(compressed);
    } else {
        out.compression = AstCompressionMode::None;
        out.blob        = std::move(raw);
    }
    return true;
}

// ── Write request types ───────────────────────────────────────────────────────

struct MetadataUpdate
{
    std::string path;
    int64_t     source_size;
    int64_t     source_mtime;
};

struct CacheEntryUpdate
{
    std::string              path;
    ASTCacheDatabase::Entry  entry;
};

using CacheWriteRequest = std::variant<MetadataUpdate, CacheEntryUpdate>;

// ── Per-file warming logic (runs in a worker thread) ─────────────────────────

static void warm_one_file(const std::filesystem::path& path,
                           ASTCacheDatabase& dbReader,
                           BlockingQueue<CacheWriteRequest>& writeQueue,
                           WarmStats& local)
{
    using clock = std::chrono::steady_clock;

    int64_t curSize = 0, curMtime = 0;
    if(!cw_stat(path, curSize, curMtime)) {
        ++local.files_failed;
        return;
    }

    ASTCacheDatabase::Metadata meta;
    bool hasMeta = dbReader.lookup_metadata(path.string(), meta);

    if(hasMeta && meta.format_version == kAstCacheFormatVersion
       && meta.source_size == curSize && meta.source_mtime == curMtime) {
        ++local.valid_entries;
        return;
    }

    if(hasMeta && meta.format_version == kAstCacheFormatVersion) {
        uint64_t curHash = cw_hash_file(path);
        if(curHash != 0 && curHash == meta.source_hash) {
            MetadataUpdate u;
            u.path         = path.string();
            u.source_size  = curSize;
            u.source_mtime = curMtime;
            writeQueue.push(std::move(u));
            ++local.valid_entries;
            return;
        }
        ++local.stale_entries;
    } else {
        ++local.missing_entries;
    }

    // Parse, serialize, compress — all in the worker, nothing written here.
    auto tParse0 = clock::now();
    std::u8string u8p = path.u8string();
    AST ast = parse(u8p.c_str());
    local.parsing_ms += std::chrono::duration<double, std::milli>(clock::now() - tParse0).count();

    if(!ast) {
        ++local.files_failed;
        return;
    }

    CacheEntryUpdate u;
    u.path = path.string();
    if(!cw_prepare_entry(path, ast, u.entry)) {
        ++local.files_failed;
        return;
    }

    writeQueue.push(std::move(u));
    ++local.files_parsed;
    ++local.files_updated;
}

// ── Batch-flush write requests to SQLite ─────────────────────────────────────

static void flush_batch(ASTCacheDatabase& db,
                         std::vector<CacheWriteRequest>& batch,
                         uint32_t& written,
                         uint32_t& failed)
{
    if(batch.empty())
        return;

    db.begin_transaction();
    for(auto& req: batch) {
        bool ok = false;
        if(auto* mu = std::get_if<MetadataUpdate>(&req)) {
            ok = db.update_mtime_size(mu->path, mu->source_size, mu->source_mtime);
        } else if(auto* eu = std::get_if<CacheEntryUpdate>(&req)) {
            ok = db.store(eu->path, eu->entry);
        }
        if(ok) ++written;
        else   ++failed;
    }
    db.commit_transaction();
}

// ── Main warming routine ──────────────────────────────────────────────────────

WarmResult warm_cache(const std::filesystem::path& root, WarmStats& stats, bool verbose)
{
    using clock = std::chrono::steady_clock;
    auto t0 = clock::now();

    std::filesystem::path cacheDir = root / ".ast-tool";
    {
        std::error_code ec;
        std::filesystem::create_directories(cacheDir, ec);
        if(ec)
            return WarmResult::DatabaseError;
    }

    WarmLock lock;
    if(!lock.try_acquire(cacheDir / "cache-warm.lock"))
        return WarmResult::LockBusy;

    std::filesystem::path dbPath = cacheDir / "ast-cache.db";

    // Init schema via a temporary write connection so worker read connections find a valid file.
    {
        ASTCacheDatabase initDb;
        if(!initDb.open(dbPath))
            return WarmResult::DatabaseError;
    }

    // Workspace file set — populated by the scanner thread, read only after scanThread.join().
    std::unordered_set<std::string> workspaceSet;

    constexpr size_t kFileQueueCap  = 256;
    constexpr size_t kWriteQueueCap = 32;
    BlockingQueue<std::filesystem::path> fileQueue(kFileQueueCap);
    BlockingQueue<CacheWriteRequest>     writeQueue(kWriteQueueCap);

    std::mutex   statsMu;
    uint32_t     writerWritten = 0;
    uint32_t     writerFailed  = 0;
    double       writerWriteMs = 0.0;

    // ── Writer thread: owns the single SQLite write connection ────────────────
    std::thread writerThread([&]() noexcept {
        ASTCacheDatabase dbWriter;
        if(!dbWriter.open(dbPath))
            return;

        constexpr size_t kBatch = 64;
        std::vector<CacheWriteRequest> batch;
        batch.reserve(kBatch);

        auto tW0 = clock::now();
        CacheWriteRequest req;

        while(writeQueue.pop(req)) {
            batch.push_back(std::move(req));
            if(batch.size() >= kBatch) {
                flush_batch(dbWriter, batch, writerWritten, writerFailed);
                batch.clear();
            }
        }
        if(!batch.empty())
            flush_batch(dbWriter, batch, writerWritten, writerFailed);

        writerWriteMs = std::chrono::duration<double, std::milli>(
            clock::now() - tW0).count();
    });

    // ── Worker threads: each has its own read-only SQLite connection ──────────
    const uint32_t hwThreads = ast::get_physical_core_count();
    const uint32_t nWorkers  = (std::max)(1u, (std::min)(4u, hwThreads));

    std::vector<std::thread> workers;
    workers.reserve(nWorkers);
    for(uint32_t i = 0; i < nWorkers; ++i) {
        workers.emplace_back([&]() noexcept {
            ASTCacheDatabase dbReader;
            if(!dbReader.open_readonly(dbPath))
                return;

            WarmStats local;
            std::filesystem::path path;

            while(fileQueue.pop(path)) {
                try {
                    warm_one_file(path, dbReader, writeQueue, local);
                } catch(...) {
                    ++local.files_failed;
                }
            }

            std::lock_guard lk(statsMu);
            stats.merge(local);
        });
    }

    // ── Scanner thread: streams paths into fileQueue ──────────────────────────
    uint32_t scanned = 0;
    std::thread scanThread([&]() noexcept {
        try {
            scan_workspace_stream(root, [&](std::filesystem::path p) {
                workspaceSet.insert(p.string());
                ++scanned;
                fileQueue.push(std::move(p));
            });
        } catch(...) {
        }
        fileQueue.markDone();
    });

    // ── Shutdown sequence (order matters) ─────────────────────────────────────
    scanThread.join();
    for(auto& w: workers)
        w.join();
    writeQueue.markDone();
    writerThread.join();

    // ── Stale entry removal (after all workers and writer have finished) ───────
    {
        ASTCacheDatabase cleanupDb;
        if(cleanupDb.open(dbPath)) {
            std::vector<std::string> allPaths = cleanupDb.list_all_paths();
            for(const std::string& p: allPaths) {
                if(workspaceSet.find(p) == workspaceSet.end()) {
                    cleanupDb.remove(p);
                    ++stats.removed_entries;
                }
            }
        }
    }

    stats.total_files = scanned;

    stats.total_ms = std::chrono::duration<double, std::milli>(clock::now() - t0).count();

    if(verbose) {
        fprintf(stdout,
                "Workspace files:     %u\n"
                "Valid cache entries: %u\n"
                "Missing entries:     %u\n"
                "Stale entries:       %u\n"
                "Files parsed:        %u\n"
                "Files updated:       %u\n"
                "Files failed:        %u\n"
                "Removed entries:     %u\n"
                "Writer failures:     %u\n"
                "\n"
                "Parsing:             %.2f ms\n"
                "DB writes:           %.2f ms\n"
                "Total elapsed:       %.2f ms\n",
                stats.total_files,
                stats.valid_entries,
                stats.missing_entries,
                stats.stale_entries,
                stats.files_parsed,
                stats.files_updated,
                stats.files_failed,
                stats.removed_entries,
                writerFailed,
                stats.parsing_ms,
                writerWriteMs,
                stats.total_ms);
    }

    return WarmResult::Success;
}

} // namespace ast
