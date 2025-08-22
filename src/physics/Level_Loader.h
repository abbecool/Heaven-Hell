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
    Vec2 m_gridSize;
    
    Vec2 m_currentChunk = Vec2{1, 0};
    Vec2 m_chunkSize = Vec2{12, 12};
    Vec2 m_levelSize;
    std::vector<Vec2> m_loadedChunks;
    std::vector<Vec2> m_chunkQueue;
    std::vector<Vec2> m_neighboringChunks;
    std::vector<EntityID> m_loadedChunkIDs;
    
public:
    LevelLoader(){}
    LevelLoader(Scene_Play* scene, const Vec2 gridSize, const std::string levelPath);
    std::vector<std::vector<std::string>> m_pixelMatrix;
    std::vector<bool> neighborCheck(const std::string &pixel, int x, int y, int width, int height);
    std::vector<std::string> neighborTag(const std::string &pixel, int x, int y, int width, int height);
    int getObstacleTextureIndex(const std::vector<bool>& neighbors);
    void createPixelMatrix(Uint32* pixels, SDL_PixelFormat* format, int width, int height);
    std::unordered_map<std::string, int> createDualGrid(int x, int y);
    EntityID loadChunk(Vec2 chunk);
    void removeChunk(Vec2 chunk);
    void update(Vec2);
    Vec2 getLevelSize();
};
