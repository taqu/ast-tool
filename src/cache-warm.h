#ifndef INC_AST_CACHE_WARM_H_
#define INC_AST_CACHE_WARM_H_

#include <cstdint>
#include <filesystem>

namespace ast
{

struct WarmStats
{
    uint32_t total_files     = 0;
    uint32_t valid_entries   = 0;
    uint32_t missing_entries = 0;
    uint32_t stale_entries   = 0;
    uint32_t files_parsed    = 0;
    uint32_t files_updated   = 0;
    uint32_t files_failed    = 0;
    uint32_t removed_entries = 0;
    double   metadata_ms     = 0.0;
    double   parsing_ms      = 0.0;
    double   total_ms        = 0.0;

    void merge(const WarmStats& other)
    {
        valid_entries   += other.valid_entries;
        missing_entries += other.missing_entries;
        stale_entries   += other.stale_entries;
        files_parsed    += other.files_parsed;
        files_updated   += other.files_updated;
        files_failed    += other.files_failed;
        parsing_ms      += other.parsing_ms;
    }
};

enum class WarmResult
{
    Success,       // Warming completed successfully.
    LockBusy,      // Another warmer holds the workspace lock.
    DatabaseError, // Could not open the cache database.
    ScanError,     // Could not scan the workspace.
};

/**
 * Warm the AST cache for the workspace rooted at @p root.
 *
 * Locking: acquires an exclusive OS-level file lock on
 * <root>/.ast-tool/cache-warm.lock before touching the database.
 * The lock is released (and cleaned up) by the OS when the process
 * exits or the lock object is destroyed, so a crash leaves no stale lock.
 */
WarmResult warm_cache(const std::filesystem::path& root, WarmStats& stats, bool verbose);

} // namespace ast
#endif // INC_AST_CACHE_WARM_H_
