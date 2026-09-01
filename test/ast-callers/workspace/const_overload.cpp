// Tests: const/non-const method overloads must remain distinct logical symbols.
// coRun() and coRun() const must not collapse into one, even though both have
// an empty parameter list.
struct CoObj {
    void coRun() {}
    void coRun() const {}
};

void coCaller(CoObj& obj) {
    obj.coRun();
}
