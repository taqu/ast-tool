#include "decl_def.h"

// Tests: callers of the header-declared / out-of-line-defined declFn and
// DeclClass::declMethod, invoked from a third file.
void declCallerFn() {
    declFn(1);
}

void declCallerMethod(DeclClass& obj) {
    obj.declMethod();
}
