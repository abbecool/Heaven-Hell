#include "physics/Level_Loader.h"
#include "scenes/Scene_Play.h"
#include "scenes/Scene_Menu.h"
#include "assets/Sprite.h"
#include "assets/Assets.h"
#include "core/Game.h"
#include "ecs/Components.h"
#include "core/Action.h"
#include "physics/RandomArray.h"
#include "ecs/ECS.hpp"

#include <SDL_image.h>
#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>

using PixelMatrix = std::vector<std::vector<std::string>>;

// TODO: rework Level_Loader

LevelLoader::LevelLoader(Scene_Play* scene, const Vec2 gridSize, const std::string levelPath){
    m_scene = scene;
    m_gridSize = gridSize;
    const char* path = levelPath.c_str();
    SDL_Surface* loadedSurface = IMG_Load(path);
    if (loadedSurface == nullptr) 
    {
        std::cerr << "Not loaded " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
    }

    // Lock the surface to access the pixels
    SDL_LockSurface(loadedSurface);
    Uint32* pixels = (Uint32*)loadedSurface->pixels;

    const int HEIGHT_PIX = loadedSurface->h;
    const int WIDTH_PIX = loadedSurface->w;
    m_levelSize = Vec2{ (float)WIDTH_PIX, (float)HEIGHT_PIX };
    createPixelMatrix(pixels, loadedSurface->format, WIDTH_PIX, HEIGHT_PIX);
    // Unlock and free the surface
    SDL_UnlockSurface(loadedSurface);
    SDL_FreeSurface(loadedSurface);
    loadChunk(m_currentChunk);
}

std::vector<bool> LevelLoader::neighborCheck(
    const std::string &pixel, 
    int x, 
    int y, 
    int width, 
    int height
) {
    std::vector<std::string> friendlyPixels(1, "");
    std::vector<bool> neighbors(4, false); // {top, bottom, left, right}
    if ( pixel == "grass" || pixel == "dirt"){
        friendlyPixels = {"grass", "dirt", "key", "goal", "player_God", "player_Devil", "dragon", "water"};
    } else if ( pixel == "water" ){
        friendlyPixels = {"water", "bridge"};
    } else if (pixel == "lava"){
        friendlyPixels = {"lava", "bridge"};
    }
    else{
        friendlyPixels = {pixel};
    }
    for ( auto pix : friendlyPixels){
        if(!neighbors[0]){neighbors[0] = (y > 0 && m_pixelMatrix[y - 1][x] == pix);}           // top
        if(!neighbors[1]){neighbors[1] = (x < width - 1 && m_pixelMatrix[y][x + 1] == pix);}   // right
        if(!neighbors[2]){neighbors[2] = (y < height - 1 && m_pixelMatrix[y + 1][x] == pix);}  // bottom
        if(!neighbors[3]){neighbors[3] = (x > 0 && m_pixelMatrix[y][x - 1] == pix);}           // left
    }
    return neighbors;

}

std::vector<std::string> LevelLoader::neighborTag(
    const std::string &pixel, 
    int x, 
    int y, 
    int width, 
    int height
) {
    
    std::vector<std::string> neighborsTags(4, "nan"); // {top, bottom, left, right}
    if(y > 0){neighborsTags[0] = m_pixelMatrix[y - 1][x];}            // top
    if(x < width - 1){neighborsTags[1] = m_pixelMatrix[y][x + 1];}    // right
    if(y < height - 1){neighborsTags[2] = m_pixelMatrix[y + 1][x];}   // bottom
    if(x > 0 ){neighborsTags[3] = m_pixelMatrix[y][x - 1];}           // left

    return neighborsTags;
}

int LevelLoader::getObstacleTextureIndex(const std::vector<bool>& neighbors) {
    int numObstacles = (int)std::count(neighbors.begin(), neighbors.end(), true);
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

void LevelLoader::createPixelMatrix(
    Uint32* pixels, 
    SDL_PixelFormat* format, 
    int width, 
    int height
) {
    m_pixelMatrix = PixelMatrix(height, std::vector<std::string>(width, ""));

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Uint32 pixel = pixels[y * width + x];

            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, format, &r, &g, &b, &a);
            
            if ((int)r == 192 && (int)g == 192 && (int)b == 192) {
                m_pixelMatrix[y][x] = "obstacle";
            } else if ((int)r == 200 && (int)g == 240 && (int)b == 255) {
                m_pixelMatrix[y][x] = "cloud";
            } else if ((int)r == 203 && (int)g == 129 && (int)b == 56) {
                m_pixelMatrix[y][x] = "dirt";
            } else if ((int)r == 0 && (int)g == 255 && (int)b == 0) {
                m_pixelMatrix[y][x] = "grass";
            } else if ((int)r == 255 && (int)g == 255 && (int)b == 255) {
                m_pixelMatrix[y][x] = "player_God";
            } else if ((int)r == 0 && (int)g == 0 && (int)b == 0) {
                m_pixelMatrix[y][x] = "player_Devil";
            } else if ((int)r == 255 && (int)g == 0 && (int)b == 255) {
                m_pixelMatrix[y][x] = "key";
            } else if ((int)r == 255 && (int)g == 255 && (int)b == 0) {
                m_pixelMatrix[y][x] = "goal";
            } else if ((int)r == 9 && (int)g == 88 && (int)b == 9) {
                m_pixelMatrix[y][x] = "dragon";
            } else if ((int)r == 255 && (int)g == 0 && (int)b == 0) {
                m_pixelMatrix[y][x] = "lava";
            } else if ((int)r == 0 && (int)g == 0 && (int)b == 255) {
                m_pixelMatrix[y][x] = "water";
            } else if ((int)r == 179 && (int)g == 0 && (int)b == 255) {
                m_pixelMatrix[y][x] = "bridge";
            } else {
                m_pixelMatrix[y][x] = "unknown";
            }
        }
    }
}


std::unordered_map<std::string, int> LevelLoader::createDualGrid(int x, int y) 
{
    std::vector<std::string> tileQ(4, "");
    std::unordered_map<std::string, int> tileTextureMap;

    tileQ[1] = m_pixelMatrix[y][x];   // Q4
    tileQ[0] = (x > 0) ? m_pixelMatrix[y][x - 1] : m_pixelMatrix[y][x];  // Q3
    tileQ[2] = (y > 0) ? m_pixelMatrix[y - 1][x] : m_pixelMatrix[y][x];  // Q1
    tileQ[3] = (x > 0 && y > 0) ? m_pixelMatrix[y - 1][x - 1] : m_pixelMatrix[y][x];  // Q2

    std::unordered_map<std::string, std::unordered_set<std::string>> friendlyNeighbors = {
        {"grass", {"key", "goal", "player_God", "dragon"}},
        {"dirt", {"key", "goal", "player_Devil", "dragon"}}
    };

    for ( std::string tile : {"grass", "dirt", "water", "lava", "cloud", "obstacle", "bridge"} ) {
        if (std::find(tileQ.begin(), tileQ.end(), "bridge") != tileQ.end()) {
            if (std::find(tileQ.begin(), tileQ.end(), "water") != tileQ.end()) {
                std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [](std::string str) {
                    return (str == "bridge") ? "water" : str;
                });
            } else if (std::find(tileQ.begin(), tileQ.end(), "lava") != tileQ.end()) {
                std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [](std::string str) {
                    return (str == "bridge") ? "lava" : str;
                });
            } else {
                std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [](std::string str) {
                    return (str == "bridge") ? "" : str;
                });
            }
        }

        std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [&](const std::string& str) {
            return friendlyNeighbors[tile].count(str) ? tile : str;
        });

        int numTiles = (int)std::count(tileQ.begin(), tileQ.end(), tile);
        std::unordered_set<std::string> uniqueStrings(tileQ.begin(), tileQ.end());

        if (numTiles > 0) {
            int textureIndex = -1;  // Initialize textureIndex

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
                if (uniqueStrings.size() == 3 && (tile == "grass")) {
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
                if (uniqueStrings.size() == 3 && (tile == "grass")) {
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

            // Add to the map if a valid textureIndex was assigned
            if (textureIndex != -1) {
                tileTextureMap[tile] = textureIndex;
            }
        }
    }

    return tileTextureMap;
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
            const std::string& pixel = m_pixelMatrix[y][x];
            std::vector<bool> neighbors = neighborCheck(pixel, x, y, m_width, m_height);
            std::vector<std::string> neighborsTags = neighborTag(pixel, x, y, m_width, m_height);
            int textureIndex = getObstacleTextureIndex(neighbors);
            std::unordered_map<std::string, int> tileIndex = createDualGrid(x, y);
            std::vector<EntityID> ids = m_scene->spawnDualTiles(Vec2 {16*(float)x - 16/2, 16*(float)y - 16/2},  tileIndex);
            chunkChildren.insert(chunkChildren.end(), ids.begin(), ids.end());

            // Spawn obsticle if it is an edge or corner tile
            if (pixel == "obstacle" && textureIndex != 10) 
            {
                EntityID id = m_scene->spawnObstacle(Vec2 {16*(float)x, 16*(float)y}, false, textureIndex);
                chunkChildren.push_back(id);
            }
            else
            {
                if (pixel == "lava") 
                {
                    EntityID id = m_scene->spawnLava(Vec2 {16*(float)x,16*(float)y}, "Lava", textureIndex);
                    chunkChildren.push_back(id);
                } 
                else if (pixel == "water" && textureIndex != 10) 
                {
                    EntityID id = m_scene->spawnWater(Vec2 {16*(float)x,16*(float)y}, "Water", textureIndex);
                    chunkChildren.push_back(id);
                }
            }
        }
    }
    EntityID chunkID = m_scene->m_ECS.addEntity();
    m_scene->m_ECS.addComponent<CChunk>(chunkID, chunk);
    m_scene->m_ECS.getComponent<CChunk>(chunkID).chunkChildern = chunkChildren;
    return chunkID;
}

void LevelLoader::removeChunk()
{
    EntityID chunkID = m_loadedChunkIDs[0];
    std::vector<EntityID> chunkChildren =  m_scene->m_ECS.getComponent<CChunk>(chunkID).chunkChildern;
    for ( EntityID id : chunkChildren )
    {
        m_scene->m_ECS.queueRemoveEntity(id);
        if ( m_scene->m_ECS.hasComponent<CAnimation>(id) )
        {
            auto layer = m_scene->m_ECS.getComponent<CAnimation>(id).layer;
            m_scene->m_rendererManager.queueRemoveEntityFromLayer(id, layer);
        }
    }
    m_loadedChunks.erase(m_loadedChunks.begin());
    m_loadedChunkIDs.erase(m_loadedChunkIDs.begin());
    m_scene->m_ECS.queueRemoveEntity(chunkID);
}

void LevelLoader::clearChunks(int chunksLeft)
{
    while ((int)m_loadedChunks.size() > chunksLeft)
    {
        removeChunk();
    }
}

void LevelLoader::update(Vec2 playerPosition)
{
    m_currentChunk = ( ( (playerPosition / m_gridSize).toInt() ) / m_chunkSize ).toInt();
    m_neighboringChunks = {m_currentChunk};
    for (int dx = -2; dx <= 2; ++dx) 
    {
        for (int dy = -1; dy <= 1; ++dy) 
        {
            Vec2 neighborChunk = {m_currentChunk.x + dx, m_currentChunk.y + dy};
            if ( neighborChunk.smaller(Vec2{0,0}) || neighborChunk.greater(m_levelSize/m_chunkSize) )
            {
                continue;
            }
            if (std::find(m_loadedChunks.begin(), m_loadedChunks.end(), neighborChunk) == m_loadedChunks.end() &&
                std::find(m_chunkQueue.begin(), m_chunkQueue.end(), neighborChunk) == m_chunkQueue.end())
            {
                m_chunkQueue.push_back(neighborChunk);
                m_neighboringChunks.push_back(neighborChunk);
            }
        }
    }
    if (!m_chunkQueue.empty()){
        Vec2 chunk = m_chunkQueue.front();
        m_chunkQueue.erase(m_chunkQueue.begin());
        EntityID chunkID = loadChunk(chunk);
        m_loadedChunkIDs.push_back(chunkID);
        m_loadedChunks.push_back(chunk);
    }
    clearChunks(15);
}

// void LevelLoader::loadLevel(Scene_Play* scene, const std::string& levelPath){
//     const char* path = levelPath.c_str();
//     SDL_Surface* loadedSurface = IMG_Load(path);
//     if (loadedSurface == nullptr) 
//     {
//         std::cerr << "Not loaded " << path << "! SDL_image Error: " << IMG_GetError() << std::endl;
//     }

//     // Lock the surface to access the pixels
//     SDL_LockSurface(loadedSurface);
//     Uint32* pixels = (Uint32*)loadedSurface->pixels;

//     const int HEIGHT_PIX = loadedSurface->h;
//     const int WIDTH_PIX = loadedSurface->w;
//     m_levelSize = Vec2{ (float)WIDTH_PIX, (float)HEIGHT_PIX };
//     createPixelMatrix(pixels, loadedSurface->format, WIDTH_PIX, HEIGHT_PIX);
//     // Unlock and free the surface
//     SDL_UnlockSurface(loadedSurface);
//     SDL_FreeSurface(loadedSurface);
//     loadChunk(m_currentChunk);
// }

Vec2 LevelLoader::getLevelSize(){
    return m_levelSize;
}