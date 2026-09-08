// Tests: shadowing — namespace-scoped name hides outer name via lexical lookup
int shadowTarget = 0;
namespace ShadowInner {
    int shadowTarget = 1;
    void shadowFunc() { (void)shadowTarget; }
}
void useTarget() {
    shadowTarget = 1;
}
