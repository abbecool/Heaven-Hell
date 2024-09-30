#pragma once

#include "Components.h"
#include "Physics.h"
#include "Camera.h"
#include "Scene.h"
#include "Level_Loader.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>

class Scene_Play : public Scene
{
    struct PlayerConfig
    {
        size_t x, y;
        float SPEED, MAXSPEED;
        int HP, DAMAGE;
    };

    protected:

    EntityID m_player;
    std::string m_levelPath;
    PlayerConfig m_playerConfig;
    Physics m_physics;
    Camera m_camera;
    Vec2 cameraPos;
    LevelLoader m_levelLoader;
    const Vec2 m_gridSize = { 64, 64 };
    Vec2 m_levelSize;
    Vec2 m_mousePosition;
    bool cameraFollow = true;
    float cameraZoom = 1;
    bool m_drawTextures = true;
    bool m_drawCollision = false;
    bool m_drawDrawGrid = false;
    bool m_newGame;

    std::unordered_map<std::string, std::unordered_set<std::string>> m_damageToEnemyMap = {
        {"fire", {"grass"}},
        {"water", {"fire"}},
        {"ice", {"water"}},
        {"explosive", {"rock"}},
        {"piercing", {"shielded"}}
    };

    void init(const std::string&);
    void loadLevel(const std::string& path);
    void loadConfig(const std::string& path);
    void saveGame(const std::string& filename);
    Vec2 gridToMidPixel(float, float, EntityID);

    // void spawnHUD();
    void spawnPlayer();
    void spawnWeapon(Vec2 pos );
    void spawnProjectile(EntityID player, Vec2 vel);
    void spawnCoin(Vec2 pos, const size_t layer);
    void spawnSmallEnemy(Vec2 pos, const size_t layer);

    void spawnObstacle  (const Vec2 pos, bool movable, const int frame );
    void spawnCloud     (const Vec2 pos, bool movable, const int frame);
    void spawnDragon    (const Vec2 pos, bool movable, const std::string &ani);
    void spawnGrass     (const Vec2 pos, const int frame);
    void spawnDirt      (const Vec2 pos, const int frame);
    void spawnGoal      (const Vec2 pos, bool movable);
    void spawnKey       (const Vec2 pos, const std::string, bool movable);
    void spawnWater     (const Vec2 pos, const std::string tag, const int frame );
    void spawnDualTiles  (const Vec2 pos, std::unordered_map<std::string, int> tileIndex );
    void spawnLava      (const Vec2 pos, const std::string tag, const int frame );
    void spawnBridge    (const Vec2 pos, const int frame );

    void sMovement();
    void sCollision();
    void sStatus();
    void sAnimation();
    void sRender();
    void spriteRender(Animation &animation);

    void sDoAction(const Action&);
    void onEnd();
    void setPaused(bool);
    void changePlayerStateTo(EntityID entity, PlayerState s);

    public:
    Scene_Play(Game* game, std::string path, bool newGame);
    Vec2 gridSize();
    Vec2 levelSize();
    void update();
};