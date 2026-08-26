#ifndef INC_AST_CACHE_H_
#define INC_AST_CACHE_H_
/**
 * @file ast-cache.h
 * @brief Persistent binary AST cache — serialization, compression, and SQLite storage.
 *
 * Lookup order within the workspace:
 *   1. In-memory cache (Workspace::tuIndex_)
 *   2. SQLite persistent cache (ASTCacheDatabase)
 *   3. Tree-sitter parse
 *
 * Binary format:
 *   Header (32 bytes): magic, version, language, node_count, source_size, filepath_hash, collisions
 *   Source text: source_size bytes (raw)
 *   Nodes: node_count × (fixed 48 bytes + 4 bytes per child)
 *
 * ASTText is reconstructed from startByte_/endByte_ into the deserialized source buffer;
 * no process-local pointers are stored in the serialized form.
 */
#include "ast-ir.h"
#include <cstdint>
#include <vector>

namespace ast
{

static constexpr uint32_t kAstCacheMagic         = 0x41535443u; // "ASTC" LE
static constexpr uint32_t kAstCacheFormatVersion = 1u;

enum class AstCompressionMode : uint8_t
{
    None = 0,
    LZ4  = 1,
};

// Serialize an AST to an uncompressed binary buffer.
// Returns an empty vector if the AST is invalid.
std::vector<uint8_t> ast_serialize(const AST& ast);

// Deserialize an AST from a binary buffer produced by ast_serialize.
// Returns an invalid (empty) AST on any error (corrupt data, version mismatch, bounds error).
AST ast_deserialize(const uint8_t* data, size_t size);

// Compress data with LZ4. Returns an empty vector on failure.
std::vector<uint8_t> ast_lz4_compress(const uint8_t* data, size_t size);

// Decompress LZ4-compressed data. uncompressedSize must be the exact original size.
// Returns an empty vector on failure.
std::vector<uint8_t> ast_lz4_decompress(const uint8_t* data, size_t size, size_t uncompressedSize);

} // namespace ast
#endif // INC_AST_CACHE_H_
