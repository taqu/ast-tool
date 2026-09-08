// Tests: namespace-qualified callee resolved via workspace fallback
namespace NsCe {
    void nsTarget() {}
}
void nsCeSource() { NsCe::nsTarget(); }
