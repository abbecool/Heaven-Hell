#include "physics/Level_Loader.hpp"
#include "scenes/Scene_Play.hpp"
#include "scenes/Scene_Menu.hpp"
#include "assets/Assets.hpp"
#include "core/Game.hpp"
#include "ecs/Components.hpp"
#include "core/Action.hpp"
#include "physics/RandomArray.hpp"
#include "ecs/ECS.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <deque>
#include <algorithm>

// TODO: rework Level_Loader

LevelLoader::LevelLoader(
    Scene_Play* scene, 
    const Vec2 gridSize, 
    const PixelImage& levelImage
){
    m_scene = scene;
    m_gridSize = gridSize;
    if (levelImage.width <= 0 || levelImage.height <= 0 || levelImage.pixels.empty()) {
        std::cerr << "Level image has no pixels." << std::endl;
        return;
    }
    m_height = levelImage.height;
    m_width = levelImage.width;
    m_levelSize = Vec2{static_cast<float>(m_width), static_cast<float>(m_height)};
    createPixelMatrix(levelImage);
    loadChunk(m_currentChunk);
}

std::array<bool, 4> LevelLoader::neighborCheck(
    int x,
    int y,
    int width,
    int height
) {
    std::array<bool, 4> neighbors = {}; // {top, bottom, left, right}
    TileType pix = m_pixelMatrix[y * width + x];
    if(!neighbors[0]){neighbors[0] = (y > 0 && m_pixelMatrix[(y - 1) * width + x] == pix);}           // top
    if(!neighbors[1]){neighbors[1] = (x < width - 1 && m_pixelMatrix[y * width + (x + 1)] == pix);}   // right
    if(!neighbors[2]){neighbors[2] = (y < height - 1 && m_pixelMatrix[(y + 1) * width + x] == pix);}  // bottom
    if(!neighbors[3]){neighbors[3] = (x > 0 && m_pixelMatrix[y * width + (x - 1)] == pix);}           // left
    return neighbors;
}

std::array<TileType, 4> LevelLoader::neighborTag(
    int x, 
    int y, 
    int width, 
    int height
) {
    
    std::array<TileType, 4> neighborsTags = {}; // {top, bottom, left, right}
    if(y > 0){neighborsTags[0] = m_pixelMatrix[(y - 1) * width + x];}            // top
    if(x < width - 1){neighborsTags[1] = m_pixelMatrix[y * width + (x + 1)];}    // right
    if(y < height - 1){neighborsTags[2] = m_pixelMatrix[(y + 1) * width + x];}   // bottom
    if(x > 0 ){neighborsTags[3] = m_pixelMatrix[y * width + (x - 1)];}           // left

    return neighborsTags;
}

int LevelLoader::getObstacleTextureIndex(const std::array<bool, 4>& neighbors) {
    int numObstacles = static_cast<int>(std::count(neighbors.begin(), neighbors.end(), true));
    if (numObstacles == 1) {
        if (neighbors[0]) return 12;    // Top
        if (neighbors[1]) return 1;     // Right
        if (neighbors[2]) return 4;     // Bottom
        if (neighbors[3]) return 3;     // Left
    } else if (numObstacles == 2) {
        if (!neighbors[0] && !neighbors[1]) return 7;  // Top & Right
        if (!neighbors[1] && !neighbors[2]) return 15; // Right & Bottom
        if (!neighbors[2] && !neighbors[3]) return 13; // Bottom & Left
        if (!neighbors[3] && !neighbors[0]) return 5;  // Left & Top
        if (!neighbors[0] && !neighbors[2]) return 2;  // Top & Bottom
        if (!neighbors[1] && !neighbors[3]) return 8; // Right & Left
    } else if (numObstacles == 3) {
        if (!neighbors[0]) return 6;    // Top is not an obstacle
        if (!neighbors[1]) return 11;   // Right is not an obstacle
        if (!neighbors[2]) return 14;   // Bottom is not an obstacle
        if (!neighbors[3]) return 9;    // Left is not an obstacle
    } else if (numObstacles == 4) {
        return 10; // All neighbors are obstacles
    }
    return 0; // No neighbors are obstacles
}

void LevelLoader::createPixelMatrix(const PixelImage& levelImage) {
    const int width = levelImage.width;
    const int height = levelImage.height;
    m_pixelMatrix.resize(height * width);

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            const PixelRGBA& pixel = levelImage.pixels[y * width + x];
            
            TileType tile = TileType::UNKNOWN;
            if (static_cast<int>(pixel.r) == 192 && static_cast<int>(pixel.g) == 192 && static_cast<int>(pixel.b) == 192) {
                tile = TileType::OBSTACLE;
            } else if (static_cast<int>(pixel.r) == 203 && static_cast<int>(pixel.g) == 129 && static_cast<int>(pixel.b) == 56) {
                tile = TileType::DIRT;
            } else if (static_cast<int>(pixel.r) == 0 && static_cast<int>(pixel.g) == 255 && static_cast<int>(pixel.b) == 0) {
                tile = TileType::GRASS;
            } else if (static_cast<int>(pixel.r) == 0 && static_cast<int>(pixel.g) == 0 && static_cast<int>(pixel.b) == 255) {
                tile = TileType::WATER;
            } else if (static_cast<int>(pixel.r) == 179 && static_cast<int>(pixel.g) == 0 && static_cast<int>(pixel.b) == 255) {
                tile = TileType::WATER; // replaced bridge with water
            }
            m_pixelMatrix[y * width + x] = tile;
        }
    }
}


std::array<int, 5> LevelLoader::createDualGrid(int x, int y) 
{
    std::array<TileType, 4> tileQ = {};
    std::array<int, 5> tileTextures = {};
    int textureIndex;

    tileQ[0] = (x > 0) ? m_pixelMatrix[y * m_width + (x - 1)] : m_pixelMatrix[y * m_width + x];  // Q3
    tileQ[1] = m_pixelMatrix[y * m_width + x];   // Q4
    tileQ[2] = (y > 0) ? m_pixelMatrix[(y - 1) * m_width + x] : m_pixelMatrix[y * m_width + x];  // Q1
    tileQ[3] = (x > 0 && y > 0) ? m_pixelMatrix[(y - 1) * m_width + (x - 1)] : m_pixelMatrix[y * m_width + x];  // Q2

    for ( TileType tile : tileQ ) {

        int numTiles = static_cast<int>(std::count(tileQ.begin(), tileQ.end(), tile));
        int uniqueCount = 0;
        bool seen[5] = {false};  // Since TileType has 5 values
        for (TileType t : tileQ) {
            int idx = static_cast<int>(t);
            if (!seen[idx]) {
                seen[idx] = true;
                uniqueCount++;
            }
        }
        textureIndex = 0;

        if (numTiles == 0) {
            continue;
        }

        if (numTiles == 4) {
            textureIndex = 10; // All quadrants are tiles
        } else if (numTiles == 3) {
            if (tileQ[0] != tile) textureIndex = 14;
            if (tileQ[1] != tile) textureIndex = 11;
            if (tileQ[2] != tile) textureIndex = 6;
            if (tileQ[3] != tile) textureIndex = 9;
        } else if (numTiles == 2) {
            if (tileQ[0] != tile && tileQ[1] != tile) textureIndex = 13;
            if (tileQ[1] != tile && tileQ[2] != tile) textureIndex = 15;
            if (tileQ[2] != tile && tileQ[3] != tile) textureIndex = 7;
            if (tileQ[3] != tile && tileQ[0] != tile) textureIndex = 5;
            if (tileQ[0] != tile && tileQ[2] != tile) textureIndex = 8;
            if (tileQ[1] != tile && tileQ[3] != tile) textureIndex = 2; 
            if (uniqueCount == 3 && (tile == TileType::GRASS)) {
                if (tileQ[0] == tile && tileQ[1] == tile) textureIndex = 16;
                if (tileQ[1] == tile && tileQ[2] == tile) textureIndex = 18;
                if (tileQ[2] == tile && tileQ[3] == tile) textureIndex = 17;
                if (tileQ[3] == tile && tileQ[0] == tile) textureIndex = 19;
            }
        } else if (numTiles == 1) {
            if (tileQ[0] == tile) textureIndex = 4;
            if (tileQ[1] == tile) textureIndex = 1;
            if (tileQ[2] == tile) textureIndex = 12;
            if (tileQ[3] == tile) textureIndex = 3;
            if (uniqueCount == 3 && (tile == TileType::GRASS)) {
                if (tileQ[0] == tile && tileQ[2] == tileQ[3]) textureIndex = 21;
                if (tileQ[0] == tile && tileQ[1] == tileQ[2]) textureIndex = 23;

                if (tileQ[1] == tile && tileQ[0] == tileQ[3]) textureIndex = 20;
                if (tileQ[1] == tile && tileQ[2] == tileQ[3]) textureIndex = 22;

                if (tileQ[2] == tile && tileQ[0] == tileQ[1]) textureIndex = 24;
                if (tileQ[2] == tile && tileQ[0] == tileQ[3]) textureIndex = 26;

                if (tileQ[3] == tile && tileQ[1] == tileQ[2]) textureIndex = 25;
                if (tileQ[3] == tile && tileQ[0] == tileQ[1]) textureIndex = 27;
            }
        }
        tileTextures[static_cast<int>(tile)] = textureIndex;
    }

    return tileTextures;
}

EntityID LevelLoader::loadChunk(Vec2 chunk)
{
    std::vector<EntityID> chunkChildren;

    // Process the pixels
    for (int y = chunk.y*m_chunkSize.y; y < (chunk.y+1)*m_chunkSize.y; ++y) 
    {
        if (y >= m_height) {
            continue; // Skip if out of bounds
        }
        for (int x = chunk.x*m_chunkSize.x; x < (chunk.x+1)*m_chunkSize.x; ++x) 
        {
            if (x >= m_width) {
                continue; // Skip if out of bounds
            }
            const TileType& pixel = m_pixelMatrix[y * m_width + x];
            std::array<bool, 4> neighbors = neighborCheck(x, y, m_width, m_height);
            std::array<TileType, 4> neighborsTags = neighborTag(
                x, 
                y, 
                m_width, 
                m_height
            );
            int textureIndex = getObstacleTextureIndex(neighbors);
            std::array<int, 5> tileIndex = createDualGrid(x, y);
            std::vector<EntityID> ids = m_scene->spawnDualTiles(
                Vec2 {16.0f * static_cast<float>(x) - 8.0f, 16.0f * static_cast<float>(y) - 8.0f},
                tileIndex
            );
            chunkChildren.reserve(chunkChildren.size() + ids.size());
            for (EntityID id : ids) {
                chunkChildren.push_back(id);
            }
            // Spawn obsticle if it is an edge or corner tile
            if (pixel == TileType::OBSTACLE && textureIndex != 10) 
            {
                EntityID id = m_scene->spawnObstacle(
                    Vec2 {16.0f * static_cast<float>(x), 16.0f * static_cast<float>(y)},
                    false, textureIndex
                );
                chunkChildren.push_back(id);
            }
            else if (pixel == TileType::WATER && textureIndex != 10) 
            {
                EntityID id = m_scene->spawnWater(
                    Vec2 {16.0f * static_cast<float>(x), 16.0f * static_cast<float>(y)},
                    "Water", 
                    textureIndex
                );
                chunkChildren.push_back(id);
            }
        }
    }
    EntityID chunkID = m_scene->m_ECS.addEntity();
    m_scene->m_ECS.addComponent<CChunk>(chunkID, chunk);
    m_scene->m_ECS.getComponent<CChunk>(chunkID).chunkChildern = chunkChildren;
    return chunkID;
}

void LevelLoader::update(Vec2 playerPosition)
{
    m_currentChunk = ( ( (playerPosition / m_gridSize).toInt() ) / m_chunkSize ).toInt();
    m_neighboringChunks.clear();
    for (int dx = -2; dx <= 2; ++dx) 
    {
        for (int dy = -1; dy <= 1; ++dy) 
        {
            Vec2 neighborChunk = {m_currentChunk.x + dx, m_currentChunk.y + dy};
            if ( neighborChunk.smaller(Vec2{0,0}) || neighborChunk.greater(m_levelSize/m_chunkSize) )
            {
                continue;
            }
            m_neighboringChunks.push_back(neighborChunk);
            if (std::find(m_loadedChunks.begin(), m_loadedChunks.end(), neighborChunk) == m_loadedChunks.end() &&
                std::find(m_chunkQueue.begin(), m_chunkQueue.end(), neighborChunk) == m_chunkQueue.end())
            {
                m_chunkQueue.push_back(neighborChunk);
            }
        }
    }
    if (m_chunkQueue.empty()){
        return;
    }
    Vec2 chunk = m_chunkQueue.front();
    m_chunkQueue.pop_front();
    EntityID chunkID = loadChunk(chunk);
    m_loadedChunkIDs.push_back(chunkID);
    m_loadedChunks.push_back(chunk);

    // std::cout << "Chunk Queue Size: " << m_chunkQueue.size() << " | Loaded Chunks: " << m_loadedChunks.size() << std::endl;

    std::vector<Vec2> chunksToRemove;
    for (Vec2 chunk : m_loadedChunks){
        if (std::find(m_neighboringChunks.begin(), m_neighboringChunks.end(), chunk) == m_neighboringChunks.end()){
            chunksToRemove.push_back(chunk);
        }
    }
    for (Vec2 chunk : chunksToRemove){
        removeChunk(chunk);
    }
 }

void LevelLoader::removeChunk(Vec2 chunk){
    auto it = std::find(m_loadedChunks.begin(), m_loadedChunks.end(), chunk);
    int index = std::distance(m_loadedChunks.begin(), it);
    EntityID chunkID = m_loadedChunkIDs[index];
    const std::vector<EntityID>& chunkChildren =  m_scene->m_ECS.getComponent<CChunk>(chunkID).chunkChildern;
    for ( EntityID id : chunkChildren )
    {
        m_scene->m_ECS.queueRemoveEntity(id);
        if ( m_scene->m_ECS.hasComponent<CSprite>(id) )
        {
            auto layer = m_scene->m_ECS.getComponent<CSprite>(id).layer;
            m_scene->m_rendererManager.queueRemoveEntityFromLayer(id, layer);
        }
    }
    m_loadedChunks.erase(m_loadedChunks.begin() + index);
    m_loadedChunkIDs.erase(m_loadedChunkIDs.begin()+ index);
    m_scene->m_ECS.queueRemoveEntity(chunkID);
}

Vec2 LevelLoader::getLevelSize(){
    return m_levelSize;
}
