// Tests: multiple callers of the same target
void mcTarget() {}
void mcFirst()  { mcTarget(); }
void mcSecond() { mcTarget(); }
