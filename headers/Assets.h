#include <map>
#include <string>
#include <SDL2/SDL.h>
#include "../src\include\SDL2\SDL_image.h"
#include "TextureManager.h"

class Assets
{
    std::map< std::string, SDL_Texture*> m_textures;
public:
    Assets(){}
    void addTexture(const std::string name, const char* path, SDL_Renderer* renderer)
    SDL_Texture getTexture(const std::string name)
    
};

void Assets::addTexture(const std::string name, const char* path, SDL_Renderer* renderer)
{
    m_texture = TextureManager::LoadTexture("assets/"+str(path), renderer);
}

SDL_Texture getTexture(const std::string name)
{
    return 
}
