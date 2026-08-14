#include "ast-regex.h"
#include <re2/re2.h>

namespace ast
{

RegexPattern::RegexPattern(std::string_view pattern)
    : re_(std::make_unique<re2::RE2>(pattern))
{
}

RegexPattern::~RegexPattern() = default;
RegexPattern::RegexPattern(RegexPattern&&) noexcept = default;
RegexPattern& RegexPattern::operator=(RegexPattern&&) noexcept = default;

bool RegexPattern::valid() const
{
    return re_ && re_->ok();
}

bool RegexPattern::matches(const std::u8string& input) const
{
    std::string_view sv((const char*)input.data(), input.size());
    return re2::RE2::PartialMatch(sv, *re_);
}

bool RegexPattern::matches(const std::filesystem::path& input) const
{
    std::u8string path = input.u8string();
    std::string_view sv((const char*)path.data(), path.size());
    return re2::RE2::PartialMatch(sv, *re_);
}

const std::string& RegexPattern::error() const
{
    static const std::string empty;
    return re_ ? re_->error() : empty;
}

} // namespace ast
