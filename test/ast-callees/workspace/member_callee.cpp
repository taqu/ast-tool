// Tests: member function callee resolved via class-scope lexical lookup
struct MbCe {
    void mbTarget() {}
    void mbSource() { mbTarget(); }
};
