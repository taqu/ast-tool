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

bool RegexPattern::matches(const std::string& input) const
{
    return re2::RE2::PartialMatch(input, *re_);
}

const std::string& RegexPattern::error() const
{
    static const std::string empty;
    return re_ ? re_->error() : empty;
}

} // namespace ast
