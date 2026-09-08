// See amb_authtoken.cpp: pairs with EuxAuthToken::validate to make an
// unqualified query for "validate" genuinely ambiguous.
class EuxValidator
{
public:
    bool validate() { return true; }
};
