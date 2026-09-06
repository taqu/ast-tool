// Case C: namespace-qualified out-of-line method definition.
// callees binsns::BiNsClass::biNsMethodFn -> binsns::BiNsClass::biNsMethodHelper

void binsns::BiNsClass::biNsMethodHelper() {}
void binsns::BiNsClass::biNsMethodFn() { biNsMethodHelper(); }
