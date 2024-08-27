#include "Assets.h"
#include <fstream>
#include <iostream>

#include <SDL2/SDL.h>
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"

Assets::Assets(){}


void Assets::addTexture(std::string name, const std::string & path, SDL_Renderer * ren)
{
    const char *path_char = path.c_str(); 
    SDL_Surface* tempSurface = IMG_Load(path_char);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(ren, tempSurface);
    SDL_FreeSurface(tempSurface);

    m_textures[name] = texture;
}

void Assets::addTexture(std::string name, SDL_Texture* texture)
{
    m_textures[name] = texture;
}

SDL_Texture * Assets::getTexture(std::string name) const
{
    return m_textures.at(name);
}

void Assets::addFont(const std::string& name, const std::string& path) {
    const char *path_char = path.c_str(); 
    TTF_Font* font = TTF_OpenFont(path_char, 12);
    if (font == nullptr) {
        std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
    }
    m_fonts[name] = font;
}

TTF_Font* Assets::getFont(const std::string& name) const {
    return m_fonts.at(name);
}

void Assets::addAnimation(const std::string& name, Animation animation) {
    m_animations[name] = animation;
}

const Animation& Assets::getAnimation(const std::string& name) const {
    return m_animations.at(name);
}

void Assets::loadFromFile(const std::string & pathImages, const std::string & pathText, SDL_Renderer * ren) {
    std::ifstream file(pathImages);
    if (!file) {
        std::cerr << "Could not load assets.txt file!\n";
        exit(-1);
    }
    std::string head;
    while (file >> head) {
        if (head == "Texture") {
            std::string name;
            std::string path;
            file >> name >> path;
            addTexture(name, path, ren);
        }
        else if (head == "Font") {
            std::string font_name;
            std::string font_path;
            file >> font_name >> font_path;
            addFont(font_name, font_path);
        }
        else if (head == "Animation") {
            std::string aniName;
            std::string texName;
            int frames, speed;
            file >> aniName >> texName >> frames >> speed;
            SDL_Texture* tex = getTexture(texName);
            addAnimation( aniName, Animation( aniName, tex, frames, speed ) );            
        }
        else {
            std::cerr << "head to " << head << "\n";
            std::cerr << "The config file format is incorrect!\n";
            exit(-1);
        }
    }
}