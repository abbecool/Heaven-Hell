#pragma once

// #include "Sprite.h"
#include "Animation.h"
#include <memory>

// set a flag: flag |= (int)PlayerState
// unset a flag: flag &= ~(int)PlayerState
// flipping a flag: flag ^= (int)PlayerState
// checking if a flag set: return (flag & (int)PlayerState) == (int)PlayerState
// checking multiple flags set: return (flag &(int)PlayerState) != 0
enum struct PlayerState {
    STAND = 1 << 0,
    STANDSHOOT = 1 << 1,
    AIR = 1 << 2,
    AIRSHOOT = 1 << 3,
    RUN = 1 << 4,
    RUNSHOOT = 1 << 5
};

class Component
{
    public:
        bool has = false;
};

class CInputs : public Component
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

class CTransform : public Component
{
public:
    Vec2 pos;    
    Vec2 prevPos;
    Vec2 vel;    
    Vec2 scale = {0.5, 0.5};    
    float angle = 0;
    bool isMovable = true;
    int speed = 400;
    CTransform() {}
    CTransform(const Vec2 & p, const Vec2 & v, bool mvbl) 
        : pos(p), prevPos(p), vel(v), isMovable(mvbl){}
    CTransform(const Vec2 & p, const Vec2 & v,const Vec2 & scl, const float ang, bool mvbl) 
    : pos(p), prevPos(p), vel(v), scale(scl), angle(ang), isMovable(mvbl){}
};

class CBoundingBox : public Component
{
    public:
        Vec2 size;
        Vec2 halfSize;
        CBoundingBox() {}
        CBoundingBox(const Vec2& s) 
            : size(s), halfSize(s/2.0) {}
};

class CShape : public Component
{
public:
    Vec2 pos;
    Vec2 size;
    CShape() {}
    CShape(const Vec2 p, const Vec2 sz) 
        : pos(p), size(sz)
        {
            // rect->x = pos.x;
            // rect->y = pos.y;
            // rect->w = size.x;
            // rect->h = size.y;
        }
};

class CTexture : public Component
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

class CName: public Component
{
public:
    std::string name;
    CName() {}
    CName(const std::string & nm)
        : name(nm) {}
};
class CKey: public Component
{
public:
    std::string unlocks;
    CKey() {}
    CKey(const std::string & unlcks)
        : unlocks(unlcks) {}
};

class CAnimation: public Component
{
public:
    // SDL_Sprite sprite;
    Animation animation;
    bool repeat = false;
    CAnimation() {}
    CAnimation(const Animation& animation, bool r)
                : animation(animation), repeat(r){}
};  