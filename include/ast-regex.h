#ifndef INC_AST_REGEX_H_
#define INC_AST_REGEX_H_
#include <memory>
#include <string>
#include <string_view>
#include <filesystem>

namespace re2 { class RE2; }

namespace ast
{

/**
 * @brief Thin wrapper around RE2 that compiles a pattern once and exposes
 *        validity checking and partial matching.
 *
 * RegexPattern is move-only; RE2 objects cannot be copied.
 * Patterns must be compiled before the search loop begins and reused
 * throughout the search — never construct inside a hot loop.
 */
class RegexPattern
{
public:
    explicit RegexPattern(std::string_view pattern);
    ~RegexPattern();
    RegexPattern(RegexPattern&&) noexcept;
    RegexPattern& operator=(RegexPattern&&) noexcept;
    RegexPattern(const RegexPattern&) = delete;
    RegexPattern& operator=(const RegexPattern&) = delete;

    bool valid() const;
    bool matches(const std::u8string& input) const;
    bool matches(const std::filesystem::path& input) const;
    const std::string& error() const;

private:
    std::unique_ptr<re2::RE2> re_;
};

} // namespace ast
#endif // INC_AST_REGEX_H_
