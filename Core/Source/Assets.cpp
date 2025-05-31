#include "Assets.h"
#include <fstream>
#include <string>
#include <iostream>
#include <limits>


#include <SDL2/SDL.h>
#include "SDL2/SDL_image.h"
#include "SDL2/SDL_ttf.h"
#include <SDL2/SDL_mixer.h>
#include "json.hpp"
using json = nlohmann::json;

Assets::Assets(){}


void Assets::addTexture(std::string name, const std::string & path, SDL_Renderer * ren)
{
    const char *path_char = ("assets/images/"+path).c_str(); 
    SDL_Surface* tempSurface = IMG_Load(path_char);
    SDL_Texture* texture = SDL_CreateTextureFromSurface(ren, tempSurface);
    SDL_FreeSurface(tempSurface);
    if (texture == nullptr) {
        std::cerr << "Failed to load texture: " << name << ", from path: " << path_char << ", IMG_Error: " << IMG_GetError() << std::endl;
        return;
    }
    m_textures[name] = texture;
}

void Assets::addTexture(std::string name, SDL_Texture* texture)
{
    m_textures[name] = texture;
}

SDL_Texture * Assets::getTexture(std::string name) const
{
    try {
        return m_textures.at(name);
    } catch (const std::out_of_range& e) {
        std::cerr << "Texture not found: " << name << std::endl;
        throw;
    }
}

void Assets::addFont(const std::string& name, const std::string& path, const int font_size) {
    const char *path_char = ("assets/fonts/" + name+".ttf").c_str(); 
    TTF_Font* font = TTF_OpenFont(path_char, font_size);
    if (font == nullptr) {
        std::cerr << "Failed to load font! TTF_Error: " << TTF_GetError() << std::endl;
    }
    m_fonts[name] = font;
}

TTF_Font* Assets::getFont(const std::string& name) const {
    try {
        return m_fonts.at(name);
    } catch (const std::out_of_range& e) {
        std::cerr << "Font not found: " << name << std::endl;
        throw;
    }
}

void Assets::addAnimation(const std::string& name, Animation animation) {
    m_animations[name] = animation;
}

const Animation& Assets::getAnimation(const std::string& name) const {
    try {
        return m_animations.at(name);
    } catch (const std::out_of_range& e) {
        std::cerr << "Animation not found: " << name << std::endl;
        throw;
    }
}

void Assets::addAudio(const std::string& name, const std::string& path) {
    const char *path_char = ("assets/audio/" + name+".wav").c_str(); 
    Mix_Chunk* audio = Mix_LoadWAV(path_char);
    if (audio == nullptr) {
        std::cerr << "Failed to load audio! Mix_Error: " << Mix_GetError() << std::endl;
    }
    m_audios[name] = audio;
}

Mix_Chunk* Assets::getAudio(const std::string& name) const {
    try {
        return m_audios.at(name);
    } catch (const std::out_of_range& e) {
        std::cerr << "Audio not found: " << name << std::endl;
        throw;
    }
}

void Assets::addMusic(const std::string& name, const std::string& path) {
    const char *path_char = ("assets/music/" + name+".ogg").c_str(); 
    Mix_Music* music = Mix_LoadMUS(path_char);
    if (music == nullptr) {
        std::cerr << "Failed to load music! Mix_Error: " << Mix_GetError() << std::endl;
    }
    m_music[name] = music;
}

Mix_Music* Assets::getMusic(const std::string& name) const {
    try {
        return m_music.at(name);
    } catch (const std::out_of_range& e) {
        std::cerr << "Music not found: " << name << std::endl;
        throw;
    }
}

void Assets::loadFromFile(const std::string & pathImages, const std::string & pathText, SDL_Renderer * ren) {
    std::ifstream file_json("config_files/assets.json");
    if (!file_json) {
        std::cerr << "Could not load assets.json file!\n";
        exit(-1);
    }
    json j;
    file_json >> j;
    file_json.close();
    for (const auto& font : j["fonts"]) {
        std::string name = font["name"];
        std::string path = font["path"];
        int size = font["size"];
        addFont(name, path, size);
    }
    for (const auto& texture : j["textures"]) {
        std::string name = texture["name"];
        std::string path = texture["path"];
        addTexture(name, path, ren);
    }
    for (const auto& audio : j["audio"]) {
        std::string name = audio["name"];
        std::string path = audio["path"];
        addAudio(name, path);
    }
    for (const auto& music : j["music"]) {
        std::string name = music["name"];
        std::string path = music["path"];
        addMusic(name, path);
    }
    std::ifstream file(pathImages);
    if (!file) {
        std::cerr << "Could not load assets.txt file!\n";
        exit(-1);
    }
    std::string head;
    while (file >> head) {
        if (head == "Animation") {
            std::string aniName;
            std::string texName;
            int frames, speed, rows, cols;
            file >> aniName >> texName >> frames >> speed >> cols >> rows;
            SDL_Texture* tex = getTexture(texName);
            addAnimation( aniName, Animation( aniName, tex, frames, speed, rows, cols) );
        }
        else {
            std::cerr << "head to " << head << "\n";
            std::cerr << "The config file format is incorrect!\n";
            exit(-1);
        }
    }

    std::ifstream file_text(pathText);
    if (!file_text) {
        std::cerr << "Could not load text.txt file!\n";
        exit(-1);
    }
    // std::string head;
    std::string font_name;
    // Uint8 r, g, b;
    int r, g, b, a;
    SDL_Color textColor = {255, 255, 255, 255};
    while (file_text >> head) {
        if (head == "Font"){
            file_text >> font_name >> r >> g >> b >> a;
            textColor = {(Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a};
        }
        if (head == "Text") {
            std::string dialog;            
            file_text.ignore(std::numeric_limits<std::streamsize>::max(), '"');  // Ignore until the first quote
            std::getline(file_text, dialog, '"');  // Read until the closing quote

            SDL_Surface* textSurface = TTF_RenderText_Solid(getFont(font_name), dialog.c_str(), textColor);
            if (textSurface == nullptr) {
                std::cerr << "Unable to render text surface! TTF_Error: " << TTF_GetError() << std::endl;
            }

            SDL_Texture* textTexture = SDL_CreateTextureFromSurface(ren, textSurface);
            if (textTexture == nullptr) {
                std::cerr << "Unable to create texture from rendered text! SDL_Error: " << SDL_GetError() << std::endl;
            }
            SDL_FreeSurface(textSurface);
            m_textures[dialog] = textTexture;
        }
    }
}