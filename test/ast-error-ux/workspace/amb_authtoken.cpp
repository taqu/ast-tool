// Exercises unqualified-name ambiguity: "validate" matches this class and
// eux_amb_validator.cpp's Validator::validate. Both are in-class method
// declarations (no out-of-line definition), so the decl/def collapse in
// resolve_symbol_query() never fires and the ambiguity is genuine.
class EuxAuthToken
{
public:
    bool validate() { return true; }
};
