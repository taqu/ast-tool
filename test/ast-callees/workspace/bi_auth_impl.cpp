// Motivating Phase 8c case: out-of-line definition for auth::AuthService::refresh.
// Before Phase 8c: callees returned empty (declaration selected, no body).
// After Phase 8c: body-bearing definition found, callees returns auth::AuthToken::validate.

void auth::AuthToken::validate() {}

void auth::AuthService::refresh() {
    token_.validate();
}
