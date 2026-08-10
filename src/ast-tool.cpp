#include "ast-tool.h"
#include <cassert>
#include <cstring>
#if defined(_WIN32) || defined(_WIN64)
#    include <sys/stat.h>
#    include <sys/types.h>
#    define STAT_STRUCT struct _stat64
#    define STAT_FUNC(path, buf) _stat64(path, buf)
#else
#    include <sys/stat.h>
#    include <sys/types.h>
#    define STAT_STRUCT struct stat
#    define STAT_FUNC(path, buf) stat(path, buf)
#endif
#include "xxhash.h"
#include <mimalloc-new-delete.h>
#include <mimalloc.h>
#include <tree_sitter/api.h>
#include <tree_sitter/tree-sitter-languages.h>
#include <absl/log/initialize.h>
#include "dump.h"
#include "symbols.h"
#include "outline.h"
#include "find.h"
#include "range.h"
#include "parent.h"
#include "children.h"
#include "search.h"

namespace ast
{
void initialize()
{
    ts_set_allocator(mi_malloc, mi_calloc, mi_realloc, mi_free);
    absl::InitializeLog();
}

bool parse(Arguments& arguments, int32_t argc, const char** argv)
{
    assert(nullptr != argv);
    arguments.sub_ = SubCommand::None;
    if(argc <= 1) {
        return false;
    }
    if(strncmp(argv[1], "dump", ::strlen("dump")) == 0) {
        return parse_dump(arguments, argc, argv);
    }else if(strncmp(argv[1], "symbols", ::strlen("symbols")) == 0) {
        return parse_symbols(arguments, argc, argv);
    } else if(strncmp(argv[1], "outline", ::strlen("outline")) == 0) {
        return parse_outline(arguments, argc, argv);
    } else if(strncmp(argv[1], "find", ::strlen("find")) == 0) {
        return parse_find(arguments, argc, argv);
    } else if(strncmp(argv[1], "range", ::strlen("range")) == 0) {
        return parse_range(arguments, argc, argv);
    } else if(strncmp(argv[1], "parent", ::strlen("parent")) == 0) {
        return parse_parent(arguments, argc, argv);
    } else if(strncmp(argv[1], "children", ::strlen("children")) == 0) {
        return parse_children(arguments, argc, argv);
    } else if(strncmp(argv[1], "search", ::strlen("search")) == 0) {
        return parse_search(arguments, argc, argv);
    } else {
        return false;
    }
}

bool dispatch(const Arguments& arguments)
{
    switch(arguments.sub_) {
    case SubCommand::Dump:
        return dump(arguments.dump_);
    case SubCommand::Symbols:
        return symbols(arguments.symbols_);
    case SubCommand::Outline:
        return outline(arguments.outline_);
    case SubCommand::Find:
        return find(arguments.find_);
    case SubCommand::Range:
        return range(arguments.range_);
    case SubCommand::Parent:
        return parent(arguments.parent_);
    case SubCommand::Children:
        return children(arguments.children_);
    case SubCommand::Search:
        return search(arguments.search_);
    default:
        return false;
    }
}

bool ASTNode::typeEquals(const char* type) const
{
    return 0 == ::strcmp(type_, type);
}

bool ASTNode::grammarEquals(const char* type) const
{
    return 0 == ::strcmp(grammar_type_, type);
}

std::string ASTText::getText() const
{
    return std::string(text_, text_ + length_);
}

std::string ASTNode::getText() const
{
    return text_.getText();
}

ASTLanguage get_language_type(const char* path)
{
    if(nullptr == path) {
        return ASTLanguage::Unknown;
    }
    const char* ext = ::strrchr(path, '.');
    if(nullptr == ext) {
        return ASTLanguage::Unknown;
    }
    ++ext;
    if(0 == strcmp("sh", ext)) {
        return ASTLanguage::Bash;
    } else if(0 == strcmp("c", ext)) {
        return ASTLanguage::C;
    } else if(0 == strcmp("h", ext)
              || 0 == strcmp("hpp", ext)
              || 0 == strcmp("cpp", ext)
              || 0 == strcmp("cxx", ext)
              || 0 == strcmp("cc", ext)
              || 0 == strcmp("ixx", ext)
              || 0 == strcmp("cppm", ext)) {
        return ASTLanguage::CPP;
    } else if(0 == strcmp("cs", ext)) {
        return ASTLanguage::CSharp;
    } else if(0 == strcmp("css", ext)) {
        return ASTLanguage::CSS;
    } else if(0 == strcmp("go", ext)) {
        return ASTLanguage::Go;
    } else if(0 == strcmp("html", ext)
              || 0 == strcmp("htm", ext)) {
        return ASTLanguage::HTML;
    } else if(0 == strcmp("java", ext)) {
        return ASTLanguage::Java;
    } else if(0 == strcmp("js", ext)) {
        return ASTLanguage::JavaScript;
    } else if(0 == strcmp("py", ext)) {
        return ASTLanguage::Python;
    } else if(0 == strcmp("rb", ext)) {
        return ASTLanguage::Ruby;
    } else if(0 == strcmp("rs", ext)) {
        return ASTLanguage::Rust;
    } else if(0 == strcmp("scala", ext)) {
        return ASTLanguage::Scala;
    } else if(0 == strcmp("tsx", ext)) {
        return ASTLanguage::TypeScriptX;
    } else if(0 == strcmp("ts", ext)) {
        return ASTLanguage::TypeScript;
    } else {
        return ASTLanguage::Unknown;
    }
}

const struct TSLanguage* get_language(ASTLanguage language)
{
    switch(language) {
    case ASTLanguage::Bash:
        return tree_sitter_bash();
    case ASTLanguage::C:
        return tree_sitter_c();
    case ASTLanguage::CPP:
        return tree_sitter_cpp();
    case ASTLanguage::CSharp:
        return tree_sitter_c_sharp();
    case ASTLanguage::CSS:
        return tree_sitter_css();
    case ASTLanguage::Go:
        return tree_sitter_go();
    case ASTLanguage::HTML:
        return tree_sitter_html();
    case ASTLanguage::Java:
        return tree_sitter_java();
    case ASTLanguage::JavaScript:
        return tree_sitter_javascript();
    case ASTLanguage::Python:
        return tree_sitter_python();
    case ASTLanguage::Ruby:
        return tree_sitter_ruby();
    case ASTLanguage::Rust:
        return tree_sitter_rust();
    case ASTLanguage::Scala:
        return tree_sitter_scala();
    case ASTLanguage::TypeScriptX:
        return tree_sitter_tsx();
    case ASTLanguage::TypeScript:
        return tree_sitter_typescript();
    default:
        return nullptr;
    }
}

namespace
{
    int64_t get_file_size(const char* path)
    {
        STAT_STRUCT st;

        if(STAT_FUNC(path, &st) == 0) {
            return (int64_t)st.st_size;
        }
        return -1;
    }
} // namespace

AST::AST()
    : language_(ASTLanguage::Unknown)
    , filepath_hash_(0)
    , size_(0)
    , text_(nullptr)
    , collisions_(0)
{
}

AST::AST(const char* path)
    : language_(ASTLanguage::Unknown)
    , filepath_hash_(0)
    , size_(0)
    , text_(nullptr)
    , collisions_(0)
{
    assert(nullptr != path);
    size_ = get_file_size(path);
    if(size_ < 0) {
        size_ = 0;
        return;
    }

    FILE* file = fopen(path, "rb");
    if(nullptr == file) {
        return;
    }
    language_ = get_language_type(path);
    filepath_hash_ = XXH32(path, ::strlen(path), 42);
    size_t size = static_cast<size_t>(size_);
    text_ = (char*)mi_calloc(size + 1, sizeof(char));
    if(nullptr == text_) {
        fclose(file);
        size_ = 0;
        return;
    }
    fread(text_, size, 1, file);
    text_[size] = '\0';
    fclose(file);
}

AST::~AST()
{
    collisions_ = 0;
    mi_free(text_);
    text_ = nullptr;
    size_ = 0;
    filepath_hash_ = 0;
    language_ = ASTLanguage::Unknown;
}

AST::AST(AST&& other)
    : language_(other.language_)
    , filepath_hash_(other.filepath_hash_)
    , size_(other.size_)
    , text_(other.text_)
    , ids_(std::move(other.ids_))
    , nodes_(std::move(other.nodes_))
    , collisions_(other.collisions_)
{
    other.language_ = ASTLanguage::Unknown;
    other.filepath_hash_ = 0;
    other.size_ = 0;
    other.text_ = nullptr;
    other.collisions_ = 0;
}

AST& AST::operator=(AST&& other)
{
    if(this != &other) {
        mi_free(text_);
        language_ = other.language_;
        filepath_hash_ = other.filepath_hash_;
        size_ = other.size_;
        text_ = other.text_;
        ids_ = std::move(other.ids_);
        nodes_ = std::move(other.nodes_);
        collisions_ = other.collisions_;

        other.language_ = ASTLanguage::Unknown;
        other.filepath_hash_ = 0;
        other.size_ = 0;
        other.text_ = nullptr;
        other.collisions_ = 0;
    }
    return *this;
}

AST::operator bool() const
{
    return nullptr != text_;
}

AST::operator const char*() const
{
    return text_;
}

ASTLanguage AST::language() const
{
    return language_;
}

int64_t AST::text_size() const
{
    return size_;
}

const char* AST::text() const
{
    return text_;
}

void AST::swap(AST& other)
{
    std::swap(language_, other.language_);
    std::swap(filepath_hash_, other.filepath_hash_);
    std::swap(size_, other.size_);
    std::swap(text_, other.text_);
    ids_.swap(other.ids_);
    nodes_.swap(other.nodes_);
    std::swap(collisions_, other.collisions_);
}

const ASTNode& AST::add(TSNode node)
{
    ASTNode astNode;
    astNode.id_ = reinterpret_cast<uintptr_t>(node.id);
    astNode.parent_ = reinterpret_cast<uintptr_t>(ts_node_parent(node).id);
    astNode.hash_ = 0;
    astNode.flags_ = ASTFlag::None;

    astNode.type_ = ts_node_type(node);
    astNode.grammar_type_ = ts_node_grammar_type(node);
    astNode.startByte_ = ts_node_start_byte(node);
    astNode.endByte_ = ts_node_end_byte(node);
    astNode.text_ = get_string(astNode.startByte_, astNode.endByte_);
    TSPoint start_point = ts_node_start_point(node);
    TSPoint end_point = ts_node_end_point(node);
    astNode.start_ = {start_point.row, start_point.column};
    astNode.end_ = {end_point.row, end_point.column};
    for(uint32_t i = 0; i < ts_node_child_count(node); ++i) {
        TSNode child = ts_node_child(node, i);
        astNode.children_.push_back(reinterpret_cast<uintptr_t>(child.id));
    }
    setHash(astNode);
    if(ts_node_is_named(node)) {
        astNode.flags_ |= ASTFlag::Named;
    }
    if(ts_node_is_missing(node)) {
        astNode.flags_ |= ASTFlag::Missing;
    }
    if(ts_node_is_extra(node)) {
        astNode.flags_ |= ASTFlag::Extra;
    }
    if(ts_node_has_error(node)) {
        astNode.flags_ |= ASTFlag::Error;
    }

    ids_.push_back(astNode.id_);
    nodes_.push_back(astNode);
    return nodes_.back();
}

void AST::remap_ids()
{
    for(ASTNode& node: nodes_) {
        node.id_ = find(node.id_);
        node.parent_ = find(node.parent_);
        for(uintptr_t& child: node.children_) {
            child = find(child);
        }
    }
}

uint32_t AST::size() const
{
    return nodes_.size();
}

const ASTNode& AST::operator[](size_t index) const
{
    return nodes_[index];
}

ASTText AST::get_string(uint32_t start, uint32_t end) const
{
    assert(start < size_);
    assert(end <= size_);
    return {end - start, text_ + start};
}

uintptr_t AST::find(uintptr_t id) const
{
    for(size_t i = 0; i < nodes_.size(); ++i) {
        if(ids_[i] == id) {
            return i;
        }
    }
    return InvalidId;
}

bool AST::existsHash(uint32_t hash) const
{
    for(const ASTNode& node : nodes_){
        if(hash == node.hash_){
            return true;
        }
    }
    return false;
}

void AST::setHash(ASTNode& node)
{
    XXH32_state_t* state = XXH32_createState();
    while(true){
        XXH32_reset(state, filepath_hash_);
        XXH32_update(state, node.type_, strlen(node.type_));
        XXH32_update(state, &node.startByte_, sizeof(node.startByte_));
        XXH32_update(state, &node.endByte_, sizeof(node.endByte_));
        XXH32_update(state, &collisions_, sizeof(collisions_));
        node.hash_ = XXH32_digest(state);
        if(existsHash(node.hash_)){
            ++collisions_;
        }else{
            break;
        }
    }
    XXH32_freeState(state);
}

namespace
{
    const char* read_stream(void* payload, uint32_t byte_offset, TSPoint position, uint32_t* bytes_read)
    {
        FILE* file = (FILE*)payload;
        fseek(file, byte_offset, SEEK_SET);
        static char buffer[1024];
        size_t read = fread(buffer, 1, sizeof(buffer), file);
        *bytes_read = (uint32_t)read;
        return (0 < read) ? buffer : NULL;
    }

    void traverse_all_nodes(AST& ast, TSNode root)
    {
        TSTreeCursor cursor = ts_tree_cursor_new(root);
        bool reached_root = false;

        while(!reached_root) {
            // 1. Process the current node
            TSNode current_node = ts_tree_cursor_current_node(&cursor);
            const ASTNode& astNode = ast.add(current_node);

            // 2. Try to move deeper to the first child
            if(ts_tree_cursor_goto_first_child(&cursor)) {
                continue;
            }

            // 3. If no child, try to move to the next sibling
            if(ts_tree_cursor_goto_next_sibling(&cursor)) {
                continue;
            }

            // 4. If no sibling, backtrack up to find a parent's sibling
            bool backtracking = true;
            while(backtracking) {
                // If we can't go to the parent, we are back at the root
                if(!ts_tree_cursor_goto_parent(&cursor)) {
                    reached_root = true;
                    backtracking = false;
                } else {
                    // If the parent has a sibling, move there and resume downward traversal
                    if(ts_tree_cursor_goto_next_sibling(&cursor)) {
                        backtracking = false;
                    }
                }
            }
        }

        // Always free the cursor memory when finished
        ts_tree_cursor_delete(&cursor);
    }
} // namespace

AST parse(const char* path)
{
    assert(nullptr != path);
    AST ast(path);
    if(!ast) {
        return ast;
    }

    const TSLanguage* language = get_language(ast.language());
    if(nullptr == language) {
        return ast;
    }
    TSParser* parser = ts_parser_new();
    if(nullptr == parser) {
        return ast;
    }
    ts_parser_set_language(parser, language);

    TSTree* tree = ts_parser_parse_string_encoding(parser, nullptr, ast, ast.text_size(), TSInputEncodingUTF8);

    traverse_all_nodes(ast, ts_tree_root_node(tree));
    ast.remap_ids();

    ts_tree_delete(tree);
    ts_parser_delete(parser);
    return ast;
}
} // namespace ast
