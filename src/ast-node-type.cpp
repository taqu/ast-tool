#include "ast-node-type.h"
#include <algorithm>
#include <array>

namespace ast
{
namespace
{
    struct Entry
    {
        std::string_view name;
        ASTNodeType      type;
    };

    // Sorted by name for binary search in ast_node_type_from_string().
    // MUST remain sorted; add new entries in alphabetical order.
    static constexpr std::array<Entry, 193> kTable = {{
        { "abstract_class_declaration",          ASTNodeType::AbstractClassDeclaration         },
        { "abstract_method_signature",           ASTNodeType::AbstractMethodSignature          },
        { "abstract_pointer_declarator",         ASTNodeType::AbstractPointerDeclarator        },
        { "abstract_reference_declarator",       ASTNodeType::AbstractReferenceDeclarator      },
        { "access_modifier",                     ASTNodeType::AccessModifier                   },
        { "access_specifier",                    ASTNodeType::AccessSpecifier                  },
        { "accessibility_modifier",              ASTNodeType::AccessibilityModifier            },
        { "accessor_list",                       ASTNodeType::AccessorList                     },
        { "alias_declaration",                   ASTNodeType::AliasDeclaration                 },
        { "alias_qualified_name",                ASTNodeType::AliasQualifiedName               },
        { "ambient_declaration",                 ASTNodeType::AmbientDeclaration               },
        { "annotation_type_declaration",         ASTNodeType::AnnotationTypeDeclaration        },
        { "annotation_type_element_declaration", ASTNodeType::AnnotationTypeElementDeclaration },
        { "argument_list",                       ASTNodeType::ArgumentList                     },
        { "array_declarator",                    ASTNodeType::ArrayDeclarator                  },
        { "arrow_expression_clause",             ASTNodeType::ArrowExpressionClause            },
        { "arrow_function",                      ASTNodeType::ArrowFunction                    },
        { "assignment",                          ASTNodeType::Assignment                       },
        { "attribute",                           ASTNodeType::Attribute                        },
        { "attribute_name",                      ASTNodeType::AttributeName                    },
        { "attribute_value",                     ASTNodeType::AttributeValue                   },
        { "block",                               ASTNodeType::Block                            },
        { "call",                                ASTNodeType::Call                             },
        { "call_expression",                     ASTNodeType::CallExpression                   },
        { "class",                               ASTNodeType::Class                            },
        { "class_declaration",                   ASTNodeType::ClassDeclaration                 },
        { "class_definition",                    ASTNodeType::ClassDefinition                  },
        { "class_name",                          ASTNodeType::ClassName                        },
        { "class_parameter",                     ASTNodeType::ClassParameter                   },
        { "class_selector",                      ASTNodeType::ClassSelector                    },
        { "class_specifier",                     ASTNodeType::ClassSpecifier                   },
        { "class_variable",                      ASTNodeType::ClassVariable                    },
        { "closure_expression",                  ASTNodeType::ClosureExpression                },
        { "compilation_unit",                    ASTNodeType::CompilationUnit                  },
        { "compound_statement",                  ASTNodeType::CompoundStatement                },
        { "const_item",                          ASTNodeType::ConstItem                        },
        { "const_spec",                          ASTNodeType::ConstSpec                        },
        { "constant",                            ASTNodeType::Constant                         },
        { "constructor_declaration",             ASTNodeType::ConstructorDeclaration           },
        { "conversion_function_id",              ASTNodeType::ConversionFunctionId             },
        { "conversion_operator_declaration",     ASTNodeType::ConversionOperatorDeclaration    },
        { "custom_property_name",                ASTNodeType::CustomPropertyName               },
        { "declaration",                         ASTNodeType::Declaration                      },
        { "declaration_list",                    ASTNodeType::DeclarationList                  },
        { "delegate_declaration",                ASTNodeType::DelegateDeclaration              },
        { "destructor_declaration",              ASTNodeType::DestructorDeclaration            },
        { "destructor_name",                     ASTNodeType::DestructorName                   },
        { "dotted_name",                         ASTNodeType::DottedName                       },
        { "enum_constant",                       ASTNodeType::EnumConstant                     },
        { "enum_declaration",                    ASTNodeType::EnumDeclaration                  },
        { "enum_item",                           ASTNodeType::EnumItem                         },
        { "enum_member_declaration",             ASTNodeType::EnumMemberDeclaration            },
        { "enum_member_declaration_list",        ASTNodeType::EnumMemberDeclarationList        },
        { "enum_specifier",                      ASTNodeType::EnumSpecifier                    },
        { "enum_variant",                        ASTNodeType::EnumVariant                      },
        { "enumerator",                          ASTNodeType::Enumerator                       },
        { "event_declaration",                   ASTNodeType::EventDeclaration                 },
        { "event_field_declaration",             ASTNodeType::EventFieldDeclaration            },
        { "field_declaration",                   ASTNodeType::FieldDeclaration                 },
        { "field_declaration_list",              ASTNodeType::FieldDeclarationList             },
        { "field_definition",                    ASTNodeType::FieldDefinition                  },
        { "field_expression",                    ASTNodeType::FieldExpression                  },
        { "field_identifier",                    ASTNodeType::FieldIdentifier                  },
        { "file_scoped_namespace_declaration",   ASTNodeType::FileScopedNamespaceDeclaration   },
        { "func_declaration",                    ASTNodeType::FuncDeclaration                  },
        { "func_literal",                        ASTNodeType::FuncLiteral                      },
        { "function_declaration",                ASTNodeType::FunctionDeclaration              },
        { "function_declaration_statement",      ASTNodeType::FunctionDeclarationStatement     },
        { "function_declarator",                 ASTNodeType::FunctionDeclarator               },
        { "function_definition",                 ASTNodeType::FunctionDefinition               },
        { "function_expression",                 ASTNodeType::FunctionExpression               },
        { "function_item",                       ASTNodeType::FunctionItem                     },
        { "function_signature_item",             ASTNodeType::FunctionSignatureItem            },
        { "function_specifier",                  ASTNodeType::FunctionSpecifier                },
        { "generator_function",                  ASTNodeType::GeneratorFunction                },
        { "generator_function_declaration",      ASTNodeType::GeneratorFunctionDeclaration     },
        { "generic_function",                    ASTNodeType::GenericFunction                  },
        { "generic_name",                        ASTNodeType::GenericName                      },
        { "generic_type",                        ASTNodeType::GenericType                      },
        { "global_variable",                     ASTNodeType::GlobalVariable                   },
        { "id_name",                             ASTNodeType::IdName                           },
        { "id_selector",                         ASTNodeType::IdSelector                       },
        { "identifier",                          ASTNodeType::Identifier                       },
        { "impl_item",                           ASTNodeType::ImplItem                         },
        { "import_declaration",                  ASTNodeType::ImportDeclaration                },
        { "import_from_statement",               ASTNodeType::ImportFromStatement              },
        { "import_spec",                         ASTNodeType::ImportSpec                       },
        { "import_statement",                    ASTNodeType::ImportStatement                  },
        { "init_declarator",                     ASTNodeType::InitDeclarator                   },
        { "interface_declaration",               ASTNodeType::InterfaceDeclaration             },
        { "interface_type",                      ASTNodeType::InterfaceType                    },
        { "internal_module",                     ASTNodeType::InternalModule                   },
        { "interpreted_string_literal",          ASTNodeType::InterpretedStringLiteral         },
        { "jsx_opening_element",                 ASTNodeType::JsxOpeningElement                },
        { "jsx_self_closing_element",            ASTNodeType::JsxSelfClosingElement            },
        { "keyframes_name",                      ASTNodeType::KeyframesName                    },
        { "keyframes_statement",                 ASTNodeType::KeyframesStatement               },
        { "lambda_expression",                   ASTNodeType::LambdaExpression                 },
        { "lexical_declaration",                 ASTNodeType::LexicalDeclaration               },
        { "local_function_statement",            ASTNodeType::LocalFunctionStatement           },
        { "member_expression",                   ASTNodeType::MemberExpression                 },
        { "method",                              ASTNodeType::Method                           },
        { "method_declaration",                  ASTNodeType::MethodDeclaration                },
        { "method_definition",                   ASTNodeType::MethodDefinition                 },
        { "method_elem",                         ASTNodeType::MethodElem                       },
        { "method_invocation",                   ASTNodeType::MethodInvocation                 },
        { "method_signature",                    ASTNodeType::MethodSignature                  },
        { "mod_item",                            ASTNodeType::ModItem                          },
        { "modifier",                            ASTNodeType::Modifier                         },
        { "modifiers",                           ASTNodeType::Modifiers                        },
        { "module",                              ASTNodeType::Module                           },
        { "module_declaration",                  ASTNodeType::ModuleDeclaration                },
        { "namespace_declaration",               ASTNodeType::NamespaceDeclaration             },
        { "namespace_definition",                ASTNodeType::NamespaceDefinition              },
        { "namespace_identifier",                ASTNodeType::NamespaceIdentifier              },
        { "object",                              ASTNodeType::Object                           },
        { "object_definition",                   ASTNodeType::ObjectDefinition                 },
        { "operator",                            ASTNodeType::Operator                         },
        { "operator_cast",                       ASTNodeType::OperatorCast                     },
        { "operator_declaration",                ASTNodeType::OperatorDeclaration              },
        { "operator_name",                       ASTNodeType::OperatorName                     },
        { "package_clause",                      ASTNodeType::PackageClause                    },
        { "package_declaration",                 ASTNodeType::PackageDeclaration               },
        { "package_identifier",                  ASTNodeType::PackageIdentifier                },
        { "package_object",                      ASTNodeType::PackageObject                    },
        { "pair",                                ASTNodeType::Pair                             },
        { "parameter_declaration",               ASTNodeType::ParameterDeclaration             },
        { "parameter_list",                      ASTNodeType::ParameterList                    },
        { "parameters",                          ASTNodeType::Parameters                       },
        { "parenthesized_declarator",            ASTNodeType::ParenthesizedDeclarator          },
        { "pointer_declarator",                  ASTNodeType::PointerDeclarator                },
        { "pointer_type",                        ASTNodeType::PointerType                      },
        { "preproc_def",                         ASTNodeType::PreprocDef                       },
        { "preproc_include",                     ASTNodeType::PreprocInclude                   },
        { "primitive_type",                      ASTNodeType::PrimitiveType                    },
        { "private_property_identifier",         ASTNodeType::PrivatePropertyIdentifier        },
        { "program",                             ASTNodeType::Program                          },
        { "property_declaration",                ASTNodeType::PropertyDeclaration              },
        { "property_identifier",                 ASTNodeType::PropertyIdentifier               },
        { "property_name",                       ASTNodeType::PropertyName                     },
        { "property_signature",                  ASTNodeType::PropertySignature                },
        { "public_field_definition",             ASTNodeType::PublicFieldDefinition            },
        { "qualified_identifier",                ASTNodeType::QualifiedIdentifier              },
        { "qualified_name",                      ASTNodeType::QualifiedName                    },
        { "qualified_type",                      ASTNodeType::QualifiedType                    },
        { "quoted_attribute_value",              ASTNodeType::QuotedAttributeValue             },
        { "record_declaration",                  ASTNodeType::RecordDeclaration                },
        { "reference_declarator",                ASTNodeType::ReferenceDeclarator              },
        { "relative_import",                     ASTNodeType::RelativeImport                   },
        { "scoped_identifier",                   ASTNodeType::ScopedIdentifier                 },
        { "self_closing_element",                ASTNodeType::SelfClosingElement               },
        { "self_parameter",                      ASTNodeType::SelfParameter                    },
        { "simple_symbol",                       ASTNodeType::SimpleSymbol                     },
        { "singleton_method",                    ASTNodeType::SingletonMethod                  },
        { "sized_type_specifier",                ASTNodeType::SizedTypeSpecifier               },
        { "source_file",                         ASTNodeType::SourceFile                       },
        { "start_tag",                           ASTNodeType::StartTag                         },
        { "statement_block",                     ASTNodeType::StatementBlock                   },
        { "static_item",                         ASTNodeType::StaticItem                       },
        { "storage_class_specifier",             ASTNodeType::StorageClassSpecifier            },
        { "string",                              ASTNodeType::String                           },
        { "string_literal",                      ASTNodeType::StringLiteral                    },
        { "struct_declaration",                  ASTNodeType::StructDeclaration                },
        { "struct_definition",                   ASTNodeType::StructDefinition                 },
        { "struct_item",                         ASTNodeType::StructItem                       },
        { "struct_specifier",                    ASTNodeType::StructSpecifier                  },
        { "struct_type",                         ASTNodeType::StructType                       },
        { "system_lib_string",                   ASTNodeType::SystemLibString                  },
        { "tag_name",                            ASTNodeType::TagName                          },
        { "template_function",                   ASTNodeType::TemplateFunction                 },
        { "template_type",                       ASTNodeType::TemplateType                     },
        { "trait_definition",                    ASTNodeType::TraitDefinition                  },
        { "trait_item",                          ASTNodeType::TraitItem                        },
        { "translation_unit",                    ASTNodeType::TranslationUnit                  },
        { "type_alias",                          ASTNodeType::TypeAlias                        },
        { "type_alias_declaration",              ASTNodeType::TypeAliasDeclaration             },
        { "type_definition",                     ASTNodeType::TypeDefinition                   },
        { "type_identifier",                     ASTNodeType::TypeIdentifier                   },
        { "type_item",                           ASTNodeType::TypeItem                         },
        { "type_qualifier",                      ASTNodeType::TypeQualifier                    },
        { "type_spec",                           ASTNodeType::TypeSpec                         },
        { "union_specifier",                     ASTNodeType::UnionSpecifier                   },
        { "use_declaration",                     ASTNodeType::UseDeclaration                   },
        { "using_directive",                     ASTNodeType::UsingDirective                   },
        { "val_definition",                      ASTNodeType::ValDefinition                    },
        { "var_definition",                      ASTNodeType::VarDefinition                    },
        { "var_spec",                            ASTNodeType::VarSpec                          },
        { "variable_assignment",                 ASTNodeType::VariableAssignment               },
        { "variable_declaration",                ASTNodeType::VariableDeclaration              },
        { "variable_declarator",                 ASTNodeType::VariableDeclarator               },
        { "variable_name",                       ASTNodeType::VariableName                     },
        { "visibility_modifier",                 ASTNodeType::VisibilityModifier               },
        { "word",                                ASTNodeType::Word                             },
    }};

    // Reverse table: ASTNodeType integer value → string name.
    // Built from kTable; index == ASTNodeType integer value.
    // Unknown (0) → "unknown"; values not in the table → "unknown".
    struct ReverseEntry
    {
        std::string_view name;
    };

    static constexpr std::string_view kUnknownName = "unknown";

    // Maximum enum value defined above.
    static constexpr uint16_t kMaxValue = 193;

    // Reverse lookup array indexed by ASTNodeType integer value.
    // Populated statically via the lambda below.
    static constexpr auto makeReverseTable() noexcept
    {
        std::array<std::string_view, kMaxValue + 1> arr{};
        for(auto& e : arr) e = kUnknownName;
        for(const auto& e : kTable) {
            auto idx = static_cast<uint16_t>(e.type);
            if(idx <= kMaxValue) arr[idx] = e.name;
        }
        return arr;
    }

    static constexpr auto kReverse = makeReverseTable();

} // anonymous namespace

ASTNodeType ast_node_type_from_string(std::string_view name) noexcept
{
    // Binary search over the sorted kTable.
    auto it = std::lower_bound(
        kTable.begin(), kTable.end(), name,
        [](const Entry& e, std::string_view n) { return e.name < n; });

    if(it != kTable.end() && it->name == name) {
        return it->type;
    }
    return ASTNodeType::Unknown;
}

std::string_view ast_node_type_to_string(ASTNodeType type) noexcept
{
    auto idx = static_cast<uint16_t>(type);
    if(idx > kMaxValue) return kUnknownName;
    return kReverse[idx];
}

} // namespace ast
