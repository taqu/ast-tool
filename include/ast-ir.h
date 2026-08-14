#ifndef INC_AST_IR_H_
#define INC_AST_IR_H_
/**
 * @file ast-ir.h
 * @brief Language-agnostic AST Intermediate Representation produced by tree-sitter.
 *
 * This header is the foundation of the ast-tool layered architecture:
 *
 *   Parser (tree-sitter)
 *       ↓
 *   AST IR  ← this header
 *       ↓
 *   Semantic Layer (ast-scope*, ast-lookup, ast-extractor)
 *       ↓
 *   Workspace Analysis (ast-workspace)
 *       ↓
 *   Semantic Services (ast-search, ast-resolver, ast-references)
 *       ↓
 *   CLI / IDE / AI  (ast-tool.h)
 *
 * Every layer above this one depends on AST IR types; no layer above this one
 * should include CLI-related headers.  CLI types live exclusively in ast-tool.h.
 *
 * Tree-sitter is an implementation detail of this layer.  Higher layers never
 * include tree-sitter headers directly.
 */
#include <cstdint>
#include <string>
#include <vector>
#include <type_traits>
#include <tree_sitter/api.h>

struct TSLanguage;

namespace ast
{
    /** Sentinel value returned by AST::find() (and stored in id_/parent_/children_) when no matching node id exists. */
    static constexpr uintptr_t InvalidId = uintptr_t(-1);

/** Source languages recognized by get_language_type()/get_language(), based on file extension. */
enum class ASTLanguage : uint32_t
{
    Unknown,
    Bash,
    C,
    CPP,
    CSharp,
    CSS,
    Go,
    HTML,
    Java,
    JavaScript,
    Python,
    Ruby,
    Rust,
    Scala,
    TypeScriptX,
    TypeScript,
};

/** Bitmask flags describing an ASTNode, mirroring tree-sitter's per-node predicates. */
enum class ASTFlag : uint32_t
{
    None    = 0,
    Named   = 0x01U<<0, ///< Node is a named (non-anonymous) node.
    Missing = 0x01U<<1, ///< Node was inserted by the parser to recover from an error.
    Extra   = 0x01U<<2, ///< Node is an "extra" node (e.g. comments/whitespace) per the grammar.
    Error   = 0x01U<<3, ///< Node or one of its descendants contains a syntax error.
};

/** Bitwise OR of two ASTFlag values. */
constexpr ASTFlag operator|(ASTFlag x0, ASTFlag x1)
{
    using T = std::underlying_type_t<ASTFlag>;
    return static_cast<ASTFlag>(static_cast<T>(x0) | static_cast<T>(x1));
}

/** Bitwise AND of two ASTFlag values. */
constexpr ASTFlag operator&(ASTFlag x0, ASTFlag x1)
{
    using T = std::underlying_type_t<ASTFlag>;
    return static_cast<ASTFlag>(static_cast<T>(x0) & static_cast<T>(x1));
}

/** Bitwise NOT of an ASTFlag value. */
constexpr ASTFlag operator~(ASTFlag x)
{
    using T = std::underlying_type_t<ASTFlag>;
    return static_cast<ASTFlag>(~static_cast<T>(x));
}

/** Bitwise OR-assignment of an ASTFlag value. */
constexpr ASTFlag operator|=(ASTFlag& x0, ASTFlag x1)
{
    x0 = x0 | x1;
    return x0;
}

/** A zero-based row/column position within a source file. */
struct ASTPoint
{
    uint32_t row_;
    uint32_t column_;
};

/** A non-owning view of a slice of source text. */
struct ASTText
{
    /** Copies the referenced slice into a new std::string. */
    std::u8string getText() const;
    /** Returns true if the slice has zero length. */
    inline bool empty() const
    {
        return length_ <= 0;
    }
    uint64_t    length_; ///< Length of the slice in bytes.
    const char* text_;   ///< Pointer to the start of the slice; not null-terminated on its own.
};

/**
 * @brief A single node of a flattened AST, as produced by AST::add().
 *
 * Node relationships (id_, parent_, children_) are expressed via ids that are only
 * resolved to array indices after AST::remap_ids() has been called.
 */
struct ASTNode
{
    /** Compares type_ against @p type using exact string equality. */
    bool typeEquals(const char* type) const;
    bool typeEquals(const char8_t* type) const;
    /** Compares grammar_type_ against @p type using exact string equality. */
    bool grammarEquals(const char* type) const;
    bool grammarEquals(const char8_t* type) const;
    /** Copies this node's source text into a new std::string. */
    std::u8string getText() const;

    uintptr_t id_;     ///< This node's identifier; InvalidId if unresolved. Before remap_ids() this is the tree-sitter node id, after it is the index into the owning AST.
    uintptr_t parent_; ///< Identifier of the parent node, or InvalidId if this is the root. Same id-vs-index semantics as #id_.
    uint32_t  hash_;   ///< Hash derived from the owning file path, node type, and byte range; identifies structurally equivalent nodes.
    ASTFlag   flags_;  ///< Bitmask of ASTFlag values describing this node.

    const char* type_;         ///< tree-sitter node type name.
    const char* grammar_type_; ///< tree-sitter grammar type name.
    ASTText     text_;         ///< Source text covered by this node.

    uint32_t startByte_; ///< Start offset of this node's text, in bytes, within the source file.
    uint32_t endByte_;   ///< End offset (exclusive) of this node's text, in bytes, within the source file.

    ASTPoint start_; ///< Start row/column of this node.
    ASTPoint end_;   ///< End row/column of this node.
    std::vector<uintptr_t> children_; ///< Identifiers of child nodes. Same id-vs-index semantics as #id_.
};

/** Determines the ASTLanguage for @p path based on its file extension. Returns ASTLanguage::Unknown if @p path is null or the extension is unrecognized. */
ASTLanguage get_language_type(const char8_t* path);
ASTLanguage get_language_type_from_extension(const char8_t* extension);
ASTLanguage get_language_type_from_extension(const wchar_t* extension);
/** Returns the tree-sitter TSLanguage for @p language, or nullptr if it has no associated grammar. */
const struct TSLanguage* get_language(ASTLanguage language);

/**
 * @brief Owns a source file's text and its flattened tree-sitter AST.
 *
 * Nodes are appended via add() during traversal; once traversal is complete,
 * remap_ids() must be called to resolve node/parent/child ids into array indices
 * so that operator[] can be used to navigate the tree.
 */
class AST
{
public:
    /** Constructs an empty, invalid AST. */
    AST();
    /** Loads and reads @p path into memory, determining its language from the file extension. Check operator bool() to see whether loading succeeded. */
    AST(const char8_t* path);
    ~AST();
    AST(AST&& other);
    AST& operator=(AST&& other);

    /** Returns true if the source text was loaded successfully. */
    operator bool() const;
    /** Returns the loaded source text, or nullptr if none was loaded. */
    operator const char*() const;
    /** Returns the language detected for this AST's source file. */
    ASTLanguage language() const;
    /** Returns the size in bytes of the loaded source text. */
    int64_t text_size() const;
    /** Returns the loaded source text, or nullptr if none was loaded. */
    const char* text() const;
    /** Exchanges the contents of this AST with @p other. */
    void swap(AST& other);
    /** Appends a new ASTNode built from the given tree-sitter @p node and returns a reference to it. */
    const ASTNode& add(TSNode node);
    /** Resolves every node's id_, parent_, and children_ from tree-sitter node ids into indices into this AST. Must be called once after all nodes have been added, before using operator[] to navigate parent/child relationships. */
    void remap_ids();

    /** Returns the number of nodes in this AST. */
    uint32_t size() const;
    /** Returns the node at @p index. */
    const ASTNode& operator[](size_t index) const;
private:
    AST(const AST&) = delete;
    AST& operator=(const AST&) = delete;
    ASTText   get_string(uint32_t start, uint32_t end) const;
    uintptr_t find(uintptr_t id) const;
    bool      existsHash(uint32_t hash) const;
    void      setHash(ASTNode& node);
    ASTLanguage language_;
    uint32_t    filepath_hash_;
    int64_t     size_;
    char*       text_;
    std::vector<uintptr_t> ids_;
    std::vector<ASTNode>   nodes_;
    uint32_t collisions_;
};

/** Loads @p path, parses it with tree-sitter according to its detected language, and returns the resulting flattened, id-remapped AST. */
AST parse(const char8_t* path);

} // namespace ast
#endif // INC_AST_IR_H_
