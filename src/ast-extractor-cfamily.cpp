#include "ast-extractor-cfamily.h"
#include "ast-tool.h"
#include "ast-extractor-common.h"
#include "ast-ir.h"
#include <unordered_map>

namespace ast
{
namespace extractor
{
namespace
{
    // ── Low-level helpers ─────────────────────────────────────────────────────

    // Returns true for node types that are type specifiers (not declarators).
    // These must be skipped when looking for the declared name inside a declaration.
    bool isTypeSpecifier(const ast::ASTNode& n)
    {
        return n.typeEquals(ASTNodeType::PrimitiveType)
            || n.typeEquals(ASTNodeType::TypeIdentifier)
            || n.typeEquals(ASTNodeType::NamespaceIdentifier)
            || n.typeEquals(ASTNodeType::QualifiedIdentifier)
            || n.typeEquals(ASTNodeType::SizedTypeSpecifier)
            || n.typeEquals(ASTNodeType::TypeQualifier)
            || n.typeEquals(ASTNodeType::StorageClassSpecifier)
            || n.typeEquals(ASTNodeType::FunctionSpecifier)
            || n.typeEquals(ASTNodeType::TemplateType)
            || n.typeEquals(ASTNodeType::ClassSpecifier)
            || n.typeEquals(ASTNodeType::ClassDefinition)
            || n.typeEquals(ASTNodeType::StructSpecifier)
            || n.typeEquals(ASTNodeType::StructDefinition)
            || n.typeEquals(ASTNodeType::UnionSpecifier)
            || n.typeEquals(ASTNodeType::EnumSpecifier);
    }

    // Extract the declared name from a declarator node (identifier / field_identifier /
    // or a wrapping declarator such as pointer_declarator, init_declarator, etc.).
    // Does NOT recurse into type specifiers.
    std::u8string declName(const ast::AST& tree, const ast::ASTNode& n)
    {
        if(n.typeEquals(ASTNodeType::Identifier) || n.typeEquals(ASTNodeType::FieldIdentifier)
           || n.grammarEquals(ASTNodeType::Identifier)) {
            return n.getText();
        }
        if(n.typeEquals(ASTNodeType::InitDeclarator)
           || n.typeEquals(ASTNodeType::PointerDeclarator)
           || n.typeEquals(ASTNodeType::ReferenceDeclarator)
           || n.typeEquals(ASTNodeType::ArrayDeclarator)
           || n.typeEquals(ASTNodeType::ParenthesizedDeclarator)) {
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

    // Get the declared variable name from a declaration or field_declaration node.
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

    // Get the typedef alias name: the LAST type_identifier / grammar-identifier among
    // direct children.  For "typedef std::string MyStr;" that is "MyStr", not "string".
    std::u8string getTypedefName(const ast::AST& tree, const ast::ASTNode& node)
    {
        std::u8string last;
        for(uintptr_t id : node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(ASTNodeType::TypeIdentifier) || child.grammarEquals(ASTNodeType::Identifier)) {
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
            if(child.typeEquals(ASTNodeType::TypeIdentifier)
               || child.typeEquals(ASTNodeType::NamespaceIdentifier)) {
                return child.getText();
            }
            // Stop once we reach the body node to avoid picking up base-class names.
            if(child.typeEquals(ASTNodeType::FieldDeclarationList)
               || child.typeEquals(ASTNodeType::DeclarationList)
               || child.typeEquals(ASTNodeType::Block)) {
                break;
            }
            if(child.grammarEquals(ASTNodeType::Identifier)) {
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
            if(child.typeEquals(ASTNodeType::DestructorName))      return {child.getText(), true,  false};
            if(child.typeEquals(ASTNodeType::QualifiedIdentifier)) return {child.getText(), false, true};
            if(child.typeEquals(ASTNodeType::OperatorName)
               || child.typeEquals(ASTNodeType::ConversionFunctionId)
               || child.typeEquals(ASTNodeType::OperatorCast)
               || child.typeEquals(ASTNodeType::Identifier)
               || child.typeEquals(ASTNodeType::FieldIdentifier)
               || child.grammarEquals(ASTNodeType::Identifier)) {
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
            if(child.typeEquals(ASTNodeType::AbstractPointerDeclarator)){
                return u8"*" + getAbstractDeclaratorName(tree, child);
            }
            if(child.typeEquals(ASTNodeType::AbstractReferenceDeclarator)){
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
            if(child.typeEquals(ASTNodeType::PrimitiveType)
                || child.typeEquals(ASTNodeType::TypeIdentifier)){
                std::u8string name = u8"operator " + child.getText() + getAbstractDeclaratorName(tree, declarator);
                return {name, false, false};
            }
        }
        return {};
    }

    void appendParameterType(const ast::AST& tree, const ast::ASTNode& node,
                             std::u8string& out)
    {
        if(node.typeEquals(ASTNodeType::QualifiedIdentifier)
           || node.typeEquals(ASTNodeType::PrimitiveType)
           || node.typeEquals(ASTNodeType::TypeIdentifier)
           || node.typeEquals(ASTNodeType::SizedTypeSpecifier)
           || node.typeEquals(ASTNodeType::TypeQualifier)
           || node.typeEquals(ASTNodeType::TemplateType)) {
            out += node.getText();
            return;
        }
        if(node.typeEquals(ASTNodeType::PointerDeclarator)
           || node.typeEquals(ASTNodeType::AbstractPointerDeclarator)) {
            out += u8"*";
        } else if(node.typeEquals(ASTNodeType::ReferenceDeclarator)
                  || node.typeEquals(ASTNodeType::AbstractReferenceDeclarator)) {
            out += u8"&";
        } else if(node.typeEquals(ASTNodeType::ArrayDeclarator)) {
            out += u8"[]";
        }
        for(uintptr_t id: node.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(ASTNodeType::Identifier)
               || child.typeEquals(ASTNodeType::FieldIdentifier)) continue;
            appendParameterType(tree, child, out);
        }
    }

    std::u8string callableSignature(const ast::AST& tree,
                                    const ast::ASTNode& declarator)
    {
        const ast::ASTNode* params = findChild(tree, declarator, ASTNodeType::ParameterList);
        std::u8string signature = u8"(";
        if(params) {
            bool first = true;
            for(uintptr_t id: params->children_) {
                if(id == ast::InvalidId) continue;
                const ast::ASTNode& param = tree[static_cast<uint32_t>(id)];
                if(!param.typeEquals(ASTNodeType::ParameterDeclaration)) continue;
                if(!first) signature += u8",";
                appendParameterType(tree, param, signature);
                first = false;
            }
        }
        signature += u8")";

        // Trailing cv-qualifiers (const/volatile) sit as direct children of the
        // function_declarator itself, as siblings of parameter_list — not nested
        // inside it — e.g. "void Foo::run() const". Without this, "run()" and
        // "run() const" would produce identical signatures and collapse into one
        // logical symbol, silently dropping the const overload.
        for(uintptr_t id: declarator.children_) {
            if(id == ast::InvalidId) continue;
            const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
            if(child.typeEquals(ASTNodeType::TypeQualifier)) {
                signature += u8" ";
                signature += child.getText();
            }
        }
        return signature;
    }

    // A C++ declaration is a global/namespace variable only when its parent is the
    // translation_unit root or a namespace body (declaration_list).
    bool isGlobalOrNamespaceDecl(const ast::AST& tree, const ast::ASTNode& node)
    {
        if(node.parent_ == ast::InvalidId) return true;
        const ast::ASTNode& parent = tree[static_cast<uint32_t>(node.parent_)];
        return parent.typeEquals(ASTNodeType::TranslationUnit) || parent.typeEquals(ASTNodeType::DeclarationList);
    }

} // anonymous namespace

std::vector<Symbol> extract_symbols_cfamily(const ast::AST& tree)
{
    std::vector<Symbol>             result;
    std::unordered_map<std::u8string, size_t> seen;
    std::vector<ScopeFrame>         scopeStack;
    char8_t buffer[BUFFER_SIZE];

    // Deduplicate repeated syntax for the same callable while preserving overloads.
    // When a forward declaration and its later definition share a translation
    // unit (e.g. a .cpp forward-declaring before defining), prefer the
    // definition so services that need a function body (callees) still work.
    auto emit = [&](Symbol sym) {
        if(sym.fqn.empty()) return;
        bool funcLike = sym.kind == SymbolKind::Function
                     || sym.kind == SymbolKind::Method
                     || sym.kind == SymbolKind::Constructor
                     || sym.kind == SymbolKind::Destructor;
        std::u8string key = funcLike
            ? (sym.fqn + sym.signature)
            : (sym.fqn + u8":" + ast::to_string_intermediate(buffer, static_cast<int32_t>(sym.kind)));
        auto it = seen.find(key);
        if(it == seen.end()) {
            seen.emplace(std::move(key), result.size());
            result.push_back(std::move(sym));
        } else if(funcLike && sym.isDefinition && !result[it->second].isDefinition) {
            result[it->second] = std::move(sym);
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
        if(node.typeEquals(ASTNodeType::AccessSpecifier)) {
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
        if(node.typeEquals(ASTNodeType::NamespaceDefinition)) {
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
        if(node.typeEquals(ASTNodeType::ClassSpecifier) || node.typeEquals(ASTNodeType::ClassDefinition)) {
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
        if(node.typeEquals(ASTNodeType::StructSpecifier) || node.typeEquals(ASTNodeType::StructDefinition)) {
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
        if(node.typeEquals(ASTNodeType::UnionSpecifier)) {
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
        if(node.typeEquals(ASTNodeType::EnumSpecifier)) {
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
        if(node.typeEquals(ASTNodeType::Enumerator)) {
            const ast::ASTNode* ident = findChild(tree, node, ASTNodeType::Identifier);
            if(ident) {
                std::u8string name = ident->getText();
                std::u8string fqn  = buildFQN(scopeStack, name);
                emit(makeSymbol(name, fqn, SymbolKind::EnumValue, Access::Unknown,
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Functions / Methods / Constructors / Destructors ──────────────
        if(node.typeEquals(ASTNodeType::FunctionDefinition) || node.typeEquals(ASTNodeType::Declaration)) {
            const ast::ASTNode* funcDecl = findChild(tree, node, ASTNodeType::FunctionDeclarator);
            if(!funcDecl) {
                const ast::ASTNode* referenceDecl = findChild(tree, node, ASTNodeType::ReferenceDeclarator);
                if(referenceDecl) {
                    funcDecl = findChild(tree, *referenceDecl, ASTNodeType::FunctionDeclarator);
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
                    sym.signature = callableSignature(tree, *funcDecl);
                    sym.isDefinition = node.typeEquals(ASTNodeType::FunctionDefinition);
                    sym.isStatic    = childHasText(tree, node, u8"static");
                    sym.isConstexpr = childHasText(tree, node, u8"constexpr");
                    sym.isInline    = childHasText(tree, node, u8"inline");
                    emit(std::move(sym));
                }
                continue;
            }
            const ast::ASTNode* operatorCastDecl = findChild(tree, node, ASTNodeType::OperatorCast);
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
                    sym.signature = callableSignature(tree, *operatorCastDecl);
                    sym.isDefinition = node.typeEquals(ASTNodeType::FunctionDefinition);
                    sym.isStatic    = childHasText(tree, node, u8"static");
                    sym.isConstexpr = childHasText(tree, node, u8"constexpr");
                    sym.isInline    = childHasText(tree, node, u8"inline");
                    emit(std::move(sym));
                }
                continue;
            }

            // Plain declaration (no function_declarator) → possible global variable
            if(node.typeEquals(ASTNodeType::Declaration) && isGlobalOrNamespaceDecl(tree, node)) {
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
        if(node.typeEquals(ASTNodeType::FieldDeclaration) && inNamedClassScope(scopeStack)) {
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
                const ast::ASTNode* funcDecl = findChild(tree, node, ASTNodeType::FunctionDeclarator);
                if(!funcDecl) {
                    const ast::ASTNode* referenceDecl = findChild(tree, node, ASTNodeType::ReferenceDeclarator);
                    if(referenceDecl){
                        funcDecl = findChild(tree, *referenceDecl, ASTNodeType::FunctionDeclarator);
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
                        sym.signature = callableSignature(tree, *funcDecl);
                        sym.isDefinition = false;
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
        if(node.typeEquals(ASTNodeType::PreprocDef)) {
            const ast::ASTNode* ident = findChild(tree, node, ASTNodeType::Identifier);
            if(ident) {
                std::u8string name = ident->getText();
                emit(makeSymbol(name, name, SymbolKind::Macro, Access::Unknown,
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Typedef ──────────────────────────────────────────────────────
        if(node.typeEquals(ASTNodeType::TypeDefinition)) {
            std::u8string name = getTypedefName(tree, node);
            if(!name.empty()) {
                std::u8string fqn = buildFQN(scopeStack, name);
                emit(makeSymbol(name, fqn, SymbolKind::Typedef, topAccess(scopeStack),
                                i, node.start_.row_, node.start_.column_));
            }
            continue;
        }

        // ── Using alias (using X = ...) ───────────────────────────────────
        if(node.typeEquals(ASTNodeType::AliasDeclaration)) {
            std::u8string name;
            for(uintptr_t id : node.children_) {
                if(id == ast::InvalidId) continue;
                const ast::ASTNode& child = tree[static_cast<uint32_t>(id)];
                if(child.typeEquals(ASTNodeType::TypeIdentifier) || child.grammarEquals(ASTNodeType::Identifier)) {
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
