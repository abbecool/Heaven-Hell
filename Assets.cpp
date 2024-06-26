#pragma once

#include "COMP4300/Assets.h"

#include <SDL2/SDL.h>
#include "src\include\SDL2\SDL_image.h"


void Assets::addTexture(const std::string& name, const char* path, SDL_Renderer * ren)
{
//     SDL_Texture* TextureManager::LoadTexture(const char* texture, SDL_Renderer* ren)
// {
    SDL_Surface* tempSurface = IMG_Load(path);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(ren, tempSurface);
    SDL_FreeSurface(tempSurface);
    // return tex;
}
