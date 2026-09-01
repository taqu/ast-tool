// Tests: declaration/definition merge (Phase 3 — Semantic Symbol Resolution)
// drRefFn is declared here and defined out-of-line in decl_def_ref.cpp.
// References must resolve to a single logical symbol regardless of whether
// the query target is this declaration or the out-of-line definition.
void drRefFn(int value);
