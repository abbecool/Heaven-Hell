#pragma once
#include <SDL2/SDL.h>
#include "../src\include\SDL2\SDL_image.h"

#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H 

class TextureManager
{
public:
    static SDL_Texture* LoadTexture(const char* fileName, SDL_Renderer* ren);
};

SDL_Texture* TextureManager::LoadTexture(const char* texture, SDL_Renderer* ren)
{
    SDL_Surface* tempSurface = IMG_Load(texture);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, tempSurface);
    SDL_FreeSurface(tempSurface);
    return tex;
}

#endif // TEXTURE_MANAGER_H