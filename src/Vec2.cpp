#include "Vec2.h"
#include <cmath>

Vec2::Vec2(){}

Vec2::Vec2(float xin, float yin)
    : x(xin), y(yin) {}

bool Vec2::operator== (const Vec2 & rhs) const
{
    return ( x == rhs.x && y == rhs.y );
}

bool Vec2::operator>= (const Vec2 & rhs) const
{
    return ( x >= rhs.x && y >= rhs.y );
}

bool Vec2::operator!= (const Vec2 & rhs) const
{
    return ( x != rhs.x || y != rhs.y );
}

Vec2 Vec2::operator+ (const Vec2 & rhs) const
{
    return Vec2 {x + rhs.x, y + rhs.y};
}

Vec2 Vec2::operator* (const float val) const
{
    return Vec2 {x*val, y*val};
}

Vec2 Vec2::operator* (const Vec2 & rhs) const
{
    return Vec2 {x*rhs.x, y*rhs.y};
}

Vec2 Vec2::operator/ (const float val) const
{
    return Vec2 {x / val, y / val};
}

Vec2 Vec2::operator- (const Vec2 & rhs) const
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

void Vec2::operator*= (const float val)
{
    x *= val;
    y *= val;
}

void Vec2::operator/= (const float val)
{
    x /= val;
    y /= val;
}

// Vec2 Vec2::operator% (const int val) const
// {
//     return Vec2 {(int)x%val, (int)y%val};
// }

// Vec2 Vec2::operator% (const Vec2 & rhs) const
// {
//     return Vec2 {(int)x%(int)rhs.x, (int)y%(int)rhs.y};
// }

void Vec2::operator++ (){
    x++;
    y++;
}

void Vec2::operator-- (){
    x--;
    y--;
}

bool Vec2::isnull () const
{
    return (std::fabs(x)+std::fabs(y) == 0);
}

float Vec2::length () const
{
    return std::hypot(x, y);
    //return sqrt(pow(x,2) + pow(y,2));
}

Vec2 Vec2::norm () const
{
    float len = length();
    return Vec2 {x/len, y/len};
}

Vec2 Vec2::norm (const float val) const
{   
    float len = length();
    return Vec2 {val*x/len, val*y/len};
}

Vec2 Vec2::abs_elem() const
{
    return Vec2 { std::fabs(x), std::fabs(y) };
}

float Vec2::angle() const
{
    return std::atan2(y, x)* 180.0f / 3.14159265358979323846;
}

Vec2 Vec2::mainDir() const {
        if (std::fabs(x) >= std::fabs(y)) {
            return Vec2{x, 0};
        } else {
            return Vec2{0, y};
        }
    }
