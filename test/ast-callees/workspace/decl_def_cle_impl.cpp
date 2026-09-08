// Out-of-line definitions — ddCleMethod calls ddCleHelper.
// After the resolver fix, callees for DdCleClass::ddCleMethod must resolve
// without reporting ambiguity from the declaration/definition split.
void DdCleClass::ddCleMethod() { ddCleHelper(); }
void DdCleClass::ddCleHelper() {}
