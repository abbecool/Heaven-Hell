#include "Assets.hpp"
#include "render/RenderBackend.hpp"

#include <fstream>
#include <string>
#include <iostream>
#include <limits>
#include <stdexcept>


#include <SDL3/SDL.h>
#include <SDL3_mixer/SDL_mixer.h>
#include "external/json.hpp"
using json = nlohmann::json;

namespace {
std::string assetNameFromPath(const std::string& path)
{
    const size_t slash = path.find_last_of("/\\");
    const size_t start = slash == std::string::npos ? 0 : slash + 1;
    const size_t dot = path.find_last_of('.');
    const size_t count = dot == std::string::npos || dot < start ? std::string::npos : dot - start;
    return path.substr(start, count);
}
}

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

}

void Assets::addSprite(const std::string& name, SpriteDefinition sprite) {
    m_sprites[name] = sprite;
}

const SpriteDefinition& Assets::getSprite(const std::string& name) const {
    try {
        return m_sprites.at(name);
    } catch (const std::out_of_range& e) {
        std::cerr << "Sprite not found: " << name << std::endl;
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
    RenderBackend& renderBackend
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
        renderBackend.loadFont(name, path, size);
    }
    const std::string pathTextures = j["textures"]["master_path"];
    for (const std::string path : j["textures"]["individual_paths"]) {
        const std::string texturePath = pathTextures + path;
        renderBackend.loadTexture(assetNameFromPath(texturePath), texturePath);
    }
    for (const auto& spriteJSON : j["animations"]) {
        const std::string name = spriteJSON["name"];
        int frames = spriteJSON["frames"];
        int frametime = spriteJSON["frametime"];
        int rows = spriteJSON["rows"];
        int cols = spriteJSON["cols"];
        TextureHandle texture{name};
        SpriteDefinition sprite = SpriteDefinition(
            name,
            texture,
            frames,
            frametime,
            rows,
            cols,
            renderBackend.textureSize(texture)
        );
        addSprite(name, sprite);
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
    while (file_text >> head) {
        if (head == "Font"){
            file_text >> font_name >> r >> g >> b >> a;
        }
    }
}
