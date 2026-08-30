// Out-of-line definition — ddRefMethod is kind=Function here, kind=Method in header.
// After the resolver fix, references to DdRefClass::ddRefMethod must resolve without
// reporting ambiguity.
void DdRefClass::ddRefMethod() { int x = ddRefField; (void)x; }
void ddRefCaller() {
    DdRefClass obj;
    obj.ddRefMethod();
}
