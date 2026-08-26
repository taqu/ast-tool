#include "test_node_type.h"
#include "ast-node-type.h"
#include "ast-ir.h"
#include <iostream>
#include <string_view>

namespace ast
{
namespace
{
    bool check(bool condition, const char* description)
    {
        if(!condition)
            std::cerr << "    FAIL: " << description << "\n";
        return condition;
    }

    // ── String → enum mapping ─────────────────────────────────────────────────

    bool test_string_to_enum_known()
    {
        bool ok = true;
        ok &= check(ast_node_type_from_string("function_definition") == ASTNodeType::FunctionDefinition,
                    "function_definition → FunctionDefinition");
        ok &= check(ast_node_type_from_string("declaration") == ASTNodeType::Declaration,
                    "declaration → Declaration");
        ok &= check(ast_node_type_from_string("class_definition") == ASTNodeType::ClassDefinition,
                    "class_definition → ClassDefinition");
        ok &= check(ast_node_type_from_string("identifier") == ASTNodeType::Identifier,
                    "identifier → Identifier");
        ok &= check(ast_node_type_from_string("translation_unit") == ASTNodeType::TranslationUnit,
                    "translation_unit → TranslationUnit");
        ok &= check(ast_node_type_from_string("namespace_definition") == ASTNodeType::NamespaceDefinition,
                    "namespace_definition → NamespaceDefinition");
        ok &= check(ast_node_type_from_string("struct_specifier") == ASTNodeType::StructSpecifier,
                    "struct_specifier → StructSpecifier");
        ok &= check(ast_node_type_from_string("enum_specifier") == ASTNodeType::EnumSpecifier,
                    "enum_specifier → EnumSpecifier");
        ok &= check(ast_node_type_from_string("preproc_def") == ASTNodeType::PreprocDef,
                    "preproc_def → PreprocDef");
        ok &= check(ast_node_type_from_string("import_statement") == ASTNodeType::ImportStatement,
                    "import_statement → ImportStatement");
        ok &= check(ast_node_type_from_string("use_declaration") == ASTNodeType::UseDeclaration,
                    "use_declaration → UseDeclaration");
        ok &= check(ast_node_type_from_string("mod_item") == ASTNodeType::ModItem,
                    "mod_item → ModItem");
        ok &= check(ast_node_type_from_string("class_declaration") == ASTNodeType::ClassDeclaration,
                    "class_declaration → ClassDeclaration");
        ok &= check(ast_node_type_from_string("arrow_function") == ASTNodeType::ArrowFunction,
                    "arrow_function → ArrowFunction");
        ok &= check(ast_node_type_from_string("source_file") == ASTNodeType::SourceFile,
                    "source_file → SourceFile");
        ok &= check(ast_node_type_from_string("word") == ASTNodeType::Word,
                    "word → Word");
        return ok;
    }

    bool test_string_to_enum_unknown()
    {
        bool ok = true;
        ok &= check(ast_node_type_from_string("not_a_real_node_type") == ASTNodeType::Unknown,
                    "unknown string → Unknown");
        ok &= check(ast_node_type_from_string("") == ASTNodeType::Unknown,
                    "empty string → Unknown");
        ok &= check(ast_node_type_from_string("FUNCTION_DEFINITION") == ASTNodeType::Unknown,
                    "wrong case → Unknown (case-sensitive)");
        return ok;
    }

    // ── Enum → string mapping ─────────────────────────────────────────────────

    bool test_enum_to_string_known()
    {
        bool ok = true;
        ok &= check(ast_node_type_to_string(ASTNodeType::FunctionDefinition) == "function_definition",
                    "FunctionDefinition → function_definition");
        ok &= check(ast_node_type_to_string(ASTNodeType::Declaration) == "declaration",
                    "Declaration → declaration");
        ok &= check(ast_node_type_to_string(ASTNodeType::ClassDefinition) == "class_definition",
                    "ClassDefinition → class_definition");
        ok &= check(ast_node_type_to_string(ASTNodeType::Identifier) == "identifier",
                    "Identifier → identifier");
        ok &= check(ast_node_type_to_string(ASTNodeType::TranslationUnit) == "translation_unit",
                    "TranslationUnit → translation_unit");
        ok &= check(ast_node_type_to_string(ASTNodeType::Word) == "word",
                    "Word → word");
        return ok;
    }

    bool test_enum_to_string_unknown()
    {
        bool ok = true;
        ok &= check(ast_node_type_to_string(ASTNodeType::Unknown) == "unknown",
                    "Unknown → \"unknown\"");
        // Out-of-range value must not crash and must return a non-empty string.
        auto s = ast_node_type_to_string(static_cast<ASTNodeType>(9999));
        ok &= check(!s.empty(), "out-of-range value returns non-empty string");
        return ok;
    }

    // ── Round-trip ─────────────────────────────────────────────────────────────

    bool test_roundtrip()
    {
        bool ok = true;
        // Every known type must survive a round-trip through string and back.
        static constexpr const char* kNames[] = {
            "function_definition", "class_definition", "struct_specifier",
            "enum_specifier", "declaration", "namespace_definition",
            "identifier", "translation_unit", "mod_item", "function_item",
            "class_declaration", "import_statement", "use_declaration",
            "arrow_function", "source_file", "program", "compilation_unit",
            "field_declaration", "preproc_def", "preproc_include",
            "string_literal", "system_lib_string", "dotted_name",
        };
        for(const char* name : kNames) {
            ASTNodeType t = ast_node_type_from_string(name);
            ok &= check(t != ASTNodeType::Unknown, (std::string("round-trip: ") + name + " is not Unknown").c_str());
            ok &= check(ast_node_type_to_string(t) == name,
                        (std::string("round-trip: ") + name + " → enum → string matches").c_str());
        }
        return ok;
    }

    // ── AST extraction uses enum type ─────────────────────────────────────────

    bool test_ast_node_has_enum_type()
    {
        bool ok = true;

        AST ast = parse(u8"data/test00.cpp");
        ok &= check(static_cast<bool>(ast), "test00.cpp parsed successfully");
        if(!ok) return false;

        bool foundFuncDef   = false;
        bool foundTransUnit = false;

        for(uint32_t i = 0; i < ast.size(); ++i) {
            const ASTNode& node = ast[i];
            if(node.type_ == ASTNodeType::FunctionDefinition) foundFuncDef   = true;
            if(node.type_ == ASTNodeType::TranslationUnit)    foundTransUnit  = true;
        }

        ok &= check(foundFuncDef,   "at least one FunctionDefinition node in test00.cpp");
        ok &= check(foundTransUnit, "at least one TranslationUnit node in test00.cpp");
        return ok;
    }

    bool test_typeEquals_uses_enum()
    {
        bool ok = true;

        AST ast = parse(u8"data/test00.cpp");
        ok &= check(static_cast<bool>(ast), "test00.cpp parsed successfully");
        if(!ok) return false;

        bool found = false;
        for(uint32_t i = 0; i < ast.size(); ++i) {
            if(ast[i].typeEquals(ASTNodeType::FunctionDefinition)) {
                found = true;
                break;
            }
        }
        ok &= check(found, "typeEquals(ASTNodeType::FunctionDefinition) finds a node");
        return ok;
    }

} // anonymous namespace

bool run_tests_node_type()
{
    std::cout << "=== ASTNodeType enum tests ===" << std::endl;
    bool ok = true;

    auto run = [&](bool (*fn)(), const char* name) {
        bool r = fn();
        std::cout << "  " << name << "... " << (r ? "PASS" : "FAIL") << std::endl;
        return r;
    };

    ok &= run(test_string_to_enum_known,    "string_to_enum: known types");
    ok &= run(test_string_to_enum_unknown,  "string_to_enum: unknown/invalid input");
    ok &= run(test_enum_to_string_known,    "enum_to_string: known types");
    ok &= run(test_enum_to_string_unknown,  "enum_to_string: unknown/out-of-range");
    ok &= run(test_roundtrip,               "roundtrip: string→enum→string");
    ok &= run(test_ast_node_has_enum_type,  "AST node type_ is ASTNodeType after parse");
    ok &= run(test_typeEquals_uses_enum,    "typeEquals(ASTNodeType) works on parsed AST");

    std::cout << "=== ASTNodeType: " << (ok ? "PASS" : "FAIL") << " ===" << std::endl;
    return ok;
}

} // namespace ast
