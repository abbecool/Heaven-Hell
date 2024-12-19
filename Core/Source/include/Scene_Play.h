#pragma once

#include "Components.h"
#include "Physics.h"
#include "Camera.h"
#include "Scene.h"
#include "Scene_Inventory.h"
#include "Level_Loader.h"
#include <memory>
#include <unordered_map>
#include <unordered_set>

class Scene_Play : public Scene
{
    struct PlayerConfig
    {
        int x, y;
        float SPEED, MAXSPEED;
        int HP, DAMAGE;
        int ATTACK_SPEED;
    };

    protected:

    friend class LevelLoader;
    EntityID m_player;
    std::string m_levelPath;
    PlayerConfig m_playerConfig;
    PlayerConfig m_rooterConfig;
    PlayerConfig m_goblinConfig;
    Physics m_physics;
    Camera m_camera;
    Vec2 cameraPos;
    float m_zoomStep = 2;
    Vec2 m_currentChunk = Vec2{1, 0};
    Vec2 m_chunkSize = Vec2{8, 8};
    std::vector<Vec2> m_loadedChunks;
    std::vector<EntityID> m_loadedChunkIDs;
    LevelLoader m_levelLoader;
    std::shared_ptr<Scene_Inventory> m_inventory_scene;
    const Vec2 m_gridSize = { 64, 64 };
    Vec2 m_levelSize;
    Vec2 m_mousePosition;
    bool cameraFollow = true;
    float cameraZoom = 0.75f;
    bool m_drawTextures = true;
    bool m_drawCollision = false;
    bool m_drawDrawGrid = false;
    bool m_inventoryOpen = false;
    bool m_newGame;
    std::vector<std::vector<std::string>> m_pixelMatrix;

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
    EntityID spawnPlayer();
    EntityID spawnWeapon(Vec2 pos);
    EntityID spawnProjectile(EntityID player, Vec2 vel);
    EntityID spawnCoin(Vec2 pos, const size_t layer);
    EntityID spawnSmallEnemy(Vec2 pos, const size_t layer, std::string type);
    EntityID spawnShadow(EntityID parentID, Vec2 relPos, int size, int layer);
    EntityID spawnDecoration(Vec2 pos, Vec2 collisionBox, const size_t layer, std::string animation);

    EntityID spawnObstacle  (const Vec2 pos, bool movable, const int frame );
    EntityID spawnDragon    (const Vec2 pos, bool movable, const std::string &ani);
    EntityID spawnGrass     (const Vec2 pos, const int frame);
    EntityID spawnDirt      (const Vec2 pos, const int frame);
    EntityID spawnCampfire  (const Vec2 pos);
    EntityID spawnKey       (const Vec2 pos, const std::string, bool movable);
    EntityID spawnWater     (const Vec2 pos, const std::string tag, const int frame );
    EntityID spawnLava      (const Vec2 pos, const std::string tag, const int frame );
    EntityID spawnBridge    (const Vec2 pos, const int frame );
    std::vector<EntityID> spawnDualTiles (const Vec2 pos, std::unordered_map<std::string, int> tileIndex );

    void sLoader();
    void sScripting();
    void sMovement();
    void sCollision();
    void sStatus();
    void sAnimation();
    void sRender();
    void spriteRender(Animation &animation);
    void sAudio();

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