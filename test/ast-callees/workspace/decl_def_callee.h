// Tests: declaration/definition merge (Phase 3 — Semantic Symbol Resolution)
// ddCeSource is declared here and defined out-of-line in decl_def_callee.cpp.
// Callees must resolve to the definition's body when queried with only the
// header declaration occurrence.
void ddCeSource();
