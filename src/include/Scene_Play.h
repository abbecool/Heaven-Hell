#pragma once

#include "Components.h"
#include "Physics.h"
#include "Scene.h"
#include <memory>

class Scene_Play : public Scene
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
    bool m_drawTextures = true;
    bool m_drawCollision = false;
    bool m_drawDrawGrid = false;
    const Vec2 m_gridSize = { 64, 64 };
    int m_speed = 400;

    void init(const std::string&);
    void loadLevel(std::string path);
    Vec2 gridToMidPixel(float, float, std::shared_ptr<Entity>);

    void spawnPlayer(const Vec2 pos, const std::string name, bool movable);
    void spawnObstacle(const Vec2 pos, bool movable, const int frame );
    void spawnCloud(const Vec2 pos, bool movable, const int frame);
    void spawnDragon(const Vec2 pos, bool movable, const std::string &ani);
    void spawnBackground(const Vec2 pos, bool movable, const int frame);
    void spawnGoal(const Vec2 pos, bool movable);
    void spawnKey(const Vec2 pos, const std::string, bool movable);
    void spawnLava(const Vec2 pos);
    void spawnWater(const Vec2 pos, const int frame );
    void spawnBridge(const Vec2 pos, const int frame );
    void spawnProjectile(std::shared_ptr<Entity>);

    void sMovement();
    void sCollision();
    void sStatus();
    void sAnimation();
    void sRender();
    
    std::vector<bool> neighborCheck(const std::vector<std::vector<std::string>>& pixelMatrix, const std::string &pixel, int x, int y, int width, int height);
    int getObstacleTextureIndex(const std::vector<bool>& neighbors);
    std::vector<std::vector<std::string>> createPixelMatrix(Uint32* pixels, SDL_PixelFormat* format, int width, int height);
    
    void sDoAction(const Action&);
    void onEnd();
    void setPaused(bool);
    void changePlayerStateTo(PlayerState s);

    public:
    Scene_Play(Game* game, std::string path);
    void update();
};