// Case G / Stage 12 false-positive guard:
// Two different namespaces each declare a function with the same unqualified name.
// callees a::biFpRun must NOT use b::biFpRun's body.
void biFpHelper() {}

namespace bifpa {
    void biFpRun();
}

namespace bifpb {
    void biFpHelper() {}
    void biFpRun() { biFpHelper(); }
}
