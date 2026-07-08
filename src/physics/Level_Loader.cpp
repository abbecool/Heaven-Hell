#include "physics/Level_Loader.hpp"
#include "scenes/Scene_Play.hpp"
#include "scenes/Scene_Menu.hpp"
#include "assets/Assets.hpp"
#include "core/Game.hpp"
#include "ecs/Components.hpp"
#include "core/Action.hpp"
#include "physics/RandomArray.hpp"
#include "ecs/ECS.hpp"
#include "render/RenderBackend.hpp"

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <deque>
#include <algorithm>
#include <utility>

// TODO: rework Level_Loader

namespace {

struct TileRect
{
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

std::vector<TileRect> mergeTileRects(const std::vector<bool>& occupied, int width, int height)
{
    std::vector<bool> visited(occupied.size(), false);
    std::vector<TileRect> rects;

    auto isOpen = [&](int x, int y) {
        const int index = y * width + x;
        return occupied[index] && !visited[index];
    };

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (!isOpen(x, y)) {
                continue;
            }

            int rectWidth = 1;
            while (x + rectWidth < width && isOpen(x + rectWidth, y)) {
                ++rectWidth;
            }

            int rectHeight = 1;
            bool canGrow = true;
            while (y + rectHeight < height && canGrow) {
                for (int dx = 0; dx < rectWidth; ++dx) {
                    if (!isOpen(x + dx, y + rectHeight)) {
                        canGrow = false;
                        break;
                    }
                }
                if (canGrow) {
                    ++rectHeight;
                }
            }

            for (int dy = 0; dy < rectHeight; ++dy) {
                for (int dx = 0; dx < rectWidth; ++dx) {
                    visited[(y + dy) * width + (x + dx)] = true;
                }
            }

            rects.push_back(TileRect{x, y, rectWidth, rectHeight});
        }
    }

    return rects;
}

void appendMergedColliderShapes(
    std::vector<ColliderShape>& shapes,
    const std::vector<bool>& occupied,
    int width,
    int height,
    const Vec2& tileSize,
    CollisionMask layer,
    CollisionMask targetMask,
    Color debugColor,
    bool isTrigger
) {
    for (const TileRect& rect : mergeTileRects(occupied, width, height)) {
        const Vec2 size{
            static_cast<float>(rect.width) * tileSize.x,
            static_cast<float>(rect.height) * tileSize.y
        };
        const Vec2 offset{
            (static_cast<float>(rect.x) + static_cast<float>(rect.width) / 2.0f) * tileSize.x,
            (static_cast<float>(rect.y) + static_cast<float>(rect.height) / 2.0f) * tileSize.y
        };
        shapes.emplace_back(offset, size, layer, targetMask, debugColor, isTrigger);
    }
}

} // namespace

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
    const int chunkStartX = static_cast<int>(chunk.x * m_chunkSize.x);
    const int chunkStartY = static_cast<int>(chunk.y * m_chunkSize.y);
    const int chunkEndX = std::min(chunkStartX + static_cast<int>(m_chunkSize.x), m_width);
    const int chunkEndY = std::min(chunkStartY + static_cast<int>(m_chunkSize.y), m_height);
    const int chunkWidth = std::max(0, chunkEndX - chunkStartX);
    const int chunkHeight = std::max(0, chunkEndY - chunkStartY);
    const Vec2 chunkOrigin{
        static_cast<float>(chunkStartX) * m_gridSize.x,
        static_cast<float>(chunkStartY) * m_gridSize.y
    };

    EntityID chunkID = m_scene->m_ECS.addEntity();
    m_scene->m_ECS.addComponent<CChunk>(chunkID);
    m_scene->m_ECS.addComponent<CStatic>(chunkID);
    m_scene->m_ECS.addComponent<CTransform>(chunkID, chunkOrigin);

    std::vector<bool> obstacleTiles(static_cast<size_t>(chunkWidth * chunkHeight), false);
    std::vector<bool> waterTiles(static_cast<size_t>(chunkWidth * chunkHeight), false);

    // Process the pixels
    for (int y = chunkStartY; y < chunkEndY; ++y) 
    {
        for (int x = chunkStartX; x < chunkEndX; ++x) 
        {
            const TileType& pixel = m_pixelMatrix[y * m_width + x];
            std::array<bool, 4> neighbors = neighborCheck(x, y, m_width, m_height);
            int textureIndex = getObstacleTextureIndex(neighbors);
            std::array<int, 5> tileIndex = createDualGrid(x, y);
            std::vector<EntityID> ids = m_scene->spawnDualTiles(
                Vec2 {16.0f * static_cast<float>(x) - 8.0f, 16.0f * static_cast<float>(y) - 8.0f},
                tileIndex
            );
            for (EntityID id : ids) {
                m_scene->m_ECS.addComponent<CStatic>(id);
                const Vec2 relativePos = m_scene->m_ECS.getComponent<CTransform>(id).pos - chunkOrigin;
                m_scene->m_ECS.attachChild(chunkID, id, relativePos);
            }

            const int localX = x - chunkStartX;
            const int localY = y - chunkStartY;
            const size_t localIndex = static_cast<size_t>(localY * chunkWidth + localX);

            if (pixel == TileType::OBSTACLE) 
            {
                obstacleTiles[localIndex] = true;
            }
            else if (pixel == TileType::WATER) 
            {
                waterTiles[localIndex] = true;
            }
        }
    }

    std::vector<ColliderShape> colliderShapes;
    const CollisionMask obstacleMask = ENEMY_LAYER | FRIENDLY_LAYER | PLAYER_LAYER | PROJECTILE_LAYER;
    appendMergedColliderShapes(
        colliderShapes,
        obstacleTiles,
        chunkWidth,
        chunkHeight,
        m_gridSize,
        OBSTACLE_LAYER,
        obstacleMask,
        Color{255, 255, 255, 255},
        false
    );
    const CollisionMask waterMask = ENEMY_LAYER | FRIENDLY_LAYER | PLAYER_LAYER;
    appendMergedColliderShapes(
        colliderShapes,
        waterTiles,
        chunkWidth,
        chunkHeight,
        m_gridSize,
        WATER_LAYER,
        waterMask,
        Color{0, 0, 255, 255},
        true
    );
    if (!colliderShapes.empty()) {
        m_scene->m_ECS.addComponent<CCollider>(chunkID, std::move(colliderShapes));
    }

    return chunkID;
}

void LevelLoader::renderChunkGrid(RenderBackend& renderer) const
{
    constexpr Color chunkGridColor{0, 255, 0, 255};

    for (auto [id, chunk, transform] : m_scene->m_ECS.constView<CChunk, CTransform>()) {
        const int chunkStartX = static_cast<int>(transform.pos.x / m_gridSize.x);
        const int chunkStartY = static_cast<int>(transform.pos.y / m_gridSize.y);
        const int chunkEndX = std::min(chunkStartX + static_cast<int>(m_chunkSize.x), m_width);
        const int chunkEndY = std::min(chunkStartY + static_cast<int>(m_chunkSize.y), m_height);
        const int chunkWidth = std::max(0, chunkEndX - chunkStartX);
        const int chunkHeight = std::max(0, chunkEndY - chunkStartY);

        if (chunkWidth == 0 || chunkHeight == 0) {
            continue;
        }

        renderer.drawWorldRect(RectF{
            transform.pos.x,
            transform.pos.y,
            static_cast<float>(chunkWidth) * m_gridSize.x,
            static_cast<float>(chunkHeight) * m_gridSize.y
        }, chunkGridColor);
    }
}

void LevelLoader::update(Vec2 playerPosition)
{
    bool chunksChanged = false;
    bool chunkRemovalsQueued = false;
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

    if (!m_chunkQueue.empty()){
        Vec2 chunk = m_chunkQueue.front();
        m_chunkQueue.pop_front();
        EntityID chunkID = loadChunk(chunk);
        m_loadedChunkIDs.push_back(chunkID);
        m_loadedChunks.push_back(chunk);
        chunksChanged = true;
    }

    // std::cout << "Chunk Queue Size: " << m_chunkQueue.size() << " | Loaded Chunks: " << m_loadedChunks.size() << std::endl;

    std::vector<Vec2> chunksToRemove;
    for (Vec2 chunk : m_loadedChunks){
        if (std::find(m_neighboringChunks.begin(), m_neighboringChunks.end(), chunk) == m_neighboringChunks.end()){
            chunksToRemove.push_back(chunk);
        }
    }
    for (Vec2 chunk : chunksToRemove){
        removeChunk(chunk);
        chunksChanged = true;
        chunkRemovalsQueued = true;
    }

    if (chunksChanged) {
        if (chunkRemovalsQueued) {
            m_scene->m_ECS.update();
            m_scene->m_rendererManager.update();
        }
        m_scene->m_collisionManager.rebuildStaticQuadtree();
    }
 }

void LevelLoader::removeChunk(Vec2 chunk){
    auto it = std::find(m_loadedChunks.begin(), m_loadedChunks.end(), chunk);
    if (it == m_loadedChunks.end()) {
        return;
    }
    int index = std::distance(m_loadedChunks.begin(), it);
    EntityID chunkID = m_loadedChunkIDs[index];
    m_loadedChunks.erase(m_loadedChunks.begin() + index);
    m_loadedChunkIDs.erase(m_loadedChunkIDs.begin()+ index);
    m_scene->m_ECS.queueRemoveEntity(chunkID);
}

Vec2 LevelLoader::getLevelSize() const {
    return m_levelSize;
}

Vec2 LevelLoader::getWorldSize() const {
    return m_levelSize * m_gridSize;
}
