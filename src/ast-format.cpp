#include "ast-format.h"
#include "ast-ir.h"

namespace ast
{
namespace
{
    /** Replaces whitespace/quote characters that would break single-line output. */
    std::u8string sanitize_text(const std::u8string& text)
    {
        std::u8string result;
        result.reserve(text.size());
        for(char8_t c: text) {
            switch(c) {
            case u8'\n':
            case u8'\r':
            case u8'\t':
                result.push_back(u8' ');
                break;
            case '"':
                result.push_back(u8'\'');
                break;
            default:
                result.push_back(c);
                break;
            }
        }
        return result;
    }
} // namespace

std::u8string preview_text(const ASTNode& node, size_t maxLength)
{
    if(node.text_.empty()) {
        return std::u8string();
    }
    std::u8string text = sanitize_text(node.getText());
    if(maxLength < text.size()) {
        text.resize(maxLength);
        text += u8"...";
    }
    return text;
}

std::u8string json_escape(std::u8string_view text)
{
    std::u8string result;
    result.reserve(text.size());
    constexpr char8_t hex[] = u8"0123456789ABCDEF";
    for(char8_t c: text) {
        switch(c) {
        case u8'"': result += u8"\\\""; break;
        case u8'\\': result += u8"\\\\"; break;
        case u8'\b': result += u8"\\b"; break;
        case u8'\f': result += u8"\\f"; break;
        case u8'\n': result += u8"\\n"; break;
        case u8'\r': result += u8"\\r"; break;
        case u8'\t': result += u8"\\t"; break;
        default:
            if(c < 0x20) {
                result += u8"\\u00";
                result.push_back(hex[(c >> 4) & 0x0F]);
                result.push_back(hex[c & 0x0F]);
            } else {
                result.push_back(c);
            }
        }
    }
    return result;
}
} // namespace ast
