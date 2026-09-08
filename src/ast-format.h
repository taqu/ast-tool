#ifndef INC_AST_FORMAT_H_
#define INC_AST_FORMAT_H_
/**
 * @file ast-format.h
 * @brief Small text-formatting helpers shared by the CLI subcommands (outline, find, ...).
 */
#include <cstddef>
#include <string>

namespace ast
{
struct ASTNode;

/** Default maximum number of characters returned by preview_text() before truncation. */
constexpr size_t PreviewTextMaxLength = 40;

/**
 * @brief Returns a single-line, human-readable preview of @p node's source text.
 *
 * Embedded newlines/tabs are collapsed to spaces and quotes are escaped so the result is
 * always safe to print on one line; text longer than @p maxLength is truncated with a
 * trailing "...". Returns an empty string if @p node has no associated source text.
 */
std::u8string preview_text(const ASTNode& node, size_t maxLength = PreviewTextMaxLength);
} // namespace ast
#endif // INC_AST_FORMAT_H_
