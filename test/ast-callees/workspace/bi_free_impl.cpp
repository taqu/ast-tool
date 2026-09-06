// Case A: free function definition — biFreeFn calls biFreeHelper.
// callees biFreeFn -> biFreeHelper

void biFreeHelper() {}
void biFreeFn() { biFreeHelper(); }
