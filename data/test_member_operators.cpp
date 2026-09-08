// Regression data for member operator and free operator symbol extraction.

// Case 1: member arithmetic operator (declaration)
struct Counter
{
    Counter& operator+=(const Counter& other);
    Counter operator-=(const Counter& other);
    Counter& operator*=(const Counter& other){return *this;}
    Counter operator/=(const Counter& other){return *this;}
};

// Validation case from task spec
namespace test
{

struct Info
{
    operator bool();
    operator int();
    operator int*();
    operator int&();
    operator int**();
    operator Info*();
    operator Info&();
    operator Info2*(){return this;}
    operator Info2&(){return *this;}
};

int operator+(Info& a, Info& b);
int operator-(Info& a, Info& b){return 0;}
int& operator*(Info& a, Info& b);
int& operator/(Info& a, Info& b){return 0;}

} // namespace test

// Case 2: nested namespace scope with member conversion operator
namespace A
{
namespace B
{
    struct Value
    {
        operator bool();
    };
} // namespace B
} // namespace A
