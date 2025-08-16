#pragma once

#include "physics/Vec2.h"

#include <SDL_image.h>
#include <memory>
#include <unordered_map>
#include <vector>

using EntityID = uint32_t;

class Scene_Play;
class LevelLoader
{
private:
    Scene_Play* m_scene;
    int m_width;
    int m_height;
    
public:
    std::vector<std::vector<std::string>> m_pixelMatrix;
    void init(Scene_Play* scene, const int width, const int height);
    std::vector<bool> neighborCheck(const std::string &pixel, int x, int y, int width, int height);
    std::vector<std::string> neighborTag(const std::string &pixel, int x, int y, int width, int height);
    int getObstacleTextureIndex(const std::vector<bool>& neighbors);
    void createPixelMatrix(Uint32* pixels, SDL_PixelFormat* format, int width, int height);
    std::unordered_map<std::string, int> createDualGrid(int x, int y);
    EntityID loadChunk(Vec2 chunk);
    void removeChunk();
    void clearChunks(int chunksLeft);
};