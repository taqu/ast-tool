// Regression data for conversion operator symbol extraction.

// Case 1: basic conversion operator (declaration)
struct Info
{
    operator bool();
    operator Info*();
};

// Case 2: multiple conversion operators
struct Value
{
    operator bool();
    operator int();
};

// Case 3: namespace + class scope
namespace A
{
    struct B
    {
        operator bool();
    };
}

// Case 4: inline (definition, not just declaration)
struct Inline
{
    operator bool()
    {
        return true;
    }
};
