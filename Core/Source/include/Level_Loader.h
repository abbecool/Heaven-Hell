#pragma once

#include <memory>
#include "Vec2.h"
#include <unordered_map>

class LevelLoader
{
    public:
        std::vector<bool> neighborCheck(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height);
        std::vector<std::string> neighborTag(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height);
        int getObstacleTextureIndex(const std::vector<bool>& neighbors);
        std::vector<std::vector<std::string>> createPixelMatrix(Uint32* pixels, SDL_PixelFormat* format, int width, int height);
        std::unordered_map<std::string, int> createDualGrid(std::vector<std::vector<std::string>>& pixelMatrix, int x, int y);
                                                            // const std::vector<std::vector<std::string>>& pixelMatrix, int x, int y, const int HEIGHT_PIX, const int WIDTH_PIX
};