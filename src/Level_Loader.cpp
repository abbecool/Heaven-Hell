#include "Scene_Play.h"
#include "Scene_Menu.h"
#include "Sprite.h"
#include "Assets.h"
#include "Game.h"
#include "Components.h"
#include "Action.h"
#include "Level_Loader.h"

#include "RandomArray.h"

#include <SDL2/SDL_image.h>

#include <iostream>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>


std::vector<bool> LevelLoader::neighborCheck(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height) {
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
        if(!neighbors[0]){neighbors[0] = (y > 0 && pixelMatrix[y - 1][x] == pix);}           // top
        if(!neighbors[1]){neighbors[1] = (x < width - 1 && pixelMatrix[y][x + 1] == pix);}   // right
        if(!neighbors[2]){neighbors[2] = (y < height - 1 && pixelMatrix[y + 1][x] == pix);}  // bottom
        if(!neighbors[3]){neighbors[3] = (x > 0 && pixelMatrix[y][x - 1] == pix);}           // left
    }
    return neighbors;

}

std::vector<std::string> LevelLoader::neighborTag(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height) {
    
    std::vector<std::string> neighborsTags(4, "nan"); // {top, bottom, left, right}
    if(y > 0){neighborsTags[0] = pixelMatrix[y - 1][x];}            // top
    if(x < width - 1){neighborsTags[1] = pixelMatrix[y][x + 1];}    // right
    if(y < height - 1){neighborsTags[2] = pixelMatrix[y + 1][x];}   // bottom
    if(x > 0 ){neighborsTags[3] = pixelMatrix[y][x - 1];}           // left

    return neighborsTags;
}

int LevelLoader::getObstacleTextureIndex(const std::vector<bool>& neighbors) {
    int numObstacles = std::count(neighbors.begin(), neighbors.end(), true);
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

std::vector<std::vector<std::string>> LevelLoader::createPixelMatrix(Uint32* pixels, SDL_PixelFormat* format, int width, int height) {
    std::vector<std::vector<std::string>> pixelMatrix(height, std::vector<std::string>(width, ""));
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            Uint32 pixel = pixels[y * width + x];

            Uint8 r, g, b, a;
            SDL_GetRGBA(pixel, format, &r, &g, &b, &a);

            if ((int)r == 192 && (int)g == 192 && (int)b == 192) {
                pixelMatrix[y][x] = "obstacle";
            } else if ((int)r == 200 && (int)g == 240 && (int)b == 255) {
                pixelMatrix[y][x] = "cloud";
            } else if ((int)r == 203 && (int)g == 129 && (int)b == 56) {
                pixelMatrix[y][x] = "dirt";
            } else if ((int)r == 0 && (int)g == 255 && (int)b == 0) {
                pixelMatrix[y][x] = "grass";
            } else if ((int)r == 255 && (int)g == 255 && (int)b == 255) {
                pixelMatrix[y][x] = "player_God";
            } else if ((int)r == 0 && (int)g == 0 && (int)b == 0) {
                pixelMatrix[y][x] = "player_Devil";
            } else if ((int)r == 255 && (int)g == 0 && (int)b == 255) {
                pixelMatrix[y][x] = "key";
            } else if ((int)r == 255 && (int)g == 255 && (int)b == 0) {
                pixelMatrix[y][x] = "goal";
            } else if ((int)r == 9 && (int)g == 88 && (int)b == 9) {
                pixelMatrix[y][x] = "dragon";
            } else if ((int)r == 255 && (int)g == 0 && (int)b == 0) {
                pixelMatrix[y][x] = "lava";
            } else if ((int)r == 0 && (int)g == 0 && (int)b == 255) {
                pixelMatrix[y][x] = "water";
            } else if ((int)r == 179 && (int)g == 0 && (int)b == 255) {
                pixelMatrix[y][x] = "bridge";
            } else {
                pixelMatrix[y][x] = "unknown";
            }
        }
    }
    return pixelMatrix;
}


std::unordered_map<std::string, int> LevelLoader::createDualGrid(
    std::vector<std::vector<std::string>>& pixelMatrix, 
    int x, int y, 
    const int HEIGHT_PIX, 
    const int WIDTH_PIX) 
{
    std::vector<std::string> tileQ(4, "");
    std::unordered_map<std::string, int> tileTextureMap;

    tileQ[1] = pixelMatrix[y][x];   // Q4
    tileQ[0] = (x > 0) ? pixelMatrix[y][x - 1] : pixelMatrix[y][x];  // Q3
    tileQ[2] = (y > 0) ? pixelMatrix[y - 1][x] : pixelMatrix[y][x];  // Q1
    tileQ[3] = (x > 0 && y > 0) ? pixelMatrix[y - 1][x - 1] : pixelMatrix[y][x];  // Q2

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

        int numTiles = std::count(tileQ.begin(), tileQ.end(), tile);
        std::unordered_set<std::string> uniqueStrings(tileQ.begin(), tileQ.end());

        if (numTiles > 0) {
            int textureIndex = -1;  // Initialize textureIndex

            if (numTiles == 4) {
                textureIndex = 6; // All quadrants are tiles
            } else if (numTiles == 3) {
                if (tileQ[0] != tile) textureIndex = 10;
                if (tileQ[1] != tile) textureIndex = 7;
                if (tileQ[2] != tile) textureIndex = 2;
                if (tileQ[3] != tile) textureIndex = 5;
            } else if (numTiles == 2) {
                if (tileQ[0] != tile && tileQ[1] != tile) textureIndex = 9;
                if (tileQ[1] != tile && tileQ[2] != tile) textureIndex = 11;
                if (tileQ[2] != tile && tileQ[3] != tile) textureIndex = 3;
                if (tileQ[3] != tile && tileQ[0] != tile) textureIndex = 1;
                if (tileQ[0] != tile && tileQ[2] != tile) textureIndex = 4;
                if (tileQ[1] != tile && tileQ[3] != tile) textureIndex = 14; 
                if (uniqueStrings.size() == 3 && (tile == "grass")) {
                    if (tileQ[0] == tile && tileQ[1] == tile) textureIndex = 19;
                    if (tileQ[1] == tile && tileQ[2] == tile) textureIndex = 17;
                    if (tileQ[2] == tile && tileQ[3] == tile) textureIndex = 16;
                    if (tileQ[3] == tile && tileQ[0] == tile) textureIndex = 18;
                }
            } else if (numTiles == 1) {
                if (tileQ[0] == tile) textureIndex = 0;
                if (tileQ[1] == tile) textureIndex = 13;
                if (tileQ[2] == tile) textureIndex = 8;
                if (tileQ[3] == tile) textureIndex = 15;
                if (uniqueStrings.size() == 3 && (tile == "grass")) {
                    if (tileQ[0] == tile && tileQ[2] == tileQ[3]) textureIndex = 20;
                    if (tileQ[0] == tile && tileQ[1] == tileQ[2]) textureIndex = 25;

                    if (tileQ[1] == tile && tileQ[0] == tileQ[3]) textureIndex = 23;
                    if (tileQ[1] == tile && tileQ[2] == tileQ[3]) textureIndex = 24;

                    if (tileQ[2] == tile && tileQ[0] == tileQ[1]) textureIndex = 22;
                    if (tileQ[2] == tile && tileQ[0] == tileQ[3]) textureIndex = 27;

                    if (tileQ[3] == tile && tileQ[1] == tileQ[2]) textureIndex = 21;
                    if (tileQ[3] == tile && tileQ[0] == tileQ[1]) textureIndex = 26;
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


// int LevelLoader::createDualGrid1(std::vector<std::vector<std::string>> pixelMatrix, int x, int y, const int HEIGHT_PIX, const int WIDTH_PIX) {
//     std::vector<std::string> tileQ = std::vector<std::string>(4, "");
//     int textureIndex;
//     tileQ[1] = pixelMatrix[y][x];   //Q4
//     if (x>0)        {tileQ[0] = pixelMatrix[y][x-1];}    else {tileQ[0] = pixelMatrix[y][x];}  // Q3
//     if (y>0)        {tileQ[2] = pixelMatrix[y-1][x];}    else {tileQ[2] = pixelMatrix[y][x];}  // Q1
//     if (x>0 && y>0) {tileQ[3] = pixelMatrix[y-1][x-1];}  else {tileQ[3] = pixelMatrix[y][x];}  // Q2

//     std::unordered_map<std::string, std::unordered_set<std::string>> friendlyNeighbors = {
//         {"grass", {"key", "goal", "player_God", "dragon"}},
//         {"dirt", {"key", "goal", "player_Devil", "dragon"}}
//     };
            
//     for (std::string tile : {"grass", "dirt", "water", "lava", "cloud", "obstacle", "bridge"})
//     {
//         if ( std::find(tileQ.begin(), tileQ.end(), "bridge") != tileQ.end() ){

//             if ( std::find(tileQ.begin(), tileQ.end(), "water") != tileQ.end() ){
//                 std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [](const std::string& str) {
//                     return (str == "bridge" ) ? "water" : str;
//                 });
//             } else if ( std::find(tileQ.begin(), tileQ.end(), "lava") != tileQ.end() ){
//                     std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [](const std::string& str) {
//                     return (str == "bridge" ) ? "lava" : str;
//                 });
//             } else {
//                 std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [](const std::string& str) {
//                     return (str == "bridge" ) ? "" : str;
//                 });
//             }
//         }
        
//          // Transform the vector based on friendly neighbors for the current tile type
//         std::transform(tileQ.begin(), tileQ.end(), tileQ.begin(), [&](const std::string& str) {
//             // Check if the current string is a friendly neighbor for the current tile
//             if (friendlyNeighbors[tile].count(str)) {
//                 return tile; // Replace it with the current tile
//             }
//             return str; // Keep the original if it's not a friendly neighbor
//         });

//         int numTiles = std::count(tileQ.begin(), tileQ.end(), tile);
//         std::unordered_set<std::string> uniqueStrings(tileQ.begin(), tileQ.end());
//         if (numTiles > 0){
//             if (numTiles == 4) {
//                 textureIndex = 6; // All quadrants are tiles
//             } else if (numTiles == 3) {
//                 if (tileQ[0] != tile) textureIndex = 10;
//                 if (tileQ[1] != tile) textureIndex = 7;
//                 if (tileQ[2] != tile) textureIndex = 2;
//                 if (tileQ[3] != tile) textureIndex = 5;
//             } else if (numTiles == 2) {
//                 if (tileQ[0] != tile && tileQ[1] != tile) textureIndex = 9;
//                 if (tileQ[1] != tile && tileQ[2] != tile) textureIndex = 11;
//                 if (tileQ[2] != tile && tileQ[3] != tile) textureIndex = 3;
//                 if (tileQ[3] != tile && tileQ[0] != tile) textureIndex = 1;
//                 if (tileQ[0] != tile && tileQ[2] != tile) textureIndex = 4;
//                 if (tileQ[1] != tile && tileQ[3] != tile) textureIndex = 14; 
//                 if (uniqueStrings.size() == 3 && (tile == "grass")){
//                     if (tileQ[0] == tile && tileQ[1] == tile) textureIndex = 19;
//                     if (tileQ[1] == tile && tileQ[2] == tile) textureIndex = 17;
//                     if (tileQ[2] == tile && tileQ[3] == tile) textureIndex = 16;
//                     if (tileQ[3] == tile && tileQ[0] == tile) textureIndex = 18;
//                 }
//             } if (numTiles == 1) {
//                 if (tileQ[0] == tile) textureIndex = 0;
//                 if (tileQ[1] == tile) textureIndex = 13;
//                 if (tileQ[2] == tile) textureIndex = 8;
//                 if (tileQ[3] == tile) textureIndex = 15;
//                 if (uniqueStrings.size() == 3 && (tile == "grass")){
//                     if (tileQ[0] == tile && tileQ[2] == tileQ[3]) textureIndex = 20;
//                     if (tileQ[0] == tile && tileQ[1] == tileQ[2]) textureIndex = 25;

//                     if (tileQ[1] == tile && tileQ[0] == tileQ[3]) textureIndex = 23;
//                     if (tileQ[1] == tile && tileQ[2] == tileQ[3]) textureIndex = 24;

//                     if (tileQ[2] == tile && tileQ[0] == tileQ[1]) textureIndex = 22;
//                     if (tileQ[2] == tile && tileQ[0] == tileQ[3]) textureIndex = 27;

//                     if (tileQ[3] == tile && tileQ[1] == tileQ[2]) textureIndex = 21;
//                     if (tileQ[3] == tile && tileQ[0] == tileQ[1]) textureIndex = 26;
//                 }
//             }
//             // if (tile != ""){
//             //     spawnDualTile(Vec2 {64*(float)x-32, 64*(float)y-32}, tile, textureIndex);
//             // }
//             return textureIndex;
//         }
//     }
// }


// 1  Q4
// 0  Q3
// 2  Q1
// 3  Q2