// Tests: simple direct call, zero callers
void calTarget() {}
void callerFn() { calTarget(); }
void noCalTarget() {}
