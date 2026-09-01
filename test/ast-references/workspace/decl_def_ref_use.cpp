#include "decl_def_ref.h"

// Tests: reference to the header-declared / out-of-line-defined drRefFn,
// used from a third file.
void drRefCaller() {
    drRefFn(1);
}
