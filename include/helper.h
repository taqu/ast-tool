#ifndef INC_AST_HELPER_H_
#define INC_AST_HELPER_H_
#include <string>
#include "ast-ir.h"
namespace ast
{
    bool isSymbol(const AST& ast, const ASTNode& node);
    bool isNamespace(const AST& ast, const ASTNode& node);
    bool hasFieldDeclarationList(const AST& ast, const ASTNode& node);
    ASTText getSymbolName(const AST& ast, const ASTNode& node);
    std::string getGetFullyQualifiedName(const AST& ast, const ASTNode& node);
}
#endif //INC_AST_HELPER_H_
