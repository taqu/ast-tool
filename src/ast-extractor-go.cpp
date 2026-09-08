#include "ast-tool.h"
#include "ast-extractor-langs.h"
#include "ast-extractor-common.h"
#include "ast-ir.h"
#include <cstdint>
#include <unordered_set>

namespace ast
{
namespace extractor
{
namespace
{
    // ── Low-level helpers ─────────────────────────────────────────────────────

    // Go's only visibility rule: an identifier is exported (public) iff its
    // first letter is uppercase. There is no access-modifier keyword to parse.
    Access accessFromName(const std::u8string& name)
    {
        if(name.empty()) return Access::Unknown;
        char8_t c = name[0];
        if(c >= u8'A' && c <= u8'Z') return Access::Public;
        if((c >= u8'a' && c <= u8'z') || c == u8'_') return Access::Private;
        return Access::Unknown; // non-ASCII leading rune: not classified here
    }

    // Unwraps pointer_type / qualified_type / generic_type down to the base
    // type name, e.g. "*io.Reader" -> "Reader", "List[int]" -> "List".
    std::u8string baseTypeName(const ast::AST& tree, const ast::ASTNode& node)
    {
        if(node.typeEquals(ASTNodeType::TypeIdentifier)) return node.getText();
        if(node.typeEquals(ASTNodeType::QualifiedType)) {
            const ast::ASTNode* ident = findChild(tree, node, ASTNodeType::TypeIdentifier);
            return ident ? ident->getText() : std::u8string();
        }
        if(node.typeEquals(ASTNodeType::PointerType) || node.typeEquals(ASTNodeType::GenericType)) {
            for(uintptr_t id : node.children_) {
                if(id == ast::InvalidId) continue;
                const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
                std::u8string name = baseTypeName(tree, child);
                if(!name.empty()) return name;
            }
        }
        return {};
    }

    // The declared name of a type_spec / type_alias: the first type_identifier
    // direct child. It always precedes the underlying type expression, so the
    // first match is unambiguous (unlike C#, Go never places a return/field
    // type before the name at this level).
    std::u8string getTypeSpecName(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* ident = findChild(tree, node, ASTNodeType::TypeIdentifier);
        return ident ? ident->getText() : std::u8string();
    }

    // The receiver type name of a method_declaration, e.g. "Foo" from both
    // "(f Foo)" and "(f *Foo)". The receiver is always the first parameter_list.
    std::u8string getReceiverTypeName(const ast::AST& tree, const ast::ASTNode& node)
    {
        const ast::ASTNode* recv = findChild(tree, node, ASTNodeType::ParameterList);
        if(!recv) return {};
        const ast::ASTNode* param = findChild(tree, *recv, ASTNodeType::ParameterDeclaration);
        if(!param) return {};
        for(uintptr_t id : param->children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            std::u8string name = baseTypeName(tree, child);
            if(!name.empty()) return name;
        }
        return {};
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_go(const ast::AST& tree)
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

        // ── Package clause ──────────────────────────────────────────────────
        // Applies to the whole file; there is no closing delimiter, so the
        // scope is given a byte range that never closes during this traversal.
        if(node.typeEquals(ASTNodeType::PackageClause)) {
            const ast::ASTNode* ident = findChild(tree, node, ASTNodeType::PackageIdentifier);
            std::u8string name = ident ? ident->getText() : std::u8string();
            if(!name.empty()) {
                emit(makeSymbol(name, name, SymbolKind::Namespace, Access::Unknown,
                                i, node.start_.row_, node.start_.column_));
                scopeStack.push_back({name, SymbolKind::Namespace, uint32_t(-1),
                                      Access::Unknown, false});
            }
            continue;
        }

        // ── Import alias: `import alias "path"` ─────────────────────────────
        if(node.typeEquals(ASTNodeType::ImportSpec)) {
            const ast::ASTNode* alias = findChild(tree, node, ASTNodeType::PackageIdentifier);
            if(alias) {
                std::u8string name = alias->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::UsingAlias, Access::Unknown,
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Type declaration: struct / interface / defined type / alias ─────
        if((node.typeEquals(ASTNodeType::TypeSpec) || node.typeEquals(ASTNodeType::TypeAlias))
           && !insideFunctionScope(scopeStack)) {
            std::u8string name = getTypeSpecName(tree, node);
            if(name.empty()) continue;

            SymbolKind kind;
            bool       isContainer = false;
            if(node.typeEquals(ASTNodeType::TypeAlias)) {
                kind = SymbolKind::UsingAlias;
            } else if(findChild(tree, node, ASTNodeType::StructType)) {
                kind = SymbolKind::Struct;
                isContainer = true;
            } else if(findChild(tree, node, ASTNodeType::InterfaceType)) {
                kind = SymbolKind::Class; // closest available SymbolKind
                isContainer = true;
            } else {
                kind = SymbolKind::Typedef;
            }

            std::u8string fqn = buildFQN(scopeStack, name, u8".");
            emit(makeSymbol(name, fqn, kind, accessFromName(name),
                            i, node.start_.row_, node.start_.column_));
            if(isContainer) {
                scopeStack.push_back({name, kind, node.endByte_, Access::Unknown, false});
            }
            continue;
        }

        // ── Struct fields (named and embedded) ───────────────────────────────
        if(node.typeEquals(ASTNodeType::FieldDeclaration) && !insideFunctionScope(scopeStack)) {
            bool any = false;
            for(uintptr_t id : node.children_) {
                if(id == ast::InvalidId) continue;
                const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
                if(!child.typeEquals(ASTNodeType::FieldIdentifier)) continue;
                any = true;
                std::u8string name = child.getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Field, accessFromName(name),
                                i, node.start_.row_, node.start_.column_));
            }
            if(!any) {
                // Embedded field: no explicit name, so Go uses the base type
                // name as the implicit field name.
                for(uintptr_t id : node.children_) {
                    if(id == ast::InvalidId) continue;
                    const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
                    std::u8string name = baseTypeName(tree, child);
                    if(name.empty()) continue;
                    std::u8string fqn = buildFQN(scopeStack, name, u8".");
                    emit(makeSymbol(name, fqn, SymbolKind::Field, accessFromName(name),
                                    i, node.start_.row_, node.start_.column_));
                    break;
                }
            }
            continue;
        }

        // ── Interface method signatures ─────────────────────────────────────
        if(node.typeEquals(ASTNodeType::MethodElem)) {
            const ast::ASTNode* ident = findChild(tree, node, ASTNodeType::FieldIdentifier);
            if(ident) {
                std::u8string name = ident->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Method, accessFromName(name),
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Free function ────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::FunctionDeclaration)) {
            const ast::ASTNode* ident = findChild(tree, node, ASTNodeType::Identifier);
            if(ident) {
                std::u8string name = ident->getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Function, accessFromName(name),
                                i, node.start_.row_, node.start_.column_));
            }
            // Pushed even without a name so nested var/const declarations in an
            // (malformed) body are still correctly recognized as local.
            scopeStack.push_back({{}, SymbolKind::Function, node.endByte_, Access::Unknown, false});
            continue;
        }

        // ── Method (function with a receiver) ────────────────────────────────
        if(node.typeEquals(ASTNodeType::MethodDeclaration)) {
            const ast::ASTNode* ident = findChild(tree, node, ASTNodeType::FieldIdentifier);
            if(ident) {
                std::u8string name      = ident->getText();
                std::u8string recvType  = getReceiverTypeName(tree, node);
                std::u8string qualified = recvType.empty() ? name : (recvType + u8"." + name);
                std::u8string fqn       = buildFQN(scopeStack, qualified, u8".");
                emit(makeSymbol(name, fqn, SymbolKind::Method, accessFromName(name),
                                i, node.start_.row_, node.start_.column_));
            }
            scopeStack.push_back({{}, SymbolKind::Method, node.endByte_, Access::Unknown, false});
            continue;
        }

        // ── Package-level const / var ────────────────────────────────────────
        if((node.typeEquals(ASTNodeType::ConstSpec) || node.typeEquals(ASTNodeType::VarSpec))
           && !insideFunctionScope(scopeStack)) {
            bool isConst = node.typeEquals(ASTNodeType::ConstSpec);
            for(uintptr_t id : node.children_) {
                if(id == ast::InvalidId) continue;
                const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
                if(!child.grammarEquals(ASTNodeType::Identifier)) continue;
                std::u8string name = child.getText();
                std::u8string fqn  = buildFQN(scopeStack, name, u8".");
                Symbol sym = makeSymbol(name, fqn, SymbolKind::Variable, accessFromName(name),
                                       i, node.start_.row_, node.start_.column_);
                sym.isConstexpr = isConst;
                emit(std::move(sym));
            }
            continue;
        }
    }

    return result;
}

} // namespace extractor
} // namespace ast
