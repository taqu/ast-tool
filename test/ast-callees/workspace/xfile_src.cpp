// Tests: cross-file callee — calls xfCeTarget defined in xfile_def.cpp
// No local declaration; resolver finds it via workspace-level fallback.
void xfCeSource() { xfCeTarget(); }
