#include "workspace/index.hpp"
#include <algorithm>

namespace eval::workspace {

void Index::addFile(FileEntry entry) {
    std::string path = entry.path;
    auto it = pathIndex_.find(path);
    if (it != pathIndex_.end()) {
        files_[it->second] = std::move(entry);
    } else {
        pathIndex_[path] = files_.size();
        files_.push_back(std::move(entry));
    }
}

const FileEntry* Index::findFile(const std::string& path) const {
    auto it = pathIndex_.find(path);
    if (it != pathIndex_.end()) {
        return &files_[it->second];
    }
    return nullptr;
}

const semantic::Symbol* Index::findSymbol(const std::string& name) const {
    for (const auto& file : files_) {
        for (const auto& sym : file.symbols) {
            if (sym.name == name) {
                return &sym;
            }
        }
    }
    return nullptr;
}

std::vector<std::string> Index::dependentsOf(const std::string& path) const {
    std::vector<std::string> result;
    for (const auto& file : files_) {
        for (const auto& inc : file.includes) {
            if (inc == path) {
                result.push_back(file.path);
                break;
            }
        }
    }
    return result;
}

std::vector<std::string> Index::dependenciesOf(const std::string& path) const {
    const FileEntry* entry = findFile(path);
    if (!entry) {
        return {};
    }
    return entry->includes;
}

} // namespace eval::workspace
