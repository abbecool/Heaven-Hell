#pragma once

#include "Animation.h"
#include <memory>

class CInputs
{
public:
    bool up = false;
    bool down = false;
    bool left = false;
    bool right = false;
    bool shift = false;
    bool ctrl = false;
    CInputs() {};
};

class CTransform
{
public:
    Vec2 pos;    
    Vec2 prevPos;
    Vec2 vel;    
    bool isMovable = true;
    int speed = 400;
    CTransform() {}
    CTransform(const Vec2 & p, const Vec2 & v, bool mvbl) 
        : pos(p), prevPos(p), vel(v), isMovable(mvbl){}
};

class CSize
{
public:
    Vec2 size = {64,64};
    CSize() {}
    CSize(const Vec2 & sz)
        : size(sz) {}
};

class CShape
{
public:
    Vec2 pos;
    Vec2 size;
    int r_val;
    int g_val;
    int b_val;
    int a_val;
    std::string color;
    // SDL_Rect *rect;
    CShape() {}
    CShape(const Vec2 p, const Vec2 sz, int r, int g, int b, int a) 
        : pos(p), size(sz), r_val(r), g_val(g), b_val(b), a_val(a)
        {
            // rect->x = pos.x;
            // rect->y = pos.y;
            // rect->w = size.x;
            // rect->h = size.y;
        }
};

class CTexture
{
public:
    Vec2 pos;
    Vec2 size;
    // SDL_Rect *rect;
    SDL_Texture * texture;

    CTexture() {}
    CTexture(const Vec2 p, const Vec2 sz, SDL_Texture* tex) 
        : pos(p), size(sz), texture(tex)
        {
            // rect->x = pos.x;
            // rect->y = pos.y;
            // rect->w = size.x;
            // rect->h = size.y;
        }
};

class CName
{
public:
    std::string name;
    CName() {}
    CName(const std::string & nm)
        : name(nm) {}
};
class CKey
{
public:
    std::string unlocks;
    CKey() {}
    CKey(const std::string & unlcks)
        : unlocks(unlcks) {}
};

class CAnimation
{
public:
    Animation animation;
    bool repeat = false;
    CAnimation() {}
    CAnimation(const Animation& animation, bool r)
    : animation(animation), repeat(r) {}
};