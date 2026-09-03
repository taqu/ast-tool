// A uniquely-named, callable, never-called symbol: exercises symbol
// resolution succeeding while the requested relationship (callers) is
// legitimately empty.
void euxLonely() {}

// A uniquely-named, non-callable symbol: exercises the "unsupported query
// form" diagnostic for callers/callees on a target that resolves but is
// not a function/method/constructor/destructor.
namespace euxNsOnly {}
