#include "Assets.hpp"
#include "core/SDLPlatform.hpp"
#include "render/RenderBackend.hpp"

#include <fstream>
#include <string>
#include <iostream>
#include <stdexcept>
#include <vector>

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

json loadJsonFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Could not load json file: " + path);
    }

    json data;
    file >> data;
    return data;
}

void loadTextures(const json& data, RenderBackend& renderBackend)
{
    if (!data.contains("textures")) {
        return;
    }

    const std::string pathTextures = data["textures"]["master_path"];
    for (const std::string path : data["textures"]["individual_paths"]) {
        const std::string texturePath = pathTextures + path;
        renderBackend.loadTexture(assetNameFromPath(texturePath), texturePath);
    }
}

SpriteDefinition spriteDefinitionFromJson(
    const json& spriteJSON,
    RenderBackend& renderBackend
)
{
    const std::string name = spriteJSON["name"];
    const int frames = spriteJSON["frames"];
    const int frametime = spriteJSON["frametime"];
    const int rows = spriteJSON["rows"];
    const int cols = spriteJSON["cols"];
    const TextureHandle texture{
        spriteJSON.value("texture", name)
    };

    if (spriteJSON.contains("atlas")) {
        const auto& atlas = spriteJSON["atlas"];
        return SpriteDefinition(
            name,
            texture,
            frames,
            frametime,
            rows,
            cols,
            RectF{
                static_cast<float>(atlas["x"].get<int>()),
                static_cast<float>(atlas["y"].get<int>()),
                static_cast<float>(atlas["w"].get<int>()),
                static_cast<float>(atlas["h"].get<int>())
            }
        );
    }

    return SpriteDefinition(
        name,
        texture,
        frames,
        frametime,
        rows,
        cols,
        renderBackend.textureSize(texture)
    );
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
    json j = loadJsonFile(pathAssets);
    for (const auto& font : j["fonts"]) {
        std::string name = font["name"];
        std::string path = font["path"];
        int size = font["size"];
        renderBackend.loadFont(name, path, size);
    }

    loadTextures(j, renderBackend);

    std::vector<json> atlasManifests;
    if (j.contains("atlas_manifests")) {
        for (const std::string path : j["atlas_manifests"]) {
            atlasManifests.push_back(loadJsonFile(path));
            loadTextures(atlasManifests.back(), renderBackend);
        }
    }

    for (const auto& spriteJSON : j["animations"]) {
        SpriteDefinition sprite = spriteDefinitionFromJson(spriteJSON, renderBackend);
        addSprite(sprite.name(), sprite);
    }
    for (const auto& atlasManifest : atlasManifests) {
        for (const auto& spriteJSON : atlasManifest["animations"]) {
            SpriteDefinition sprite = spriteDefinitionFromJson(spriteJSON, renderBackend);
            addSprite(sprite.name(), sprite);
        }
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
