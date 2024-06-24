#pragma once
#include <SDL2/SDL.h>

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H 

class TextureManager
{
public:
    static SDL_Texture* LoadTexture(const char* fileName, SDL_Renderer* ren);
};

#endif // TEXTURE_MANAGER_H