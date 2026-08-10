struct Vector {
    int x;
    int y;
    Vector operator+(const Vector& other) const;
    Vector& operator+=(const Vector& other);
    operator bool() const;
};

Vector operator-(const Vector& a, const Vector& b);
