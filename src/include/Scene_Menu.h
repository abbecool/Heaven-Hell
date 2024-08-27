#pragma once

#include "Components.h"
#include "Physics.h"
#include "Scene.h"
#include <memory>

class Scene_Menu : public Scene
{
    struct PlayerConfig
    {
        float X, Y, CX, CY, SPEED, MAXSPEED, JUMP, GRAVITY;
        std::string WEAPON; 
    };

    protected:

    std::shared_ptr<Entity> m_player;
    std::string m_levelPath;
    PlayerConfig m_playerConfig;
    Physics m_physics;
    Vec2 levelSize;
    Vec2 cameraPos;
    bool cameraFollow = true;
    float cameraZoom = 1;
    bool m_drawTextures = true;
    bool m_drawCollision = false;
    bool m_drawDrawGrid = false;
    bool m_drawCoordinates = false;
    const Vec2 m_gridSize = { 64, 64 };

    void init();
    void loadMenu();
    Vec2 gridToMidPixel(float, float, std::shared_ptr<Entity>);

    void spawnButton(const Vec2 pos, const std::string& unpressed, const std::string& pressed);
    void spawnLevel(const Vec2 pos, const std::string tile);
    void spawnDualTile(const Vec2 pos, const std::string tile, const int frame );

    void sMovement();
    void sCollision();
    void sStatus();
    void sAnimation();
    void sRender();
    
    std::vector<bool> neighborCheck(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height);
    std::vector<std::string> neighborTag(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height);
    int getObstacleTextureIndex(const std::vector<bool>& neighbors);
    std::vector<std::vector<std::string>> createPixelMatrix(Uint32* pixels, SDL_PixelFormat* format, int width, int height);
    void spawnDualGrid(std::vector<std::vector<std::string>> pixelMatrix, int x, int y);
    
    void sDoAction(const Action&);
    void onEnd();
    void setPaused(bool);
    void changePlayerStateTo(PlayerState s);

    public:
    Scene_Menu(Game* game);
    void update();
};