// Case D: inline method definition — body is present directly in class.
// No declaration→definition fallback needed; existing body is used.
void biInlineHelper() {}

struct BiInlineClass {
    void biInlineFn() { biInlineHelper(); }
};
