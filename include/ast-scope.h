#ifndef INC_AST_SCOPE_H_
#define INC_AST_SCOPE_H_
/**
 * @file ast-scope.h
 * @brief Language-independent lexical scope model for the AST library.
 *
 * Provides the Scope data structure, the ScopeKind enumeration, and the
 * ScopeTree container that owns all scopes for a single translation unit.
 *
 * This module defines the data model only. Scope construction, name
 * resolution, and visible-symbol computation are left to future layers.
 */
#include <cstddef>
#include <cstdint>
#include <vector>

namespace ast
{

/** Category of a lexical scope, language-independently named. */
enum class ScopeKind : uint32_t
{
    Global,      ///< The outermost translation-unit scope.
    Namespace,   ///< A named or anonymous namespace (C++, Python packages, etc.).
    Module,      ///< A module or package boundary.
    Class,       ///< A class body.
    Struct,      ///< A struct body.
    Enum,        ///< An enum or enum class body.
    Function,    ///< A free function body.
    Method,      ///< A member-function body.
    Lambda,      ///< An anonymous function or closure.
    Block,       ///< A bare braced block or compound statement.
    Conditional, ///< The body of an if/else branch.
    Loop,        ///< The body of a for/while/do loop.
    Switch,      ///< A switch statement body.
    Unknown,     ///< Scope kind could not be determined.
};

/** Returns a human-readable name for @p kind (e.g. "Class", "Loop"), or "Unknown" if unrecognized. */
const char* getScopeKindName(ScopeKind kind);

/**
 * @brief A single lexical scope within a translation unit.
 *
 * Scopes are owned by a ScopeTree. All IDs stored in this struct are indices
 * into the owning ScopeTree (for scope IDs) or into the source AST (for
 * nodeIndex_). They are not valid across different ScopeTree instances.
 */
struct Scope
{
    uintptr_t id_;      ///< Unique identifier; equals this scope's index in the owning ScopeTree.
    ScopeKind kind_;    ///< Category of this lexical scope.

    size_t   nodeIndex_; ///< Index of the owning ASTNode in the source AST, or ScopeTree::InvalidNodeIndex if none.
    uint32_t startByte_; ///< First byte of this scope's source range (inclusive).
    uint32_t endByte_;   ///< One-past-last byte of this scope's source range (exclusive).

    uintptr_t parent_; ///< ID of the directly enclosing scope, or ScopeTree::InvalidId for the root.

    std::vector<uintptr_t> children_; ///< IDs of directly nested child scopes, in insertion order.
    std::vector<size_t>    symbols_;  ///< Indices of symbols declared in this scope (into an external symbol list).
};

/**
 * @brief Owns and indexes all Scope%s belonging to a single translation unit.
 *
 * Scopes are stored in a flat vector; a scope's ID equals its position in
 * that vector, enabling O(1) lookup by ID.  Lookup by AST node index and by
 * symbol index are provided; their complexity is currently O(N) and may be
 * improved by future implementations without changing the public interface.
 */
class ScopeTree
{
public:
    /** Sentinel scope ID meaning "no scope". */
    static constexpr uintptr_t InvalidId = uintptr_t(-1);
    /** Sentinel node index meaning "no associated AST node". */
    static constexpr size_t InvalidNodeIndex = size_t(-1);

    ScopeTree() = default;

    /**
     * @brief Appends a new Scope and registers it as a child of @p parentId.
     * @param kind       Category of the new scope.
     * @param nodeIndex  Index of the owning ASTNode, or InvalidNodeIndex.
     * @param startByte  Inclusive start byte of the scope's source range.
     * @param endByte    Exclusive end byte of the scope's source range.
     * @param parentId   ID of the enclosing scope, or InvalidId for a root scope.
     * @return The new scope's ID.
     */
    uintptr_t add(ScopeKind kind, size_t nodeIndex, uint32_t startByte, uint32_t endByte, uintptr_t parentId = InvalidId);

    /**
     * @brief Associates a symbol index with a scope.
     * @param scopeId     ID of the scope that declares the symbol.
     * @param symbolIndex Index of the symbol in an external symbol list.
     */
    void addSymbol(uintptr_t scopeId, size_t symbolIndex);

    /** Returns the number of scopes in this tree. */
    uint32_t size() const;

    /** Returns the scope at @p id. Behaviour is undefined if @p id >= size(). */
    Scope&       operator[](uintptr_t id);
    const Scope& operator[](uintptr_t id) const;

    /**
     * @brief Returns the ID of the first scope whose nodeIndex_ equals @p nodeIndex.
     * @return The matching scope's ID, or InvalidId if none is found.
     * @note O(N) linear scan. Use getNodeScope() for O(1) lookup after build_scope_tree().
     */
    uintptr_t findByNodeIndex(size_t nodeIndex) const;

    /**
     * @brief Returns the ID of the first scope that contains @p symbolIndex in its symbols_ list.
     * @return The matching scope's ID, or InvalidId if none is found.
     * @note O(N) linear scan. Use getScopeOfSymbol() for O(1) lookup after associate_symbols().
     */
    uintptr_t findBySymbol(size_t symbolIndex) const;

    /**
     * @brief Pre-allocates the symbol-to-scope reverse map for @p symbolCount symbols.
     *
     * Call this before setScopeOfSymbol() when the total symbol count is known.
     * All entries are initialized to InvalidId.
     */
    void reserveSymbolMap(size_t symbolCount);

    /**
     * @brief Records which scope owns symbol @p symbolIndex.
     * @param symbolIndex  Index into the external symbol list.
     * @param scopeId      ID of the owning scope.
     */
    void setScopeOfSymbol(size_t symbolIndex, uintptr_t scopeId);

    /**
     * @brief Returns the ID of the scope that owns symbol @p symbolIndex.
     * @return The scope's ID, or InvalidId if the symbol index is out of range or unmapped.
     * @note O(1) after reserveSymbolMap() and setScopeOfSymbol() have been called.
     */
    uintptr_t getScopeOfSymbol(size_t symbolIndex) const;

    /**
     * @brief Pre-allocates the node-to-scope map for @p nodeCount AST nodes.
     *
     * Call this before setNodeScope() when the total node count is known.
     * All entries are initialized to InvalidId.
     */
    void reserveNodeMap(size_t nodeCount);

    /**
     * @brief Records the innermost scope containing AST node @p nodeIndex.
     * @param nodeIndex  Index into the source AST.
     * @param scopeId    ID of the innermost scope that contains this node.
     */
    void setNodeScope(size_t nodeIndex, uintptr_t scopeId);

    /**
     * @brief Returns the innermost scope containing AST node @p nodeIndex.
     * @return The scope's ID, or InvalidId if the node index is out of range or unmapped.
     * @note O(1) after reserveNodeMap() and setNodeScope() have been called.
     */
    uintptr_t getNodeScope(size_t nodeIndex) const;

private:
    std::vector<Scope>     scopes_;
    std::vector<uintptr_t> nodeScope_;   ///< nodeScope_[i] is the innermost scope ID for AST node i.
    std::vector<uintptr_t> symbolScope_; ///< symbolScope_[i] is the scope ID owning symbol i.
};

} // namespace ast
#endif // INC_AST_SCOPE_H_
