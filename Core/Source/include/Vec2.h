#pragma once

class Vec2
{
public:
    float x = 0;
    float y = 0;

    Vec2();
    Vec2(float xin, float yin);

    bool operator == (const Vec2 & rhs) const;
    bool operator >= (const Vec2 & rhs) const;
    bool operator != (const Vec2 & rhs) const;

    Vec2 operator + (const Vec2 & rhs) const;
    Vec2 operator - (const Vec2 & rhs) const;
    Vec2 operator * (const Vec2 & rhs) const;
    Vec2 operator * (const float val) const;
    Vec2 operator / (const float val) const;
    Vec2 operator / (const Vec2 & rhs) const;
    Vec2 operator % (const Vec2 & rhs) const;
    Vec2 operator % (const int val) const;

    void operator ++ ();
    void operator -- ();

    void operator += (const Vec2 & rhs);
    void operator -= (const Vec2 &rhs);
    void operator *= (const float val);
    void operator /= (const float val);

    float dist(const Vec2 & rhs) const;
    bool isnull() const;
    float length() const;
    Vec2 abs_elem() const;
    Vec2 norm() const;
    Vec2 norm(const float val) const;
    float angle() const;
    Vec2 mainDir() const;
    Vec2 toInt();
    bool smaller(Vec2 rhs);
    bool greater(Vec2 rhs);
};
