// Tests: const/non-const method overloads must remain distinct logical symbols.
struct CoCeObj {
    void coCeRun() {}
    void coCeRun() const {}
};

void coCeCaller(CoCeObj& obj) {
    obj.coCeRun();
}
