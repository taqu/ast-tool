#pragma once

#include "../semantic/symbol.hpp"
#include <string>
#include <vector>
#include <unordered_map>

namespace eval::workspace {

struct FileEntry {
    std::string path;
    std::vector<std::string> includes;
    std::vector<semantic::Symbol> symbols;
};

class Index {
public:
    Index() = default;

    void addFile(FileEntry entry);
    const FileEntry* findFile(const std::string& path) const;
    const std::vector<FileEntry>& allFiles() const { return files_; }

    const semantic::Symbol* findSymbol(const std::string& name) const;

    std::vector<std::string> dependentsOf(const std::string& path) const;
    std::vector<std::string> dependenciesOf(const std::string& path) const;

    template<typename Predicate>
    std::vector<const semantic::Symbol*> findSymbolIf(Predicate pred) const {
        std::vector<const semantic::Symbol*> result;
        for (const auto& file : files_) {
            for (const auto& sym : file.symbols) {
                if (pred(sym)) {
                    result.push_back(&sym);
                }
            }
        }
        return result;
    }

private:
    std::vector<FileEntry> files_;
    std::unordered_map<std::string, size_t> pathIndex_;
};

} // namespace eval::workspace
