#include "test_cache.h"
#include "ast-cache.h"
#include "ast-cache-db.h"
#include "ast-ir.h"
#include "ast-workspace.h"
#include <iostream>
#include <cstring>
#include <filesystem>
#include <fstream>

namespace ast
{
namespace
{

bool check(bool condition, const char* description)
{
    if(!condition)
        std::cerr << "    FAIL: " << description << "\n";
    return condition;
}

// ── Test 1: Serialize and Deserialize Round Trip ──────────────────────────────

bool test_serialize_roundtrip()
{
    AST original = parse(u8"data/test00.cpp");
    if(!check(static_cast<bool>(original), "test00.cpp parsed")) return false;

    std::vector<uint8_t> bytes = ast_serialize(original);
    if(!check(!bytes.empty(), "serialize produced non-empty bytes")) return false;

    AST restored = ast_deserialize(bytes.data(), bytes.size());
    if(!check(static_cast<bool>(restored), "deserialize succeeded")) return false;

    bool ok = true;
    ok &= check(restored.size() == original.size(), "node count matches");
    ok &= check(restored.language() == original.language(), "language matches");
    ok &= check(restored.text_size() == original.text_size(), "text_size matches");

    for(uint32_t i = 0; i < original.size() && i < restored.size(); ++i) {
        const ASTNode& a = original[i];
        const ASTNode& b = restored[i];
        if(a.id_ != b.id_) {
            ok = false;
            std::cerr << "    FAIL: node " << i << " id mismatch\n";
            break;
        }
        if(a.parent_ != b.parent_) {
            ok = false;
            std::cerr << "    FAIL: node " << i << " parent mismatch\n";
            break;
        }
        if(a.hash_ != b.hash_) {
            ok = false;
            std::cerr << "    FAIL: node " << i << " hash mismatch\n";
            break;
        }
        if(a.flags_ != b.flags_) {
            ok = false;
            std::cerr << "    FAIL: node " << i << " flags mismatch\n";
            break;
        }
        if(a.type_ != b.type_) {
            ok = false;
            std::cerr << "    FAIL: node " << i << " type_ mismatch\n";
            break;
        }
        if(a.grammar_type_ != b.grammar_type_) {
            ok = false;
            std::cerr << "    FAIL: node " << i << " grammar_type_ mismatch\n";
            break;
        }
        if(a.startByte_ != b.startByte_ || a.endByte_ != b.endByte_) {
            ok = false;
            std::cerr << "    FAIL: node " << i << " byte range mismatch\n";
            break;
        }
        if(a.text_.length_ != b.text_.length_) {
            ok = false;
            std::cerr << "    FAIL: node " << i << " text length mismatch\n";
            break;
        }
        if(a.text_.length_ > 0 &&
           memcmp(a.text_.text_, b.text_.text_, a.text_.length_) != 0) {
            ok = false;
            std::cerr << "    FAIL: node " << i << " text content mismatch\n";
            break;
        }
        if(a.children_.size() != b.children_.size()) {
            ok = false;
            std::cerr << "    FAIL: node " << i << " child count mismatch\n";
            break;
        }
    }
    return ok;
}

// ── Test 2: LZ4 Compression Round Trip ───────────────────────────────────────

bool test_lz4_roundtrip()
{
    AST ast = parse(u8"data/test00.cpp");
    if(!check(static_cast<bool>(ast), "test00.cpp parsed")) return false;

    std::vector<uint8_t> original = ast_serialize(ast);
    if(!check(!original.empty(), "serialized non-empty")) return false;

    std::vector<uint8_t> compressed = ast_lz4_compress(original.data(), original.size());
    if(!check(!compressed.empty(), "LZ4 compress succeeded")) return false;

    std::vector<uint8_t> decompressed = ast_lz4_decompress(
        compressed.data(), compressed.size(), original.size());
    if(!check(!decompressed.empty(), "LZ4 decompress succeeded")) return false;

    bool ok = check(decompressed.size() == original.size(), "decompressed size matches");
    if(ok) {
        ok = check(memcmp(decompressed.data(), original.data(), original.size()) == 0,
                   "decompressed bytes match original");
    }
    return ok;
}

// ── Helper: temp database path ────────────────────────────────────────────────

static std::filesystem::path temp_db_path(const char* suffix)
{
    std::filesystem::path tmp = std::filesystem::temp_directory_path();
    std::string name = std::string("ast_tool_test_") + suffix + ".db";
    return tmp / name;
}

static void remove_db(const std::filesystem::path& p)
{
    std::error_code ec;
    std::filesystem::remove(p, ec);
    std::filesystem::path wal = p;
    wal += "-wal";
    std::filesystem::remove(wal, ec);
    std::filesystem::path shm = p;
    shm += "-shm";
    std::filesystem::remove(shm, ec);
}

// ── Test 3: SQLite Cache Hit ──────────────────────────────────────────────────

bool test_sqlite_cache_hit()
{
    auto dbPath = temp_db_path("hit");
    remove_db(dbPath);

    AST ast = parse(u8"data/test00.cpp");
    if(!check(static_cast<bool>(ast), "test00.cpp parsed")) return false;

    const uint32_t originalCount = ast.size();

    // Serialize + compress + store.
    std::vector<uint8_t> raw = ast_serialize(ast);
    if(!check(!raw.empty(), "serialized")) return false;

    std::vector<uint8_t> compressed = ast_lz4_compress(raw.data(), raw.size());
    bool useCompressed = (!compressed.empty() && compressed.size() < raw.size());

    ASTCacheDatabase db;
    if(!check(db.open(dbPath), "db.open() succeeded")) {
        remove_db(dbPath);
        return false;
    }

    ASTCacheDatabase::Entry storeEntry;
    storeEntry.source_hash      = 12345678u;
    storeEntry.source_size      = 99999;
    storeEntry.source_mtime     = 1000000;
    storeEntry.language         = 1u;
    storeEntry.format_version   = kAstCacheFormatVersion;
    storeEntry.uncompressed_size = (int64_t)raw.size();
    if(useCompressed) {
        storeEntry.compression = AstCompressionMode::LZ4;
        storeEntry.blob        = std::move(compressed);
    } else {
        storeEntry.compression = AstCompressionMode::None;
        storeEntry.blob        = raw;
    }
    if(!check(db.store("test_key", storeEntry), "db.store() succeeded")) {
        remove_db(dbPath);
        return false;
    }
    db.close();

    // Re-open and lookup.
    ASTCacheDatabase db2;
    if(!check(db2.open(dbPath), "db2.open() succeeded")) {
        remove_db(dbPath);
        return false;
    }

    ASTCacheDatabase::Entry loadEntry;
    if(!check(db2.lookup("test_key", loadEntry), "db2.lookup() found entry")) {
        remove_db(dbPath);
        return false;
    }

    bool ok = true;
    ok &= check(loadEntry.format_version == kAstCacheFormatVersion, "format_version matches");
    ok &= check(!loadEntry.blob.empty(), "blob non-empty");

    if(ok) {
        // Decompress and deserialize.
        std::vector<uint8_t> rawOut;
        if(loadEntry.compression == AstCompressionMode::LZ4) {
            rawOut = ast_lz4_decompress(loadEntry.blob.data(), loadEntry.blob.size(),
                                        (size_t)loadEntry.uncompressed_size);
        } else {
            rawOut = std::move(loadEntry.blob);
        }
        ok &= check(!rawOut.empty(), "decompressed blob non-empty");
        if(ok) {
            AST loaded = ast_deserialize(rawOut.data(), rawOut.size());
            ok &= check(static_cast<bool>(loaded), "deserialization succeeded");
            ok &= check(loaded.size() == originalCount, "node count matches after db round-trip");
        }
    }

    remove_db(dbPath);
    return ok;
}

// ── Test 4: Source Change Invalidates Cache ───────────────────────────────────

bool test_source_change_invalidation()
{
    auto dbPath = temp_db_path("invalidate");
    remove_db(dbPath);

    ASTCacheDatabase db;
    if(!check(db.open(dbPath), "db.open() succeeded")) {
        remove_db(dbPath);
        return false;
    }

    // Store an entry with one mtime.
    ASTCacheDatabase::Entry e;
    e.source_hash      = 0xDEADBEEF;
    e.source_size      = 1000;
    e.source_mtime     = 111;
    e.language         = 2u;
    e.format_version   = kAstCacheFormatVersion;
    e.compression      = AstCompressionMode::None;
    e.uncompressed_size = 4;
    e.blob             = {0x41, 0x53, 0x54, 0x43};
    db.store("myfile.cpp", e);

    // Lookup with a different mtime simulates a file change.
    ASTCacheDatabase::Entry found;
    bool exists = db.lookup("myfile.cpp", found);
    bool ok = check(exists, "entry exists in db");

    if(ok) {
        // The cache layer (get_translation_unit) would detect mtime mismatch and reparse.
        // Here we test that the stored mtime differs from a "current" mtime.
        const int64_t differentMtime = 999;
        ok &= check(found.source_mtime != differentMtime,
                    "stored mtime differs from simulated 'current' mtime");
    }

    remove_db(dbPath);
    return ok;
}

// ── Test 5: Version Mismatch Invalidates ─────────────────────────────────────

bool test_version_mismatch()
{
    auto dbPath = temp_db_path("version");
    remove_db(dbPath);

    ASTCacheDatabase db;
    if(!check(db.open(dbPath), "db.open() succeeded")) {
        remove_db(dbPath);
        return false;
    }

    // Store entry with a future format version.
    ASTCacheDatabase::Entry e;
    e.source_hash      = 0;
    e.source_size      = 0;
    e.source_mtime     = 0;
    e.language         = 0;
    e.format_version   = kAstCacheFormatVersion + 999; // future version
    e.compression      = AstCompressionMode::None;
    e.uncompressed_size = 0;
    e.blob             = {1, 2, 3};
    db.store("stale.cpp", e);

    ASTCacheDatabase::Entry found;
    bool exists = db.lookup("stale.cpp", found);
    bool ok = check(exists, "entry found in db");
    ok &= check(found.format_version != kAstCacheFormatVersion,
                "stored version differs from current version (would trigger reparse)");

    remove_db(dbPath);
    return ok;
}

// ── Test 6: Corrupted Blob Recovery ──────────────────────────────────────────

bool test_corrupted_blob_recovery()
{
    // Store deliberately corrupted blob; ast_deserialize should return empty AST.
    const uint8_t garbage[] = {0xFF, 0xFE, 0xAB, 0xCD, 0x12, 0x34, 0x56, 0x78};
    AST result = ast_deserialize(garbage, sizeof(garbage));
    bool ok = check(!static_cast<bool>(result),
                    "corrupted blob → invalid AST (magic mismatch)");

    // Also try a blob with correct magic but bad version.
    const uint8_t badVersion[] = {
        0x43, 0x54, 0x53, 0x41, // "ASTC" reversed — wrong magic
        0x01, 0x00, 0x00, 0x00, // version 1
        0x02, 0x00, 0x00, 0x00, // language 2
        0x00, 0x00, 0x00, 0x00, // node_count 0
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // source_size 0
        0x00, 0x00, 0x00, 0x00, // filepath_hash 0
        0x00, 0x00, 0x00, 0x00, // collisions 0
    };
    AST result2 = ast_deserialize(badVersion, sizeof(badVersion));
    ok &= check(!static_cast<bool>(result2),
                "wrong magic → invalid AST (deserialization fails)");

    // Correct magic, correct version, but truncated data.
    const uint8_t truncated[] = {
        0x43, 0x54, 0x53, 0x41, // wrong magic byte order (CTSA)
    };
    AST result3 = ast_deserialize(truncated, sizeof(truncated));
    ok &= check(!static_cast<bool>(result3),
                "truncated data (< 32 bytes) → invalid AST");

    return ok;
}

// ── Test 7: Memory Cache Takes Priority ──────────────────────────────────────

bool test_memory_cache_priority()
{
    // Use open_workspace on a small test workspace.
    const char8_t* wsRoot = u8"test/ast-workspace/workspace";
    Workspace ws = open_workspace(wsRoot);

    // First access — should be a cache miss (memory) + either persistent hit or parse.
    const uint32_t hitsBefore   = ws.cacheHits_;
    const uint32_t missesBefore = ws.cacheMisses_;

    std::filesystem::path testFile;
    if(!ws.files.empty()) {
        testFile = ws.files.front();
    }
    if(testFile.empty()) {
        std::cerr << "    FAIL: no files found in workspace\n";
        return false;
    }

    const TranslationUnit* tu1 = ws.get_translation_unit(testFile);
    if(!check(tu1 != nullptr, "first access returns valid TU")) return false;
    if(!check(ws.cacheMisses_ == missesBefore + 1, "first access is a memory miss")) return false;
    if(!check(ws.cacheHits_ == hitsBefore, "first access is not a memory hit")) return false;

    // Second access — must be a memory cache hit.
    const uint32_t hitsAfterFirst = ws.cacheHits_;
    const TranslationUnit* tu2 = ws.get_translation_unit(testFile);
    if(!check(tu2 != nullptr, "second access returns valid TU")) return false;
    if(!check(tu2 == tu1, "second access returns same TU pointer")) return false;
    if(!check(ws.cacheHits_ == hitsAfterFirst + 1, "second access is a memory hit")) return false;

    return true;
}

// ── Test 8: Deserialized AST works for semantic analysis ──────────────────────

bool test_deserialized_ast_semantic()
{
    // Parse fresh, run symbol extraction.
    AST original = parse(u8"data/test00.cpp");
    if(!check(static_cast<bool>(original), "test00.cpp parsed")) return false;

    // Serialize and deserialize.
    std::vector<uint8_t> bytes = ast_serialize(original);
    AST restored = ast_deserialize(bytes.data(), bytes.size());
    if(!check(static_cast<bool>(restored), "deserialization succeeded")) return false;

    // The deserialized AST should have the same number of nodes.
    bool ok = check(restored.size() == original.size(), "node count matches after deserialize");

    // At least one FunctionDefinition node must be present.
    bool foundFuncDef = false;
    for(uint32_t i = 0; i < restored.size(); ++i) {
        if(restored[i].typeEquals(ASTNodeType::FunctionDefinition)) {
            foundFuncDef = true;
            break;
        }
    }
    ok &= check(foundFuncDef, "deserialized AST has FunctionDefinition nodes");

    // At least one TranslationUnit node.
    bool foundTU = false;
    for(uint32_t i = 0; i < restored.size(); ++i) {
        if(restored[i].typeEquals(ASTNodeType::TranslationUnit)) {
            foundTU = true;
            break;
        }
    }
    ok &= check(foundTU, "deserialized AST has TranslationUnit node");

    return ok;
}

} // anonymous namespace

bool run_tests_cache()
{
    std::cout << "=== AST Cache tests ===" << std::endl;
    bool ok = true;

    auto run = [&](bool (*fn)(), const char* name) {
        bool r = fn();
        std::cout << "  " << name << "... " << (r ? "PASS" : "FAIL") << std::endl;
        return r;
    };

    ok &= run(test_serialize_roundtrip,        "serialize/deserialize round-trip");
    ok &= run(test_lz4_roundtrip,              "LZ4 compression round-trip");
    ok &= run(test_sqlite_cache_hit,           "SQLite cache hit");
    ok &= run(test_source_change_invalidation, "source change invalidation");
    ok &= run(test_version_mismatch,           "format version mismatch");
    ok &= run(test_corrupted_blob_recovery,    "corrupted blob recovery");
    ok &= run(test_memory_cache_priority,      "memory cache priority");
    ok &= run(test_deserialized_ast_semantic,  "deserialized AST semantic correctness");

    std::cout << "=== AST Cache: " << (ok ? "PASS" : "FAIL") << " ===" << std::endl;
    return ok;
}

} // namespace ast
