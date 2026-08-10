// Tests: cross-file call — calls xfileTarget defined in xfile_fn.cpp
// No local declaration; resolver finds it via workspace-level fallback.
void xfileCaller() { xfileTarget(); }
