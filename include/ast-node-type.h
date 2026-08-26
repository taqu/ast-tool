#ifndef INC_AST_NODE_TYPE_H_
#define INC_AST_NODE_TYPE_H_
/**
 * @file ast-node-type.h
 * @brief Stable enum identifiers for tree-sitter AST node type names.
 *
 * ASTNodeType replaces raw `const char*` node-type pointers in ASTNode,
 * producing a compact, pointer-free representation suitable for binary
 * serialization.  Values are explicitly assigned and must not be renumbered;
 * new entries always append at the end with the next available integer.
 *
 * Each enum value corresponds to exactly one tree-sitter grammar node name
 * string (one-to-one mapping, language-agnostic).  The same grammar string
 * "class_definition" used by both C++ and Python maps to the single value
 * ASTNodeType::ClassDefinition.
 */
#include <cstdint>
#include <string_view>

namespace ast
{

/**
 * @brief Compact, stable identifier for a tree-sitter node type name.
 *
 * ASTNodeType::Unknown (0) is the sentinel for an unrecognized grammar name.
 * All other values correspond one-to-one to a specific grammar node name string.
 */
enum class ASTNodeType : uint16_t
{
    Unknown = 0,

    AbstractClassDeclaration         =   1,
    AbstractMethodSignature          =   2,
    AbstractPointerDeclarator        =   3,
    AbstractReferenceDeclarator      =   4,
    AccessModifier                   =   5,
    AccessSpecifier                  =   6,
    AccessibilityModifier            =   7,
    AccessorList                     =   8,
    AliasDeclaration                 =   9,
    AliasQualifiedName               =  10,
    AmbientDeclaration               =  11,
    AnnotationTypeDeclaration        =  12,
    AnnotationTypeElementDeclaration =  13,
    ArgumentList                     =  14,
    ArrayDeclarator                  =  15,
    ArrowExpressionClause            =  16,
    ArrowFunction                    =  17,
    Assignment                       =  18,
    Attribute                        =  19,
    AttributeName                    =  20,
    AttributeValue                   =  21,
    Block                            =  22,
    Call                             =  23,
    CallExpression                   =  24,
    Class                            =  25,
    ClassDeclaration                 =  26,
    ClassDefinition                  =  27,
    ClassName                        =  28,
    ClassParameter                   =  29,
    ClassSelector                    =  30,
    ClassSpecifier                   =  31,
    ClassVariable                    =  32,
    ClosureExpression                =  33,
    CompilationUnit                  =  34,
    CompoundStatement                =  35,
    ConstItem                        =  36,
    ConstSpec                        =  37,
    Constant                         =  38,
    ConstructorDeclaration           =  39,
    ConversionFunctionId             =  40,
    ConversionOperatorDeclaration    =  41,
    CustomPropertyName               =  42,
    Declaration                      =  43,
    DeclarationList                  =  44,
    DelegateDeclaration              =  45,
    DestructorDeclaration            =  46,
    DestructorName                   =  47,
    DottedName                       =  48,
    EnumConstant                     =  49,
    EnumDeclaration                  =  50,
    EnumItem                         =  51,
    EnumMemberDeclaration            =  52,
    EnumMemberDeclarationList        =  53,
    EnumSpecifier                    =  54,
    EnumVariant                      =  55,
    Enumerator                       =  56,
    EventDeclaration                 =  57,
    EventFieldDeclaration            =  58,
    FieldDeclaration                 =  59,
    FieldDeclarationList             =  60,
    FieldDefinition                  =  61,
    FieldExpression                  =  62,
    FieldIdentifier                  =  63,
    FileScopedNamespaceDeclaration   =  64,
    FuncDeclaration                  =  65,
    FuncLiteral                      =  66,
    FunctionDeclaration              =  67,
    FunctionDeclarationStatement     =  68,
    FunctionDeclarator               =  69,
    FunctionDefinition               =  70,
    FunctionExpression               =  71,
    FunctionItem                     =  72,
    FunctionSignatureItem            =  73,
    FunctionSpecifier                =  74,
    GeneratorFunction                =  75,
    GeneratorFunctionDeclaration     =  76,
    GenericFunction                  =  77,
    GenericName                      =  78,
    GenericType                      =  79,
    GlobalVariable                   =  80,
    IdName                           =  81,
    IdSelector                       =  82,
    Identifier                       =  83,
    ImplItem                         =  84,
    ImportDeclaration                =  85,
    ImportFromStatement              =  86,
    ImportSpec                       =  87,
    ImportStatement                  =  88,
    InitDeclarator                   =  89,
    InterfaceDeclaration             =  90,
    InterfaceType                    =  91,
    InternalModule                   =  92,
    InterpretedStringLiteral         =  93,
    JsxOpeningElement                =  94,
    JsxSelfClosingElement            =  95,
    KeyframesName                    =  96,
    KeyframesStatement               =  97,
    LambdaExpression                 =  98,
    LexicalDeclaration               =  99,
    LocalFunctionStatement           = 100,
    MemberExpression                 = 101,
    Method                           = 102,
    MethodDeclaration                = 103,
    MethodDefinition                 = 104,
    MethodElem                       = 105,
    MethodInvocation                 = 106,
    MethodSignature                  = 107,
    ModItem                          = 108,
    Modifier                         = 109,
    Modifiers                        = 110,
    Module                           = 111,
    ModuleDeclaration                = 112,
    NamespaceDeclaration             = 113,
    NamespaceDefinition              = 114,
    NamespaceIdentifier              = 115,
    Object                           = 116,
    ObjectDefinition                 = 117,
    Operator                         = 118,
    OperatorCast                     = 119,
    OperatorDeclaration              = 120,
    OperatorName                     = 121,
    PackageClause                    = 122,
    PackageDeclaration               = 123,
    PackageIdentifier                = 124,
    PackageObject                    = 125,
    Pair                             = 126,
    ParameterDeclaration             = 127,
    ParameterList                    = 128,
    Parameters                       = 129,
    ParenthesizedDeclarator          = 130,
    PointerDeclarator                = 131,
    PointerType                      = 132,
    PreprocDef                       = 133,
    PreprocInclude                   = 134,
    PrimitiveType                    = 135,
    PrivatePropertyIdentifier        = 136,
    Program                          = 137,
    PropertyDeclaration              = 138,
    PropertyIdentifier               = 139,
    PropertyName                     = 140,
    PropertySignature                = 141,
    PublicFieldDefinition            = 142,
    QualifiedIdentifier              = 143,
    QualifiedName                    = 144,
    QualifiedType                    = 145,
    QuotedAttributeValue             = 146,
    RecordDeclaration                = 147,
    ReferenceDeclarator              = 148,
    RelativeImport                   = 149,
    ScopedIdentifier                 = 150,
    SelfClosingElement               = 151,
    SelfParameter                    = 152,
    SimpleSymbol                     = 153,
    SingletonMethod                  = 154,
    SizedTypeSpecifier               = 155,
    SourceFile                       = 156,
    StartTag                         = 157,
    StatementBlock                   = 158,
    StaticItem                       = 159,
    StorageClassSpecifier            = 160,
    String                           = 161,
    StringLiteral                    = 162,
    StructDeclaration                = 163,
    StructDefinition                 = 164,
    StructItem                       = 165,
    StructSpecifier                  = 166,
    StructType                       = 167,
    SystemLibString                  = 168,
    TagName                          = 169,
    TemplateFunction                 = 170,
    TemplateType                     = 171,
    TraitDefinition                  = 172,
    TraitItem                        = 173,
    TranslationUnit                  = 174,
    TypeAlias                        = 175,
    TypeAliasDeclaration             = 176,
    TypeDefinition                   = 177,
    TypeIdentifier                   = 178,
    TypeItem                         = 179,
    TypeQualifier                    = 180,
    TypeSpec                         = 181,
    UnionSpecifier                   = 182,
    UseDeclaration                   = 183,
    UsingDirective                   = 184,
    ValDefinition                    = 185,
    VarDefinition                    = 186,
    VarSpec                          = 187,
    VariableAssignment               = 188,
    VariableDeclaration              = 189,
    VariableDeclarator               = 190,
    VariableName                     = 191,
    VisibilityModifier               = 192,
    Word                             = 193,
};

/**
 * @brief Returns the ASTNodeType for the given tree-sitter grammar node name.
 *
 * Performs a binary search over a sorted table.  Returns ASTNodeType::Unknown
 * when @p name is not recognized.  Called once per node at the AST::add()
 * boundary; not in any per-traversal hot path.
 */
ASTNodeType ast_node_type_from_string(std::string_view name) noexcept;

/**
 * @brief Returns the canonical tree-sitter grammar name for @p type.
 *
 * Returns "unknown" for ASTNodeType::Unknown and any out-of-range value.
 * Used for human-readable output and for the hash computation in AST::add().
 */
std::string_view ast_node_type_to_string(ASTNodeType type) noexcept;

} // namespace ast
#endif // INC_AST_NODE_TYPE_H_
