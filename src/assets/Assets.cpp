#include "Assets.h"
#include <fstream>
#include <string>
#include <iostream>
#include <limits>


#include <SDL3/SDL.h>
#include <SDL3_image/SDL_image.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <SDL3_mixer/SDL_mixer.h>
#include "external/json.hpp"
using json = nlohmann::json;

Assets::Assets(){}

Assets::~Assets(){
    shutdown();
}

bool Assets::ensureMixer()
{
    if (m_mixer) {
        return true;
    }
    if (!MIX_Init()) {
        std::cerr << "Failed to initialize SDL_mixer! SDL_Error: " << SDL_GetError() << std::endl;
        return false;
    }
    m_mixer = MIX_CreateMixerDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, nullptr);
    if (!m_mixer) {
        std::cerr << "Failed to create audio mixer! SDL_Error: " << SDL_GetError() << std::endl;
        MIX_Quit();
        return false;
    }
    return true;
}

void Assets::shutdown()
{
    for (auto& [name, audio] : m_audios) {
        MIX_DestroyAudio(audio);
    }
    m_audios.clear();

    for (auto& [name, music] : m_music) {
        MIX_DestroyAudio(music);
    }
    m_music.clear();

    if (m_mixer) {
        MIX_DestroyMixer(m_mixer);
        m_mixer = nullptr;
        MIX_Quit();
    }

    for (auto& [name, font] : m_fonts) {
        TTF_CloseFont(font);
    }
    m_fonts.clear();

    for (auto& [name, texture] : m_textures) {
        SDL_DestroyTexture(texture);
    }
    m_textures.clear();
}

void Assets::addTexture(const std::string & path, SDL_Renderer * ren)
{
    const char *path_char = path.c_str(); 
    std::string name = path.substr(0, path.find_last_of('.')).substr(path.find_last_of('/') + 1);
    SDL_Surface* tempSurface = IMG_Load(path_char);
    if (tempSurface == nullptr) {
        std::cerr   << "Failed to load image: "
                    << name
                    << ", from path: "
                    << path_char
                    << ", SDL_Error: "
                    << SDL_GetError()
                    << std::endl;
        return;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(ren, tempSurface);
    SDL_DestroySurface(tempSurface);
    if (texture == nullptr) {
        std::cerr   << "Failed to load texture: " 
                    << name 
                    << ", from path: " 
                    << path_char 
                    << ", SDL_Error: " 
                    << SDL_GetError() 
                    << std::endl;
        return;
    }
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);
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
    std::string fontPath = "assets/fonts/" + name + ".ttf";
    TTF_Font* font = TTF_OpenFont(fontPath.c_str(), font_size);
    if (font == nullptr) {
        std::cerr << "Failed to load font! SDL_Error: " << SDL_GetError() << std::endl;
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
    if (!ensureMixer()) {
        return;
    }
    std::string audioPath = "assets/audio/" + path; 
    MIX_Audio* audio = MIX_LoadAudio(m_mixer, audioPath.c_str(), true);
    if (audio == nullptr) {
        std::cerr << "Failed to load audio! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    m_audios[name] = audio;
}

MIX_Audio* Assets::getAudio(const std::string& name) const {
    try {
        return m_audios.at(name);
    } catch (const std::out_of_range& e) {
        std::cerr << "Audio not found: " << name << std::endl;
        throw;
    }
}

void Assets::addMusic(const std::string& name, const std::string& path) {
    if (!ensureMixer()) {
        return;
    }
    std::string pathString = "assets/music/" + path; 
    MIX_Audio* music = MIX_LoadAudio(m_mixer, pathString.c_str(), true);
    if (music == nullptr) {
        std::cerr << "Failed to load music! SDL_Error: " << SDL_GetError() << std::endl;
        return;
    }
    m_music[name] = music;
}

MIX_Audio* Assets::getMusic(const std::string& name) const {
    try {
        return m_music.at(name);
    } catch (const std::out_of_range& e) {
        std::cerr << "Music not found: " << name << std::endl;
        throw;
    }
}

void Assets::playAudio(const std::string& name)
{
    if (!ensureMixer()) {
        return;
    }
    MIX_Audio* audio = getAudio(name);
    if (!MIX_PlayAudio(m_mixer, audio)) {
        std::cerr << "Failed to play audio: " << name << ", SDL_Error: " << SDL_GetError() << std::endl;
    }
}

void Assets::loadFromFile(
    const std::string & pathAssets, 
    const std::string & pathText, 
    SDL_Renderer * ren
){
    std::ifstream file_assets(pathAssets);
    if (!file_assets) {
        throw std::runtime_error("Could not load assets file: " + pathAssets);
    }
    json j;
    file_assets >> j;
    file_assets.close();
    for (const auto& font : j["fonts"]) {
        std::string name = font["name"];
        std::string path = font["path"];
        int size = font["size"];
        addFont(name, path, size);
    }
    const std::string pathTextures = j["textures"]["master_path"];
    for (const std::string path : j["textures"]["individual_paths"]) {
        addTexture(pathTextures+path, ren);
    }
    for (const auto& animationJSON : j["animations"]) {
        const auto name = animationJSON["name"];
        int frames = animationJSON["frames"];
        int frametime = animationJSON["frametime"];
        int rows = animationJSON["rows"];
        int cols = animationJSON["cols"];
        SDL_Texture* tex = getTexture(animationJSON["name"]);
        Animation animation = Animation( name, tex, frames, frametime, rows, cols);
        addAnimation(name, animation);
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
    
    std::ifstream file_text(pathText);
    if (!file_text) {
        throw std::runtime_error("Could not load text file: " + pathText);
    }
    std::string head;
    std::string font_name;
    int r, g, b, a;
    SDL_Color textColor = {255, 255, 255, 255};
    while (file_text >> head) {
        if (head == "Font"){
            file_text >> font_name >> r >> g >> b >> a;
            textColor = {(Uint8)r, (Uint8)g, (Uint8)b, (Uint8)a};
        }
    }
}
