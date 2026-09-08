// Out-of-line definitions for DdCalClass.
// Exercises the declaration/definition deduplication in resolve_symbol_query:
// DdCalClass::ddCalMethod appears here as Function (isQualified), but as Method
// in decl_def_class.h — the resolver must not report ambiguity.
DdCalClass::DdCalClass() {}
DdCalClass::~DdCalClass() {}
void DdCalClass::ddCalMethod() { ddCalHelper(); }
void DdCalClass::ddCalHelper() {}
void ddCalCaller() {
    DdCalClass obj;
    obj.ddCalMethod();
}
