// Tests: calls to undeclared functions are silently skipped with no false positives
void unresCaller() { unknownFunc42(); }
void unresDefined() {}
