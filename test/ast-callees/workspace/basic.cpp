// Tests: single callee, multiple callees, nested calls, no callees, recursive

// --- single callee ---
void scTarget() {}
void scSource() { scTarget(); }

// --- multiple callees ---
void mcAlpha() {}
void mcBeta()  {}
void mcSource() { mcAlpha(); mcBeta(); }

// --- nested call expressions in one statement: foo(bar()) calls both foo and bar ---
void ncPrint(int) {}
int  ncGet()      { return 1; }
void ncNestedSource() { ncPrint(ncGet()); }

// Only traverses ncNestedSource's body — ncGet's body (above) is a separate subtree.
// ncTransitive calls ncNestedSource, but ncNestedSource's callees do not include
// ncTransitive's own callees.
void ncTransitive() { ncNestedSource(); }

// --- function with no callees ---
void noCallee() {}

// --- recursive function ---
int recCe(int n) {
    if(n <= 0) return 0;
    return recCe(n - 1);
}
