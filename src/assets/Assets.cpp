#include "Assets.hpp"
#include "core/SDLPlatform.hpp"
#include "render/RenderBackend.hpp"

#include <fstream>
#include <string>
#include <iostream>
#include <stdexcept>

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

void Assets::shutdown()
{
    m_sprites.clear();
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

void Assets::loadFromFile(
    const std::string & pathAssets, 
    const std::string & pathText, 
    RenderBackend& renderBackend,
    SDLPlatform& platform
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
        platform.loadAudio(name, path);
    }
    for (const auto& music : j["music"]) {
        std::string name = music["name"];
        std::string path = music["path"];
        platform.loadMusic(name, path);
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
