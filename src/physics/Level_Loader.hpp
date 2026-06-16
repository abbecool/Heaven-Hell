#pragma once

#include "physics/Vec2.hpp"

#include <SDL3/SDL.h>
#include <memory>
#include <vector>
#include <deque>
#include <array>

using EntityID = uint32_t;

enum struct TileType {
    OBSTACLE = 0,
    DIRT = 1,
    GRASS = 2,
    WATER = 3,
    UNKNOWN = 4
};

using PixelMatrix = std::vector<TileType>;

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
    std::deque<Vec2> m_chunkQueue;
    std::vector<Vec2> m_neighboringChunks;
    std::vector<EntityID> m_loadedChunkIDs;
    
public:
    LevelLoader(){}
    LevelLoader(Scene_Play* scene, const Vec2 gridSize, const std::string levelPath);
    std::vector<TileType> m_pixelMatrix;
    std::array<bool, 4> neighborCheck(int x, int y, int width, int height);
    std::array<TileType, 4> neighborTag(int x, int y, int width, int height);
    int getObstacleTextureIndex(const std::array<bool, 4>& neighbors);
    void createPixelMatrix(Uint32* pixels, const SDL_PixelFormatDetails* format, int width, int height, int pitchPixels);
    std::array<int, 5> createDualGrid(int x, int y);
    EntityID loadChunk(Vec2 chunk);
    void removeChunk(Vec2 chunk);
    void update(Vec2);
    Vec2 getLevelSize();
};
