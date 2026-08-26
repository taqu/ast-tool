#include "ast-cache.h"
#include "ast-ir.h"
#include "lz4/lz4.h"
#include <mimalloc.h>
#include <cstdint>
#include <cstring>
#include <cassert>
#include <limits>

namespace ast
{

// ── Binary I/O helpers ───────────────────────────────────────────────────────

namespace
{

struct BinaryWriter
{
    std::vector<uint8_t> buffer_;

    void write_u8(uint8_t v)  { buffer_.push_back(v); }

    void write_u16(uint16_t v) {
        buffer_.push_back((uint8_t)(v & 0xFF));
        buffer_.push_back((uint8_t)((v >> 8) & 0xFF));
    }

    void write_u32(uint32_t v) {
        buffer_.push_back((uint8_t)(v & 0xFF));
        buffer_.push_back((uint8_t)((v >> 8) & 0xFF));
        buffer_.push_back((uint8_t)((v >> 16) & 0xFF));
        buffer_.push_back((uint8_t)((v >> 24) & 0xFF));
    }

    void write_u64(uint64_t v) {
        write_u32((uint32_t)(v & 0xFFFFFFFFu));
        write_u32((uint32_t)(v >> 32));
    }

    void write_bytes(const void* p, size_t n) {
        const uint8_t* s = (const uint8_t*)p;
        buffer_.insert(buffer_.end(), s, s + n);
    }
};

struct BinaryReader
{
    const uint8_t* data_;
    size_t         size_;
    size_t         pos_ = 0;
    bool           ok_  = true;

    BinaryReader(const uint8_t* data, size_t size)
        : data_(data), size_(size)
    {}

    bool read_u8(uint8_t& v) {
        if(pos_ + 1 > size_) { ok_ = false; return false; }
        v = data_[pos_++];
        return true;
    }

    bool read_u16(uint16_t& v) {
        if(pos_ + 2 > size_) { ok_ = false; return false; }
        v = (uint16_t)data_[pos_] | ((uint16_t)data_[pos_+1] << 8);
        pos_ += 2;
        return true;
    }

    bool read_u32(uint32_t& v) {
        if(pos_ + 4 > size_) { ok_ = false; return false; }
        v = (uint32_t)data_[pos_]
          | ((uint32_t)data_[pos_+1] << 8)
          | ((uint32_t)data_[pos_+2] << 16)
          | ((uint32_t)data_[pos_+3] << 24);
        pos_ += 4;
        return true;
    }

    bool read_u64(uint64_t& v) {
        uint32_t lo, hi;
        if(!read_u32(lo) || !read_u32(hi)) return false;
        v = (uint64_t)lo | ((uint64_t)hi << 32);
        return true;
    }

    bool read_bytes(void* dst, size_t n) {
        if(pos_ + n > size_) { ok_ = false; return false; }
        memcpy(dst, data_ + pos_, n);
        pos_ += n;
        return true;
    }

    bool skip(size_t n) {
        if(pos_ + n > size_) { ok_ = false; return false; }
        pos_ += n;
        return true;
    }
};

// Encode uintptr_t (array index or InvalidId) as uint32_t.
// InvalidId → UINT32_MAX; valid indices must fit in 32 bits.
static constexpr uint32_t kSerialInvalidId = 0xFFFFFFFFu;

uint32_t encode_id(uintptr_t id) {
    if(id == InvalidId) return kSerialInvalidId;
    assert(id <= (uintptr_t)0xFFFFFFFEu);
    return (uint32_t)id;
}

uintptr_t decode_id(uint32_t raw) {
    if(raw == kSerialInvalidId) return InvalidId;
    return (uintptr_t)raw;
}

} // anonymous namespace

// ── Serialization ─────────────────────────────────────────────────────────────

std::vector<uint8_t> ast_serialize(const AST& ast)
{
    if(!ast) return {};

    const uint32_t nodeCount = ast.size();
    const int64_t  srcSize   = ast.size_ < 0 ? 0 : ast.size_;

    BinaryWriter w;
    // Rough reservation to avoid many reallocations.
    w.buffer_.reserve(32 + (size_t)srcSize + (size_t)nodeCount * 64);

    // ── Header (32 bytes) ─────────────────────────────────────────────────────
    w.write_u32(kAstCacheMagic);
    w.write_u32(kAstCacheFormatVersion);
    w.write_u32(static_cast<uint32_t>(ast.language_));
    w.write_u32(nodeCount);
    w.write_u64(static_cast<uint64_t>(srcSize));
    w.write_u32(ast.filepath_hash_);
    w.write_u32(ast.collisions_);

    // ── Source text ───────────────────────────────────────────────────────────
    if(srcSize > 0 && ast.text_ != nullptr) {
        w.write_bytes(ast.text_, (size_t)srcSize);
    }

    // ── Nodes ─────────────────────────────────────────────────────────────────
    for(uint32_t i = 0; i < nodeCount; ++i) {
        const ASTNode& n = ast.nodes_[i];
        w.write_u32(encode_id(n.id_));
        w.write_u32(encode_id(n.parent_));
        w.write_u32(n.hash_);
        w.write_u32(static_cast<uint32_t>(n.flags_));
        w.write_u16(static_cast<uint16_t>(n.type_));
        w.write_u16(static_cast<uint16_t>(n.grammar_type_));
        w.write_u32(n.startByte_);
        w.write_u32(n.endByte_);
        w.write_u32(n.start_.row_);
        w.write_u32(n.start_.column_);
        w.write_u32(n.end_.row_);
        w.write_u32(n.end_.column_);
        const uint32_t childCount = static_cast<uint32_t>(n.children_.size());
        w.write_u32(childCount);
        for(uintptr_t c : n.children_) {
            w.write_u32(encode_id(c));
        }
    }

    return std::move(w.buffer_);
}

// ── Deserialization ───────────────────────────────────────────────────────────

AST ast_deserialize(const uint8_t* data, size_t size)
{
    if(!data || size < 32) return AST{};

    BinaryReader r(data, size);

    // ── Header ────────────────────────────────────────────────────────────────
    uint32_t magic, version, langRaw, nodeCount;
    uint64_t srcSize64;
    uint32_t filepathHash, collisions;

    if(!r.read_u32(magic))        return AST{};
    if(magic != kAstCacheMagic)   return AST{};

    if(!r.read_u32(version))                    return AST{};
    if(version != kAstCacheFormatVersion)       return AST{};

    if(!r.read_u32(langRaw))  return AST{};
    if(!r.read_u32(nodeCount)) return AST{};
    if(!r.read_u64(srcSize64)) return AST{};
    if(!r.read_u32(filepathHash)) return AST{};
    if(!r.read_u32(collisions)) return AST{};

    // Sanity limits.
    if(srcSize64 > (uint64_t)512 * 1024 * 1024) return AST{}; // 512 MB max
    if(nodeCount > 10'000'000u) return AST{};

    const size_t srcSize = (size_t)srcSize64;
    const ASTLanguage lang = static_cast<ASTLanguage>(langRaw);

    // ── Source text ───────────────────────────────────────────────────────────
    char* textBuf = nullptr;
    if(srcSize > 0) {
        if(r.pos_ + srcSize > size) return AST{};
        textBuf = (char*)mi_malloc(srcSize + 1);
        if(!textBuf) return AST{};
        r.read_bytes(textBuf, srcSize);
        textBuf[srcSize] = '\0';
    }

    // ── Nodes ─────────────────────────────────────────────────────────────────
    std::vector<ASTNode> nodes;
    nodes.reserve(nodeCount);

    for(uint32_t i = 0; i < nodeCount; ++i) {
        uint32_t rawId, rawParent, hash, flagsRaw;
        uint16_t typeRaw, grammarRaw;
        uint32_t startByte, endByte;
        uint32_t startRow, startCol, endRow, endCol;
        uint32_t childCount;

        if(!r.read_u32(rawId))      goto fail;
        if(!r.read_u32(rawParent))  goto fail;
        if(!r.read_u32(hash))       goto fail;
        if(!r.read_u32(flagsRaw))   goto fail;
        if(!r.read_u16(typeRaw))    goto fail;
        if(!r.read_u16(grammarRaw)) goto fail;
        if(!r.read_u32(startByte))  goto fail;
        if(!r.read_u32(endByte))    goto fail;
        if(!r.read_u32(startRow))   goto fail;
        if(!r.read_u32(startCol))   goto fail;
        if(!r.read_u32(endRow))     goto fail;
        if(!r.read_u32(endCol))     goto fail;
        if(!r.read_u32(childCount)) goto fail;
        if(childCount > nodeCount)  goto fail; // sanity

        {
            ASTNode n;
            n.id_           = decode_id(rawId);
            n.parent_       = decode_id(rawParent);
            n.hash_         = hash;
            n.flags_        = static_cast<ASTFlag>(flagsRaw);
            n.type_         = static_cast<ASTNodeType>(typeRaw);
            n.grammar_type_ = static_cast<ASTNodeType>(grammarRaw);
            n.startByte_    = startByte;
            n.endByte_      = endByte;
            n.start_        = {startRow, startCol};
            n.end_          = {endRow, endCol};

            // Reconstruct non-owning text view into the source buffer.
            if(textBuf && endByte <= srcSize && startByte <= endByte) {
                n.text_.length_ = endByte - startByte;
                n.text_.text_   = textBuf + startByte;
            } else {
                n.text_.length_ = 0;
                n.text_.text_   = textBuf; // safe even if null
            }

            n.children_.reserve(childCount);
            for(uint32_t ci = 0; ci < childCount; ++ci) {
                uint32_t rawChild;
                if(!r.read_u32(rawChild)) goto fail;
                n.children_.push_back(decode_id(rawChild));
            }

            nodes.push_back(std::move(n));
        }
        continue;
    fail:
        mi_free(textBuf);
        return AST{};
    }

    if(!r.ok_) {
        mi_free(textBuf);
        return AST{};
    }

    // Build ids_ as identity map (nodes are already index-remapped).
    std::vector<uintptr_t> ids(nodeCount);
    for(uint32_t i = 0; i < nodeCount; ++i) ids[i] = i;

    return AST(AST::DeserializedTag{}, lang, filepathHash,
               (int64_t)srcSize, textBuf,
               std::move(ids), std::move(nodes), collisions);
}

// ── LZ4 wrappers ─────────────────────────────────────────────────────────────

std::vector<uint8_t> ast_lz4_compress(const uint8_t* data, size_t size)
{
    if(!data || size == 0) return {};

    const int32_t srcSize = (size > (size_t)0x7FFFFFFF) ? 0 : (int32_t)size;
    if(srcSize == 0) return {};

    const int32_t bound = LZ4_compressBound(srcSize);
    if(bound <= 0) return {};

    std::vector<uint8_t> out((size_t)bound);
    const int32_t compressedSize = LZ4_compress_default(
        (const char*)data, (char*)out.data(), srcSize, bound);

    if(compressedSize <= 0) return {};
    out.resize((size_t)compressedSize);
    return out;
}

std::vector<uint8_t> ast_lz4_decompress(const uint8_t* data, size_t size, size_t uncompressedSize)
{
    if(!data || size == 0 || uncompressedSize == 0) return {};

    const int32_t srcSize = (size > (size_t)0x7FFFFFFF) ? 0 : (int32_t)size;
    const int32_t dstSize = (uncompressedSize > (size_t)0x7FFFFFFF) ? 0 : (int32_t)uncompressedSize;
    if(srcSize == 0 || dstSize == 0) return {};

    std::vector<uint8_t> out((size_t)dstSize);
    const int32_t result = LZ4_decompress_safe(
        (const char*)data, (char*)out.data(), srcSize, dstSize);

    if(result != dstSize) return {};
    return out;
}

// ── Private AST deserialization constructor ───────────────────────────────────

AST::AST(DeserializedTag, ASTLanguage lang, uint32_t filepathHash,
         int64_t size, char* text,
         std::vector<uintptr_t> ids, std::vector<ASTNode> nodes, uint32_t collisions)
    : language_(lang)
    , filepath_hash_(filepathHash)
    , size_(size)
    , text_(text)
    , ids_(std::move(ids))
    , nodes_(std::move(nodes))
    , collisions_(collisions)
{
}

} // namespace ast
