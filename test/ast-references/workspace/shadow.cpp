// Tests: shadowing — inner declaration hides outer
int shadowTarget = 0;
void shadowFunc() {
    int shadowTarget = 99;
    (void)shadowTarget;
}
void useTarget() {
    shadowTarget = 1;
}
