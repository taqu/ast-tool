// Tests: member function call resolved via class-scope lexical lookup
struct MbObj {
    void mbTarget() {}
    void mbCaller() { mbTarget(); }
};
