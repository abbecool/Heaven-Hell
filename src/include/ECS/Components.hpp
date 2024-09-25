#pragma once

#include "Vec2.h"
#include "Animation.h"

struct CTransform_new
{
    Vec2 pos;    
    Vec2 prevPos;
    Vec2 vel = {0, 0};    
    Vec2 scale = {0.5, 0.5};    
    float angle = 0;
    int speed = 0;
    bool isMovable = false;
    float tempo = 1.0f;
    CTransform_new() {}
    CTransform_new(const Vec2 & p) : pos(p), prevPos(p) {}
    CTransform_new(const Vec2 & p, const Vec2 & v, const Vec2 & scl, const float ang, bool mvbl) 
    : pos(p), prevPos(p), vel(v), scale(scl), angle(ang), speed(300), isMovable(mvbl){}
    CTransform_new(const Vec2 & p, const Vec2 & v, bool mvbl) 
        : pos(p), prevPos(p), vel(v), speed(300), isMovable(mvbl){}
    CTransform_new(const Vec2 & p, const Vec2 & v, const Vec2 & scl, const float ang, int spd, bool mvbl) 
    : pos(p), prevPos(p), vel(v), scale(scl), angle(ang), speed(spd), isMovable(mvbl){}
};

struct CBoundingBox_new
{
    Vec2 size;
    Vec2 halfSize;
    CBoundingBox_new() {}
    CBoundingBox_new(const Vec2& s) 
        : size(s), halfSize(s/2.0) {}
};

struct CAnimation_new
{
    Animation animation;
    bool repeat = false;
    int layer = 0;
    CAnimation_new() {}
    CAnimation_new(const Animation& animation, bool r)
                : animation(animation), repeat(r){}
    CAnimation_new(const Animation& animation, bool r, int l)
                : animation(animation), repeat(r), layer(l){}
}; 