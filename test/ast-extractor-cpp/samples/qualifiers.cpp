// Tests: cv-qualified method overloads must remain distinct symbols
// (Phase 3 — Semantic Symbol Resolution).
class Foo {
public:
    void run();
    void run() const;
    void run(int value);
};

void Foo::run() {
}

void Foo::run() const {
}

void Foo::run(int value) {
}
