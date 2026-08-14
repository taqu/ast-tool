#include "ast-extractor-cfamily.h"
#include "ast-tool.h"
#include "ast-extractor-common.h"
#include "ast-ir.h"
#include <unordered_set>

namespace ast
{
namespace extractor
{
namespace
{
    // ── Node type string constants (C / C++ grammars) ───────────────────────
    inline constexpr char k_declaration[] = "declaration";
    inline constexpr char k_class_definition[] = "class_definition";
    inline constexpr char k_class_specifier[] = "class_specifier";
    inline constexpr char k_struct_definition[] = "struct_definition";
    inline constexpr char k_struct_specifier[] = "struct_specifier";
    inline constexpr char k_function_declarator[] = "function_declarator";
    inline constexpr char k_function_definition[] = "function_definition";
    inline constexpr char k_preproc_def[] = "preproc_def";
    inline constexpr char k_namespace_definition[] = "namespace_definition";
    inline constexpr char k_field_declaration[] = "field_declaration";
    inline constexpr char k_field_declaration_list[] = "field_declaration_list";
    inline constexpr char k_identifier[] = "identifier";
    inline constexpr char k_init_declarator[] = "init_declarator";

    constexpr const char* k_union_specifier      = "union_specifier";
    constexpr const char* k_enum_specifier       = "enum_specifier";
    constexpr const char* k_enumerator           = "enumerator";
    constexpr const char* k_type_definition      = "type_definition";
    constexpr const char* k_alias_declaration    = "alias_declaration";
    constexpr const char* k_access_specifier     = "access_specifier";
    constexpr const char* k_type_identifier      = "type_identifier";
    constexpr const char* k_namespace_identifier = "namespace_identifier";
    constexpr const char* k_qualified_identifier = "qualified_identifier";
    constexpr const char* k_destructor_name      = "destructor_name";
    constexpr const char* k_operator_name        = "operator_name";
    constexpr const char* k_conversion_function_id = "conversion_function_id"; // newer tree-sitter-cpp: operator bool()
    constexpr const char* k_operator_cast          = "operator_cast";           // older tree-sitter-cpp: operator bool()
    constexpr const char* k_translation_unit     = "translation_unit";
    constexpr const char* k_declaration_list     = "declaration_list";
    constexpr const char* k_field_identifier     = "field_identifier";
    constexpr const char* k_pointer_declarator   = "pointer_declarator";
    constexpr const char* k_abstract_pointer_declarator = "abstract_pointer_declarator";
    constexpr const char* k_reference_declarator = "reference_declarator";
    constexpr const char* k_abstract_reference_declarator = "abstract_reference_declarator";
    constexpr const char* k_array_declarator     = "array_declarator";
    constexpr const char* k_parenthesized_decl   = "parenthesized_declarator";
    constexpr const char* k_primitive_type       = "primitive_type";
    constexpr const char* k_sized_type           = "sized_type_specifier";
    constexpr const char* k_type_qualifier       = "type_qualifier";
    constexpr const char* k_storage_class        = "storage_class_specifier";
    constexpr const char* k_function_specifier   = "function_specifier";
    constexpr const char* k_template_type        = "template_type";

    // ── Low-level helpers ─────────────────────────────────────────────────────

    // Returns true for node types that are type specifiers (not declarators).
    // These must be skipped when looking for the declared name inside a k_declaration.
    bool isTypeSpecifier(const ast::ASTNode& n)
    {
        return n.typeEquals(k_primitive_type)
            || n.typeEquals(k_type_identifier)
            || n.typeEquals(k_namespace_identifier)
            || n.typeEquals(k_qualified_identifier)
            || n.typeEquals(k_sized_type)
            || n.typeEquals(k_type_qualifier)
            || n.typeEquals(k_storage_class)
            || n.typeEquals(k_function_specifier)
            || n.typeEquals(k_template_type)
            || n.typeEquals(k_class_specifier)
            || n.typeEquals(k_class_definition)
            || n.typeEquals(k_struct_specifier)
            || n.typeEquals(k_struct_definition)
            || n.typeEquals(k_union_specifier)
            || n.typeEquals(k_enum_specifier);
    }

    // Extract the declared name from a declarator node (k_identifier / field_identifier /
    // or a wrapping declarator such as pointer_declarator, k_init_declarator, etc.).
    // Does NOT recurse into type specifiers.
    std::u8string declName(const ast::AST& tree, const ast::ASTNode& n)
    {
        if(n.typeEquals(k_identifier) || n.typeEquals(k_field_identifier)
           || n.grammarEquals(k_identifier)) {
            return n.getText();
        }
        if(n.typeEquals(k_init_declarator)
           || n.typeEquals(k_pointer_declarator)
           || n.typeEquals(k_reference_declarator)
           || n.typeEquals(k_array_declarator)
           || n.typeEquals(k_parenthesized_decl)) {
            for(uintptr_t id : n.children_) {
                if(id == ast::InvalidId) continue;
                const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
                if(isTypeSpecifier(child)) continue;
                std::u8string name = declName(tree, child);
                if(!name.empty()) return name;
            }
        }
        return {};
    }

    // Get the declared variable name from a k_declaration or k_field_declaration node.
    // Skips all type-specifier children so identifiers inside type expressions
    // (e.g. "std" in std::string) are never mistaken for the declared name.
    std::u8string getVarName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(isTypeSpecifier(child)) continue;
            std::u8string name = declName(tree, child);
            if(!name.empty()) return name;
        }
        return {};
    }

    // Get the typedef alias name: the LAST type_identifier / grammar-k_identifier among
    // direct children.  For "typedef std::string MyStr;" that is "MyStr", not "string".
    std::u8string getTypedefName(const ast::AST& tree, const ast::ASTNode& node)
    {
        std::u8string last;
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(k_type_identifier) || child.grammarEquals(k_identifier)) {
                last = child.getText();
            }
        }
        return last;
    }

    // Extract the simple name from namespace / class / struct / union / enum nodes.
    // Only examines direct children; stops when the body starts.
    std::u8string getTypeName(const ast::AST& tree, const ast::ASTNode& node)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(k_type_identifier)
               || child.typeEquals(k_namespace_identifier)) {
                return child.getText();
            }
            // Stop once we reach the body node to avoid picking up base-class names.
            if(child.typeEquals(k_field_declaration_list)
               || child.typeEquals(k_declaration_list)
               || child.typeEquals("block")) {
                break;
            }
            if(child.grammarEquals(k_identifier)) {
                return child.getText();
            }
        }
        return {};
    }

    // Parsed function name and kind hints (C++)
    struct FuncName {
        std::u8string name;
        bool isDestructor = false;
        bool isQualified  = false; // qualified names appear in out-of-line definitions
    };

    FuncName getFuncName(const ast::AST& tree, const ast::ASTNode& declarator)
    {
        for(uintptr_t id : declarator.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(k_destructor_name))      return {child.getText(), true,  false};
            if(child.typeEquals(k_qualified_identifier)) return {child.getText(), false, true};
            if(child.typeEquals(k_operator_name)
               || child.typeEquals(k_conversion_function_id)
               || child.typeEquals(k_operator_cast)
               || child.typeEquals(k_identifier)
               || child.typeEquals(k_field_identifier)
               || child.grammarEquals(k_identifier)) {
                return {child.getText(), false, false};
            }
        }
        return {};
    }

    std::u8string getAbstractDeclaratorName(const ast::AST& tree, const ast::ASTNode& declarator)
    {
        for(uintptr_t id : declarator.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(k_abstract_pointer_declarator)){
                return u8"*" + getAbstractDeclaratorName(tree, child);
            }
            if(child.typeEquals(k_abstract_reference_declarator)){
                return u8"&" + getAbstractDeclaratorName(tree, child);
            }
        }
        return {};
    }

    FuncName getOperatorCastName(const ast::AST& tree, const ast::ASTNode& declarator)
    {
        for(uintptr_t id : declarator.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(k_primitive_type)
                || child.typeEquals(k_type_identifier)){
                std::u8string name = u8"operator " + child.getText() + getAbstractDeclaratorName(tree, declarator);
                return {name, false, false};
            }
        }
        return {};
    }

    // A C++ k_declaration is a global/namespace variable only when its parent is the
    // translation_unit root or a namespace body (declaration_list).
    bool isGlobalOrNamespaceDecl(const ast::AST& tree, const ast::ASTNode& node)
    {
        if(node.parent_ == ast::InvalidId) return true;
        const ast::ASTNode& parent = tree[static_cast<uint32_t>(node.parent_)];
        return parent.typeEquals(k_translation_unit) || parent.typeEquals(k_declaration_list);
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_cfamily(const ast::AST& tree)
{
    std::vector<Symbol>             result;
    std::unordered_set<std::u8string> seen;
    std::vector<ScopeFrame>         scopeStack;
    char8_t buffer[BUFFER_SIZE];

    // For functions, deduplicate by FQN alone (k_declaration + out-of-line definition).
    auto emit = [&](Symbol sym) {
        bool funcLike = sym.kind == SymbolKind::Function
                     || sym.kind == SymbolKind::Method
                     || sym.kind == SymbolKind::Constructor
                     || sym.kind == SymbolKind::Destructor;
        std::u8string key = funcLike
            ? sym.fqn
            : (sym.fqn + u8":" + ast::to_string_intermediate(buffer, static_cast<int32_t>(sym.kind)));
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

        // ── Access specifier ─────────────────────────────────────────────
        if(node.typeEquals(k_access_specifier)) {
            std::u8string text = node.getText();
            for(int j = (int)scopeStack.size() - 1; j >= 0; --j) {
                if(!scopeStack[j].isAccessAware) continue;
                if(text.find(u8"public") != std::string::npos)
                    scopeStack[j].currentAccess = Access::Public;
                else if(text.find(u8"protected") != std::string::npos)
                    scopeStack[j].currentAccess = Access::Protected;
                else if(text.find(u8"private") != std::string::npos)
                    scopeStack[j].currentAccess = Access::Private;
                break;
            }
            continue;
        }

        // ── Namespace ────────────────────────────────────────────────────
        if(node.typeEquals(k_namespace_definition)) {
            std::u8string name = getTypeName(tree, node);
            if(name.empty()) {
                // Anonymous namespace: push a blank-named scope so members resolve
                // their FQN directly against the enclosing named scope (buildFQN
                // skips empty names), but don't emit it as a user-visible symbol.
                scopeStack.push_back({std::u8string(), SymbolKind::Namespace, node.endByte_,
                                      Access::Unknown, false});
                continue;
            }
            std::u8string fqn = buildFQN(scopeStack, name);
            emit(makeSymbol(name, fqn, SymbolKind::Namespace, Access::Unknown,
                            i, node.start_.row_, node.start_.column_));
            scopeStack.push_back({name, SymbolKind::Namespace, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Class ────────────────────────────────────────────────────────
        if(node.typeEquals(k_class_specifier) || node.typeEquals(k_class_definition)) {
            std::u8string name = getTypeName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name);
                emit(makeSymbol(name, fqn, SymbolKind::Class, topAccess(scopeStack),
                                i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({name, SymbolKind::Class, node.endByte_,
                                  Access::Private, true});
            continue;
        }

        // ── Struct ───────────────────────────────────────────────────────
        if(node.typeEquals(k_struct_specifier) || node.typeEquals(k_struct_definition)) {
            std::u8string name = getTypeName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name);
                emit(makeSymbol(name, fqn, SymbolKind::Struct, topAccess(scopeStack),
                                i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({name, SymbolKind::Struct, node.endByte_,
                                  Access::Public, true});
            continue;
        }

        // ── Union ────────────────────────────────────────────────────────
        if(node.typeEquals(k_union_specifier)) {
            std::u8string name = getTypeName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name);
                emit(makeSymbol(name, fqn, SymbolKind::Union, topAccess(scopeStack),
                                i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({name, SymbolKind::Union, node.endByte_,
                                  Access::Public, true});
            continue;
        }

        // ── Enum ─────────────────────────────────────────────────────────
        if(node.typeEquals(k_enum_specifier)) {
            std::u8string name = getTypeName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name);
                emit(makeSymbol(name, fqn, SymbolKind::Enum, topAccess(scopeStack),
                                i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({name, SymbolKind::Enum, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Enum values ──────────────────────────────────────────────────
        if(node.typeEquals(k_enumerator)) {
            const ast::ASTNode* ident = findChild(tree, node, (const char8_t*)k_identifier);
            if(ident) {
                std::u8string name = ident->getText();
                std::u8string fqn  = buildFQN(scopeStack, name);
                emit(makeSymbol(name, fqn, SymbolKind::EnumValue, Access::Unknown,
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Functions / Methods / Constructors / Destructors ──────────────
        if(node.typeEquals(k_function_definition) || node.typeEquals(k_declaration)) {
            const ast::ASTNode* funcDecl = findChild(tree, node, (const char8_t*)k_function_declarator);
            if(!funcDecl) {
                const ast::ASTNode* referenceDecl = findChild(tree, node, (const char8_t*)k_reference_declarator);
                if(referenceDecl) {
                    funcDecl = findChild(tree, *referenceDecl, (const char8_t*)k_function_declarator);
                }
            }

            if(funcDecl) {
                FuncName fn = getFuncName(tree, *funcDecl);
                if(!fn.name.empty()) {
                    std::u8string fqn;
                    if(fn.isQualified) {
                        // Out-of-line definition: qualified name already encodes the
                        // class scope; only prepend enclosing namespace prefix.
                        std::u8string nsPrefix;
                        for(const auto& f : scopeStack) {
                            if(f.kind != SymbolKind::Namespace || f.name.empty()) continue;
                            if(!nsPrefix.empty()) nsPrefix += u8"::";
                            nsPrefix += f.name;
                        }
                        fqn = nsPrefix.empty() ? fn.name : (nsPrefix + u8"::" + fn.name);
                    } else {
                        fqn = buildFQN(scopeStack, fn.name);
                    }

                    SymbolKind kind = SymbolKind::Function;
                    if(!fn.isQualified && inNamedClassScope(scopeStack)) {
                        if(fn.isDestructor) {
                            kind = SymbolKind::Destructor;
                        } else {
                            kind = SymbolKind::Method;
                            for(int j = (int)scopeStack.size() - 1; j >= 0; --j) {
                                auto fk = scopeStack[j].kind;
                                if(fk != SymbolKind::Class && fk != SymbolKind::Struct
                                   && fk != SymbolKind::Union) continue;
                                if(fn.name == scopeStack[j].name) kind = SymbolKind::Constructor;
                                break;
                            }
                        }
                    }

                    Symbol sym = makeSymbol(fn.name, fqn, kind, topAccess(scopeStack),
                                           i, node.start_.row_, node.start_.column_);
                    sym.isStatic    = childHasText(tree, node, u8"static");
                    sym.isConstexpr = childHasText(tree, node, u8"constexpr");
                    sym.isInline    = childHasText(tree, node, u8"inline");
                    emit(std::move(sym));
                }
                continue;
            }
            const ast::ASTNode* operatorCastDecl = findChild(tree, node, (const char8_t*)k_operator_cast);
            if(operatorCastDecl) {
                FuncName fn = getOperatorCastName(tree, *operatorCastDecl);
                if(!fn.name.empty()) {
                    std::u8string fqn;
                    if(fn.isQualified) {
                        // Out-of-line definition: qualified name already encodes the
                        // class scope; only prepend enclosing namespace prefix.
                        std::u8string nsPrefix;
                        for(const auto& f : scopeStack) {
                            if(f.kind != SymbolKind::Namespace || f.name.empty()) continue;
                            if(!nsPrefix.empty()) nsPrefix += u8"::";
                            nsPrefix += f.name;
                        }
                        fqn = nsPrefix.empty() ? fn.name : (nsPrefix + u8"::" + fn.name);
                    } else {
                        fqn = buildFQN(scopeStack, fn.name);
                    }

                    SymbolKind kind = SymbolKind::Function;
                    if(!fn.isQualified && inNamedClassScope(scopeStack)) {
                        if(fn.isDestructor) {
                            kind = SymbolKind::Destructor;
                        } else {
                            kind = SymbolKind::Method;
                            for(int j = (int)scopeStack.size() - 1; j >= 0; --j) {
                                auto fk = scopeStack[j].kind;
                                if(fk != SymbolKind::Class && fk != SymbolKind::Struct
                                   && fk != SymbolKind::Union) continue;
                                if(fn.name == scopeStack[j].name) kind = SymbolKind::Constructor;
                                break;
                            }
                        }
                    }

                    Symbol sym = makeSymbol(fn.name, fqn, kind, topAccess(scopeStack),
                                           i, node.start_.row_, node.start_.column_);
                    sym.isStatic    = childHasText(tree, node, u8"static");
                    sym.isConstexpr = childHasText(tree, node, u8"constexpr");
                    sym.isInline    = childHasText(tree, node, u8"inline");
                    emit(std::move(sym));
                }
                continue;
            }

            // Plain k_declaration (no k_function_declarator) → possible global variable
            if(node.typeEquals(k_declaration) && isGlobalOrNamespaceDecl(tree, node)) {
                std::u8string name = getVarName(tree, node);
                if(!name.empty()) {
                    std::u8string fqn = buildFQN(scopeStack, name);
                    Symbol sym = makeSymbol(name, fqn, SymbolKind::Variable, Access::Unknown,
                                           i, node.start_.row_, node.start_.column_);
                    sym.isStatic    = childHasText(tree, node, u8"static");
                    sym.isConstexpr = childHasText(tree, node, u8"constexpr");
                    emit(std::move(sym));
                }
            }
            continue;
        }

        // ── Field declarations (class / struct / union members) ───────────
        if(node.typeEquals(k_field_declaration) && inNamedClassScope(scopeStack)) {
            std::u8string name = getVarName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name);
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Field, topAccess(scopeStack),
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic    = childHasText(tree, node, u8"static");
                sym.isConstexpr = childHasText(tree, node, u8"constexpr");
                emit(std::move(sym));
            }
            if(name.empty()){
                const ast::ASTNode* funcDecl = findChild(tree, node, (const char8_t*)k_function_declarator);;
                if(!funcDecl) {
                    const ast::ASTNode* referenceDecl = findChild(tree, node, (const char8_t*)k_reference_declarator);
                    if(referenceDecl){
                        funcDecl = findChild(tree, *referenceDecl, (const char8_t*)k_function_declarator);
                    }
                }
                if(funcDecl) {
                    FuncName fn = getFuncName(tree, *funcDecl);
                    if(!fn.name.empty()) {
                        std::u8string fqn;
                        if(fn.isQualified) {
                            // Out-of-line definition: qualified name already encodes the
                            // class scope; only prepend enclosing namespace prefix.
                            std::u8string nsPrefix;
                            for(const auto& f: scopeStack) {
                                if(f.kind != SymbolKind::Namespace || f.name.empty())
                                    continue;
                                if(!nsPrefix.empty())
                                    nsPrefix += u8"::";
                                nsPrefix += f.name;
                            }
                            fqn = nsPrefix.empty() ? fn.name : (nsPrefix + u8"::" + fn.name);
                        } else {
                            fqn = buildFQN(scopeStack, fn.name);
                        }

                        SymbolKind kind = SymbolKind::Function;
                        if(!fn.isQualified && inNamedClassScope(scopeStack)) {
                            if(fn.isDestructor) {
                                kind = SymbolKind::Destructor;
                            } else {
                                kind = SymbolKind::Method;
                                for(int j = (int)scopeStack.size() - 1; j >= 0; --j) {
                                    auto fk = scopeStack[j].kind;
                                    if(fk != SymbolKind::Class && fk != SymbolKind::Struct
                                       && fk != SymbolKind::Union)
                                        continue;
                                    if(fn.name == scopeStack[j].name)
                                        kind = SymbolKind::Constructor;
                                    break;
                                }
                            }
                        }

                        Symbol sym = makeSymbol(fn.name, fqn, kind, topAccess(scopeStack),
                                                i, node.start_.row_, node.start_.column_);
                        sym.isStatic = childHasText(tree, node, u8"static");
                        sym.isConstexpr = childHasText(tree, node, u8"constexpr");
                        sym.isInline = childHasText(tree, node, u8"inline");
                        emit(std::move(sym));
                    }
                    continue;
                }
            }
            continue;
        }

        // ── Macros ───────────────────────────────────────────────────────
        if(node.typeEquals(k_preproc_def)) {
            const ast::ASTNode* ident = findChild(tree, node, (const char8_t*)k_identifier);
            if(ident) {
                std::u8string name = ident->getText();
                emit(makeSymbol(name, name, SymbolKind::Macro, Access::Unknown,
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Typedef ──────────────────────────────────────────────────────
        if(node.typeEquals(k_type_definition)) {
            std::u8string name = getTypedefName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name);
                emit(makeSymbol(name, fqn, SymbolKind::Typedef, topAccess(scopeStack),
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Using alias (using X = ...) ───────────────────────────────────
        if(node.typeEquals(k_alias_declaration)) {
            std::u8string name;
            for(uintptr_t id : node.children_) {
                if(id == ast::InvalidId) continue;
                const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
                if(child.typeEquals(k_type_identifier) || child.grammarEquals(k_identifier)) {
                    name = child.getText();
                    break;
                }
            }
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name);
                emit(makeSymbol(name, fqn, SymbolKind::UsingAlias, topAccess(scopeStack),
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }
    }

    return result;
}

} // namespace extractor
} // namespace ast
