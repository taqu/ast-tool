// Tests: declaration/definition merge (Phase 3 — Semantic Symbol Resolution)
// declFn and DeclClass::declMethod are declared here and defined in decl_def.cpp.
// Callers must resolve to a single logical symbol regardless of whether the
// query target is this declaration or the out-of-line definition.
void declFn(int value);

class DeclClass {
public:
    void declMethod();
};
