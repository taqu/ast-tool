#include "ast-format.h"
#include "ast-ir.h"

namespace ast
{
namespace
{
    /** Replaces whitespace/quote characters that would break single-line output. */
    std::string sanitize_text(const std::string& text)
    {
        std::string result;
        result.reserve(text.size());
        for(char c: text) {
            switch(c) {
            case '\n':
            case '\r':
            case '\t':
                result.push_back(' ');
                break;
            case '"':
                result.push_back('\'');
                break;
            default:
                result.push_back(c);
                break;
            }
        }
        return result;
    }
} // namespace

std::string preview_text(const ASTNode& node, size_t maxLength)
{
    if(node.text_.empty()) {
        return std::string();
    }
    std::string text = sanitize_text(node.getText());
    if(maxLength < text.size()) {
        text.resize(maxLength);
        text += "...";
    }
    return text;
}
} // namespace ast
