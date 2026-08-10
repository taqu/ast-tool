#include "ast-find.h"
#include <string>
#include "ast-ir.h"

namespace ast
{
namespace
{
    /** True if the zero-based (row, column) position lies within [node.start_, node.end_). */
    bool contains_position(const ASTNode& node, uint32_t row, uint32_t column)
    {
        bool geStart = (node.start_.row_ < row)
            || (node.start_.row_ == row && node.start_.column_ <= column);
        bool ltEnd = (row < node.end_.row_)
            || (row == node.end_.row_ && column < node.end_.column_);
        return geStart && ltEnd;
    }

    bool matches_text(const ASTNode& node, const char* needle)
    {
        if(node.text_.empty()) {
            return false;
        }
        std::string text = node.getText();
        return std::string::npos != text.find(needle);
    }
} // namespace

bool matches(const ASTNode& node, const FindCriteria& criteria)
{
    if(nullptr != criteria.type_ && !node.typeEquals(criteria.type_)) {
        return false;
    }
    if(nullptr != criteria.grammar_ && !node.grammarEquals(criteria.grammar_)) {
        return false;
    }
    if(criteria.hasId_ && node.hash_ != criteria.id_) {
        return false;
    }
    if(criteria.hasPosition_ && !contains_position(node, criteria.line_, criteria.column_)) {
        return false;
    }
    if(nullptr != criteria.text_ && !matches_text(node, criteria.text_)) {
        return false;
    }
    return true;
}

std::vector<const ASTNode*> find_nodes(const AST& ast, const FindCriteria& criteria)
{
    std::vector<const ASTNode*> result;
    for(uint32_t i = 0; i < ast.size(); ++i) {
        const ASTNode& node = ast[i];
        if(matches(node, criteria)) {
            result.push_back(&node);
        }
    }
    if(criteria.hasPosition_ && result.size() > 1) {
        const ASTNode* best = result[0];
        for(size_t i = 1; i < result.size(); ++i) {
            uint32_t bestRange = best->endByte_ - best->startByte_;
            uint32_t currRange = result[i]->endByte_ - result[i]->startByte_;
            if(currRange < bestRange) best = result[i];
        }
        result = {best};
    }
    return result;
}
} // namespace ast
