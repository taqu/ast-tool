#include "ast-cache-db.h"
#include "sqlite/sqlite3.h"
#include <cstdio>
#include <cstring>

namespace ast
{

// ── Lifecycle ─────────────────────────────────────────────────────────────────

ASTCacheDatabase::~ASTCacheDatabase()
{
    close();
}

ASTCacheDatabase::ASTCacheDatabase(ASTCacheDatabase&& other) noexcept
    : db_(other.db_)
{
    other.db_ = nullptr;
}

ASTCacheDatabase& ASTCacheDatabase::operator=(ASTCacheDatabase&& other) noexcept
{
    if(this != &other) {
        close();
        db_ = other.db_;
        other.db_ = nullptr;
    }
    return *this;
}

bool ASTCacheDatabase::open(const std::filesystem::path& dbPath)
{
    close();
    std::string path = dbPath.string();
    int rc = sqlite3_open(path.c_str(), &db_);
    if(rc != SQLITE_OK) {
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }
    // Configure for single-writer, multiple-reader concurrent safety.
    sqlite3_exec(db_, "PRAGMA journal_mode=WAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA synchronous=NORMAL;", nullptr, nullptr, nullptr);
    sqlite3_exec(db_, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr);
    return init_schema();
}

bool ASTCacheDatabase::open_readonly(const std::filesystem::path& dbPath)
{
    close();
    std::string path = dbPath.string();
    int rc = sqlite3_open_v2(path.c_str(), &db_,
                             SQLITE_OPEN_READONLY | SQLITE_OPEN_NOMUTEX, nullptr);
    if(rc != SQLITE_OK) {
        sqlite3_close(db_);
        db_ = nullptr;
        return false;
    }
    sqlite3_exec(db_, "PRAGMA busy_timeout=5000;", nullptr, nullptr, nullptr);
    return true;
}

bool ASTCacheDatabase::begin_transaction()
{
    if(!db_) return false;
    return sqlite3_exec(db_, "BEGIN;", nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool ASTCacheDatabase::commit_transaction()
{
    if(!db_) return false;
    return sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr) == SQLITE_OK;
}

bool ASTCacheDatabase::rollback_transaction()
{
    if(!db_) return false;
    return sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr) == SQLITE_OK;
}

void ASTCacheDatabase::close()
{
    if(db_) {
        sqlite3_close(db_);
        db_ = nullptr;
    }
}

bool ASTCacheDatabase::init_schema()
{
    const char* sql =
        "CREATE TABLE IF NOT EXISTS ast_cache ("
        "  path             TEXT    PRIMARY KEY,"
        "  source_hash      BLOB    NOT NULL,"
        "  source_size      INTEGER NOT NULL,"
        "  source_mtime     INTEGER NOT NULL,"
        "  language         INTEGER NOT NULL,"
        "  format_version   INTEGER NOT NULL,"
        "  compression      INTEGER NOT NULL DEFAULT 0,"
        "  uncompressed_size INTEGER NOT NULL,"
        "  compressed_size  INTEGER NOT NULL,"
        "  ast_blob         BLOB    NOT NULL"
        ");";
    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, sql, nullptr, nullptr, &errmsg);
    if(rc != SQLITE_OK) {
        sqlite3_free(errmsg);
        return false;
    }
    return true;
}

// ── Lookup ────────────────────────────────────────────────────────────────────

bool ASTCacheDatabase::lookup(const std::string& key, Entry& out) const
{
    if(!db_)
        return false;

    const char* sql =
        "SELECT source_hash, source_size, source_mtime, language,"
        "       format_version, compression, uncompressed_size,"
        "       compressed_size, ast_blob"
        " FROM ast_cache WHERE path = ?;";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);

    bool found = false;
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        // source_hash: 8 bytes
        const void* hashBlob = sqlite3_column_blob(stmt, 0);
        int hashBytes = sqlite3_column_bytes(stmt, 0);
        if(hashBlob && hashBytes == 8) {
            memcpy(&out.source_hash, hashBlob, 8);
        }
        out.source_size = sqlite3_column_int64(stmt, 1);
        out.source_mtime = sqlite3_column_int64(stmt, 2);
        out.language = (uint32_t)sqlite3_column_int64(stmt, 3);
        out.format_version = (uint32_t)sqlite3_column_int64(stmt, 4);
        out.compression = (AstCompressionMode)(uint8_t)sqlite3_column_int64(stmt, 5);
        out.uncompressed_size = sqlite3_column_int64(stmt, 6);
        // compressed_size from column 7 (we reconstruct from blob size)
        const void* blobData = sqlite3_column_blob(stmt, 8);
        int blobBytes = sqlite3_column_bytes(stmt, 8);
        if(blobData && blobBytes > 0) {
            out.blob.assign((const uint8_t*)blobData,
                            (const uint8_t*)blobData + blobBytes);
        }
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

// ── Store ─────────────────────────────────────────────────────────────────────

bool ASTCacheDatabase::store(const std::string& key, const Entry& e)
{
    if(!db_) {
        return false;
    }

    const char* sql =
        "INSERT OR REPLACE INTO ast_cache"
        " (path, source_hash, source_size, source_mtime, language,"
        "  format_version, compression, uncompressed_size, compressed_size, ast_blob)"
        " VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if(rc != SQLITE_OK) {
        return false;
    }

    bool manage_txn = (sqlite3_get_autocommit(db_) != 0);
    if(manage_txn)
        sqlite3_exec(db_, "BEGIN;", nullptr, nullptr, nullptr);

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_blob(stmt, 2, &e.source_hash, 8, SQLITE_STATIC);
    sqlite3_bind_int64(stmt, 3, e.source_size);
    sqlite3_bind_int64(stmt, 4, e.source_mtime);
    sqlite3_bind_int64(stmt, 5, (int64_t)e.language);
    sqlite3_bind_int64(stmt, 6, (int64_t)e.format_version);
    sqlite3_bind_int64(stmt, 7, (int64_t)e.compression);
    sqlite3_bind_int64(stmt, 8, e.uncompressed_size);
    sqlite3_bind_int64(stmt, 9, (int64_t)e.blob.size());
    if(!e.blob.empty()) {
        sqlite3_bind_blob(stmt, 10, e.blob.data(), (int)e.blob.size(), SQLITE_STATIC);
    } else {
        sqlite3_bind_null(stmt, 10);
    }

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);

    if(manage_txn) {
        if(ok) sqlite3_exec(db_, "COMMIT;", nullptr, nullptr, nullptr);
        else   sqlite3_exec(db_, "ROLLBACK;", nullptr, nullptr, nullptr);
    }
    return ok;
}

// ── Metadata-only lookup ──────────────────────────────────────────────────────

bool ASTCacheDatabase::lookup_metadata(const std::string& key, Metadata& out) const
{
    if(!db_)
        return false;

    const char* sql =
        "SELECT source_hash, source_size, source_mtime, language, format_version"
        " FROM ast_cache WHERE path = ?;";

    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);

    bool found = false;
    if(sqlite3_step(stmt) == SQLITE_ROW) {
        const void* hashBlob = sqlite3_column_blob(stmt, 0);
        int hashBytes = sqlite3_column_bytes(stmt, 0);
        if(hashBlob && hashBytes == 8)
            memcpy(&out.source_hash, hashBlob, 8);
        out.source_size    = sqlite3_column_int64(stmt, 1);
        out.source_mtime   = sqlite3_column_int64(stmt, 2);
        out.language       = (uint32_t)sqlite3_column_int64(stmt, 3);
        out.format_version = (uint32_t)sqlite3_column_int64(stmt, 4);
        found = true;
    }

    sqlite3_finalize(stmt);
    return found;
}

// ── List all paths ────────────────────────────────────────────────────────────

std::vector<std::string> ASTCacheDatabase::list_all_paths() const
{
    std::vector<std::string> result;
    if(!db_)
        return result;

    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(db_, "SELECT path FROM ast_cache;", -1, &stmt, nullptr) != SQLITE_OK)
        return result;

    while(sqlite3_step(stmt) == SQLITE_ROW) {
        const char* p = (const char*)sqlite3_column_text(stmt, 0);
        if(p)
            result.emplace_back(p);
    }
    sqlite3_finalize(stmt);
    return result;
}

// ── Metadata update ───────────────────────────────────────────────────────────

bool ASTCacheDatabase::update_mtime_size(const std::string& key, int64_t newSize, int64_t newMtime)
{
    if(!db_)
        return false;

    const char* sql =
        "UPDATE ast_cache SET source_size = ?, source_mtime = ? WHERE path = ?;";
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int64(stmt, 1, newSize);
    sqlite3_bind_int64(stmt, 2, newMtime);
    sqlite3_bind_text(stmt, 3, key.c_str(), -1, SQLITE_STATIC);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt);
    return ok;
}

// ── Aggregate queries ─────────────────────────────────────────────────────────

int64_t ASTCacheDatabase::entry_count() const
{
    if(!db_)
        return 0;
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(db_, "SELECT COUNT(*) FROM ast_cache;", -1, &stmt, nullptr) != SQLITE_OK)
        return 0;
    int64_t n = 0;
    if(sqlite3_step(stmt) == SQLITE_ROW)
        n = sqlite3_column_int64(stmt, 0);
    sqlite3_finalize(stmt);
    return n;
}

int64_t ASTCacheDatabase::db_size_bytes(const std::filesystem::path& dbPath) const
{
    std::error_code ec;
    auto sz = std::filesystem::file_size(dbPath, ec);
    return ec ? -1 : (int64_t)sz;
}

// ── Remove ────────────────────────────────────────────────────────────────────

void ASTCacheDatabase::remove(const std::string& key)
{
    if(!db_) {
        return;
    }
    sqlite3_stmt* stmt = nullptr;
    if(sqlite3_prepare_v2(db_, "DELETE FROM ast_cache WHERE path = ?;", -1, &stmt, nullptr) != SQLITE_OK) {
        return;
    }
    sqlite3_bind_text(stmt, 1, key.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

} // namespace ast
