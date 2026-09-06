// Case B: out-of-line method definition — biMethodFn calls biMethodHelper.
// callees BiMethodClass::biMethodFn -> BiMethodClass::biMethodHelper

void BiMethodClass::biMethodHelper() {}
void BiMethodClass::biMethodFn() { biMethodHelper(); }
