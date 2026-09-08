#include "ast-extractor.h"
#include "ast-ir.h"
#include "ast-extractor-langs.h"

namespace ast
{

std::vector<Symbol> extract_symbols(const ast::AST& tree)
{
    switch(tree.language()) {
    case ASTLanguage::Bash:
        return extractor::extract_symbols_bash(tree);

    case ASTLanguage::C:
        return extractor::extract_symbols_c(tree);

    case ASTLanguage::CPP:
        return extractor::extract_symbols_cpp(tree);

    case ASTLanguage::CSharp:
        return extractor::extract_symbols_csharp(tree);

    case ASTLanguage::CSS:
        return extractor::extract_symbols_css(tree);

    case ASTLanguage::Go:
        return extractor::extract_symbols_go(tree);

    case ASTLanguage::HTML:
        return extractor::extract_symbols_html(tree);

    case ASTLanguage::Java:
        return extractor::extract_symbols_java(tree);

    case ASTLanguage::JavaScript:
        return extractor::extract_symbols_javascript(tree);

    case ASTLanguage::Python:
        return extractor::extract_symbols_python(tree);

    case ASTLanguage::Ruby:
        return extractor::extract_symbols_ruby(tree);

    case ASTLanguage::Rust:
        return extractor::extract_symbols_rust(tree);

    case ASTLanguage::Scala:
        return extractor::extract_symbols_scala(tree);

    case ASTLanguage::TypeScriptX:
        return extractor::extract_symbols_tsx(tree);

    case ASTLanguage::TypeScript:
        return extractor::extract_symbols_typescript(tree);

    case ASTLanguage::Unknown:
    default:
        return {};
    }
}

const char* getSymbolKindName(SymbolKind kind)
{
    switch(kind){
    case SymbolKind::Namespace:
        return "Namespace";
case SymbolKind::Class:
        return "Class";
    case SymbolKind::Struct:
        return "Struct";
    case SymbolKind::Union:
        return "Union";
    case SymbolKind::Enum:
        return "Enum";
    case SymbolKind::Function:
        return "Function";
    case SymbolKind::Method:
        return "Method";
    case SymbolKind::Constructor:
        return "Constructor";
    case SymbolKind::Destructor:
        return "Destructor";
    case SymbolKind::Variable:
        return "Variable";
    case SymbolKind::Field:
        return "Field";
    case SymbolKind::EnumValue:
        return "EnumValue";
    case SymbolKind::Macro:
        return "Macro";
    case SymbolKind::Typedef:
        return "Typedef";
    case SymbolKind::UsingAlias:
        return "UsingAlias";
    default:
        return "Unknown";
    }
}

const char* getAccessName(Access access)
{
    switch(access){
    case Access::Public:
        return "public";
    case Access::Private:
        return "private";
    case Access::Protected:
        return "protected";
    case Access::Unknown:
    default:
        return "unknown";
    }
}
} // namespace ast
