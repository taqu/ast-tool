#include "ast-extractor-langs.h"
#include "ast-extractor-common.h"
#include "ast-ir.h"
#include <initializer_list>
#include <unordered_set>

namespace ast
{
namespace extractor
{
namespace
{
    // ── Node type string constants (tree-sitter-c-sharp grammar) ────────────
    constexpr const char* k_namespace_declaration            = "namespace_declaration";
    constexpr const char* k_file_scoped_namespace_declaration = "file_scoped_namespace_declaration";
    constexpr const char* k_class_declaration                = "class_declaration";
    constexpr const char* k_struct_declaration                = "struct_declaration";
    constexpr const char* k_interface_declaration              = "interface_declaration";
    constexpr const char* k_record_declaration                = "record_declaration";
    constexpr const char* k_enum_declaration                  = "enum_declaration";
    constexpr const char* k_enum_member_declaration            = "enum_member_declaration";
    constexpr const char* k_enum_member_declaration_list       = "enum_member_declaration_list";
    constexpr const char* k_delegate_declaration               = "delegate_declaration";
    constexpr const char* k_method_declaration                 = "method_declaration";
    constexpr const char* k_constructor_declaration            = "constructor_declaration";
    constexpr const char* k_destructor_declaration             = "destructor_declaration";
    constexpr const char* k_field_declaration                  = "field_declaration";
    constexpr const char* k_property_declaration               = "property_declaration";
    constexpr const char* k_event_declaration                  = "event_declaration";
    constexpr const char* k_event_field_declaration            = "event_field_declaration";
    constexpr const char* k_operator_declaration                = "operator_declaration";
    constexpr const char* k_conversion_operator_declaration    = "conversion_operator_declaration";
    constexpr const char* k_local_function_statement           = "local_function_statement";
    constexpr const char* k_using_directive                    = "using_directive";
    constexpr const char* k_variable_declaration                = "variable_declaration";
    constexpr const char* k_variable_declarator                 = "variable_declarator";
    constexpr const char* k_modifier                            = "modifier";
    constexpr const char* k_identifier                          = "identifier";
    constexpr const char* k_declaration_list                    = "declaration_list";
    constexpr const char* k_parameter_list                      = "parameter_list";
    constexpr const char* k_accessor_list                       = "accessor_list";
    constexpr const char* k_arrow_expression_clause             = "arrow_expression_clause";
    constexpr const char* k_qualified_name                      = "qualified_name";
    constexpr const char* k_generic_name                        = "generic_name";
    constexpr const char* k_alias_qualified_name                = "alias_qualified_name";
    constexpr const char* k_operator_kw                         = "operator";

    // ── Low-level helpers ─────────────────────────────────────────────────────

    bool isNameLike(const ast::ASTNode& n)
    {
        return n.grammarEquals(k_identifier)
            || n.typeEquals(k_qualified_name)
            || n.typeEquals(k_generic_name)
            || n.typeEquals(k_alias_qualified_name);
    }

    // Returns the LAST name-like direct child of `node` that appears before any
    // child whose type is in `stopTypes`. Declarations place their declared name
    // immediately before the parameter list / body, while a preceding return
    // type (also an identifier/qualified_name/generic_name in this grammar) may
    // appear earlier - taking the last match instead of the first sidesteps that
    // ambiguity without having to classify type nodes.
    std::string getLastNameBefore(const ast::AST& tree, const ast::ASTNode& node,
                                   std::initializer_list<const char*> stopTypes)
    {
        std::string last;
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            bool stop = false;
            for(const char* st : stopTypes) {
                if(child.typeEquals(st)) { stop = true; break; }
            }
            if(stop) break;
            if(isNameLike(child)) last = child.getText();
        }
        return last;
    }

    bool hasModifier(const ast::AST& tree, const ast::ASTNode& node, const char* word)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(k_modifier) && child.getText() == word) return true;
        }
        return false;
    }

    // Access is not tracked via block state in C# (unlike C++'s `public:` labels);
    // every declaration carries its own modifiers, defaulting to `fallback` when
    // none are present (the enclosing scope's own default member access).
    Access getAccess(const ast::AST& tree, const ast::ASTNode& node, Access fallback)
    {
        bool pub = false, priv = false, prot = false;
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(!child.typeEquals(k_modifier)) continue;
            std::string t = child.getText();
            if(t == "public") pub = true;
            else if(t == "private") priv = true;
            else if(t == "protected") prot = true;
        }
        if(pub) return Access::Public;
        if(priv) return Access::Private;
        if(prot) return Access::Protected;
        return fallback;
    }

    // Name of an operator_declaration / conversion_operator_declaration: the
    // token(s) immediately following the `operator` keyword. `prefix` supplies
    // the separator convention ("operator+" for overloaded operators,
    // "operator " for conversion operators, matching the C++ extractor).
    std::string getOperatorSymbolName(const ast::AST& tree, const ast::ASTNode& node,
                                       const char* prefix)
    {
        bool found = false;
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(found) return std::string(prefix) + child.getText();
            if(child.typeEquals(k_operator_kw)) found = true;
        }
        return {};
    }

    // `using Alias = Some.Qualified.Name;` - the alias identifier is the last
    // identifier that appears before the `=` token. Plain `using Some.Name;`
    // (no `=`) is an import, not a declaration, and returns empty.
    std::string getUsingAliasName(const ast::AST& tree, const ast::ASTNode& node)
    {
        std::string candidate;
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.getText() == "=") return candidate;
            if(child.grammarEquals(k_identifier)) candidate = child.getText();
        }
        return {};
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_csharp(const ast::AST& tree)
{
    std::vector<Symbol>             result;
    std::unordered_set<std::string> seen;   // key = fqn + ":" + kind ordinal
    std::vector<ScopeFrame>         scopeStack;

    auto emit = [&](Symbol sym) {
        std::string key = sym.fqn + ":" + std::to_string(static_cast<int>(sym.kind));
        if(!sym.fqn.empty() && seen.insert(key).second) {
            result.push_back(std::move(sym));
        }
    };

    for(uint32_t i = 0; i < tree.size(); ++i) {
        const ast::ASTNode& node = tree[i];

        // Pop scopes whose byte range ended before this node begins.
        // Works because nodes are stored in DFS pre-order.
        while(!scopeStack.empty() && scopeStack.back().endByte <= node.startByte_) {
            scopeStack.pop_back();
        }

        // ── Namespace (block-scoped: `namespace A.B { ... }`) ──────────────
        if(node.typeEquals(k_namespace_declaration)) {
            std::string name = getLastNameBefore(tree, node, {k_declaration_list});
            if(!name.empty()) {
                std::string fqn = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Namespace, Access::Unknown,
                                i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({name, SymbolKind::Namespace, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Namespace (file-scoped: `namespace A.B;`) ───────────────────────
        if(node.typeEquals(k_file_scoped_namespace_declaration)) {
            std::string name = getLastNameBefore(tree, node, {k_declaration_list});
            if(!name.empty()) {
                std::string fqn = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Namespace, Access::Unknown,
                                i, node.start_.row_, node.start_.column_));
                // No closing brace: this scope naturally spans to the node's end,
                // which is the end of the file.
                scopeStack.push_back({name, SymbolKind::Namespace, node.endByte_,
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Class / Struct / Interface / Record ─────────────────────────────
        if(node.typeEquals(k_class_declaration) || node.typeEquals(k_struct_declaration)
           || node.typeEquals(k_interface_declaration) || node.typeEquals(k_record_declaration)) {
            std::string name = getLastNameBefore(tree, node, {k_declaration_list, k_parameter_list});

            SymbolKind kind          = SymbolKind::Class;
            Access     defaultAccess = Access::Private; // class/struct/record member default
            if(node.typeEquals(k_struct_declaration)) {
                kind = SymbolKind::Struct;
            } else if(node.typeEquals(k_interface_declaration)) {
                kind          = SymbolKind::Class; // closest available SymbolKind
                defaultAccess = Access::Public;     // interface members default to public
            } else if(node.typeEquals(k_record_declaration)) {
                kind = childHasText(tree, node, "struct") ? SymbolKind::Struct : SymbolKind::Class;
            }

            if(!name.empty()) {
                Access access = getAccess(tree, node, topAccess(scopeStack));
                std::string fqn = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, kind, access, i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({name, kind, node.endByte_, defaultAccess, true});
            continue;
        }

        // ── Enum ─────────────────────────────────────────────────────────
        if(node.typeEquals(k_enum_declaration)) {
            std::string name = getLastNameBefore(tree, node, {k_enum_member_declaration_list});
            if(!name.empty()) {
                Access access = getAccess(tree, node, topAccess(scopeStack));
                std::string fqn = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Enum, access,
                                i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({name, SymbolKind::Enum, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Enum members ─────────────────────────────────────────────────
        if(node.typeEquals(k_enum_member_declaration)) {
            const ast::ASTNode* ident = findChild(tree, node, k_identifier);
            if(ident) {
                std::string name = ident->getText();
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::EnumValue, Access::Public,
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Delegate (closest SymbolKind: a named function-signature type) ──
        if(node.typeEquals(k_delegate_declaration)) {
            std::string name = getLastNameBefore(tree, node, {k_parameter_list});
            if(!name.empty()) {
                Access access = getAccess(tree, node, topAccess(scopeStack));
                std::string fqn = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Typedef, access,
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Methods (and local functions nested inside method bodies) ───────
        if(node.typeEquals(k_method_declaration) || node.typeEquals(k_local_function_statement)) {
            bool        isLocal = node.typeEquals(k_local_function_statement);
            std::string name    = getLastNameBefore(tree, node, {k_parameter_list});
            SymbolKind  kind    = (!isLocal && inNamedClassScope(scopeStack))
                                     ? SymbolKind::Method : SymbolKind::Function;
            if(!name.empty()) {
                Access access = isLocal ? Access::Unknown
                                         : getAccess(tree, node, topAccess(scopeStack));
                std::string fqn = buildFQN(scopeStack, name, ".");
                Symbol sym = makeSymbol(name, fqn, kind, access,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = hasModifier(tree, node, "static");
                emit(std::move(sym));
            }
            scopeStack.push_back({name, kind, node.endByte_, Access::Unknown, false});
            continue;
        }

        // ── Constructor ──────────────────────────────────────────────────
        if(node.typeEquals(k_constructor_declaration)) {
            std::string name = getLastNameBefore(tree, node, {k_parameter_list});
            if(!name.empty()) {
                Access access = getAccess(tree, node, topAccess(scopeStack));
                std::string fqn = buildFQN(scopeStack, name, ".");
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Constructor, access,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = hasModifier(tree, node, "static"); // static constructors
                emit(std::move(sym));
            }
            scopeStack.push_back({name, SymbolKind::Constructor, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Destructor ───────────────────────────────────────────────────
        if(node.typeEquals(k_destructor_declaration)) {
            std::string base = getLastNameBefore(tree, node, {k_parameter_list});
            if(!base.empty()) {
                std::string name = "~" + base;
                std::string fqn  = buildFQN(scopeStack, name, ".");
                emit(makeSymbol(name, fqn, SymbolKind::Destructor, Access::Public,
                                i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({base, SymbolKind::Destructor, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Operator overloads / conversion operators ───────────────────────
        if(node.typeEquals(k_operator_declaration) || node.typeEquals(k_conversion_operator_declaration)) {
            bool isConversion = node.typeEquals(k_conversion_operator_declaration);
            std::string name  = getOperatorSymbolName(tree, node, isConversion ? "operator " : "operator");
            if(!name.empty()) {
                Access access = getAccess(tree, node, topAccess(scopeStack));
                std::string fqn = buildFQN(scopeStack, name, ".");
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Method, access,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = true; // operator overloads are always static in C#
                emit(std::move(sym));
            }
            continue;
        }

        // ── Fields (and event fields: `event Handler Foo;`) ─────────────────
        if(node.typeEquals(k_field_declaration) || node.typeEquals(k_event_field_declaration)) {
            const ast::ASTNode* varDecl = findChild(tree, node, k_variable_declaration);
            if(varDecl) {
                Access access   = getAccess(tree, node, topAccess(scopeStack));
                bool   isStatic = hasModifier(tree, node, "static");
                bool   isConst  = hasModifier(tree, node, "const");
                for(uintptr_t id : varDecl->children_) {
                    if(id == ast::InvalidId) continue;
                    const ast::ASTNode& decl = tree[static_cast<uint32_t>(id)];
                    if(!decl.typeEquals(k_variable_declarator)) continue;
                    const ast::ASTNode* ident = findChild(tree, decl, k_identifier);
                    if(!ident) continue;
                    std::string name = ident->getText();
                    std::string fqn  = buildFQN(scopeStack, name, ".");
                    Symbol sym = makeSymbol(name, fqn, SymbolKind::Field, access,
                                           i, node.start_.row_, node.start_.column_);
                    sym.isStatic    = isStatic || isConst; // const members are implicitly static
                    sym.isConstexpr = isConst;
                    emit(std::move(sym));
                }
            }
            continue;
        }

        // ── Properties / events (closest SymbolKind: Field) ──────────────────
        if(node.typeEquals(k_property_declaration) || node.typeEquals(k_event_declaration)) {
            std::string name = getLastNameBefore(tree, node, {k_accessor_list, k_arrow_expression_clause});
            if(!name.empty()) {
                Access access = getAccess(tree, node, topAccess(scopeStack));
                std::string fqn = buildFQN(scopeStack, name, ".");
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Field, access,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = hasModifier(tree, node, "static");
                emit(std::move(sym));
            }
            continue;
        }

        // ── `using Alias = Namespace.Type;` ──────────────────────────────────
        if(node.typeEquals(k_using_directive)) {
            std::string alias = getUsingAliasName(tree, node);
            if(!alias.empty()) {
                std::string fqn = buildFQN(scopeStack, alias, ".");
                emit(makeSymbol(alias, fqn, SymbolKind::UsingAlias, Access::Unknown,
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }
    }

    return result;
}

} // namespace extractor
} // namespace ast
