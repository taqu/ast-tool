// Tests: overloaded functions
// Same-scope overloads: resolver returns Ambiguous — expect 0 callers found
void ovTarget(int)    {}
void ovTarget(double) {}
void ovCaller()       { ovTarget(1); }

// Unique name in namespace: resolver finds exactly one via lexical lookup
namespace OvNs {
    void ovNsTarget() {}
    void ovNsCaller() { ovNsTarget(); }
}
