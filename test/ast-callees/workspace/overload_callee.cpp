// Tests: overloaded functions
// Same-scope overloads: resolver returns Ambiguous — expect 0 callees found
void ovCeTarget(int)    {}
void ovCeTarget(double) {}
void ovCeAmbig() { ovCeTarget(1); }

// Unique name inside a namespace: resolver finds exactly one via lexical lookup
namespace OvCeNs {
    void ovCeNsTarget() {}
    void ovCeNsSource() { ovCeNsTarget(); }
}
