// Motivating Phase 8c case: namespace + class method declaration without body.
namespace auth {

struct AuthToken {
    void validate();
};

class AuthService {
public:
    void refresh();
private:
    AuthToken token_;
};

} // namespace auth
