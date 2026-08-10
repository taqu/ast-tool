#ifndef INC_AST_CALL_UTILS_H_
#define INC_AST_CALL_UTILS_H_
/**
 * @file ast-call-utils.h
 * @brief Internal utilities shared by the Callers and Callees semantic services.
 *
 * This header is not part of the public API.  It lives in src/ to emphasise
 * that it is an implementation detail of the call-analysis layer.  All
 * functions are inline so the header can be included by multiple translation
 * units without ODR violations.
 */
#include "ast-ir.h"
#include "ast-scope.h"
#include "ast-workspace.h"
#include <cstring>

namespace ast
{
namespace call_utils
{

// Returns true if @p type names a call-expression node across supported languages.
inline bool is_call_expression(const char* type)
{
    if(!type) return false;
    return strcmp(type, "call_expression")   == 0  // C/C++, Go, Rust, JS/TS
        || strcmp(type, "method_invocation") == 0  // Java
        || strcmp(type, "call")              == 0; // Python, Ruby
}

// Returns true if @p type names an identifier-like AST node across supported languages.
inline bool is_identifier_type(const char* type)
{
    if(!type) return false;
    return strcmp(type, "identifier")           == 0
        || strcmp(type, "type_identifier")      == 0
        || strcmp(type, "field_identifier")     == 0
        || strcmp(type, "namespace_identifier") == 0
        || strcmp(type, "property_identifier")  == 0;
}

// Returns true when @p a and @p b refer to the same declaration.
inline bool same_declaration(const WorkspaceSymbol& a, const WorkspaceSymbol& b)
{
    return a.symbol.fqn  == b.symbol.fqn
        && a.symbol.kind == b.symbol.kind
        && a.sourceFile  == b.sourceFile
        && a.symbol.line == b.symbol.line;
}

// Returns the AST node index of the identifier that names the called function
// in a call-expression node.  Returns size_t(-1) when no identifier can be found.
//
// Handles the common callee forms:
//   Direct:    foo()        → identifier "foo"
//   Member:    obj.m()      → field_identifier "m"  (last child of field_expression)
//   Qualified: Ns::foo()    → identifier "foo"       (last child of qualified_identifier)
//   Template:  foo<T>()     → identifier "foo"       (first child of template_function)
inline size_t find_callee_identifier(const AST& ast, size_t callNodeIndex)
{
    const ASTNode& callNode = ast[callNodeIndex];
    if(callNode.children_.empty()) return size_t(-1);

    uintptr_t firstChildId = callNode.children_[0];
    if(firstChildId == InvalidId) return size_t(-1);

    size_t         calleeIdx  = static_cast<size_t>(firstChildId);
    const ASTNode& calleeNode = ast[calleeIdx];

    // Direct identifier: foo()
    if(is_identifier_type(calleeNode.type_)) return calleeIdx;

    // Field / member access: obj.method(), obj->method()
    // Take the last identifier-like child (method name, not the object).
    if(strcmp(calleeNode.type_, "field_expression")  == 0 ||
       strcmp(calleeNode.type_, "member_expression") == 0)
    {
        for(int i = static_cast<int>(calleeNode.children_.size()) - 1; i >= 0; --i) {
            uintptr_t c = calleeNode.children_[static_cast<size_t>(i)];
            if(c == InvalidId) continue;
            if(is_identifier_type(ast[static_cast<size_t>(c)].type_))
                return static_cast<size_t>(c);
        }
        return size_t(-1);
    }

    // Qualified / scoped: Ns::foo(), a::b::c()
    // Take the last identifier-like child (function name, not the qualifier).
    if(strcmp(calleeNode.type_, "qualified_identifier") == 0 ||
       strcmp(calleeNode.type_, "scoped_identifier")    == 0)
    {
        for(int i = static_cast<int>(calleeNode.children_.size()) - 1; i >= 0; --i) {
            uintptr_t c = calleeNode.children_[static_cast<size_t>(i)];
            if(c == InvalidId) continue;
            if(is_identifier_type(ast[static_cast<size_t>(c)].type_))
                return static_cast<size_t>(c);
        }
        return size_t(-1);
    }

    // Template instantiation: foo<T>(), bar<int>()
    // Take the first identifier-like child (function name, not the template argument).
    if(strcmp(calleeNode.type_, "template_function") == 0 ||
       strcmp(calleeNode.type_, "generic_function")  == 0)
    {
        for(uintptr_t c : calleeNode.children_) {
            if(c == InvalidId) continue;
            if(is_identifier_type(ast[static_cast<size_t>(c)].type_))
                return static_cast<size_t>(c);
        }
        return size_t(-1);
    }

    return size_t(-1);
}

// Returns a pointer into workspace.symbols for the first entry that matches @p sym,
// or nullptr if no match is found.
inline const WorkspaceSymbol* find_in_workspace(const Workspace& workspace,
                                                 const WorkspaceSymbol& sym)
{
    for(const WorkspaceSymbol& ws : workspace.symbols) {
        if(same_declaration(ws, sym)) return &ws;
    }
    return nullptr;
}

// Returns a pointer into workspace.symbols for the function/method/lambda that
// lexically encloses @p scopeId in @p tu, or nullptr if the call is at file scope
// or the enclosing function has no matching workspace symbol.
inline const WorkspaceSymbol* find_enclosing_caller(
    const TranslationUnit& tu,
    const Workspace& workspace,
    uintptr_t scopeId)
{
    uintptr_t current = scopeId;
    while(current != ScopeTree::InvalidId) {
        const Scope& scope = tu.scopeTree[current];
        if(scope.kind_ == ScopeKind::Function ||
           scope.kind_ == ScopeKind::Method   ||
           scope.kind_ == ScopeKind::Lambda)
        {
            size_t nodeIdx = scope.nodeIndex_;
            for(const WorkspaceSymbol& ws : workspace.symbols) {
                if(ws.sourceFile == tu.path && ws.symbol.nodeIndex == nodeIdx)
                    return &ws;
            }
            return nullptr;
        }
        current = scope.parent_;
    }
    return nullptr;
}

} // namespace call_utils
} // namespace ast
#endif // INC_AST_CALL_UTILS_H_
