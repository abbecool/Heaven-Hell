#include <iostream>
#include <SDL2/SDL.h>
#include "Vec2.h"
#include "../TextureManager.cpp"

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
    Vec2 pos     = {0,0};
    Vec2 prevPos = {0,0};
    Vec2 vel     = {0,0};
    bool isMovable = true;
    int speed = 400;
    CTransform() {}
    CTransform(const Vec2 & p, const Vec2 & v, bool mbl) 
        : pos(p), prevPos(p), vel(v), isMovable(mbl){}
};

class CSize
{
public:
    Vec2 size = {25,25};
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
    SDL_Rect rect;
    CShape() {}
    CShape(const Vec2 p, const Vec2 sz, int r, int g, int b, int a) 
        : pos(p), size(sz), r_val(r), g_val(g), b_val(b), a_val(a)
        {
            rect.x = pos.x;
            rect.x = pos.y;
            rect.w = size.x;
            rect.h = size.y;
        }
    SDL_Rect* getPtrRect();
    
    void setPosition(Vec2 pos) 
    {
        rect.x = pos.x;
        rect.y = pos.y;
    }
};

SDL_Rect* CShape::getPtrRect()
{
    return &rect;
}

class CTexture
{
public:
    Vec2 pos;
    Vec2 size;
    SDL_Rect rect;

    SDL_Texture * texture;// = TextureManager::LoadTexture(e->cTexture->charTexture, m_renderer);

    CTexture() {}
    CTexture(const Vec2 p, const Vec2 sz) 
        : pos(p), size(sz)
        {
            rect.x = pos.x;
            rect.y = pos.y;
            rect.w = size.x;
            rect.h = size.y;
        }
    SDL_Rect* getPtrRect();
    SDL_Texture* getPtrTexture();
    
    void setPosition(Vec2 pos) 
    {
        rect.x = pos.x;
        rect.y = pos.y;
    }
};

SDL_Rect* CTexture::getPtrRect()
{
    return &rect;
}

SDL_Texture* CTexture::getPtrTexture()
{
    return texture;
}

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