#include "ast-tool.h"
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
    // ── Low-level helpers ─────────────────────────────────────────────────────

    bool isNameLike(const ast::ASTNode& n)
    {
        return n.grammarEquals(ASTNodeType::Identifier)
            || n.typeEquals(ASTNodeType::QualifiedName)
            || n.typeEquals(ASTNodeType::GenericName)
            || n.typeEquals(ASTNodeType::AliasQualifiedName);
    }

    // Returns the LAST name-like direct child of `node` that appears before any
    // child whose type is in `stopTypes`. Declarations place their declared name
    // immediately before the parameter list / body, while a preceding return
    // type (also an identifier/qualified_name/generic_name in this grammar) may
    // appear earlier - taking the last match instead of the first sidesteps that
    // ambiguity without having to classify type nodes.
    std::u8string getLastNameBefore(const ast::AST& tree, const ast::ASTNode& node,
                                   std::initializer_list<ASTNodeType> stopTypes)
    {
        std::u8string last;
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            bool stop = false;
            for(ASTNodeType st : stopTypes) {
                if(child.typeEquals(st)) { stop = true; break; }
            }
            if(stop) break;
            if(isNameLike(child)) last = child.getText();
        }
        return last;
    }

    bool hasModifier(const ast::AST& tree, const ast::ASTNode& node, const char8_t* word)
    {
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(ASTNodeType::Modifier) && child.getText() == word) return true;
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
            if(!child.typeEquals(ASTNodeType::Modifier)) continue;
            std::u8string t = child.getText();
            if(t == u8"public") pub = true;
            else if(t == u8"private") priv = true;
            else if(t == u8"protected") prot = true;
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
    std::u8string getOperatorSymbolName(const ast::AST& tree, const ast::ASTNode& node,
                                       const char8_t* prefix)
    {
        bool found = false;
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(found) return std::u8string(prefix) + child.getText();
            if(child.typeEquals(ASTNodeType::Operator)) found = true;
        }
        return {};
    }

    // `using Alias = Some.Qualified.Name;` - the alias identifier is the last
    // identifier that appears before the `=` token. Plain `using Some.Name;`
    // (no `=`) is an import, not a declaration, and returns empty.
    std::u8string getUsingAliasName(const ast::AST& tree, const ast::ASTNode& node)
    {
        std::u8string candidate;
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.getText() == u8"=") return candidate;
            if(child.grammarEquals(ASTNodeType::Identifier)) candidate = child.getText();
        }
        return {};
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_csharp(const ast::AST& tree)
{
    std::vector<Symbol>             result;
    std::unordered_set<std::u8string> seen;   // key = fqn + ":" + kind ordinal
    std::vector<ScopeFrame>         scopeStack;

    auto emit = [&](Symbol sym) {
        char8_t buffer[BUFFER_SIZE];
        std::u8string key = sym.fqn + u8":" + ast::to_string_intermediate(buffer, static_cast<int>(sym.kind));
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
        if(node.typeEquals(ASTNodeType::NamespaceDeclaration)) {
            std::u8string name = getLastNameBefore(tree, node, {ASTNodeType::DeclarationList});
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Namespace, Access::Unknown,
                                i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({name, SymbolKind::Namespace, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Namespace (file-scoped: `namespace A.B;`) ───────────────────────
        if(node.typeEquals(ASTNodeType::FileScopedNamespaceDeclaration)) {
            std::u8string name = getLastNameBefore(tree, node, {ASTNodeType::DeclarationList});
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
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
        if(node.typeEquals(ASTNodeType::ClassDeclaration) || node.typeEquals(ASTNodeType::StructDeclaration)
           || node.typeEquals(ASTNodeType::InterfaceDeclaration) || node.typeEquals(ASTNodeType::RecordDeclaration)) {
            std::u8string name = getLastNameBefore(tree, node, {ASTNodeType::DeclarationList, ASTNodeType::ParameterList});

            SymbolKind kind          = SymbolKind::Class;
            Access     defaultAccess = Access::Private; // class/struct/record member default
            if(node.typeEquals(ASTNodeType::StructDeclaration)) {
                kind = SymbolKind::Struct;
            } else if(node.typeEquals(ASTNodeType::InterfaceDeclaration)) {
                kind          = SymbolKind::Class; // closest available SymbolKind
                defaultAccess = Access::Public;     // interface members default to public
            } else if(node.typeEquals(ASTNodeType::RecordDeclaration)) {
                kind = childHasText(tree, node, u8"struct") ? SymbolKind::Struct : SymbolKind::Class;
            }

            if(!name.empty()) {
                Access access = getAccess(tree, node, topAccess(scopeStack));
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, kind, access, i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({name, kind, node.endByte_, defaultAccess, true});
            continue;
        }

        // ── Enum ─────────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::EnumDeclaration)) {
            std::u8string name = getLastNameBefore(tree, node, {ASTNodeType::EnumMemberDeclarationList});
            if(!name.empty()) {
                Access access = getAccess(tree, node, topAccess(scopeStack));
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Enum, access,
                                i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({name, SymbolKind::Enum, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Enum members ─────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::EnumMemberDeclaration)) {
            const ast::ASTNode* ident = findChild(tree, node, ASTNodeType::Identifier);
            if(ident) {
                std::u8string name = ident->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::EnumValue, Access::Public,
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Delegate (closest SymbolKind: a named function-signature type) ──
        if(node.typeEquals(ASTNodeType::DelegateDeclaration)) {
            std::u8string name = getLastNameBefore(tree, node, {ASTNodeType::ParameterList});
            if(!name.empty()) {
                Access access = getAccess(tree, node, topAccess(scopeStack));
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Typedef, access,
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Methods (and local functions nested inside method bodies) ───────
        if(node.typeEquals(ASTNodeType::MethodDeclaration) || node.typeEquals(ASTNodeType::LocalFunctionStatement)) {
            bool        isLocal = node.typeEquals(ASTNodeType::LocalFunctionStatement);
            std::u8string name    = getLastNameBefore(tree, node, {ASTNodeType::ParameterList});
            SymbolKind  kind    = (!isLocal && inNamedClassScope(scopeStack))
                                     ? SymbolKind::Method : SymbolKind::Function;
            if(!name.empty()) {
                Access access = isLocal ? Access::Unknown
                                         : getAccess(tree, node, topAccess(scopeStack));
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                Symbol sym = makeSymbol(name, fqn, kind, access,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = hasModifier(tree, node, u8"static");
                emit(std::move(sym));
            }
            scopeStack.push_back({name, kind, node.endByte_, Access::Unknown, false});
            continue;
        }

        // ── Constructor ──────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::ConstructorDeclaration)) {
            std::u8string name = getLastNameBefore(tree, node, {ASTNodeType::ParameterList});
            if(!name.empty()) {
                Access access = getAccess(tree, node, topAccess(scopeStack));
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Constructor, access,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = hasModifier(tree, node, u8"static"); // static constructors
                emit(std::move(sym));
            }
            scopeStack.push_back({name, SymbolKind::Constructor, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Destructor ───────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::DestructorDeclaration)) {
            std::u8string base = getLastNameBefore(tree, node, {ASTNodeType::ParameterList});
            if(!base.empty()) {
                std::u8string name = u8"~" + base;
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Destructor, Access::Public,
                                i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({base, SymbolKind::Destructor, node.endByte_,
                                  Access::Unknown, false});
            continue;
        }

        // ── Operator overloads / conversion operators ───────────────────────
        if(node.typeEquals(ASTNodeType::OperatorDeclaration) || node.typeEquals(ASTNodeType::ConversionOperatorDeclaration)) {
            bool isConversion = node.typeEquals(ASTNodeType::ConversionOperatorDeclaration);
            std::u8string name  = getOperatorSymbolName(tree, node, isConversion ? u8"operator " : u8"operator");
            if(!name.empty()) {
                Access access = getAccess(tree, node, topAccess(scopeStack));
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Method, access,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = true; // operator overloads are always static in C#
                emit(std::move(sym));
            }
            continue;
        }

        // ── Fields (and event fields: `event Handler Foo;`) ─────────────────
        if(node.typeEquals(ASTNodeType::FieldDeclaration) || node.typeEquals(ASTNodeType::EventFieldDeclaration)) {
            const ast::ASTNode* varDecl = findChild(tree, node, ASTNodeType::VariableDeclaration);
            if(varDecl) {
                Access access   = getAccess(tree, node, topAccess(scopeStack));
                bool   isStatic = hasModifier(tree, node, u8"static");
                bool   isConst  = hasModifier(tree, node, u8"const");
                for(uintptr_t id : varDecl->children_) {
                    if(id == ast::InvalidId) continue;
                    const ast::ASTNode& decl = tree[static_cast<uint32_t>(id)];
                    if(!decl.typeEquals(ASTNodeType::VariableDeclarator)) continue;
                    const ast::ASTNode* ident = findChild(tree, decl, ASTNodeType::Identifier);
                    if(!ident) continue;
                    std::u8string name = ident->getText();
                    std::u8string fqn  = buildFQN(scopeStack, name, u8".");
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
        if(node.typeEquals(ASTNodeType::PropertyDeclaration) || node.typeEquals(ASTNodeType::EventDeclaration)) {
            std::u8string name = getLastNameBefore(tree, node, {ASTNodeType::AccessorList, ASTNodeType::ArrowExpressionClause});
            if(!name.empty()) {
                Access access = getAccess(tree, node, topAccess(scopeStack));
                std::u8string fqn = buildFQN(scopeStack, name, u8".");
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Field, access,
                                       i, node.start_.row_, node.start_.column_);
                sym.isStatic = hasModifier(tree, node, u8"static");
                emit(std::move(sym));
            }
            continue;
        }

        // ── `using Alias = Namespace.Type;` ──────────────────────────────────
        if(node.typeEquals(ASTNodeType::UsingDirective)) {
            std::u8string alias = getUsingAliasName(tree, node);
            if(!alias.empty()) {
                std::u8string fqn = buildFQN(scopeStack, alias, u8".");
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
