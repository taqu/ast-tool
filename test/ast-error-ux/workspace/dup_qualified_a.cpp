// Exercises already-qualified ambiguity: two free-function definitions that
// extract to the same FQN "euxns::dup" (never legal C++, but the extractor
// analyzes each file independently and does not check for ODR violations).
// Neither is Method/Constructor/Destructor kind, so resolve_symbol_query()'s
// decl/def collapse does not apply and both survive as genuine candidates.
namespace euxns
{
    void dup() {}
}
