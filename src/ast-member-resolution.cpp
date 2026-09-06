#include "ast-member-resolution.h"
#include "ast-call-utils.h"
#include "ast-ir.h"
#include <algorithm>
#include <unordered_set>

namespace ast
{
namespace
{
using namespace call_utils;

bool is_type_kind(SymbolKind kind)
{
    return kind == SymbolKind::Class
        || kind == SymbolKind::Struct
        || kind == SymbolKind::Union;
}

bool is_callable_kind(SymbolKind kind)
{
    return kind == SymbolKind::Function
        || kind == SymbolKind::Method
        || kind == SymbolKind::Constructor
        || kind == SymbolKind::Destructor;
}

bool contains_node_type(const AST& ast, size_t root, ASTNodeType wanted)
{
    std::vector<size_t> stack{root};
    while(!stack.empty()) {
        size_t current = stack.back();
        stack.pop_back();
        if(ast[current].type_ == wanted) return true;
        for(uintptr_t child : ast[current].children_) {
            if(child != InvalidId) stack.push_back(static_cast<size_t>(child));
        }
    }
    return false;
}

bool subtree_has_text(const AST& ast, size_t root, std::u8string_view text)
{
    std::vector<size_t> stack{root};
    while(!stack.empty()) {
        size_t current = stack.back();
        stack.pop_back();
        const ASTNode& node = ast[current];
        if(is_identifier_type(node.type_) && !node.text_.empty()
           && node.text_.getText() == text) {
            return true;
        }
        for(uintptr_t child : node.children_) {
            if(child != InvalidId) stack.push_back(static_cast<size_t>(child));
        }
    }
    return false;
}

size_t declared_member_count(
    const TranslationUnit& typeTu,
    const WorkspaceSymbol& type,
    std::u8string_view memberName)
{
    if(type.symbol.nodeIndex >= typeTu.ast.size()) return 0;
    const ASTNode& typeNode = typeTu.ast[type.symbol.nodeIndex];
    size_t count = 0;
    for(uintptr_t childId : typeNode.children_) {
        if(childId == InvalidId) continue;
        const ASTNode& child = typeTu.ast[static_cast<size_t>(childId)];
        if(child.type_ != ASTNodeType::FieldDeclarationList) continue;
        for(uintptr_t declarationId : child.children_) {
            if(declarationId == InvalidId) continue;
            size_t declaration = static_cast<size_t>(declarationId);
            ASTNodeType declarationType = typeTu.ast[declaration].type_;
            if(declarationType != ASTNodeType::FieldDeclaration
               && declarationType != ASTNodeType::FunctionDefinition) continue;
            if(contains_node_type(typeTu.ast, declaration, ASTNodeType::FunctionDeclarator)
               && subtree_has_text(typeTu.ast, declaration, memberName)) {
                ++count;
            }
        }
    }
    return count;
}

std::u8string declared_field_type(const TranslationUnit& tu, const WorkspaceSymbol& field)
{
    if(field.symbol.nodeIndex >= tu.ast.size()) return {};
    const ASTNode& declaration = tu.ast[field.symbol.nodeIndex];
    if(declaration.type_ != ASTNodeType::FieldDeclaration) return {};
    for(uintptr_t childId : declaration.children_) {
        if(childId == InvalidId) continue;
        const ASTNode& child = tu.ast[static_cast<size_t>(childId)];
        if(child.type_ == ASTNodeType::TypeIdentifier
           || child.type_ == ASTNodeType::QualifiedIdentifier) {
            return child.getText();
        }
    }
    return {};
}

struct DeclaredReceiver
{
    bool found = false;
    std::u8string typeName;
};

DeclaredReceiver declared_local_type(
    const TranslationUnit& tu,
    size_t memberIdentifier,
    std::u8string_view receiverName)
{
    size_t function = memberIdentifier;
    while(function < tu.ast.size()
          && tu.ast[function].type_ != ASTNodeType::FunctionDefinition) {
        uintptr_t parent = tu.ast[function].parent_;
        if(parent == InvalidId) return {};
        function = static_cast<size_t>(parent);
    }
    if(function >= tu.ast.size()) return {};

    DeclaredReceiver result;
    size_t bestScopeDistance = size_t(-1);
    uintptr_t useScope = tu.scopeTree.getNodeScope(memberIdentifier);
    std::vector<size_t> stack{function};
    while(!stack.empty()) {
        size_t current = stack.back();
        stack.pop_back();
        if(current >= memberIdentifier) continue;
        const ASTNode& node = tu.ast[current];
        if(node.type_ == ASTNodeType::Declaration
           || node.type_ == ASTNodeType::ParameterDeclaration) {
            bool declaresReceiver = false;
            std::u8string typeName;
            for(uintptr_t childId : node.children_) {
                if(childId == InvalidId) continue;
                const ASTNode& child = tu.ast[static_cast<size_t>(childId)];
                if(child.type_ == ASTNodeType::TypeIdentifier
                   || child.type_ == ASTNodeType::QualifiedIdentifier) {
                    typeName = child.getText();
                }
            }
            std::vector<size_t> declarationStack{current};
            while(!declarationStack.empty()) {
                size_t declarationNode = declarationStack.back();
                declarationStack.pop_back();
                const ASTNode& child = tu.ast[declarationNode];
                if(child.type_ == ASTNodeType::Identifier && !child.text_.empty()
                   && child.text_.getText() == receiverName) {
                    declaresReceiver = true;
                }
                for(uintptr_t childId : child.children_) {
                    if(childId != InvalidId) {
                        declarationStack.push_back(static_cast<size_t>(childId));
                    }
                }
            }
            if(declaresReceiver) {
                uintptr_t declarationScope = tu.scopeTree.getNodeScope(current);
                uintptr_t visibleScope = useScope;
                size_t distance = 0;
                while(visibleScope != InvalidId && visibleScope != declarationScope) {
                    visibleScope = tu.scopeTree[visibleScope].parent_;
                    ++distance;
                }
                if(visibleScope == declarationScope) {
                    if(distance < bestScopeDistance) {
                        result = {true, std::move(typeName)};
                        bestScopeDistance = distance;
                    } else if(distance == bestScopeDistance && result.typeName != typeName) {
                        return {true, {}};
                    }
                }
            }
            continue;
        }
        for(uintptr_t childId : node.children_) {
            if(childId != InvalidId) stack.push_back(static_cast<size_t>(childId));
        }
    }
    return result;
}

const WorkspaceSymbol* resolve_receiver_field(
    const TranslationUnit& callTu,
    const Workspace& workspace,
    uintptr_t scopeId,
    std::u8string_view receiverName)
{
    if(const WorkspaceSymbol* caller = find_enclosing_caller(callTu, workspace, scopeId)) {
        std::u8string_view fqn = caller->symbol.fqn;
        size_t separator = fqn.rfind(u8"::");
        if(separator != std::u8string_view::npos) {
            std::u8string expected{fqn.substr(0, separator)};
            expected += u8"::";
            expected += receiverName;
            const WorkspaceSymbol* match = nullptr;
            for(const WorkspaceSymbol& symbol : workspace.symbols) {
                if(symbol.symbol.kind != SymbolKind::Field || symbol.symbol.fqn != expected) continue;
                if(match) return nullptr;
                match = &symbol;
            }
            if(match) return match;
        }
    }

    // A free function's local/parameter can share a name with a field elsewhere
    // in the workspace. Without an enclosing class identity, treating a unique
    // field name as the receiver would create a false relationship.
    return nullptr;
}

const WorkspaceSymbol* resolve_receiver_type(
    const Workspace& workspace,
    std::u8string_view contextFqn,
    std::u8string_view typeName)
{
    std::vector<std::u8string> expectedNames;
    if(typeName.find(u8"::") != std::u8string_view::npos) {
        expectedNames.emplace_back(typeName);
    } else {
        std::u8string_view context = contextFqn;
        size_t functionSeparator = context.rfind(u8"::");
        context = functionSeparator == std::u8string_view::npos
            ? std::u8string_view{} : context.substr(0, functionSeparator);
        while(!context.empty()) {
            std::u8string expected{context};
            expected += u8"::";
            expected += typeName;
            expectedNames.push_back(std::move(expected));
            size_t separator = context.rfind(u8"::");
            context = separator == std::u8string_view::npos
                ? std::u8string_view{} : context.substr(0, separator);
        }
        expectedNames.emplace_back(typeName);
    }

    std::vector<const WorkspaceSymbol*> candidates;
    for(const std::u8string& expected : expectedNames) {
        candidates.clear();
        for(const WorkspaceSymbol& symbol : workspace.symbols) {
            if(is_type_kind(symbol.symbol.kind) && symbol.symbol.fqn == expected) {
                candidates.push_back(&symbol);
            }
        }
        if(!candidates.empty()) break;
    }
    if(candidates.size() == 1) return candidates[0];

    // Prefer the single class/struct definition when a forward declaration of
    // the same canonical type is also present.
    const WorkspaceSymbol* definition = nullptr;
    std::u8string canonical;
    for(const WorkspaceSymbol* candidate : candidates) {
        if(canonical.empty()) canonical = candidate->symbol.fqn;
        if(candidate->symbol.fqn != canonical) return nullptr;
        const TranslationUnit* tu = workspace.get_translation_unit(candidate->sourceFile);
        if(!tu || candidate->symbol.nodeIndex >= tu->ast.size()) continue;
        if(contains_node_type(tu->ast, candidate->symbol.nodeIndex, ASTNodeType::FieldDeclarationList)) {
            if(definition) return nullptr;
            definition = candidate;
        }
    }
    return definition;
}

ResolutionResult resolve_field_member_call(
    const TranslationUnit& callTu,
    const Workspace& workspace,
    size_t memberIdentifier,
    uintptr_t scopeId)
{
    const ASTNode& member = callTu.ast[memberIdentifier];
    if(member.parent_ == InvalidId) return ResolutionResult::make_unresolved();
    const ASTNode& access = callTu.ast[static_cast<size_t>(member.parent_)];
    if(access.type_ != ASTNodeType::FieldExpression
       && access.type_ != ASTNodeType::MemberExpression) {
        return ResolutionResult::make_unresolved();
    }
    if(access.parent_ == InvalidId
       || !is_call_expression(callTu.ast[static_cast<size_t>(access.parent_)].type_)) {
        return ResolutionResult::make_unresolved();
    }

    size_t receiverIdentifier = size_t(-1);
    for(uintptr_t childId : access.children_) {
        if(childId == InvalidId || childId == memberIdentifier) continue;
        size_t child = static_cast<size_t>(childId);
        if(is_identifier_type(callTu.ast[child].type_)) {
            receiverIdentifier = child;
            break;
        }
    }
    if(receiverIdentifier == size_t(-1)) return ResolutionResult::make_unresolved();

    std::u8string receiverName = callTu.ast[receiverIdentifier].text_.getText();
    const WorkspaceSymbol* caller = find_enclosing_caller(callTu, workspace, scopeId);
    if(!caller) return ResolutionResult::make_unresolved();

    DeclaredReceiver local = declared_local_type(callTu, memberIdentifier, receiverName);
    std::u8string typeName;
    if(local.found) {
        typeName = std::move(local.typeName);
    } else {
        const WorkspaceSymbol* field = resolve_receiver_field(
            callTu, workspace, scopeId, receiverName);
        if(!field) return ResolutionResult::make_unresolved();
        const TranslationUnit* fieldTu = workspace.get_translation_unit(field->sourceFile);
        if(!fieldTu) return ResolutionResult::make_unresolved();
        typeName = declared_field_type(*fieldTu, *field);
    }
    if(typeName.empty()) return ResolutionResult::make_unresolved();

    const WorkspaceSymbol* type = resolve_receiver_type(workspace, caller->symbol.fqn, typeName);
    if(!type) return ResolutionResult::make_unresolved();
    const TranslationUnit* typeTu = workspace.get_translation_unit(type->sourceFile);
    if(!typeTu) return ResolutionResult::make_unresolved();
    std::u8string memberName = member.text_.getText();
    if(declared_member_count(*typeTu, *type, memberName) != 1) {
        return ResolutionResult::make_unresolved();
    }

    std::u8string expected = type->symbol.fqn + u8"::" + memberName;
    std::vector<WorkspaceSymbol> candidates;
    std::unordered_set<std::u8string> properCallableFqns;
    for(const WorkspaceSymbol& symbol : workspace.symbols) {
        if(symbol.symbol.fqn != expected || !is_callable_kind(symbol.symbol.kind)) continue;
        candidates.push_back(symbol);
        if(symbol.symbol.kind == SymbolKind::Method
           || symbol.symbol.kind == SymbolKind::Constructor
           || symbol.symbol.kind == SymbolKind::Destructor) {
            properCallableFqns.insert(symbol.symbol.fqn);
        }
    }
    if(!properCallableFqns.empty()) {
        std::erase_if(candidates, [&](const WorkspaceSymbol& symbol) {
            return symbol.symbol.kind == SymbolKind::Function
                && properCallableFqns.contains(symbol.symbol.fqn);
        });
    }
    if(candidates.size() != 1) return ResolutionResult::make_unresolved();
    return ResolutionResult::make_resolved(std::move(candidates[0]));
}

} // namespace

ResolutionResult resolve_relationship_identifier(
    const TranslationUnit& tu,
    const Workspace& workspace,
    const IdentifierResolver& resolver,
    size_t identifierNodeIndex,
    uintptr_t scopeId)
{
    if(identifierNodeIndex >= tu.ast.size()) return ResolutionResult::make_unresolved();
    const ASTNode& identifier = tu.ast[identifierNodeIndex];
    if(identifier.parent_ != InvalidId) {
        const ASTNode& parent = tu.ast[static_cast<size_t>(identifier.parent_)];
        if(parent.type_ == ASTNodeType::FieldExpression
           || parent.type_ == ASTNodeType::MemberExpression) {
            // Only the member-name child is eligible. Receiver identifiers keep
            // their ordinary lexical resolution behavior.
            if(parent.parent_ != InvalidId
               && is_call_expression(tu.ast[static_cast<size_t>(parent.parent_)].type_)
               && find_callee_identifier(tu.ast, static_cast<size_t>(parent.parent_)) == identifierNodeIndex) {
                return resolve_field_member_call(tu, workspace, identifierNodeIndex, scopeId);
            }
        }
    }
    if(identifier.text_.empty()) return ResolutionResult::make_unresolved();
    return resolver.resolve(identifier.text_.getText(), scopeId);
}

} // namespace ast
