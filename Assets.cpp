#pragma once

#include "src/include/Assets.h"
#include <fstream>
#include <iostream>

#include <SDL2/SDL.h>
#include "src\include\SDL2\SDL_image.h"

Assets::Assets(){}

void Assets::addTexture(std::string name, const std::string & path, SDL_Renderer * ren)
{
    // std::string path1 = path;
    // path1.erase(remove(path1.begin(), path1.end(), '"'), path1.end()); //remove A from string
    const char *path_char = path.c_str(); 
    SDL_Surface* tempSurface = IMG_Load(path_char);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(ren, tempSurface);
    SDL_FreeSurface(tempSurface);

    m_textures[name] = texture;
}

SDL_Texture * Assets::getTexture(std::string name) const
{
    return m_textures.at(name);
}

void Assets::loadFromFile(const std::string & path, SDL_Renderer * ren) {
    std::ifstream file(path);
    if (!file) {
        std::cerr << "Could not load config.txt file!\n";
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
        // else if (head == "Font") {
        //     std::string font_name;
        //     std::string font_path;
        //     file >> font_name >> font_path;
        //     addFont(font_name, font_path);
        // }
        // else if (head == "Animation") {
        //     std::string aniName;
        //     std::string texName;
        //     int frames, speed;
        //     file >> aniName >> texName >> frames >> speed;
        //     const sf::Texture& tex = getTexture(texName);
        //     addAnimation(
        //         aniName, 
        //         Animation(aniName, tex, frames, speed)
        //     );
        // }
        else {
            std::cerr << "head to " << head << "\n";
            std::cerr << "The config file format is incorrect!\n";
            exit(-1);
        }
    }
}