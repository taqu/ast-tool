#ifndef INC_AST_MEMBER_RESOLUTION_H_
#define INC_AST_MEMBER_RESOLUTION_H_

#include "ast-resolver.h"

namespace ast
{

/**
 * Resolve an identifier reference, adding conservative C++ member-call support
 * for a simple receiver with a directly declared, unambiguous static type.
 * All other forms retain IdentifierResolver behavior or remain unresolved.
 */
ResolutionResult resolve_relationship_identifier(
    const TranslationUnit& tu,
    const Workspace& workspace,
    const IdentifierResolver& resolver,
    size_t identifierNodeIndex,
    uintptr_t scopeId);

} // namespace ast
#endif // INC_AST_MEMBER_RESOLUTION_H_
