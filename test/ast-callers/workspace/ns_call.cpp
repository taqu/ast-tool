// Tests: namespace-qualified call resolved via workspace fallback
namespace NsCal {
    void nsTarget() {}
}
void nsCaller() { NsCal::nsTarget(); }
