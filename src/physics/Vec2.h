#pragma once
#include <iostream>
#include "external/json.hpp"

using json = nlohmann::json;

class Vec2
{
public:
    float x = 0;
    float y = 0;

    Vec2();
    // Vec2(int xin, int yin);
    // Vec2(float xin, float yin);

    // Constructor from floats
    Vec2(float x_, float y_) : x(x_), y(y_) {}
    // Constructor from ints (converts to float)
    Vec2(int x_, int y_) : x(static_cast<float>(x_)), y(static_cast<float>(y_)) {}
    Vec2(int x_, float y_) : x(static_cast<float>(x_)), y(y_) {}
    Vec2(float x_, int y_) : x(x_), y(static_cast<float>(y_)) {}

    Vec2(json j) : x(j["x"]), y(j["y"]){}
    Vec2(json jx, json jy) : x(jx), y(jy){}

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
    bool isNull() const;
    float length() const;
    Vec2 abs_elem() const;
    Vec2 norm() const;
    Vec2 norm(const float val) const;
    float angle() const;
    Vec2 mainDir() const;
    Vec2 toInt();
    bool smaller(Vec2 rhs);
    bool greater(Vec2 rhs);
    void print(std::string text);
    bool hasPositive();
    bool hasNegative();
    bool isPositive();
    bool isNegative();
};
