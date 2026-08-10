#include "ast-workspace.h"
#include <algorithm>
#include <cstring>
#include <filesystem>
#include <unordered_set>
#include "ast-extractor.h"
#include "ast-scope-builder.h"
#include "ast-symbol-scope.h"
#include "ast-ir.h"

namespace ast
{
namespace
{
    // -----------------------------------------------------------------------
    // Include / import collection
    // -----------------------------------------------------------------------

    /** Strips one leading and one trailing character (quotes or angle brackets). */
    static std::string strip_delimiters(const std::string& s)
    {
        if(s.size() >= 2) return s.substr(1, s.size() - 2);
        return s;
    }

    /**
     * Collects direct include/import paths from @p ast.
     * Paths are returned deduplicated and in source order.
     */
    std::vector<std::string> collect_includes(const AST& ast)
    {
        std::vector<std::string> result;
        std::unordered_set<std::string> seen;

        auto push = [&](std::string path) {
            if(!path.empty() && seen.insert(path).second) {
                result.push_back(std::move(path));
            }
        };

        for(uint32_t i = 0; i < ast.size(); ++i) {
            const ASTNode& node = ast[i];
            const char* t = node.type_;

            // C / C++: #include "foo.h" or #include <foo.h>
            if(0 == ::strcmp(t, "preproc_include")) {
                for(uintptr_t childIdx : node.children_) {
                    if(childIdx == InvalidId) continue;
                    const ASTNode& child = ast[childIdx];
                    const char* ct = child.type_;
                    if(0 == ::strcmp(ct, "string_literal")
                    || 0 == ::strcmp(ct, "system_lib_string")) {
                        push(strip_delimiters(child.getText()));
                        break;
                    }
                }
                continue;
            }

            // Python: import numpy  /  from os import path
            if(0 == ::strcmp(t, "import_statement")
            || 0 == ::strcmp(t, "import_from_statement")) {
                // Look for dotted_name or relative_import child as the module identifier
                for(uintptr_t childIdx : node.children_) {
                    if(childIdx == InvalidId) continue;
                    const ASTNode& child = ast[childIdx];
                    const char* ct = child.type_;
                    if(0 == ::strcmp(ct, "dotted_name")
                    || 0 == ::strcmp(ct, "relative_import")) {
                        push(child.getText());
                        break;
                    }
                }
                continue;
            }

            // JavaScript / TypeScript: import ... from "module"
            if(0 == ::strcmp(t, "import_statement")) {
                for(uintptr_t childIdx : node.children_) {
                    if(childIdx == InvalidId) continue;
                    const ASTNode& child = ast[childIdx];
                    if(0 == ::strcmp(child.type_, "string")) {
                        push(strip_delimiters(child.getText()));
                        break;
                    }
                }
                continue;
            }

            // Rust: use std::collections::HashMap;
            if(0 == ::strcmp(t, "use_declaration")) {
                push(node.getText());
                continue;
            }

            // Java / Go: import java.util.List  /  import "fmt"
            if(0 == ::strcmp(t, "import_declaration")) {
                push(node.getText());
                continue;
            }

            // Go: individual path inside import block: import_spec → interpreted_string_literal
            if(0 == ::strcmp(t, "import_spec")) {
                for(uintptr_t childIdx : node.children_) {
                    if(childIdx == InvalidId) continue;
                    const ASTNode& child = ast[childIdx];
                    if(0 == ::strcmp(child.type_, "interpreted_string_literal")) {
                        push(strip_delimiters(child.getText()));
                        break;
                    }
                }
                continue;
            }
        }

        return result;
    }

    // -----------------------------------------------------------------------
    // Per-file analysis
    // -----------------------------------------------------------------------

    void analyze_one(const std::string& path, Workspace& ws)
    {
        AST ast = parse(path.c_str());
        if(!ast) {
            ++ws.failedCount;
            return;
        }
        ++ws.parsedCount;

        // Extract symbols
        std::vector<Symbol> syms = extract_symbols(ast);

        // Scope analysis for owningScope
        ScopeTree tree = build_scope_tree(ast);
        associate_symbols(tree, syms);

        // Merge symbols into workspace symbol table
        for(size_t i = 0; i < syms.size(); ++i) {
            const Symbol& s = syms[i];

            uintptr_t scopeId = tree.getScopeOfSymbol(i);
            ScopeKind owning  = (scopeId != ScopeTree::InvalidId)
                              ? tree[scopeId].kind_
                              : ScopeKind::Unknown;

            ws.symbols.push_back({
                s.name, s.fqn, s.kind, s.access,
                s.isStatic, s.isConstexpr, s.isInline,
                path, s.line, s.column, s.nodeIndex, owning
            });
        }

        // Collect include/import dependencies
        FileDependencies dep;
        dep.file     = path;
        dep.includes = collect_includes(ast);
        ws.deps.push_back(std::move(dep));
    }

} // namespace

// -----------------------------------------------------------------------
// Public API
// -----------------------------------------------------------------------

std::vector<std::string> scan_workspace(const char* root)
{
    std::vector<std::string> files;
    if(nullptr == root) return files;

    std::error_code ec;
    std::filesystem::recursive_directory_iterator it(root, ec);
    if(ec) return files;

    for(const auto& entry : it) {
        if(!entry.is_regular_file()) continue;
        std::string path = entry.path().generic_string();
        if(get_language_type(path.c_str()) != ASTLanguage::Unknown) {
            files.push_back(std::move(path));
        }
    }

    std::sort(files.begin(), files.end());
    return files;
}

Workspace analyze_workspace(const char* root)
{
    return analyze_files(scan_workspace(root));
}

Workspace analyze_files(const std::vector<std::string>& files)
{
    Workspace ws;
    ws.files = files;
    ws.symbols.reserve(files.size() * 8); // rough heuristic
    ws.deps.reserve(files.size());

    for(const std::string& path : files) {
        analyze_one(path, ws);
    }

    return ws;
}

} // namespace ast
