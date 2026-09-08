#ifndef INC_AST_CACHE_DB_H_
#define INC_AST_CACHE_DB_H_

#include "ast-cache.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

struct sqlite3;

namespace ast
{

/**
 * @brief SQLite-backed persistent AST cache.
 *
 * Schema (one row per source file):
 *   path TEXT PRIMARY KEY
 *   source_hash BLOB (8 bytes, XXH64 of source content)
 *   source_size INTEGER
 *   source_mtime INTEGER
 *   language INTEGER
 *   format_version INTEGER
 *   compression INTEGER (0=None, 1=LZ4)
 *   uncompressed_size INTEGER
 *   compressed_size INTEGER
 *   ast_blob BLOB
 */
class ASTCacheDatabase
{
public:
    ASTCacheDatabase() = default;
    ~ASTCacheDatabase();
    ASTCacheDatabase(ASTCacheDatabase&&) noexcept;
    ASTCacheDatabase& operator=(ASTCacheDatabase&&) noexcept;
    ASTCacheDatabase(const ASTCacheDatabase&) = delete;
    ASTCacheDatabase& operator=(const ASTCacheDatabase&) = delete;

    bool open(const std::filesystem::path& dbPath);
    bool open_readonly(const std::filesystem::path& dbPath);
    void close();
    bool is_open() const noexcept { return db_ != nullptr; }

    bool begin_transaction();
    bool commit_transaction();
    bool rollback_transaction();

    struct Entry
    {
        uint64_t           source_hash       = 0;
        int64_t            source_size       = 0;
        int64_t            source_mtime      = 0;
        uint32_t           language          = 0;
        uint32_t           format_version    = 0;
        AstCompressionMode compression       = AstCompressionMode::None;
        int64_t            uncompressed_size = 0;
        std::vector<uint8_t> blob;
    };

    // Retrieve cached entry for `key`. Returns true on success.
    bool lookup(const std::string& key, Entry& out) const;

    // Insert or replace the cache entry for `key`.
    bool store(const std::string& key, const Entry& e);

    // Delete the cache entry for `key`.
    void remove(const std::string& key);

    // Lightweight metadata-only entry (no blob) for cache validation without blob I/O.
    struct Metadata
    {
        uint64_t source_hash    = 0;
        int64_t  source_size    = 0;
        int64_t  source_mtime   = 0;
        uint32_t language       = 0;
        uint32_t format_version = 0;
    };

    // Retrieve only cache metadata for `key` (no AST blob). Returns true on success.
    bool lookup_metadata(const std::string& key, Metadata& out) const;

    // Return all cached paths (for stale-entry cleanup).
    std::vector<std::string> list_all_paths() const;

    // Update only mtime/size in an existing entry (no blob load/store).
    bool update_mtime_size(const std::string& key, int64_t newSize, int64_t newMtime);

    // Return the number of cached entries and database file size in bytes.
    int64_t entry_count() const;
    int64_t db_size_bytes(const std::filesystem::path& dbPath) const;

private:
    sqlite3* db_ = nullptr;
    bool init_schema();
};

} // namespace ast
#endif // INC_AST_CACHE_DB_H_
