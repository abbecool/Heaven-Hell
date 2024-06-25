#pragma once

#include <cmath>

class Vec2
{
public:
    float x = 0;
    float y = 0;
    Vec2() {}
    Vec2(float xin, float yin)
        : x(xin)
        , y(yin)
    {
        
    }
    bool operator == (const Vec2 & rhs);
    bool operator != (const Vec2 & rhs);

    Vec2 operator + (const Vec2 & rhs);
    Vec2 operator - (const Vec2 & rhs);
    Vec2 operator * (const float val);
    Vec2 operator / (const float val);

    void operator += (const Vec2 & rhs);
    void operator -=(const Vec2 &rhs);
    void operator *= (const float val) const;
    void operator /= (const float val) const;

    float dist(const Vec2 & rhs) const;
    float isnull() const;
    float length() const;
    Vec2 abs_elem();
    Vec2 norm();
    Vec2 norm(const float val);

};

bool Vec2::operator== (const Vec2 & rhs)
{
    return ( x == rhs.x && y == rhs.y );
}

bool Vec2::operator!= (const Vec2 & rhs)
{
    return ( x != rhs.x || y != rhs.y );
}

Vec2 Vec2::operator+ (const Vec2 & rhs)
{
    return Vec2 {x + rhs.x, y + rhs.y};
}

Vec2 Vec2::operator* (const float val)
{
    return Vec2 {x*val, y*val};
}

Vec2 Vec2::operator/ (const float val)
{
    return Vec2 {x / val, y / val};
}

Vec2 Vec2::operator- (const Vec2 & rhs)
{
    return Vec2 {x - rhs.x, y - rhs.y};
}

void Vec2::operator+= (const Vec2 & rhs)
{
    x += rhs.x;
    y += rhs.y;
}

void Vec2::operator-= (const Vec2 & rhs)
{
    x -= rhs.x;
    y -= rhs.y;
}

float Vec2::isnull () const
{
    if (std::fabs(x)+std::fabs(y) == 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}

float Vec2::length () const
{
    return sqrt(pow(x,2) + pow(y,2));
}

Vec2 Vec2::norm ()
{
    x = x/(Vec2{x,y}).length();
    y = y/(Vec2{x,y}).length();
    return Vec2 {x, y};
}

Vec2 Vec2::norm (const float val)
{   
    const float x_temp = val*x/(Vec2{x,y}).length();
    y = val*y/(Vec2{x,y}).length();
    return Vec2 {x_temp, y};
}

Vec2 Vec2::abs_elem()
{
    return Vec2 { std::fabs(x), std::fabs(y) };
}