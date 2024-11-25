#pragma once

#include <memory>
#include "Vec2.h"
#include <unordered_map>

class Scene_Play;
class LevelLoader
{
    Scene_Play* m_scene;
    int m_width;
    int m_height;
    public:
        void init(Scene_Play* scene, const int width, const int height);
        std::vector<bool> neighborCheck(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height);
        std::vector<std::string> neighborTag(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height);
        int getObstacleTextureIndex(const std::vector<bool>& neighbors);
        std::vector<std::vector<std::string>> createPixelMatrix(Uint32* pixels, SDL_PixelFormat* format, int width, int height);
        std::unordered_map<std::string, int> createDualGrid(std::vector<std::vector<std::string>>& pixelMatrix, int x, int y);
                                                            // const std::vector<std::vector<std::string>>& pixelMatrix, int x, int y, const int HEIGHT_PIX, const int WIDTH_PIX
        void loadChunk();
};