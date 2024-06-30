#pragma once

#include "SDL2/SDL.h"

class Sprite
{
    SDL_Texture* texture;
    SDL_Rect rect;
public:
    Sprite(/* args */);
    Sprite(SDL_Texture* tex) : texture(tex){};
    void setTextureRect(const float x, const float y, const float w, const float h);
};

Sprite::Sprite(/* args */)
{
}

void Sprite::setTextureRect(const float x, const float y, const float w, const float h)
{
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = y;
}